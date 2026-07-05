#!/usr/bin/env python3
"""
Captura a telemetria dos testes de bancada (reta / giro) e salva em CSV.

Os scripts_teste (teste_reta_imu, teste_giro_imu) gravam as amostras num buffer
em RAM durante o movimento e despejam tudo no fim, delimitado por:

    ---- INICIO TELEMETRIA (CSV) ----
    <linha de header com os nomes das colunas>
    <linhas de dados>
    ---- FIM (N amostras) ----

Este script conecta ao robô (Wi-Fi/TCP por padrao, ou Serial), manda 's' para
disparar o teste, captura tudo entre os marcadores, grava num arquivo com
timestamp em tools/logs/ e — se matplotlib estiver instalado — plota no fim.

Exemplos:
    # Wi-Fi (use o IP que o robô imprime no boot). Rotulo vira nome do arquivo.
    python3 captura_log.py --ip 192.168.0.42 --nome reta_labirinto

    # Serial
    python3 captura_log.py --serial /dev/ttyUSB0 --nome giro_labirinto

    # Modo continuo: fica escutando e salva um arquivo por rajada (mande 's'
    # pelo Serial Monitor manualmente; nao dispara sozinho).
    python3 captura_log.py --ip 192.168.0.42 --continuo
"""

import argparse
import datetime as dt
import os
import socket
import sys
import time

INICIO = "---- INICIO TELEMETRIA"
FIM = "---- FIM"
PORTA_TCP = 8080
DIR_LOGS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")


# --------------------------------------------------------------------------- #
# Camada de transporte: abstrai TCP e Serial atras de send()/linhas().
# --------------------------------------------------------------------------- #
class FonteTCP:
    def __init__(self, ip, porta=PORTA_TCP, timeout=1.0):
        print(f"[captura] Conectando via TCP em {ip}:{porta} ...")
        self.sock = socket.create_connection((ip, porta), timeout=8)
        self.sock.settimeout(timeout)
        self._buf = b""

    def send(self, texto):
        self.sock.sendall(texto.encode())

    def linhas(self):
        """Gera linhas (str, sem \\n). Bloqueante ate a conexao cair."""
        while True:
            try:
                dados = self.sock.recv(4096)
            except socket.timeout:
                yield None  # tick sem dado (deixa o chamador cuidar de Ctrl-C)
                continue
            if not dados:
                return
            self._buf += dados
            while b"\n" in self._buf:
                linha, self._buf = self._buf.split(b"\n", 1)
                yield linha.decode(errors="replace").rstrip("\r")

    def close(self):
        self.sock.close()


class FonteSerial:
    def __init__(self, porta, baud=115200, timeout=1.0):
        try:
            import serial  # pyserial
        except ImportError:
            sys.exit("[captura] Falta pyserial:  pip install pyserial")
        print(f"[captura] Abrindo Serial {porta} @ {baud} ...")
        self.ser = serial.Serial(porta, baud, timeout=timeout)
        time.sleep(0.3)

    def send(self, texto):
        self.ser.write(texto.encode())

    def linhas(self):
        while True:
            raw = self.ser.readline()
            if not raw:
                yield None
                continue
            yield raw.decode(errors="replace").rstrip("\r\n")

    def close(self):
        self.ser.close()


# --------------------------------------------------------------------------- #
# Captura de uma rajada de telemetria (entre INICIO e FIM).
# --------------------------------------------------------------------------- #
def capturar_rajada(fonte, eco=True):
    """Retorna a lista de linhas CSV (header + dados) de UMA rajada, ou None."""
    coletando = False
    linhas_csv = []
    for linha in fonte.linhas():
        if linha is None:
            continue
        if eco:
            print("  " + linha)
        if INICIO in linha:
            coletando = True
            linhas_csv = []
            continue
        if FIM in linha:
            if coletando:
                return linhas_csv
            continue
        if coletando:
            linhas_csv.append(linha)
    return None


