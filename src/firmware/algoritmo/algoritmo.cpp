#include <iostream>
#include <string>
#include <vector>
#include "API.h"
#include <queue>
#include <algorithm>


#include <sstream>


struct area {
    bool visitado = false;
    bool Norte = false;
    bool Sul = false;
    bool Leste = false;
    bool Oeste = false;
};

const int MAZE_LIMIT = 32; 
int offset = 16;
area mapGlobal[MAZE_LIMIT][MAZE_LIMIT];
int direcao = 0;
int posX = 0, posY = 0;

int centroX = -1, centroY = -1;
bool achouCentro = false;
bool chegou = false;

int minX = 0, maxX = 0;
int minY = 0, maxY = 0;
int celulasVisitadasContador = 0;


struct Passo {
    int x, y, dirOriginal;
};

int getoffset(int val) {
    return val + offset;
}

void atualizarLimitesMapa(int x, int y) {
    if (x < minX) minX = x;
    if (x > maxX) maxX = x;
    if (y < minY) minY = y;
    if (y > maxY) maxY = y;
}

void Paredes(int x, int y, int direcao) {
    int ox = getoffset(x);
    int oy = getoffset(y);

    if (direcao == 0) { 
        if (API::wallFront()) { 
            API::setWall(x, y, 'n'); mapGlobal[ox][oy].Norte = true; 
            if(oy+1 < 32) mapGlobal[ox][oy+1].Sul = true; 
        }
        if (API::wallRight()) { 
            API::setWall(x, y, 'e'); mapGlobal[ox][oy].Leste = true; 
            if(ox+1 < 32) mapGlobal[ox+1][oy].Oeste = true; 
        }
        if (API::wallLeft())  { 
            API::setWall(x, y, 'w'); mapGlobal[ox][oy].Oeste = true; 
            if(ox-1 >= 0) mapGlobal[ox-1][oy].Leste = true; 
        }
    } 
    else if (direcao == 1) { 
        if (API::wallFront()) { 
            API::setWall(x, y, 'e'); mapGlobal[ox][oy].Leste = true; 
            if(ox+1 < 32) mapGlobal[ox+1][oy].Oeste = true; 
        }
        if (API::wallRight()) { 
            API::setWall(x, y, 's'); mapGlobal[ox][oy].Sul = true;   
            if(oy-1 >= 0) mapGlobal[ox][oy-1].Norte = true; 
        }
        if (API::wallLeft())  { 
            API::setWall(x, y, 'n'); mapGlobal[ox][oy].Norte = true; 
            if(oy+1 < 32) mapGlobal[ox][oy+1].Sul = true; 
        }
    }
    else if (direcao == 2) { 
        if (API::wallFront()) { 
            API::setWall(x, y, 's'); mapGlobal[ox][oy].Sul = true;   
            if(oy-1 >= 0) mapGlobal[ox][oy-1].Norte = true; 
        }
        if (API::wallRight()) { 
            API::setWall(x, y, 'w'); mapGlobal[ox][oy].Oeste = true; 
            if(ox-1 >= 0) mapGlobal[ox-1][oy].Leste = true; 
        }
        if (API::wallLeft())  { 
            API::setWall(x, y, 'e'); mapGlobal[ox][oy].Leste = true; 
            if(ox+1 < 32) mapGlobal[ox+1][oy].Oeste = true; 
        }
    }
    else if (direcao == 3) { 
        if (API::wallFront()) { 
            API::setWall(x, y, 'w'); mapGlobal[ox][oy].Oeste = true; 
            if(ox-1 >= 0) mapGlobal[ox-1][oy].Leste = true; 
        }
        if (API::wallRight()) { 
            API::setWall(x, y, 'n'); mapGlobal[ox][oy].Norte = true; 
            if(oy+1 < 32) mapGlobal[ox][oy+1].Sul = true; 
        }
        if (API::wallLeft())  { 
            API::setWall(x, y, 's'); mapGlobal[ox][oy].Sul = true;   
            if(oy-1 >= 0) mapGlobal[ox][oy-1].Norte = true; 
        }
    }
}

