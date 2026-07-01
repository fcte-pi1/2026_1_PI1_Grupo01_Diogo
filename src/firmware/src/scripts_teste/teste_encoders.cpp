#include <Arduino.h>
#include "../sensores/encoders.h" // Puxa a sua biblioteca que validamos

void setup() {
    Serial.begin(115200);
    encodersInit(); // Inicializa os pinos e o PCNT
    
    Serial.println("===============================");
    Serial.println("   TESTE DE ENCODERS INICIADO  ");
    Serial.println("===============================");
    Serial.println("Gire as rodas com a mao para ver a contagem...");
}

void loop() {
    // Lê os pulsos puros do hardware
    int32_t p_esq = encoderLerEsquerdo();
    int32_t p_dir = encoderLerDireito();
    
    // Imprime na tela
    Serial.print("ESQUERDA: ");
    Serial.print(p_esq);
    Serial.print("\t|\tDIREITA: ");
    Serial.println(p_dir);

    delay(100); // Pausa de 100ms para nao travar o terminal
}