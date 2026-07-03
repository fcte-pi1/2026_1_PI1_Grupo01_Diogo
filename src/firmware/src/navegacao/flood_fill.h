#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Flood Fill básico para micromouse.
//
// Representação:
//   - Labirinto NxN células (MAZE_N). Padrão da competição = 16.
//   - Origem (0,0) no canto INFERIOR-ESQUERDO. Robô começa olhando p/ NORTE.
//   - Eixo x cresce para LESTE, eixo y cresce para NORTE.
//   - Cada célula guarda 4 bits de parede (N, L, S, O).
//
// Uso típico (a cada célula):
//   1. definirParede(...) com o que os sensores enxergam;
//   2. calcular();  -> preenche as distâncias até o objetivo (BFS);
//   3. melhorDirecao(...) -> vizinho acessível de menor distância.
// -----------------------------------------------------------------------------

#ifndef MAZE_N
#define MAZE_N 4          // tamanho do labirinto (NxN). Mude aqui se necessário.
#endif

// Direções absolutas (bússola). A ORDEM importa:
// girar à DIREITA soma +1 (mod 4); girar à ESQUERDA soma +3 (mod 4).
enum Direcao : uint8_t {
    NORTE = 0,
    LESTE = 1,
    SUL   = 2,
    OESTE = 3
};

struct Celula {
    uint8_t x;
    uint8_t y;
};

class FloodFill {
public:
    void iniciar();                                  // zera paredes internas e fecha as bordas externas
    void definirObjetivoCentro();                    // objetivo = bloco central (2x2 se N par)
    void definirObjetivo(uint8_t x, uint8_t y);      // objetivo = uma única célula

    // Paredes: ao marcar, a célula vizinha também é atualizada (consistência).
    void definirParede(uint8_t x, uint8_t y, Direcao d, bool existe);
    bool temParede(uint8_t x, uint8_t y, Direcao d) const;

    void    calcular();                              // roda o flood fill (BFS a partir do objetivo)
    uint8_t distancia(uint8_t x, uint8_t y) const;
    bool    ehObjetivo(uint8_t x, uint8_t y) const;

    // Melhor direção a seguir a partir de (x,y): vizinho acessível de menor distância.
    // Em caso de empate, prefere continuar reto (dirAtual). Retorna false se preso.
    bool melhorDirecao(uint8_t x, uint8_t y, Direcao dirAtual, Direcao& saida) const;

    void imprimirSerial() const;                     // depuração via Serial

    static constexpr uint8_t INFINITO = 255;

private:
    uint8_t _paredes[MAZE_N][MAZE_N];   // bits: 1<<NORTE, 1<<LESTE, 1<<SUL, 1<<OESTE
    uint8_t _dist[MAZE_N][MAZE_N];

    Celula  _objetivos[4];
    uint8_t _numObjetivos;

    bool dentro(int x, int y) const;
    void vizinho(int x, int y, Direcao d, int& nx, int& ny) const;
};
