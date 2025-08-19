// Team gas sensors
// 
// Arc transformer timed to generate ozone in aeroponic chamber
// Cold plasma with dialectric barrier

const int RELAY_PIN   = 4;      // any of: 4,5,23,25,26,27,32,33
const bool ACTIVE_LOW = true;   // LOW input: true,  HIGH input: false

inline void relayOn()  { digitalWrite(RELAY_PIN, ACTIVE_LOW ? LOW  : HIGH); }
inline void relayOff() { digitalWrite(RELAY_PIN, ACTIVE_LOW ? HIGH : LOW ); }

void setup() {
  // drive the OFF level before making it an output (prevents a glitch)
  digitalWrite(RELAY_PIN, ACTIVE_LOW ? HIGH : LOW);
  pinMode(RELAY_PIN, OUTPUT);
  relayOff();
}

void loop() {
  relayOn();  delay(57500);
  relayOff(); delay(2500);
}
