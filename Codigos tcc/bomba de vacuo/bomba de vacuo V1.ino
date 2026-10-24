/*
  Teste da bomba de vacuo + valvula via rele (ESP32)

  Ligacao:
    GPIO 23 -> IN do rele
    GND ESP32 -> DC- do rele (e GND da fonte)
    3V3 fonte -> DC+ do rele
    5V fonte -> COM do rele | NO do rele -> + bomba e + valvula

  Comandos pelo Monitor Serial (115200 baud, "Nova linha" ativada):
    l  -> liga a bomba (segura a peca)
    d  -> desliga a bomba (solta a peca)
    a  -> liga/desliga o modo automatico (ciclo de teste)
*/

const int PIN_RELE = 23;

// Jumper do modulo em "High": true. Em "Low": coloque false.
const bool RELE_ATIVO_EM_HIGH = true;

// Tempos do modo automatico (milissegundos)
const unsigned long TEMPO_LIGADO = 4000;
const unsigned long TEMPO_DESLIGADO = 4000;

bool bombaLigada = false;
bool modoAuto = false;
unsigned long ultimaTroca = 0;

void setBomba(bool ligar) {
  bombaLigada = ligar;
  bool nivel = RELE_ATIVO_EM_HIGH ? ligar : !ligar;
  digitalWrite(PIN_RELE, nivel ? HIGH : LOW);
  Serial.println(ligar ? "Bomba LIGADA (segurando)" : "Bomba DESLIGADA (solta)");
}

void setup() {
  Serial.begin(115200);

  // Define o nivel de desligado ANTES de configurar como saida,
  // para o rele nao dar um clique na inicializacao.
  digitalWrite(PIN_RELE, RELE_ATIVO_EM_HIGH ? LOW : HIGH);
  pinMode(PIN_RELE, OUTPUT);
  setBomba(false);

  Serial.println();
  Serial.println("=== Teste da bomba de vacuo ===");
  Serial.println("l = liga | d = desliga | a = modo automatico");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();

    if (c == 'l') {
      modoAuto = false;
      setBomba(true);
    } else if (c == 'd') {
      modoAuto = false;
      setBomba(false);
    } else if (c == 'a') {
      modoAuto = !modoAuto;
      Serial.println(modoAuto ? "Modo automatico ATIVADO" : "Modo automatico DESATIVADO");
      if (modoAuto) {
        setBomba(true);
        ultimaTroca = millis();
      } else {
        setBomba(false);
      }
    }
  }

  if (modoAuto) {
    unsigned long espera = bombaLigada ? TEMPO_LIGADO : TEMPO_DESLIGADO;
    if (millis() - ultimaTroca >= espera) {
      setBomba(!bombaLigada);
      ultimaTroca = millis();
    }
  }
}
