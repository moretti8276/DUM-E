/*
  Teste dos servos no PCA9685 com controle visual (sliders no navegador)

  Ligacao:
    PCA9685 VCC -> 3V3 | GND -> GND (comum com a fonte)
    PCA9685 SDA -> GPIO 21 | SCL -> GPIO 22
    PCA9685 V+ (borne) -> 5V a 6V da fonte (alimentacao dos servos)

  Como usar:
    1. Grave o codigo e ligue a fonte dos servos.
    2. No PC ou celular, conecte no Wi-Fi "DUM-E" (senha 12345678).
    3. Abra no navegador: http://192.168.4.1
    4. Mexa os sliders para controlar cada servo.

  Biblioteca necessaria (Gerenciador de Bibliotecas):
    "Adafruit PWM Servo Driver Library"
*/

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_PWMServoDriver.h>

// ---------- Configuracao ----------
const int PIN_SDA = 21;
const int PIN_SCL = 22;

const char* AP_SSID = "DUM-E";
const char* AP_PASS = "12345678";

const uint8_t NUM_SERVOS = 5;
// Canais do PCA9685 onde cada servo esta ligado (troque se precisar)
const uint8_t CANAIS[NUM_SERVOS] = {0, 1, 2, 3, 4};

// Limites de seguranca de cada servo (graus)
const int ANG_MIN[NUM_SERVOS] = {0, 0, 0, 0, 0};
const int ANG_MAX[NUM_SERVOS] = {180, 180, 180, 180, 180};

const int ANG_INICIAL = 90;

// Pulso do servo (microssegundos) para 0 e 180 graus
const int US_MIN = 500;
const int US_MAX = 2500;
// ----------------------------------

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
WebServer server(80);
int anguloAtual[NUM_SERVOS];

void setAngulo(int i, int ang) {
  ang = constrain(ang, ANG_MIN[i], ANG_MAX[i]);
  anguloAtual[i] = ang;
  int us = map(ang, 0, 180, US_MIN, US_MAX);
  pwm.writeMicroseconds(CANAIS[i], us);
}

const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Teste de Servos</title>
<style>
  body { background:#0e1116; color:#e6edf3; font-family:system-ui,sans-serif; margin:0; padding:20px; }
  h1 { font-size:20px; margin:0 0 4px; }
  p.sub { color:#8b949e; margin:0 0 20px; font-size:14px; }
  .card { background:#161b22; border:1px solid #30363d; border-radius:8px; padding:14px 16px; margin-bottom:12px; max-width:520px; }
  .topo { display:flex; justify-content:space-between; align-items:baseline; margin-bottom:8px; }
  .topo small { color:#8b949e; }
  .valor { font-family:monospace; font-size:20px; color:#58a6ff; }
  input[type=range] { width:100%; accent-color:#58a6ff; }
  button { background:#238636; color:#fff; border:0; border-radius:6px; padding:10px 18px; font-size:15px; cursor:pointer; }
  button:active { background:#2ea043; }
</style>
</head>
<body>
  <h1>Teste de Servos - PCA9685</h1>
  <p class="sub">Arraste os sliders para mover cada servo.</p>
  <div id="box"></div>
  <button onclick="centralizar()">Centralizar tudo (90&deg;)</button>

<script>
let st = null;
let ultimoEnvio = 0;

function enviar(i, ang, forcar) {
  const agora = Date.now();
  if (!forcar && agora - ultimoEnvio < 40) return;
  ultimoEnvio = agora;
  fetch('/set?i=' + i + '&a=' + ang).catch(() => {});
}

async function atualizar() {
  st = await (await fetch('/state')).json();
  const box = document.getElementById('box');
  if (box.children.length === 0) {
    st.canais.forEach((c, i) => {
      const d = document.createElement('div');
      d.className = 'card';
      d.innerHTML =
        '<div class="topo"><span>Servo ' + i + ' <small>(canal ' + c + ')</small></span>' +
        '<span class="valor" id="v' + i + '"></span></div>' +
        '<input type="range" id="s' + i + '" min="' + st.min[i] + '" max="' + st.max[i] + '">';
      box.appendChild(d);
      const s = d.querySelector('input');
      s.addEventListener('input', () => {
        document.getElementById('v' + i).textContent = s.value + '\u00B0';
        enviar(i, s.value, false);
      });
      s.addEventListener('change', () => enviar(i, s.value, true));
    });
  }
  st.ang.forEach((a, i) => {
    document.getElementById('s' + i).value = a;
    document.getElementById('v' + i).textContent = a + '\u00B0';
  });
}

async function centralizar() {
  await fetch('/center');
  atualizar();
}

atualizar();
</script>
</body>
</html>
)rawliteral";

void handleRaiz() {
  server.send_P(200, "text/html; charset=utf-8", PAGINA);
}

void handleState() {
  String j = "{\"canais\":[";
  for (int i = 0; i < NUM_SERVOS; i++) { if (i) j += ","; j += CANAIS[i]; }
  j += "],\"ang\":[";
  for (int i = 0; i < NUM_SERVOS; i++) { if (i) j += ","; j += anguloAtual[i]; }
  j += "],\"min\":[";
  for (int i = 0; i < NUM_SERVOS; i++) { if (i) j += ","; j += ANG_MIN[i]; }
  j += "],\"max\":[";
  for (int i = 0; i < NUM_SERVOS; i++) { if (i) j += ","; j += ANG_MAX[i]; }
  j += "]}";
  server.send(200, "application/json", j);
}

void handleSet() {
  if (!server.hasArg("i") || !server.hasArg("a")) {
    server.send(400, "text/plain", "faltam parametros");
    return;
  }
  int i = server.arg("i").toInt();
  int a = server.arg("a").toInt();
  if (i < 0 || i >= NUM_SERVOS) {
    server.send(400, "text/plain", "servo invalido");
    return;
  }
  setAngulo(i, a);
  server.send(200, "text/plain", "ok");
}

void handleCenter() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    setAngulo(i, ANG_INICIAL);
    delay(100);
  }
  server.send(200, "text/plain", "ok");
}

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  // Servos iniciam em 90 graus, um de cada vez para evitar pico de corrente
  for (int i = 0; i < NUM_SERVOS; i++) {
    setAngulo(i, ANG_INICIAL);
    delay(300);
  }
  Serial.println("Servos em 90 graus.");

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("Wi-Fi: ");
  Serial.print(AP_SSID);
  Serial.print(" | Acesse: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRaiz);
  server.on("/state", handleState);
  server.on("/set", handleSet);
  server.on("/center", handleCenter);
  server.begin();
}

void loop() {
  server.handleClient();
}