bool estaEmArea2x2(int x, int y) {
    int cx[] = {x, x-1, x, x-1};
    int cy[] = {y, y, y-1, y-1};

    for (int i = 0; i < 4; i++) {
        int x0 = cx[i], y0 = cy[i];
        
        if (getoffset(x0) < 0 || getoffset(x0+1) >= 32 || getoffset(y0) < 0 || getoffset(y0+1) >= 32) continue;

        bool quadradoEsquerdoS = mapGlobal[getoffset(x0)][getoffset(y0)].visitado;
        bool quadradoEsquerdoN =  mapGlobal[getoffset(x0)][getoffset(y0+1)].visitado;

        bool quadradoDireitoS =  mapGlobal[getoffset(x0+1)][getoffset(y0)].visitado;
        bool quadradoDireitoN = mapGlobal[getoffset(x0+1)][getoffset(y0+1)].visitado;

        bool PossivelLocal2X2 = quadradoDireitoS && quadradoDireitoN && quadradoEsquerdoS && quadradoEsquerdoN;


        if (PossivelLocal2X2) {
            
            bool ParedeHorizontal = !mapGlobal[getoffset(x0)][getoffset(y0)].Norte && !mapGlobal[getoffset(x0+1)][getoffset(y0)].Norte;

            bool ParedeVertical = !mapGlobal[getoffset(x0)][getoffset(y0)].Leste && !mapGlobal[getoffset(x0)][getoffset(y0+1)].Leste;

            bool local2X2 = ParedeHorizontal && ParedeVertical;
            
            if (local2X2) {
                return true;
            }
        }
    }
    return false;
}

int buscarMelhorCaminho(int x, int y) {
    std::queue<Passo> fila;
    bool explorado[32][32] = {false};
    
    int dx[] = {0, 1, 0, -1}; 
    int dy[] = {1, 0, -1, 0};

    explorado[getoffset(x)][getoffset(y)] = true;


    for (int i = 0; i < 4; i++) {
        int ox = getoffset(x);
        int oy = getoffset(y);
        bool paredes[] = {mapGlobal[ox][oy].Norte, mapGlobal[ox][oy].Leste, mapGlobal[ox][oy].Sul, mapGlobal[ox][oy].Oeste};
        
        if (!paredes[i]) {
            int xv = x + dx[i];
            int yv = y + dy[i];
            if (getoffset(xv) >= 0 && getoffset(xv) < 32 && getoffset(yv) >= 0 && getoffset(yv) < 32) {
                if (!mapGlobal[getoffset(xv)][getoffset(yv)].visitado){ 
                    API::setColor(xv,yv,'R');
                    return i;
                }
                fila.push({xv, yv, i});
                explorado[getoffset(xv)][getoffset(yv)] = true;
            }
        }
    }

    while (!fila.empty()) {
        Passo atual = fila.front();
        fila.pop();

        for (int i = 0; i < 4; i++) {
            int nx = atual.x + dx[i];
            int ny = atual.y + dy[i];
            int cox = getoffset(atual.x);
            int coy = getoffset(atual.y);
            
            if (getoffset(nx) >= 0 && getoffset(nx) < 32 && getoffset(ny) >= 0 && getoffset(ny) < 32 && !explorado[getoffset(nx)][getoffset(ny)]) {
                bool paredes[] = {mapGlobal[cox][coy].Norte, mapGlobal[cox][coy].Leste, mapGlobal[cox][coy].Sul, mapGlobal[cox][coy].Oeste};
               
                if (!paredes[i]) {
                    if (!mapGlobal[getoffset(nx)][getoffset(ny)].visitado){ 
                        API::setColor(nx,ny,'R'); 
                        return atual.dirOriginal;
                    }
                    explorado[getoffset(nx)][getoffset(ny)] = true;
                    fila.push({nx, ny, atual.dirOriginal});
                }
            }
        }
    }
    return -1;
}

