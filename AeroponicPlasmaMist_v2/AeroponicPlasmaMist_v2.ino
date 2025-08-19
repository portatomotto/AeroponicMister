/*
  EXACT mister cycle (matches your standalone .ino) inside combined sketch
  - Fan:   GPIO19 (D19)
  - Plasma:GPIO4  (D4)
  - Mister:GPIO5  (D5)
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3D
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== MQ137 =====
#define ADC_RES     4095
#define MQ137_PIN   A3
float readAmmoniaPercent() {
  int raw = analogRead(MQ137_PIN);
  float pct = (raw / (float)ADC_RES) * 100.0f;
  return pct - 21.0f;
}

// ===== Relays & polarity flags =====
#define FAN_RELAY_PIN         19
#define PLASMA_RELAY_PIN      4
#define MISTER_RELAY_PIN      5

bool FAN_ACTIVE_LOW     = true;
bool PLASMA_ACTIVE_LOW  = true;

// EXACT match to your mister sketch: PRESS = HIGH, RELEASE = LOW
bool MISTER_ACTIVE_LOW  = false;

inline void relayWrite(int pin, bool activeLow, bool on) {
  digitalWrite(pin, on ? (activeLow ? LOW : HIGH) : (activeLow ? HIGH : LOW));
}

// ===== Fan control (unchanged) =====
enum FanMode { FAN_TOGGLE = 0, FAN_THRESHOLD = 1, FAN_MANUAL = 2 };
FanMode fan_mode = FAN_TOGGLE;
bool fan_on = false;
unsigned long fan_lastToggle_ms = 0;
unsigned long FAN_TOGGLE_PERIOD_MS = 5000;
float FAN_ON_THRESHOLD  = 5.0f;
float FAN_OFF_THRESHOLD = 3.0f;

// ===== Plasma control (unchanged) =====
enum PlasmaMode { PLASMA_DUTY = 0, PLASMA_MANUAL = 1 };
PlasmaMode plasma_mode = PLASMA_DUTY;
bool plasma_on = false;
unsigned long plasma_lastChange_ms = 0;
unsigned long PLASMA_ON_MS  = 20000;
unsigned long PLASMA_OFF_MS = 5000;

// ===== Mister: EXACT replica of your standalone timing =====
enum MisterState {
  M_IDLE,
  M_P1_PRESS, M_P1_RELEASE,   // cycle();   (press 200, release 200)
  M_WAIT_A,                   // 20000
  M_P2_PRESS, M_P2_RELEASE,   // cycle();
  M_P3_PRESS, M_P3_RELEASE,   // cycle();
  M_WAIT_B                    // 5000
};
MisterState mister_state = M_IDLE;
unsigned long mister_state_ts = 0;

// Timings EXACT as your file (configurable via serial if you want)
unsigned long MISTER_PRESS_MS   = 200;
unsigned long MISTER_RELEASE_MS = 200;
unsigned long MISTER_WAIT_A_MS  = 20000;
unsigned long MISTER_WAIT_B_MS  = 5000;

void misterStartCycle() {
  mister_state = M_P1_PRESS;
  mister_state_ts = millis();
}
void misterStopCycle() {
  mister_state = M_IDLE;
  relayWrite(MISTER_RELAY_PIN, MISTER_ACTIVE_LOW, false); // ensure released
}

void runMisterTask(unsigned long now) {
  switch (mister_state) {
    case M_IDLE:
      relayWrite(MISTER_RELAY_PIN, MISTER_ACTIVE_LOW, false);
      break;

    case M_P1_PRESS:
      relayWrite(MISTER_RELAY_PIN, MISTER_ACTIVE_LOW, true);
      if (now - mister_state_ts >= MISTER_PRESS_MS) { mister_state = M_P1_RELEASE; mister_state_ts = now; }
      break;

    case M_P1_RELEASE:
      relayWrite(MISTER_RELAY_PIN, MISTER_ACTIVE_LOW, false);
      if (now - mister_state_ts >= MISTER_RELEASE_MS) { mister_state = M_WAIT_A; mister_state_ts = now; }
      break;

    case M_WAIT_A:
      if (now - mister_state_ts >= MISTER_WAIT_A_MS) { mister_state = M_P2_PRESS; mister_state_ts = now; }
      break;

    case M_P2_PRESS:
      relayWrite(MISTER_RELAY_PIN, MISTER_ACTIVE_LOW, true);
      if (now - mister_state_ts >= MISTER_PRESS_MS) { mister_state = M_P2_RELEASE; mister_state_ts = now; }
      break;

    case M_P2_RELEASE:
      relayWrite(MISTER_RELAY_PIN, MISTER_ACTIVE_LOW, false);
      if (now - mister_state_ts >= MISTER_RELEASE_MS) { mister_state = M_P3_PRESS; mister_state_ts = now; }
      break;

    case M_P3_PRESS:
      relayWrite(MISTER_RELAY_PIN, MISTER_ACTIVE_LOW, true);
      if (now - mister_state_ts >= MISTER_PRESS_MS) { mister_state = M_P3_RELEASE; mister_state_ts = now; }
      break;

    case M_P3_RELEASE:
      relayWrite(MISTER_RELAY_PIN, MISTER_ACTIVE_LOW, false);
      if (now - mister_state_ts >= MISTER_RELEASE_MS) { mister_state = M_WAIT_B; mister_state_ts = now; }
      break;

    case M_WAIT_B:
      if (now - mister_state_ts >= MISTER_WAIT_B_MS) { mister_state = M_P1_PRESS; mister_state_ts = now; }
      break;
  }
}

// ===== OLED =====
void drawStatus(float ammoniaPct) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("NH3: "); display.print(ammoniaPct, 1); display.println("%");

  display.setCursor(0, 12);
  display.print("Fan  : "); display.print(fan_on ? "ON " : "OFF"); display.print(" (mode "); display.print((int)fan_mode); display.println(")");

  display.setCursor(0, 22);
  display.print("Plasma: "); display.print(plasma_on ? "ON " : "OFF"); display.print(" (mode "); display.print((int)plasma_mode); display.println(")");

  display.setCursor(0, 32);
  display.print("Mister: ");
  switch (mister_state) {
    case M_IDLE:       display.println("IDLE"); break;
    case M_P1_PRESS:   display.println("P1_PRESS"); break;
    case M_P1_RELEASE: display.println("P1_RELEASE"); break;
    case M_WAIT_A:     display.println("WAIT_A"); break;
    case M_P2_PRESS:   display.println("P2_PRESS"); break;
    case M_P2_RELEASE: display.println("P2_RELEASE"); break;
    case M_P3_PRESS:   display.println("P3_PRESS"); break;
    case M_P3_RELEASE: display.println("P3_RELEASE"); break;
    case M_WAIT_B:     display.println("WAIT_B"); break;
  }
  display.display();
}

// ===== Fan task =====
void runFanTask(float ammonia, unsigned long now) {
  switch (fan_mode) {
    case FAN_TOGGLE:
      if (now - fan_lastToggle_ms >= FAN_TOGGLE_PERIOD_MS) { fan_on = !fan_on; relayWrite(FAN_RELAY_PIN, FAN_ACTIVE_LOW, fan_on); fan_lastToggle_ms = now; }
      break;
    case FAN_THRESHOLD:
      if (!fan_on && ammonia > FAN_ON_THRESHOLD) { fan_on = true; relayWrite(FAN_RELAY_PIN, FAN_ACTIVE_LOW, true); }
      if (fan_on && ammonia < FAN_OFF_THRESHOLD) { fan_on = false; relayWrite(FAN_RELAY_PIN, FAN_ACTIVE_LOW, false); }
      break;
    case FAN_MANUAL:
      break;
  }
}

// ===== Plasma task =====
void runPlasmaTask(unsigned long now) {
  switch (plasma_mode) {
    case PLASMA_DUTY:
      if (plasma_on) {
        if (now - plasma_lastChange_ms >= PLASMA_ON_MS) { plasma_on = false; relayWrite(PLASMA_RELAY_PIN, PLASMA_ACTIVE_LOW, false); plasma_lastChange_ms = now; }
      } else {
        if (now - plasma_lastChange_ms >= PLASMA_OFF_MS) { plasma_on = true; relayWrite(PLASMA_RELAY_PIN, PLASMA_ACTIVE_LOW, true); plasma_lastChange_ms = now; }
      }
      break;
    case PLASMA_MANUAL:
      break;
  }
}

// ===== Serial menu (minimal; mister timings are fixed to exact defaults) =====
String inputLine;
void printHelp() {
  Serial.println("\n=== Control Menu ===");
  Serial.println("h/s : help/status");
  Serial.println("# Mister (EXACT cadence)");
  Serial.println("ms  : start mister cycle");
  Serial.println("mx  : stop mister cycle");
  Serial.println("====================");
  Serial.print("> ");
}
void printStatus(float ammonia) {
  Serial.println("\n--- Status ---");
  Serial.print("Ammonia %: "); Serial.println(ammonia, 2);
  Serial.print("Mister state: ");
  const char* names[]={"IDLE","P1_PRESS","P1_RELEASE","WAIT_A","P2_PRESS","P2_RELEASE","P3_PRESS","P3_RELEASE","WAIT_B"};
  Serial.println(names[mister_state]);
  Serial.println("Mister timings (ms): press=200, release=200, waitA=20000, waitB=5000");
  Serial.println("--------------");
  Serial.print("> ");
}
void handleCommandLine(String line) {
  line.trim(); line.toLowerCase();
  if (!line.length()) { Serial.print("> "); return; }
  if (line=="h"||line=="help"||line=="?") { printHelp(); return; }
  if (line=="s"||line=="status") { printStatus(readAmmoniaPercent()); return; }
  if (line=="ms") { misterStartCycle(); Serial.println("Mister cycle started."); Serial.print("> "); return; }
  if (line=="mx") { misterStopCycle(); Serial.println("Mister cycle stopped."); Serial.print("> "); return; }
  Serial.println("Unrecognized. Type 'h'."); Serial.print("> ");
}

// ===== Setup / Loop =====
void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 init failed. Running headless.");
  } else {
    display.clearDisplay(); display.display();
  }

  pinMode(FAN_RELAY_PIN,     OUTPUT);
  pinMode(PLASMA_RELAY_PIN,  OUTPUT);
  pinMode(MISTER_RELAY_PIN,  OUTPUT);

  // Set all relays released/inactive
  relayWrite(FAN_RELAY_PIN,     FAN_ACTIVE_LOW,     false);
  relayWrite(PLASMA_RELAY_PIN,  PLASMA_ACTIVE_LOW,  false);
  relayWrite(MISTER_RELAY_PIN,  MISTER_ACTIVE_LOW,  false);

  unsigned long now = millis();
  fan_lastToggle_ms    = now;
  plasma_lastChange_ms = now;

  // Start mister EXACT cycle by default (like your original loop begins immediately)
  misterStartCycle();

  printHelp();
}

void loop() {
  unsigned long now = millis();

  float ammonia = readAmmoniaPercent();
  runFanTask(ammonia, now);
  runPlasmaTask(now);
  runMisterTask(now);

  drawStatus(ammonia);

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { handleCommandLine(inputLine); inputLine = ""; }
    else {
      inputLine += c;
      if (inputLine.length() > 128) inputLine = "";
    }
  }

  // Keep timing resolution tight (avoid 50ms granularity)
  delay(1);
}
