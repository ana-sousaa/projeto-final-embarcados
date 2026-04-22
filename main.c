#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <LittleFS.h>

// =========================
// Mapeamento LAB01 oficial
// =========================
// IO1  -> LDR
// IO8  -> OLED_SDA
// IO9  -> OLED_SCL
// IO15 -> DHT11
// IO14 -> LED VERMELHO
// IO13 -> LED VERDE
// IO12 -> LED AZUL
// IO7  -> BT1
// IO6  -> BT2
// IO5  -> BT3

#define PIN_LDR        1
#define PIN_OLED_SDA   8
#define PIN_OLED_SCL   9
#define PIN_DHT        15

#define PIN_LED_RED    14   // atuador/aquecedor
#define PIN_LED_GREEN  13   // sistema ligado
#define PIN_LED_BLUE   12   // logging ligado

#define BTN_OK         7
#define BTN_UP         6
#define BTN_DOWN       5

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_ADDR      0x3C

#define LOG_FILE       "/log.txt"
#define SERIAL_BAUD    115200

// Em projeto real LAB01: DHT11.
// No Wokwi, normalmente o sensor disponível/documentado é o DHT22.
// Para simulação, este código usa DHTesp, que funciona bem no ESP32.
// Se usar DHT11 real na placa, ajuste conforme sua biblioteca/sensor.
DHTesp dht;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =========================
// Configurações do sistema
// =========================
float setPointTemp = 28.0f;
float hysteresis = 1.0f;

bool systemEnabled = true;
bool controlEnabled = true;
bool loggingEnabled = true;

bool heaterOn = false;

unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastLogWrite = 0;

const unsigned long SENSOR_INTERVAL_MS = 2000;
const unsigned long DISPLAY_INTERVAL_MS = 250;
const unsigned long LOG_INTERVAL_MS = 10000;

// =========================
// Dados medidos
// =========================
float temperatureC = NAN;
float humidityPct = NAN;
int ldrRaw = 0;
int lightPct = 0;

// =========================
// Interface/menu
// =========================
enum ScreenState {
  SCREEN_MAIN,
  SCREEN_MENU,
  SCREEN_EDIT_SETPOINT
};

ScreenState screenState = SCREEN_MAIN;

enum MenuItem {
  MENU_SETPOINT = 0,
  MENU_CONTROL,
  MENU_SYSTEM,
  MENU_LOGGING,
  MENU_EXIT,
  MENU_COUNT
};

int menuIndex = 0;

// =========================
// Botões com debounce
// =========================
struct Button {
  uint8_t pin;
  bool lastReading;
  bool stableState;
  unsigned long lastDebounceTime;
};

Button btnOk   = {BTN_OK, HIGH, HIGH, 0};
Button btnUp   = {BTN_UP, HIGH, HIGH, 0};
Button btnDown = {BTN_DOWN, HIGH, HIGH, 0};

const unsigned long debounceDelay = 40;

// =========================
// Protótipos
// =========================
void initPins();
void initDisplay();
void initFS();
void readSensors();
void updateControl();
void updateIndicators();
void drawMainScreen();
void drawMenuScreen();
void drawEditSetpointScreen();
void handleUI();
void handleSerial();
void appendLog();
void printStatusSerial();
void printHelpSerial();
void readLogSerial();
void clearLogSerial();
bool buttonPressed(Button &b);
String nowStamp();
int rawToPercent(int raw);

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  initPins();
  initDisplay();
  initFS();

  dht.setup(PIN_DHT, DHTesp::DHT22); 
  // Para LAB01 físico com DHT11, troque para DHTesp::DHT11
  // se a sua versão da biblioteca suportar, ou adapte para a biblioteca DHT.

  Serial.println("\n=== Sistema de Estufa - Franzininho WiFi / LAB01 ===");
  Serial.println("Inicializando...");
  printHelpSerial();
}

// =========================
// Loop
// =========================
void loop() {
  unsigned long now = millis();

  handleUI();
  handleSerial();

  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = now;
    readSensors();
    updateControl();
    updateIndicators();
  }

  if (loggingEnabled && (now - lastLogWrite >= LOG_INTERVAL_MS)) {
    lastLogWrite = now;
    appendLog();
  }

  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL_MS) {
    lastDisplayUpdate = now;

    switch (screenState) {
      case SCREEN_MAIN:
        drawMainScreen();
        break;
      case SCREEN_MENU:
        drawMenuScreen();
        break;
      case SCREEN_EDIT_SETPOINT:
        drawEditSetpointScreen();
        break;
    }
  }
}

// =========================
// Inicializações
// =========================
void initPins() {
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);

  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
}

void initDisplay() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Falha ao inicializar OLED");
    while (true) delay(10);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void initFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("Falha ao montar LittleFS");
    return;
  }

  if (!LittleFS.exists(LOG_FILE)) {
    File f = LittleFS.open(LOG_FILE, FILE_WRITE);
    if (f) {
      f.println("timestamp,temp_c,umid_pct,ldr_raw,luz_pct,setpoint,system,control,heater");
      f.close();
    }
  }
}

