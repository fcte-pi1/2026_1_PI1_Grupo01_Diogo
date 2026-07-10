# Contrato de API - Telemetria Micromouse

Especificação do protocolo de comunicação HTTP entre o firmware (ESP32) e o servidor backend.

## Informações Gerais

- **Endpoint:** `POST` `http://[IP_LOCAL]:3000/api/telemetria`
- **Content-Type:** `application/json`

### Trigger
1. **Mudança de Coordenada:** O ESP32 deve disparar a requisição imediatamente a cada alteração de posição `(X, Y)`.
2. **Envio de Segurança:** Caso o robô permaneça na mesma célula por mais de **1000 ms**, um envio deve ser feito obrigatoriamente.
3. **Reset do Timer:** Qualquer envio bem-sucedido reinicia a contagem de tempo.

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
  } // <-- Faltava fechar essa chave
}   // <-- E faltava fechar a chave principal
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

---

## Ciclo de Vida da Corrida (Run)

Cada corrida (`Run`) agrupa as telemetrias de uma execução. **Quem salva e
encerra a corrida é o robô**; a interface web apenas **visualiza** os dados.

| Quem | Quando                                    | Efeito                                                                       |
| ---- | ----------------------------------------- | --------------------------------------------------------------------------- |
| Robô | Primeiro POST de telemetria               | A corrida é criada/identificada automaticamente (ver regra de agrupamento). |
| Robô | Envia telemetria com `estado_robo` final  | A corrida é finalizada automaticamente (ver tabela de estados).            |
| Web  | Telas de "Histórico" e "Acompanhar"       | Apenas lê os dados via API (não cria nem finaliza corridas).               |

### Regra de agrupamento da telemetria
- Se o payload trouxer `runId`, a telemetria é gravada nessa corrida (criada se não existir).
- Sem `runId`, é anexada à corrida `EM_ANDAMENTO` mais recente; se não houver nenhuma, uma nova é criada para não perder o dado.

### Estados da corrida (`status`)
- `EM_ANDAMENTO` — corrida aberta, recebendo telemetria.
- `CONCLUIDA` — o robô completou o objetivo.
- `NAO_CONCLUIDA` — encerrada pelo robô sem completar (erro).

### `estado_robo` que encerram a corrida
| `estado_robo`          | `status` resultante |
| ---------------------- | ------------------- |
| `OBJETIVO_ENCONTRADO`  | `CONCLUIDA`         |
| `CONCLUIDO`            | `CONCLUIDA`         |
| `ERRO`                 | `NAO_CONCLUIDA`     |

### Endpoints de corrida (consumidos pela web)

| Método   | Rota                                    | Descrição                                            |
| -------- | --------------------------------------- | ---------------------------------------------------- |
| `GET`    | `/api/telemetria/latest`                | Última telemetria recebida (descobre a corrida ativa). |
| `GET`    | `/api/telemetria/runs`                  | Lista as corridas (mais recentes primeiro).         |
| `GET`    | `/api/telemetria/runs/:id`              | Detalhes de uma corrida. → `404` se não existir.     |
| `GET`    | `/api/telemetria/runs/:id/telemetries`  | Telemetrias da corrida (ordenadas por tempo).        |
| `DELETE` | `/api/telemetria/runs/:id`              | Remove a corrida e suas telemetrias. → `204` / `404`. |
