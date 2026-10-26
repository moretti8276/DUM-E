/*
  Teste dos servos no PCA9685 com controle visual (sliders no navegador)

  Os servos dos canais 1 e 2 sao um par espelhado: movem ao mesmo tempo
  e em sentidos opostos (se um vai a 60 graus, o outro vai a 120).

  Ligacao:
    PCA9685 VCC -> 3V3 | GND -> GND (comum com a fonte)
    PCA9685 SDA -> GPIO 21 | SCL -> GPIO 22
    PCA9685 V+ (borne) -> 5V a 6V da fonte (alimentacao dos servos)

    Modulo rele (com trigger selecionado em HIGH):
      VCC (3,3V, apenas se o modulo suportar) -> 3V3 do ESP32
      GND -> GND do ESP32
      IN/SINAL -> GPIO 18 do ESP32
      COM/NO -> circuito de alimentacao do conjunto bomba + valvula
      A bomba e a valvula devem usar fonte apropriada, NUNCA pino do ESP32.
      Confira o esquema e as tensoes do kit antes de energizar.

  Como usar:
    1. Grave o codigo e ligue a fonte dos servos.
    2. No PC ou celular, conecte no Wi-Fi "DUM-E" (senha 12345678).
    3. Abra no navegador: http://192.168.4.1
    4. Mexa os sliders para controlar cada servo.
       O slider do par controla os canais 1 e 2 juntos.
    5. Clique em "Ativar ventosa" para ligar o rele; clique novamente
       em "Desativar ventosa" para desligar. Inicia sempre desligada.

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

// GPIO livre para controle digital do rele (sem PWM).
const uint8_t PIN_RELE_VENTOSA = 18;
// Jumper do rele em HIGH: HIGH = ligado, LOW = desligado.
// Se selecionar LOW no jumper, altere para LOW aqui tambem.
const uint8_t RELE_NIVEL_ATIVO = HIGH;

const char* AP_SSID = "DUM-E";
const char* AP_PASS = "12345678";

const uint8_t NUM_SERVOS = 5;
// Canais do PCA9685 onde cada servo esta ligado (troque se precisar)
const uint8_t CANAIS[NUM_SERVOS] = {0, 1, 2, 3, 4};

// Par espelhado: posicoes no vetor CANAIS (aqui coincidem com os canais 1 e 2).
// O servo B sempre fica em 180 - angulo do servo A.
const uint8_t PAR_A = 1;
const uint8_t PAR_B = 2;

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
bool ventosaLigada = false;

// Liga/desliga a alimentacao do conjunto atraves de um UNICO rele.
void setVentosa(bool ligar) {
  digitalWrite(PIN_RELE_VENTOSA,
               ligar ? RELE_NIVEL_ATIVO : (RELE_NIVEL_ATIVO == HIGH ? LOW : HIGH));
  ventosaLigada = ligar;
  Serial.println(ligar ? "Ventosa: LIGADA" : "Ventosa: DESLIGADA");
}

// Limites efetivos do servo. Para o servo A do par, leva em conta
// os limites do servo B (que se move ao contrario).
int limMin(int i) {
  if (i == PAR_A) return max(ANG_MIN[PAR_A], 180 - ANG_MAX[PAR_B]);
  return ANG_MIN[i];
}

int limMax(int i) {
  if (i == PAR_A) return min(ANG_MAX[PAR_A], 180 - ANG_MIN[PAR_B]);
  return ANG_MAX[i];
}

void escreveServo(int i, int ang) {
  anguloAtual[i] = ang;
  int us = map(ang, 0, 180, US_MIN, US_MAX);
  pwm.writeMicroseconds(CANAIS[i], us);
}

void setAngulo(int i, int ang) {
  // Pedir um angulo para o servo B equivale a pedir o espelho para o A
  if (i == PAR_B) {
    i = PAR_A;
    ang = 180 - ang;
  }

  ang = constrain(ang, limMin(i), limMax(i));
  escreveServo(i, ang);

  // Servo B acompanha o A ao mesmo tempo, em sentido oposto
  if (i == PAR_A) escreveServo(PAR_B, 180 - ang);
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
  button:disabled { opacity:0.55; cursor:wait; }
  .ventosa-card { border-color:#344453; margin-top:16px; }
  .ventosa-topo { display:flex; align-items:center; justify-content:space-between; gap:12px; margin-bottom:12px; }
  .ventosa-topo strong { font-size:16px; }
  .ventosa-estado { display:inline-flex; align-items:center; gap:7px; color:#8b949e; font-size:13px; }
  .ventosa-led { width:9px; height:9px; background:#6e7681; border-radius:50%; display:inline-block; }
  .ventosa-led.ativa { background:#3fb950; box-shadow:0 0 8px #3fb95099; }
  .ventosa-estado.ativa { color:#3fb950; }
  #btnVentosa { display:block; width:100%; padding:13px 18px; background:#238636; font-weight:600; }
  #btnVentosa.ativa { background:#b34343; }
  #btnVentosa.ativa:active { background:#c85050; }
  .ajuda { font-size:12px; color:#8b949e; margin:10px 0 0; }
</style>
</head>
<body>
  <h1>Teste de Servos - PCA9685</h1>
  <p class="sub">Arraste os sliders para mover cada servo.</p>
  <div id="box"></div>
  <button onclick="centralizar()">Centralizar tudo (90&deg;)</button>

  <section class="card ventosa-card" aria-label="Controle da ventosa">
    <div class="ventosa-topo">
      <strong>Ventosa a vácuo</strong>
      <span class="ventosa-estado" id="estadoVentosa" role="status" aria-live="polite">
        <span class="ventosa-led" id="ledVentosa"></span>
        <span id="textoVentosa">Carregando...</span>
      </span>
    </div>
    <button id="btnVentosa" type="button" disabled onclick="alternarVentosa()">Ativar ventosa</button>
    <p class="ajuda">Um toque liga a bomba/valvula pelo rele; outro toque desliga.</p>
  </section>

<script>
let st = null;
let ultimoEnvio = 0;
let ventosaLigada = false;
let ventosaOcupada = false;

function mostrarVentosa(ligada) {
  ventosaLigada = !!ligada;
  const botao = document.getElementById('btnVentosa');
  const estado = document.getElementById('estadoVentosa');
  const led = document.getElementById('ledVentosa');
  botao.textContent = ventosaLigada ? 'Desativar ventosa' : 'Ativar ventosa';
  botao.classList.toggle('ativa', ventosaLigada);
  estado.classList.toggle('ativa', ventosaLigada);
  led.classList.toggle('ativa', ventosaLigada);
  document.getElementById('textoVentosa').textContent = ventosaLigada ? 'LIGADA' : 'DESLIGADA';
  if (!ventosaOcupada) botao.disabled = false;
}

async function alternarVentosa() {
  if (ventosaOcupada) return;
  ventosaOcupada = true;
  const botao = document.getElementById('btnVentosa');
  botao.disabled = true;
  try {
    const proximoEstado = ventosaLigada ? 0 : 1;
    const resposta = await fetch('/ventosa?on=' + proximoEstado, {method:'POST'});
    if (!resposta.ok) throw new Error('Falha ao comandar rele');
    const dados = await resposta.json();
    mostrarVentosa(dados.ventosa);
  } catch (e) {
    document.getElementById('textoVentosa').textContent = 'ERRO DE CONEXAO';
    console.error(e);
  } finally {
    ventosaOcupada = false;
    botao.disabled = false;
  }
}

function enviar(i, ang, forcar) {
  const agora = Date.now();
  if (!forcar && agora - ultimoEnvio < 40) return;
  ultimoEnvio = agora;
  fetch('/set?i=' + i + '&a=' + ang).catch(() => {});
}

function textoValor(i, a) {
  if (i === st.par[0]) return a + '\u00B0 / ' + (180 - a) + '\u00B0';
  return a + '\u00B0';
}

async function atualizar() {
  st = await (await fetch('/state')).json();
  mostrarVentosa(st.ventosa);
  const parA = st.par[0];
  const parB = st.par[1];
  const box = document.getElementById('box');
  if (box.children.length === 0) {
    st.canais.forEach((c, i) => {
      if (i === parB) return; // o par tem um slider so
      const d = document.createElement('div');
      d.className = 'card';
      const nome = (i === parA)
        ? 'Servos ' + parA + ' e ' + parB + ' <small>(canais ' + c + ' e ' + st.canais[parB] + ', espelhados)</small>'
        : 'Servo ' + i + ' <small>(canal ' + c + ')</small>';
      d.innerHTML =
        '<div class="topo"><span>' + nome + '</span>' +
        '<span class="valor" id="v' + i + '"></span></div>' +
        '<input type="range" id="s' + i + '" min="' + st.min[i] + '" max="' + st.max[i] + '">';
      box.appendChild(d);
      const s = d.querySelector('input');
      s.addEventListener('input', () => {
        document.getElementById('v' + i).textContent = textoValor(i, parseInt(s.value));
        enviar(i, s.value, false);
      });
      s.addEventListener('change', () => enviar(i, s.value, true));
    });
  }
  st.ang.forEach((a, i) => {
    if (i === parB) return;
    document.getElementById('s' + i).value = a;
    document.getElementById('v' + i).textContent = textoValor(i, a);
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
  for (int i = 0; i < NUM_SERVOS; i++) { if (i) j += ","; j += limMin(i); }
  j += "],\"max\":[";
  for (int i = 0; i < NUM_SERVOS; i++) { if (i) j += ","; j += limMax(i); }
  j += "],\"par\":[";
  j += PAR_A;
  j += ",";
  j += PAR_B;
  j += "],\"ventosa\":";
  j += (ventosaLigada ? "true" : "false");
  j += "}";
  server.send(200, "application/json", j);
}

// Recebe POST /ventosa?on=1 (liga) ou /ventosa?on=0 (desliga).
void handleVentosa() {
  if (!server.hasArg("on") ||
      (server.arg("on") != "0" && server.arg("on") != "1")) {
    server.send(400, "text/plain", "use on=0 ou on=1");
    return;
  }
  setVentosa(server.arg("on") == "1");
  server.send(200, "application/json", ventosaLigada ? "{\"ventosa\":true}" : "{\"ventosa\":false}");
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
    if (i == PAR_B) continue; // o B acompanha o A
    setAngulo(i, ANG_INICIAL);
    delay(100);
  }
  server.send(200, "text/plain", "ok");
}

void setup() {
  Serial.begin(115200);

  // Inicializa o rele em DESLIGADO antes dos servos e do Wi-Fi.
  digitalWrite(PIN_RELE_VENTOSA, RELE_NIVEL_ATIVO == HIGH ? LOW : HIGH);
  pinMode(PIN_RELE_VENTOSA, OUTPUT);
  setVentosa(false);

  Wire.begin(PIN_SDA, PIN_SCL);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  // Servos iniciam em 90 graus, um de cada vez para evitar pico de corrente.
  // O par espelhado (A e B) parte junto.
  for (int i = 0; i < NUM_SERVOS; i++) {
    if (i == PAR_B) continue;
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
  server.on("/ventosa", HTTP_POST, handleVentosa);
  server.begin();
}

void loop() {
  server.handleClient();
}
