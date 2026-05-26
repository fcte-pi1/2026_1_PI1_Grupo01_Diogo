# Contrato de API - Telemetria Micromouse

Especificação do protocolo de comunicação HTTP entre o firmware (ESP32) e o servidor backend.

## Informações Gerais

- **Endpoint:** `POST` `http://[IP_LOCAL]:3000/api/telemetria`
- **Content-Type:** `application/json`

### Gatilho de Envio (Trigger)
1. **Mudança de Coordenada:** O ESP32 deve disparar a requisição imediatamente a cada alteração de posição `(X, Y)`.
2. **Envio de Segurança (Heartbeat):** Caso o robô permaneça na mesma célula por mais de **1000 ms**, um envio deve ser feito obrigatoriamente.
3. **Reset do Timer:** Qualquer envio bem-sucedido reinicia a contagem de tempo do gatilho de segurança.

---

## Estrutura do Payload (JSON)

O firmware deve enviar e a API Web deve validar estritamente a estrutura e os tipos de dados abaixo.

### Exemplo de Payload

```json
{
  "tempo_corrida_ms": 15400,
  "posicao_x": 3,
  "posicao_y": 4,
  "direcao_atual": "NORTE",
  "estado_robo": "EXPLORANDO",
  "bateria_pct": 82,
  "leitura_sensores": {
    "dist_frente_cm": 12.5,
    "dist_esquerda_cm": 4.1,
    "dist_direita_cm": 15.0
  }
```
## Dicionário de Dados e Tipagem

| Campo              | Tipo      | Descrição                               |
| ------------------ | --------- | --------------------------------------- |
| `tempo_corrida_ms` | `integer` | Tempo total da corrida em milissegundos |
| `posicao_x`        | `integer` | Coordenada X atual do robô              |
| `posicao_y`        | `integer` | Coordenada Y atual do robô              |
| `direcao_atual`    | `string`  | Direção atual do robô                   |
| `estado_robo`      | `string`  | Estado operacional atual                |
| `bateria_pct`      | `integer` | Percentual da bateria                   |
| `leitura_sensores` | `object`  | Leituras atuais dos sensores            |
| `dist_frente_cm`   | `float`   | Distância frontal em centímetros        |
| `dist_esquerda_cm` | `float`   | Distância à esquerda em centímetros     |
| `dist_direita_cm`  | `float`   | Distância à direita em centímetros      |


---

## Respostas da API (Status Codes)
201 Created: Telemetria processada e armazenada com sucesso.

400 Bad Request: Payload malformado, campos obrigatórios ausentes ou violação dos tipos/restrições definidos acima.

500 Internal Server Error: Erro inesperado no processamento por parte do servidor.