def salvar_csv(linhas_csv, nome):
    os.makedirs(DIR_LOGS, exist_ok=True)
    ts = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    caminho = os.path.join(DIR_LOGS, f"{nome}_{ts}.csv")
    with open(caminho, "w") as f:
        f.write("\n".join(linhas_csv) + "\n")
    print(f"[captura] {len(linhas_csv) - 1} amostras salvas em {caminho}")
    return caminho


# --------------------------------------------------------------------------- #
# Plot opcional (so roda se matplotlib estiver instalado).
# --------------------------------------------------------------------------- #
def plotar(caminho):
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("[captura] matplotlib nao instalado; pulei o grafico. "
              "(pip install matplotlib para habilitar)")
        return

    with open(caminho) as f:
        linhas = [l.strip() for l in f if l.strip()]
    if len(linhas) < 2:
        print("[captura] CSV sem dados; nada a plotar.")
        return

    cols = linhas[0].split(",")
    dados = {c: [] for c in cols}
    for linha in linhas[1:]:
        vals = linha.split(",")
        if len(vals) != len(cols):
            continue
        for c, v in zip(cols, vals):
            try:
                dados[c].append(float(v))
            except ValueError:
                dados[c].append(float("nan"))

    if "t_ms" not in dados:
        print("[captura] Sem coluna t_ms; nada a plotar.")
        return
    t = dados["t_ms"]

    # Painel 1: sinais de angulo/erro; Painel 2: PWM aplicado.
    ang = [c for c in ("ang_z", "erro", "velGiro") if c in dados]
    pwm = [c for c in ("pwmEsq", "pwmDir", "correcao") if c in dados]

    fig, axs = plt.subplots(2, 1, sharex=True, figsize=(10, 7))
    for c in ang:
        axs[0].plot(t, dados[c], label=c)
    axs[0].set_ylabel("graus / graus/s")
    axs[0].legend(); axs[0].grid(True, alpha=0.3)
    axs[0].set_title(os.path.basename(caminho))

    for c in pwm:
        axs[1].plot(t, dados[c], label=c)
    axs[1].set_ylabel("PWM / correcao")
    axs[1].set_xlabel("t (ms)")
    axs[1].legend(); axs[1].grid(True, alpha=0.3)

    fig.tight_layout()
    png = caminho.rsplit(".", 1)[0] + ".png"
    fig.savefig(png, dpi=110)
    print(f"[captura] Grafico salvo em {png}")
    plt.show()


# --------------------------------------------------------------------------- #
def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--ip", help="IP do robo (Wi-Fi/TCP porta 8080)")
    g.add_argument("--serial", help="porta serial, ex: /dev/ttyUSB0")
    ap.add_argument("--nome", default="teste", help="prefixo do arquivo de log")
    ap.add_argument("--comando", default="s",
                    help="tecla que dispara o teste: reta usa 's'; giro usa 1/2/3/4")
    ap.add_argument("--continuo", action="store_true",
                    help="fica escutando; salva uma rajada por vez (nao dispara comando)")
    ap.add_argument("--sem-plot", action="store_true", help="nao plota no fim")
    args = ap.parse_args()

    fonte = FonteTCP(args.ip) if args.ip else FonteSerial(args.serial)

    try:
        if args.continuo:
            print("[captura] Modo continuo. Dispare 's' no robo (Ctrl-C para sair).")
            while True:
                linhas = capturar_rajada(fonte)
                if linhas:
                    caminho = salvar_csv(linhas, args.nome)
                    if not args.sem_plot:
                        plotar(caminho)
        else:
            print(f"[captura] Enviando '{args.comando}' para disparar o teste...")
            fonte.send(args.comando)
            linhas = capturar_rajada(fonte)
            if not linhas:
                sys.exit("[captura] Conexao caiu antes do FIM da telemetria.")
            caminho = salvar_csv(linhas, args.nome)
            if not args.sem_plot:
                plotar(caminho)
    except KeyboardInterrupt:
        print("\n[captura] Encerrado pelo usuario.")
    finally:
        fonte.close()


if __name__ == "__main__":
    main()
