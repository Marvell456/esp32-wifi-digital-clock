#include <WiFi.h>
#include <time.h>

/*
   =====================================================
                     ESP32 WIFI CLOCK
   =====================================================

   Features:
   - Syncs time using internet (NTP)
   - 4 digit 7-segment display
   - Button changes display mode:
       TIME -> DATE -> YEAR
   - Multiplexed display system

   Notes:
   - Common anode 7 segment
   - Built using breadboard + jumper wires
   - Earlier, I experimented with different
     resistor placements and OLED integration

*/

// =====================================================
// WIFI SETTINGS
// =====================================================

const char* ssid = "MCJnata";
const char* password = "MCJ123456789";

// =====================================================
// BUTTON PIN
// =====================================================

#define BUTTON_PIN 22

// =====================================================
// SEGMENT PINS
// Order:
// A B C D E F G DP
// =====================================================

int segmentPins[8] = {

  13, // A
  15, // B
  27, // C
  26, // D
  25, // E
  33, // F
  32, // G
  23  // Decimal Point
};

// =====================================================
// DIGIT CONTROL PINS
// Left to right
// =====================================================

int digitPins[4] = {

  18,
  19,
  5,
  4
};

// =====================================================
// DISPLAY MODES
// =====================================================

enum Mode {

  TIME_MODE,
  DATE_MODE,
  YEAR_MODE
};

Mode currentMode = TIME_MODE;

// =====================================================
// NUMBER MAP FOR 7 SEGMENT
// Each row represents:
// A B C D E F G
// =====================================================

byte digits[10][7] = {

  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}  // 9
};

// =====================================================
// GLOBAL VARIABLES
// =====================================================

// Stores current digits being displayed
int displayValues[4] = {0, 0, 0, 0};

// Used to update time every second
unsigned long lastTimeUpdate = 0;

// =====================================================
// BUTTON VARIABLES
// =====================================================

bool lastButtonState = HIGH;

unsigned long lastDebounceTime = 0;

const int debounceDelay = 50;

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // =================================================
  // SET SEGMENT PINS AS OUTPUT
  // =================================================

  for (int i = 0; i < 8; i++) {

    pinMode(segmentPins[i], OUTPUT);
  }

  // =================================================
  // SET DIGIT PINS AS OUTPUT
  // =================================================

  for (int i = 0; i < 4; i++) {

    pinMode(digitPins[i], OUTPUT);

    // Keep digits OFF initially
    digitalWrite(digitPins[i], LOW);
  }

  // =================================================
  // BUTTON SETUP
  // =================================================

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // =================================================
  // CONNECT TO WIFI
  // =================================================

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
  }

  // =================================================
  // GET TIME FROM INTERNET
  // GMT +7 (Indonesia)
  // =================================================

  configTime(7 * 3600, 0, "pool.ntp.org");

  delay(1000);

  // Initial display update
  updateTimeValues();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // Refresh display constantly
  multiplexDisplay();

  // Check button input
  handleButton();

  // Update actual clock values every second
  if (millis() - lastTimeUpdate >= 1000) {

    lastTimeUpdate = millis();

    updateTimeValues();
  }
}

// =====================================================
// BUTTON HANDLING
// =====================================================

void handleButton() {

  static bool previousReading = HIGH;

  bool reading = digitalRead(BUTTON_PIN);

  // Detect button press
  if (previousReading == HIGH && reading == LOW) {

    // Switch between modes
    currentMode = (Mode)((currentMode + 1) % 3);

    updateTimeValues();

    // Simple debounce delay
    delay(250);
  }

  previousReading = reading;
}

// =====================================================
// UPDATE DISPLAY VALUES
// Gets current local time from ESP32
// =====================================================

void updateTimeValues() {

  struct tm timeinfo;

  // Stop if time fetch fails
  if (!getLocalTime(&timeinfo)) {

    return;
  }

  // =================================================
  // TIME MODE
  // Shows HH:MM
  // =================================================

  if (currentMode == TIME_MODE) {

    displayValues[0] = timeinfo.tm_hour / 10;
    displayValues[1] = timeinfo.tm_hour % 10;

    displayValues[2] = timeinfo.tm_min / 10;
    displayValues[3] = timeinfo.tm_min % 10;
  }

  // =================================================
  // DATE MODE
  // Shows MM:DD
  // =================================================

  else if (currentMode == DATE_MODE) {

    int month = timeinfo.tm_mon + 1;

    displayValues[0] = month / 10;
    displayValues[1] = month % 10;

    displayValues[2] = timeinfo.tm_mday / 10;
    displayValues[3] = timeinfo.tm_mday % 10;
  }

  // =================================================
  // YEAR MODE
  // Shows full year
  // =================================================

  else {

    int year = timeinfo.tm_year + 1900;

    displayValues[0] = (year / 1000) % 10;
    displayValues[1] = (year / 100) % 10;
    displayValues[2] = (year / 10) % 10;
    displayValues[3] = year % 10;
  }
}

// =====================================================
// DISPLAY ONE DIGIT
// =====================================================

void showDigit(int number, int digitIndex) {

  // Turn OFF all digits first
  for (int i = 0; i < 4; i++) {

    digitalWrite(digitPins[i], LOW);
  }

  // Set segment states
  for (int i = 0; i < 7; i++) {

    bool state = digits[number][i];

    // Inverted because display is common anode
    state = !state;

    digitalWrite(segmentPins[i], state);
  }

  // Turn ON selected digit
  digitalWrite(digitPins[digitIndex], HIGH);

  // Small delay for multiplexing
  delayMicroseconds(2000);
}

// =====================================================
// MULTIPLEX DISPLAY
// Rapidly cycles through all digits
// =====================================================

void multiplexDisplay() {

  for (int i = 0; i < 4; i++) {

    showDigit(displayValues[i], i);
  }
}