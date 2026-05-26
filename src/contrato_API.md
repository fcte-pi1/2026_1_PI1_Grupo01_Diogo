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
}
