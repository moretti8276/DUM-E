/*
  Teste do sensor de cor TCS34725 (ESP32)
  Detecta: VERMELHO, VERDE, AZUL

  Ligacao:
    VIN -> 3V3 | GND -> GND
    SDA -> GPIO 21 | SCL -> GPIO 22
    LED -> GPIO 25 (liga o LED branco do sensor)

  Biblioteca necessaria (Gerenciador de Bibliotecas):
    "Adafruit TCS34725" (instale tambem a Adafruit BusIO se pedir)

  Abra o Monitor Serial em 115200 baud.
*/

#include <Wire.h>
#include <Adafruit_TCS34725.h>

const int PIN_LED = 25;
const int PIN_SDA = 21;
const int PIN_SCL = 22;

// Se o LED do sensor ficar apagado, troque para false (depende do modulo)
const bool LED_ATIVO_EM_HIGH = true;

// Ajustes de deteccao
const uint16_t CLEAR_MINIMO = 200;   // abaixo disso, considera "sem objeto"
const float MARGEM = 1.25;           // a cor dominante precisa ser 25% maior que as outras

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

void setLed(bool ligado) {
  digitalWrite(PIN_LED, (ligado == LED_ATIVO_EM_HIGH) ? HIGH : LOW);
}

const char* detectaCor(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
  if (c < CLEAR_MINIMO) return "NENHUMA";

  if (r > g * MARGEM && r > b * MARGEM) return "VERMELHO";
  if (g > r * MARGEM && g > b * MARGEM) return "VERDE";
  if (b > r * MARGEM && b > g * MARGEM) return "AZUL";

  return "INDEFINIDA";
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  setLed(true);

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!tcs.begin()) {
    Serial.println("Sensor TCS34725 NAO encontrado! Confira SDA, SCL e a alimentacao 3V3.");
    while (true) delay(1000);
  }

  Serial.println("Sensor encontrado. Aproxime um objeto vermelho, verde ou azul.");
}

void loop() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  Serial.print("R: "); Serial.print(r);
  Serial.print("  G: "); Serial.print(g);
  Serial.print("  B: "); Serial.print(b);
  Serial.print("  Clear: "); Serial.print(c);
  Serial.print("  -> ");
  Serial.println(detectaCor(r, g, b, c));

  delay(500);
}
