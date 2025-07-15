#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include "BluetoothSerial.h"
#include <SPI.h>
#include <MFRC522.h>

// === PIN CONFIGURATION ===
#define SS_PIN 33
#define RST_PIN 5
#define PulseSensor_PIN 15
#define LDR_PIN 2
#define RGB_LED_PIN 4
#define BLE_LED 25
#define DHT_PIN 32
#define MQ2_PIN 12
#define BUZZER_PIN 14
#define MOTOR_PIN_2 27
#define MOTOR_PIN_1 26
#define PANIC_BUTTON_PIN 35
#define DHT_TYPE DHT11

// === OBJECTS ===
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 16×2 I2C LCD
DHT dht(DHT_PIN, DHT_TYPE);
BluetoothSerial SerialBT;
MFRC522 rfid(SS_PIN, RST_PIN);

// === RFID TAGS & NAMES ===
const byte NUM_TAGS = 4;
String names[NUM_TAGS] = {"Pushkar", "Kunal", "Pranaav", "IShowSpeed"};
byte knownTags[NUM_TAGS][4] = {
  {0xF2, 0x8D, 0x28, 0x03},
  {0x1C, 0x19, 0x29, 0x03},
  {0xB6, 0x16, 0xC8, 0x01},
  {0xB7, 0xCA, 0x45, 0x03}
};
bool isPresent[NUM_TAGS] = {false, false, false, false};

// === STATE VARIABLES ===
bool systemActive = false;
bool btLostHandled = false;
unsigned long sessionStart = 0;
unsigned long lastBlink = 0;
unsigned long gasSensorWarmUp = 0;

// === SENSOR THRESHOLDS ===
int lightThreshold = 2200;
int gasThreshold   = 1900;

// === SENSOR VALUES ===
int pulseValue, ldrValue, mq2Value;
float temperature, humidity;

// Simple melody for Bluetooth connection (notes in Hz)
const int btTuneNotes[] = {523, 659, 784}; // C5, E5, G5
const int btTuneDurations[] = {200, 200, 200}; // each 200 ms
const int btTuneLength = 3;

void setup() {
  // Serial for debugging
  Serial.begin(115200);
  // Bluetooth setup
  SerialBT.begin("SmartJacket");
  // LCD
  lcd.init();
  lcd.backlight();
  // DHT
  dht.begin();

  // Pin modes
  pinMode(RGB_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_PIN_2, OUTPUT);
  pinMode(MOTOR_PIN_1, OUTPUT);
  pinMode(PANIC_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLE_LED, OUTPUT);

  // RFID SPI init
  SPI.begin();
  rfid.PCD_Init();
  delay(4);

  // Initial welcome message
  lcd.setCursor(0, 0);
  lcd.print("Smart Jacket <3");
  digitalWrite(MOTOR_PIN_2, HIGH);
  digitalWrite(MOTOR_PIN_1, HIGH);
  delay(1000);
  digitalWrite(MOTOR_PIN_2, LOW);
  digitalWrite(MOTOR_PIN_1, LOW);
  delay(1500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tap RFID Card");
  delay(1500);

  // Start gas sensor warm-up timer
  gasSensorWarmUp = millis();
}

void loop() {
  bool prevSystemActive = systemActive;

  // Check for RFID tap (start/stop session)
  checkRFID();

  // If system is not active, show “TEAM SCATTERED” and buzz continuously
  if (!systemActive) {
    if (!prevSystemActive) {
      showTeamScattered();
    }
    return;
  }

  // Panic mode: check button
  if (digitalRead(PANIC_BUTTON_PIN) == LOW) {
    panicMode();
    return;
  }

  // ===== Bluetooth Connectivity Check =====
  static bool wasConnected = false;
  bool nowConnected = SerialBT.hasClient();

  if (nowConnected) {
    // Just turned on a connection?
    if (!wasConnected) {
      // Stop any “TEAM SCATTERED” tone
      noTone(BUZZER_PIN);

      // Play Bluetooth melody
      playBTTune();

      // Show "Connected" briefly
      lcd.setCursor(0, 1);
      lcd.print(" BT:Connected  ");
      digitalWrite(MOTOR_PIN_2, HIGH);
      digitalWrite(MOTOR_PIN_1, HIGH);
      delay(1000);
      digitalWrite(MOTOR_PIN_2, LOW);
      digitalWrite(MOTOR_PIN_1, LOW);
      delay(1000);
      lcd.clear();
    }
    wasConnected = true;
    digitalWrite(BLE_LED, LOW);   // LED off = connected
  }
  else {
    // Just lost the connection?
    if (wasConnected) {
      btLostHandled = false;
    }
    wasConnected = false;

    if (!btLostHandled) {
      showTeamScattered();      // Show "TEAM SCATTERED" once
      digitalWrite(MOTOR_PIN_2, HIGH);
      digitalWrite(MOTOR_PIN_1, HIGH);
      delay(1000);
      digitalWrite(MOTOR_PIN_2, LOW);
      digitalWrite(MOTOR_PIN_1, LOW);
      btLostHandled = true;
    }
    digitalWrite(BLE_LED, HIGH);  // LED on = not connected
  }

  // ===== SENSOR READINGS =====
  pulseValue   = analogRead(PulseSensor_PIN);
  ldrValue     = analogRead(LDR_PIN);
  humidity     = dht.readHumidity();
  temperature  = dht.readTemperature();
  mq2Value     = analogRead(MQ2_PIN);

  // ===== SERIAL OUTPUT =====
  pulseValue = map(pulseValue, 0, 4095, 0, 150);

  Serial.print("Temprature:"); Serial.print(temperature);
  Serial.print("-Pulse:"); Serial.print(pulseValue);
  
  //Serial.print("°C  Humidity: "); Serial.print(humidity);
  //Serial.print("%  LDR: "); Serial.print(ldrValue);
  Serial.print("-Gas:"); Serial.print(mq2Value);
  Serial.print("-BT connected:"); Serial.println(nowConnected);

  // ===== LCD OUTPUT =====
  // Row 0: Pulse & Temperature
  lcd.setCursor(0, 0);
  lcd.print("P:"); lcd.print(pulseValue);
  lcd.print(" T:"); lcd.print(temperature, 1); lcd.print("C ");

  // Row 1: Humidity & LDR
  lcd.setCursor(0, 1);
  lcd.print("H:"); lcd.print(humidity, 1);
  lcd.print("% L:"); lcd.print(ldrValue);

  // ===== ALERT LOGIC =====
  // Prioritize Gas alert over Light alert if both exceed thresholds
  bool gasAlert   = (millis() - gasSensorWarmUp > 5000) && (mq2Value > gasThreshold);
  bool lightAlert = (ldrValue > lightThreshold);
  if (temperature>=30)
    {
    tone(BUZZER_PIN, 1000);
    digitalWrite(RGB_LED_PIN, LOW);
    digitalWrite(MOTOR_PIN_2, HIGH);
    digitalWrite(MOTOR_PIN_1, HIGH);
    delay(1000);
    digitalWrite(MOTOR_PIN_2, LOW);
    digitalWrite(MOTOR_PIN_1, LOW);
    }

  if (gasAlert) {
    // Gas alert: 800 Hz tone, LED off
    tone(BUZZER_PIN, 800);
    digitalWrite(RGB_LED_PIN, LOW);
    digitalWrite(MOTOR_PIN_2, HIGH);
    digitalWrite(MOTOR_PIN_1, HIGH);
    delay(1000);
    digitalWrite(MOTOR_PIN_2, LOW);
    digitalWrite(MOTOR_PIN_1, LOW);
  }
  else if (lightAlert) {
    // Light alert: 1000 Hz tone, LED on
    digitalWrite(RGB_LED_PIN, HIGH);
  }
  else {
    // No alerts
    noTone(BUZZER_PIN);
    digitalWrite(RGB_LED_PIN, LOW);
  }

  delay(1000);
}

// Shows "TEAM SCATTERED" on the LCD and continuously buzzes at 500 Hz,
// then holds the message for 2 seconds before allowing it to be cleared.
void showTeamScattered() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("TEAM SCATTERED");
  digitalWrite(BLE_LED, HIGH);
  tone(BUZZER_PIN, 500); // Continuous 500 Hz tone for "scattered"
  delay(2000);           // Hold "TEAM SCATTERED" for 2 seconds
  // Note: Do not clear here; next call to LCD print will overwrite.
}

