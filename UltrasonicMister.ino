// By Team Gas Sensors
// Main lead on this specific code: Grant Harold
// ESP32 control script for ultrasonic mister
// Mister module has physical button to turn on;
// each time it powers down the button has to be pressed again!
// This script automates the button by soldering to the mist microcontroller button pins
// Relay replaces the user's physical button press
// Cycles are necessary due to the nature of the microcontroller's programs
// One button press turns the module on, a second button press cycles to another program, third turns it off.


const int RELAY_PIN = 5;  // GPIO 

void cycle() {
  
  digitalWrite(RELAY_PIN, HIGH);  
  delay(200);
  digitalWrite(RELAY_PIN, LOW);
  delay(200);
  
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);   // 
}

void loop() {
cycle();                          // Button press on 
delay(20000);                     // On 20 seconds
cycle();                          // Button press (for mister's microcontroller program cycle)
cycle();                          // Button press (for program cycle)
delay(5000);                      // Off 5 seconds
}