// =========================
// Sensores e controle
// =========================
void readSensors() {
  TempAndHumidity data = dht.getTempAndHumidity();

  if (!isnan(data.temperature)) {
    temperatureC = data.temperature;
  }

  if (!isnan(data.humidity)) {
    humidityPct = data.humidity;
  }

  ldrRaw = analogRead(PIN_LDR);
  lightPct = rawToPercent(ldrRaw);
}

void updateControl() {
  if (!systemEnabled || !controlEnabled || isnan(temperatureC)) {
    heaterOn = false;
    return;
  }

  // Controle simples com histerese
  if (temperatureC < (setPointTemp - hysteresis)) {
    heaterOn = true;
  } else if (temperatureC > (setPointTemp + hysteresis)) {
    heaterOn = false;
  }
}

void updateIndicators() {
  digitalWrite(PIN_LED_RED, heaterOn ? HIGH : LOW);
  digitalWrite(PIN_LED_GREEN, systemEnabled ? HIGH : LOW);
  digitalWrite(PIN_LED_BLUE, loggingEnabled ? HIGH : LOW);
}

int rawToPercent(int raw) {
  // ADC ESP32 costuma ir de 0 a 4095
  int pct = map(raw, 0, 4095, 0, 100);
  pct = constrain(pct, 0, 100);
  return pct;
}

// =========================
// Interface OLED
// =========================
void drawMainScreen() {
  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("ESTUFA - MONITOR");

  display.setCursor(0, 12);
  display.print("Temp: ");
  if (isnan(temperatureC)) display.println("--.- C");
  else {
    display.print(temperatureC, 1);
    display.println(" C");
  }

  display.setCursor(0, 22);
  display.print("Umid: ");
  if (isnan(humidityPct)) display.println("--.- %");
  else {
    display.print(humidityPct, 1);
    display.println(" %");
  }

  display.setCursor(0, 32);
  display.print("Luz : ");
  display.print(lightPct);
  display.println(" %");

  display.setCursor(0, 42);
  display.print("SP  : ");
  display.print(setPointTemp, 1);
  display.println(" C");

  display.setCursor(0, 52);
  display.print(systemEnabled ? "SYS:ON " : "SYS:OFF");
  display.print(controlEnabled ? " CTRL:ON " : " CTRL:OFF");
  display.print(heaterOn ? "H:1" : "H:0");

  display.display();
}

void drawMenuScreen() {
  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("MENU");

  const char* items[MENU_COUNT] = {
    "SetPoint",
    "Controle",
    "Sistema",
    "Logging",
    "Sair"
  };

  for (int i = 0; i < MENU_COUNT; i++) {
    display.setCursor(0, 12 + i * 10);

    if (i == menuIndex) display.print(">");
    else display.print(" ");

    display.print(items[i]);

    if (i == MENU_CONTROL) {
      display.print(":");
      display.print(controlEnabled ? "ON" : "OFF");
    } else if (i == MENU_SYSTEM) {
      display.print(":");
      display.print(systemEnabled ? "ON" : "OFF");
    } else if (i == MENU_LOGGING) {
      display.print(":");
      display.print(loggingEnabled ? "ON" : "OFF");
    }
  }

  display.display();
}

void drawEditSetpointScreen() {
  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("AJUSTE SETPOINT");

  display.setTextSize(2);
  display.setCursor(12, 20);
  display.print(setPointTemp, 1);
  display.print("C");

  display.setTextSize(1);
  display.setCursor(0, 54);
  display.println("UP/DN ajusta | OK salva");

  display.display();
}

// =========================
// Botões / UI
// =========================
bool buttonPressed(Button &b) {
  bool reading = digitalRead(b.pin);

  if (reading != b.lastReading) {
    b.lastDebounceTime = millis();
  }

  if ((millis() - b.lastDebounceTime) > debounceDelay) {
    if (reading != b.stableState) {
      b.stableState = reading;
      b.lastReading = reading;

      // botão pressionado em INPUT_PULLUP => LOW
      if (b.stableState == LOW) {
        return true;
      }
    }
  }

  b.lastReading = reading;
  return false;
}

void handleUI() {
  bool okPressed = buttonPressed(btnOk);
  bool upPressed = buttonPressed(btnUp);
  bool downPressed = buttonPressed(btnDown);

  switch (screenState) {
    case SCREEN_MAIN:
      if (okPressed) {
        screenState = SCREEN_MENU;
      }
      break;

    case SCREEN_MENU:
      if (upPressed) {
        menuIndex--;
        if (menuIndex < 0) menuIndex = MENU_COUNT - 1;
      }

      if (downPressed) {
        menuIndex++;
        if (menuIndex >= MENU_COUNT) menuIndex = 0;
      }

      if (okPressed) {
        switch (menuIndex) {
          case MENU_SETPOINT:
            screenState = SCREEN_EDIT_SETPOINT;
            break;

          case MENU_CONTROL:
            controlEnabled = !controlEnabled;
            break;

          case MENU_SYSTEM:
            systemEnabled = !systemEnabled;
            if (!systemEnabled) heaterOn = false;
            break;

          case MENU_LOGGING:
            loggingEnabled = !loggingEnabled;
            break;

          case MENU_EXIT:
            screenState = SCREEN_MAIN;
            break;
        }
      }
      break;

    case SCREEN_EDIT_SETPOINT:
      if (upPressed) {
        setPointTemp += 0.5f;
        if (setPointTemp > 50.0f) setPointTemp = 50.0f;
      }

      if (downPressed) {
        setPointTemp -= 0.5f;
        if (setPointTemp < 10.0f) setPointTemp = 10.0f;
      }

      if (okPressed) {
        screenState = SCREEN_MENU;
      }
      break;
  }
}

