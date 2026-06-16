// Mock do PrismaClient — isola completamente o banco de dados nos testes.
// O Jest substitui @prisma/client por este arquivo graças ao moduleNameMapper no package.json.

const prismaMock = {
  telemetry: {
    create: jest.fn(),
    findFirst: jest.fn(),
    findMany: jest.fn(),
    deleteMany: jest.fn(),
  },
  run: {
    create: jest.fn(),
    findUnique: jest.fn(),
    findMany: jest.fn(),
    deleteMany: jest.fn(),
  },
  $connect: jest.fn(),
  $disconnect: jest.fn(),
};

// Reseta todos os mocks entre os testes para evitar contaminação de estado.
beforeEach(() => {
  jest.clearAllMocks();
});

// A classe PrismaClient exportada é um construtor que retorna o mock acima.
export const PrismaClient = jest.fn(() => prismaMock);

// Exporta o mock direto para que os testes possam fazer asserções nele.
export { prismaMock };