// Play a simple 3-note tune for Bluetooth connection
void playBTTune() {
  for (int i = 0; i < btTuneLength; i++) {
    tone(BUZZER_PIN, btTuneNotes[i]);
    delay(btTuneDurations[i]);
    noTone(BUZZER_PIN);
    delay(50);
  }
}

void panicMode() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("!! PANIC MODE !!");
  while (digitalRead(PANIC_BUTTON_PIN) == LOW) {
    digitalWrite(RGB_LED_PIN, HIGH);
    tone(BUZZER_PIN, 1200);  // 1200 Hz siren tone
    delay(300);
    noTone(BUZZER_PIN);
    delay(300);
  }
  digitalWrite(RGB_LED_PIN, LOW);
  noTone(BUZZER_PIN);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tap RFID Card");
}

void checkRFID() {
  // If no new card or failed read, return
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  byte* uid = rfid.uid.uidByte;
  byte uidSize = rfid.uid.size;

  int matchedIndex = -1;
  for (int i = 0; i < NUM_TAGS; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (knownTags[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      matchedIndex = i;
      break;
    }
  }

  lcd.clear();

  if (matchedIndex != -1) {
    String name = names[matchedIndex];

    if (!isPresent[matchedIndex]) {
      // Tapped IN
      isPresent[matchedIndex] = true;
      systemActive = true;
      sessionStart = millis();

      // Stop "scattered" tone if it was running
      noTone(BUZZER_PIN);

      lcd.setCursor(0, 0);
      lcd.print("Hello, " + name);
      Serial.println("Hello, " + name);
      Serial.println("--- Session Started ---");
    } 
    else {
      // Tapped OUT
      isPresent[matchedIndex] = false;
      systemActive = false;
      unsigned long sessionEnd = millis();
      unsigned long durationSec = (sessionEnd - sessionStart) / 1000;

      lcd.setCursor(0, 0);
      lcd.print("Bye, " + name);
      lcd.setCursor(0, 1);
      lcd.print("Dur: "); lcd.print(durationSec); lcd.print(" sec");
      Serial.print("Goodbye, " + name);
      Serial.print(" | Duration: ");
      Serial.print(durationSec);
      Serial.println(" sec");
      digitalWrite(MOTOR_PIN_2, HIGH);
      digitalWrite(MOTOR_PIN_1, HIGH);
      delay(1000);
      digitalWrite(MOTOR_PIN_2, LOW);
      digitalWrite(MOTOR_PIN_1, LOW);
    }
  } 
  else {
    lcd.setCursor(0, 0);
    lcd.print("Unknown Tag");
    Serial.println("Unknown tag detected.");
  }

  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tap RFID Card");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