// =========================
// Serial
// =========================
void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "help") {
    printHelpSerial();
  } else if (cmd == "status") {
    printStatusSerial();
  } else if (cmd == "readlog") {
    readLogSerial();
  } else if (cmd == "clearlog") {
    clearLogSerial();
  } else if (cmd == "log on") {
    loggingEnabled = true;
    Serial.println("Logging habilitado.");
  } else if (cmd == "log off") {
    loggingEnabled = false;
    Serial.println("Logging desabilitado.");
  } else if (cmd == "ctrl on") {
    controlEnabled = true;
    Serial.println("Controle habilitado.");
  } else if (cmd == "ctrl off") {
    controlEnabled = false;
    heaterOn = false;
    Serial.println("Controle desabilitado.");
  } else if (cmd == "system on") {
    systemEnabled = true;
    Serial.println("Sistema habilitado.");
  } else if (cmd == "system off") {
    systemEnabled = false;
    heaterOn = false;
    Serial.println("Sistema desabilitado.");
  } else {
    Serial.println("Comando desconhecido. Digite: help");
  }
}

void printHelpSerial() {
  Serial.println("\nComandos disponiveis:");
  Serial.println("  help");
  Serial.println("  status");
  Serial.println("  readlog");
  Serial.println("  clearlog");
  Serial.println("  log on");
  Serial.println("  log off");
  Serial.println("  ctrl on");
  Serial.println("  ctrl off");
  Serial.println("  system on");
  Serial.println("  system off");
}

void printStatusSerial() {
  Serial.println("\n=== STATUS ===");
  Serial.print("Temperatura: ");
  if (isnan(temperatureC)) Serial.println("N/A");
  else Serial.println(String(temperatureC, 1) + " C");

  Serial.print("Umidade: ");
  if (isnan(humidityPct)) Serial.println("N/A");
  else Serial.println(String(humidityPct, 1) + " %");

  Serial.print("Luminosidade: ");
  Serial.println(String(lightPct) + " %");

  Serial.print("SetPoint: ");
  Serial.println(String(setPointTemp, 1) + " C");

  Serial.print("Sistema: ");
  Serial.println(systemEnabled ? "ON" : "OFF");

  Serial.print("Controle: ");
  Serial.println(controlEnabled ? "ON" : "OFF");

  Serial.print("Logging: ");
  Serial.println(loggingEnabled ? "ON" : "OFF");

  Serial.print("Atuador: ");
  Serial.println(heaterOn ? "ON" : "OFF");
}

void readLogSerial() {
  if (!LittleFS.exists(LOG_FILE)) {
    Serial.println("Arquivo de log nao encontrado.");
    return;
  }

  File f = LittleFS.open(LOG_FILE, FILE_READ);
  if (!f) {
    Serial.println("Falha ao abrir log.");
    return;
  }

  Serial.println("\n=== CONTEUDO DE log.txt ===");
  while (f.available()) {
    Serial.write(f.read());
  }
  Serial.println("\n=== FIM DO LOG ===");
  f.close();
}

void clearLogSerial() {
  File f = LittleFS.open(LOG_FILE, FILE_WRITE);
  if (!f) {
    Serial.println("Falha ao limpar log.");
    return;
  }

  f.println("timestamp,temp_c,umid_pct,ldr_raw,luz_pct,setpoint,system,control,heater");
  f.close();

  Serial.println("Log limpo com sucesso.");
}

// =========================
// Logging
// =========================
String nowStamp() {
  unsigned long s = millis() / 1000UL;
  unsigned long h = s / 3600UL;
  unsigned long m = (s % 3600UL) / 60UL;
  unsigned long sec = s % 60UL;

  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", h, m, sec);
  return String(buffer);
}

void appendLog() {
  File f = LittleFS.open(LOG_FILE, FILE_APPEND);
  if (!f) {
    Serial.println("Falha ao abrir log para escrita.");
    return;
  }

  f.print(nowStamp());
  f.print(",");
  if (isnan(temperatureC)) f.print("nan"); else f.print(String(temperatureC, 1));
  f.print(",");
  if (isnan(humidityPct)) f.print("nan"); else f.print(String(humidityPct, 1));
  f.print(",");
  f.print(ldrRaw);
  f.print(",");
  f.print(lightPct);
  f.print(",");
  f.print(String(setPointTemp, 1));
  f.print(",");
  f.print(systemEnabled ? 1 : 0);
  f.print(",");
  f.print(controlEnabled ? 1 : 0);
  f.print(",");
  f.println(heaterOn ? 1 : 0);

  f.close();
}