void walkMin(){
    atualizarLimitesMapa(posX, posY);
    while (true) {
    mapGlobal[getoffset(posX)][getoffset(posY)].visitado = true;
    API::setColor(posX, posY, 'G');
    
    Paredes(posX, posY, direcao);

    int proxDir = buscarMelhorCaminho(posX, posY);
    if(estaEmArea2x2(posX, posY)){
        break;
    }
    if (proxDir == -1) break;

    int diff = (proxDir - direcao + 4) % 4;
    if (diff == 1) API::turnRight();

    else if (diff == 2) {
         API::turnRight(); 
         API::turnRight(); 
    }

    else if (diff == 3) API::turnLeft();

    API::moveForward();
    direcao = proxDir;
    if (direcao == 0) posY++;
    else if (direcao == 1) posX++;
    else if (direcao == 2) posY--;
    else if (direcao == 3) posX--;
    }
}

int buscarCaminhoParaAlvo(int x, int y, int alvoX, int alvoY) {
    if (x == alvoX && y == alvoY) return -1;
    std::queue<Passo> fila;
    bool explorado[32][32] = {false};
    int dx[] = {0, 1, 0, -1}; int dy[] = {1, 0, -1, 0};
    explorado[getoffset(x)][getoffset(y)] = true;

    for (int i = 0; i < 4; i++) {
        int ox = getoffset(x); int oy = getoffset(y);
        bool paredes[] = {mapGlobal[ox][oy].Norte, mapGlobal[ox][oy].Leste, mapGlobal[ox][oy].Sul, mapGlobal[ox][oy].Oeste};
        if (!paredes[i]) {
            int xv = x + dx[i]; int yv = y + dy[i];
            if (xv == alvoX && yv == alvoY) return i;
            fila.push({xv, yv, i});
            explorado[getoffset(xv)][getoffset(yv)] = true;
        }
    }
    while (!fila.empty()) {
        Passo atual = fila.front(); fila.pop();
        for (int i = 0; i < 4; i++) {
            int nx = atual.x + dx[i]; int ny = atual.y + dy[i];
            int cox = getoffset(atual.x); int coy = getoffset(atual.y);

            if (getoffset(nx) >= 0 && getoffset(nx) < 32 && !explorado[getoffset(nx)][getoffset(ny)]) {
                bool paredes[] = {mapGlobal[cox][coy].Norte, mapGlobal[cox][coy].Leste, mapGlobal[cox][coy].Sul, mapGlobal[cox][coy].Oeste};
                if (!paredes[i]) {
                    if (nx == alvoX && ny == alvoY) return atual.dirOriginal;
                    explorado[getoffset(nx)][getoffset(ny)] = true;
                    fila.push({nx, ny, atual.dirOriginal});
                }
            }
        }
    }
    return -1;
}

void moverParaDirecao(int proxDir) {
    int diff = (proxDir - direcao + 4) % 4;
    if (diff == 1) API::turnRight();

    else if (diff == 2) { 
        API::turnRight(); 
        API::turnRight(); 
    }

    else if (diff == 3) API::turnLeft();

    API::moveForward();
    direcao = proxDir;
    if (direcao == 0) posY++;
    else if (direcao == 1) posX++;
    else if (direcao == 2) posY--;
    else if (direcao == 3) posX--;

    atualizarLimitesMapa(posX, posY);
}


void walkMax(){
    atualizarLimitesMapa(posX, posY);
    chegou = false;
    while (true) {
        mapGlobal[getoffset(posX)][getoffset(posY)].visitado = true;
        API::setColor(posX, posY, 'B');
        
        if(!achouCentro && estaEmArea2x2(posX, posY)) {
            centroX = posX;
            centroY = posY;
            achouCentro = true;
        }

        Paredes(posX, posY, direcao);

        int proxDir = buscarMelhorCaminho(posX, posY);
        
        if (proxDir == -1) {
            break;
        }

        moverParaDirecao(proxDir);
    }

    if (achouCentro) {
        while (posX != centroX || posY != centroY) {
            int proxDir = buscarCaminhoParaAlvo(posX, posY, centroX, centroY);
            chegou = true;
            if (proxDir == -1) break;
            moverParaDirecao(proxDir);
        }
        API::setColor(posX, posY, 'G');
    }
}