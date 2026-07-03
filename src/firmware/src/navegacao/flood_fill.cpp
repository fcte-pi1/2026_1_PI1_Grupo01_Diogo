#include "flood_fill.h"

// Deslocamento (dx, dy) para cada direção, na ordem N, L, S, O.
static const int8_t DX[4] = {  0, +1,  0, -1 };
static const int8_t DY[4] = { +1,  0, -1,  0 };

static inline Direcao oposta(Direcao d) {
    return (Direcao)((d + 2) & 0x03);
}

bool FloodFill::dentro(int x, int y) const {
    return (x >= 0 && x < MAZE_N && y >= 0 && y < MAZE_N);
}

void FloodFill::vizinho(int x, int y, Direcao d, int& nx, int& ny) const {
    nx = x + DX[d];
    ny = y + DY[d];
}

void FloodFill::iniciar() {
    for (int y = 0; y < MAZE_N; y++) {
        for (int x = 0; x < MAZE_N; x++) {
            _paredes[y][x] = 0;
            _dist[y][x]    = INFINITO;
        }
    }

    // Fecha as bordas externas do labirinto (sempre há parede na moldura).
    for (int i = 0; i < MAZE_N; i++) {
        _paredes[MAZE_N - 1][i] |= (1 << NORTE);  // linha de cima
        _paredes[0][i]          |= (1 << SUL);    // linha de baixo
        _paredes[i][MAZE_N - 1] |= (1 << LESTE);  // coluna da direita
        _paredes[i][0]          |= (1 << OESTE);  // coluna da esquerda
    }

    definirObjetivoCentro();
}

void FloodFill::definirObjetivoCentro() {
    if (MAZE_N % 2 == 0) {
        // Bloco central 2x2 (padrão micromouse 16x16).
        uint8_t c = MAZE_N / 2;
        _objetivos[0] = { (uint8_t)(c - 1), (uint8_t)(c - 1) };
        _objetivos[1] = { (uint8_t)(c),     (uint8_t)(c - 1) };
        _objetivos[2] = { (uint8_t)(c - 1), (uint8_t)(c)     };
        _objetivos[3] = { (uint8_t)(c),     (uint8_t)(c)     };
        _numObjetivos = 4;
    } else {
        uint8_t c = MAZE_N / 2;
        _objetivos[0] = { c, c };
        _numObjetivos = 1;
    }
}

void FloodFill::definirObjetivo(uint8_t x, uint8_t y) {
    _objetivos[0]  = { x, y };
    _numObjetivos  = 1;
}

void FloodFill::definirParede(uint8_t x, uint8_t y, Direcao d, bool existe) {
    if (!dentro(x, y)) return;

    if (existe) _paredes[y][x] |=  (1 << d);
    else        _paredes[y][x] &= ~(1 << d);

    // Mantém a parede coerente na célula vizinha.
    int nx, ny;
    vizinho(x, y, d, nx, ny);
    if (dentro(nx, ny)) {
        Direcao od = oposta(d);
        if (existe) _paredes[ny][nx] |=  (1 << od);
        else        _paredes[ny][nx] &= ~(1 << od);
    }
}

bool FloodFill::temParede(uint8_t x, uint8_t y, Direcao d) const {
    if (!dentro(x, y)) return true;   // fora do labirinto = parede
    return (_paredes[y][x] & (1 << d)) != 0;
}

void FloodFill::calcular() {
    // BFS a partir das células-objetivo (distância 0), propagando +1
    // apenas por passagens SEM parede.
    for (int y = 0; y < MAZE_N; y++)
        for (int x = 0; x < MAZE_N; x++)
            _dist[y][x] = INFINITO;

    // Fila circular simples (capacidade = número de células).
    static Celula fila[MAZE_N * MAZE_N];
    int inicio = 0, fim = 0;

    for (uint8_t i = 0; i < _numObjetivos; i++) {
        Celula c = _objetivos[i];
        if (!dentro(c.x, c.y)) continue;
        _dist[c.y][c.x] = 0;
        fila[fim++] = c;
    }

    while (inicio < fim) {
        Celula c = fila[inicio++];
        uint8_t d = _dist[c.y][c.x];

        for (uint8_t dir = 0; dir < 4; dir++) {
            if (temParede(c.x, c.y, (Direcao)dir)) continue;  // não passa por parede

            int nx = c.x + DX[dir];
            int ny = c.y + DY[dir];
            if (!dentro(nx, ny)) continue;

            if (_dist[ny][nx] > (uint8_t)(d + 1)) {
                _dist[ny][nx] = d + 1;
                fila[fim++] = { (uint8_t)nx, (uint8_t)ny };
            }
        }
    }
}

uint8_t FloodFill::distancia(uint8_t x, uint8_t y) const {
    if (!dentro(x, y)) return INFINITO;
    return _dist[y][x];
}

bool FloodFill::ehObjetivo(uint8_t x, uint8_t y) const {
    for (uint8_t i = 0; i < _numObjetivos; i++)
        if (_objetivos[i].x == x && _objetivos[i].y == y) return true;
    return false;
}

bool FloodFill::melhorDirecao(uint8_t x, uint8_t y, Direcao dirAtual, Direcao& saida) const {
    if (!dentro(x, y)) return false;

    uint8_t melhorDist = INFINITO;
    int     melhorDir  = -1;

    // Ordem de teste com desempate: primeiro a direção atual (seguir reto),
    // depois direita, esquerda e por fim ré — reduz curvas desnecessárias.
    Direcao ordem[4] = {
        dirAtual,
        (Direcao)((dirAtual + 1) & 3),  // direita
        (Direcao)((dirAtual + 3) & 3),  // esquerda
        (Direcao)((dirAtual + 2) & 3)   // ré
    };

    for (uint8_t k = 0; k < 4; k++) {
        Direcao dir = ordem[k];
        if (temParede(x, y, dir)) continue;

        int nx = x + DX[dir];
        int ny = y + DY[dir];
        if (!dentro(nx, ny)) continue;

        uint8_t d = _dist[ny][nx];
        if (d < melhorDist) {     // "<" (não "<=") preserva o desempate pela ordem
            melhorDist = d;
            melhorDir  = dir;
        }
    }

    if (melhorDir < 0) return false;   // preso (não deveria acontecer se calcular() rodou)
    saida = (Direcao)melhorDir;
    return true;
}

void FloodFill::imprimirSerial() const {
    // Imprime de cima (NORTE) para baixo para bater com a orientação física.
    for (int y = MAZE_N - 1; y >= 0; y--) {
        for (int x = 0; x < MAZE_N; x++) {
            uint8_t d = _dist[y][x];
            if (d == INFINITO) Serial.print("  . ");
            else               Serial.printf("%3d ", d);
        }
        Serial.println();
    }
    Serial.println();
}
