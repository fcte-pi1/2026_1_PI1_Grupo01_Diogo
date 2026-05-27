export const openApiSpec = {
  openapi: "3.0.3",
  info: {
    title: "PI1 Backend API",
    version: "1.0.0",
    description: "API para recebimento e consulta de telemetria do robo.",
  },
  servers: [
    {
      url: "http://localhost:3000",
      description: "Ambiente local",
    },
  ],
  paths: {
    "/api/health": {
      get: {
        summary: "Verifica se a API esta online",
        tags: ["Health"],
        responses: {
          "200": {
            description: "API online",
            content: {
              "application/json": {
                schema: {
                  type: "object",
                  properties: {
                    status: {
                      type: "string",
                      example: "ok",
                    },
                  },
                },
              },
            },
          },
        },
      },
    },
    "/api/telemetry": {
      get: {
        summary: "Lista registros de telemetria",
        tags: ["Telemetry"],
        parameters: [
          {
            name: "limit",
            in: "query",
            required: false,
            schema: {
              type: "integer",
              minimum: 1,
              maximum: 100,
              default: 50,
            },
          },
        ],
        responses: {
          "200": {
            description: "Registros retornados com sucesso",
            content: {
              "application/json": {
                schema: {
                  type: "object",
                  properties: {
                    data: {
                      type: "array",
                      items: {
                        $ref: "#/components/schemas/TelemetryRecord",
                      },
                    },
                  },
                },
              },
            },
          },
          "500": {
            $ref: "#/components/responses/InternalServerError",
          },
        },
      },
      post: {
        summary: "Registra telemetria enviada pelo robo",
        tags: ["Telemetry"],
        requestBody: {
          required: true,
          content: {
            "application/json": {
              schema: {
                $ref: "#/components/schemas/TelemetryCreateRequest",
              },
              example: {
                robotId: "micromouse-01",
                sessionId: "teste-labirinto-01",
                sequence: 1,
                batteryLevel: 87.5,
                positionX: 2.4,
                positionY: 1.2,
                headingDegrees: 90,
                linearVelocity: 0.35,
                angularVelocity: 0.02,
              },
            },
          },
        },
        responses: {
          "201": {
            description: "Telemetria registrada com sucesso",
            content: {
              "application/json": {
                schema: {
                  type: "object",
                  properties: {
                    data: {
                      $ref: "#/components/schemas/TelemetryRecord",
                    },
                  },
                },
              },
            },
          },
          "400": {
            description: "JSON invalido",
            content: {
              "application/json": {
                schema: {
                  $ref: "#/components/schemas/ErrorResponse",
                },
              },
            },
          },
          "422": {
            description: "Payload com campos invalidos",
            content: {
              "application/json": {
                schema: {
                  allOf: [
                    {
                      $ref: "#/components/schemas/ErrorResponse",
                    },
                    {
                      type: "object",
                      properties: {
                        details: {
                          type: "array",
                          items: {
                            type: "string",
                          },
                        },
                      },
                    },
                  ],
                },
              },
            },
          },
          "500": {
            $ref: "#/components/responses/InternalServerError",
          },
        },
      },
    },
  },
  components: {
    responses: {
      InternalServerError: {
        description: "Erro interno ao acessar o banco ou processar a requisicao",
        content: {
          "application/json": {
            schema: {
              $ref: "#/components/schemas/ErrorResponse",
            },
          },
        },
      },
    },
    schemas: {
      TelemetryCreateRequest: {
        type: "object",
        additionalProperties: true,
        properties: {
          sessionId: {
            type: "string",
            example: "teste-labirinto-01",
          },
          robotId: {
            type: "string",
            example: "micromouse-01",
          },
          sequence: {
            type: "number",
            example: 1,
          },
          batteryLevel: {
            type: "number",
            example: 87.5,
          },
          positionX: {
            type: "number",
            example: 2.4,
          },
          positionY: {
            type: "number",
            example: 1.2,
          },
          headingDegrees: {
            type: "number",
            example: 90,
          },
          linearVelocity: {
            type: "number",
            example: 0.35,
          },
          angularVelocity: {
            type: "number",
            example: 0.02,
          },
        },
      },
      TelemetryRecord: {
        allOf: [
          {
            $ref: "#/components/schemas/TelemetryCreateRequest",
          },
          {
            type: "object",
            properties: {
              id: {
                type: "string",
                format: "uuid",
              },
              payload: {
                type: "string",
                description: "JSON original recebido, serializado como string.",
              },
              receivedAt: {
                type: "string",
                format: "date-time",
              },
            },
          },
        ],
      },
      ErrorResponse: {
        type: "object",
        properties: {
          error: {
            type: "string",
          },
        },
      },
    },
  },
  tags: [
    {
      name: "Health",
    },
    {
      name: "Telemetry",
    },
  ],
};
