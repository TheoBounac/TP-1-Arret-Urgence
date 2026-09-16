#include <EEPROM.h>
#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>

#define CE_PIN 10
#define CSN_PIN 9
#define RESET_BUTTON_PIN 2
#define RELAY_PIN 5

// IMPORTANT : logique relais pour branchement robot sur COM + NC
// RELAY_RUN  = état qui OUVRE COM-NC  -> robot autorisé
// RELAY_STOP = état qui FERME COM-NC  -> arrêt urgence robot
#define RELAY_RUN  LOW
#define RELAY_STOP HIGH

int EEPROM_ADDRESS = 0;
int ChannelNumber = 0;

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "1234";

int state = 0; // 0: ARMED, 1: SECURED
int relay_state = RELAY_RUN;
bool msg = true;

bool radioOK = false;

// Startup grace period
const unsigned long STARTUP_GRACE_TIME = 3000;
unsigned long startupTime = 0;
bool startupGraceActive = true;

// Reset button parameters
bool buttonPressed = false;
unsigned long buttonPressTime = 0;
const unsigned long RESET_HOLD_TIME = 3000;

// Leaky bucket parameters
const int BUCKET_CAPACITY = 6;
int bucketLevel = BUCKET_CAPACITY;
unsigned long lastLeakTime = 0;
const long LEAK_INTERVAL = 150;

// Debug parameters
unsigned long lastDebugPrint = 0;
const unsigned long DEBUG_INTERVAL = 500;

void setRelayRun() {
  relay_state = RELAY_RUN;
  digitalWrite(RELAY_PIN, relay_state);
}

void setRelayStop() {
  relay_state = RELAY_STOP;
  digitalWrite(RELAY_PIN, relay_state);
}

void triggerAlarm(const char* reason) {
  if (state == 0) {
    state = 1;
    setRelayStop();

    Serial.print("Alarm triggered: ");
    Serial.println(reason);
  } else {
    Serial.print("Already secured, alarm ignored: ");
    Serial.println(reason);
  }
}

void printDebugStatus() {
  if (millis() - lastDebugPrint >= DEBUG_INTERVAL) {
    lastDebugPrint = millis();

    Serial.print("D2 reset=");
    Serial.print(digitalRead(RESET_BUTTON_PIN) == LOW ? "APPUYE" : "RELACHE");

    Serial.print(" | D5 relais=");
    Serial.print(digitalRead(RELAY_PIN) == HIGH ? "HIGH" : "LOW");

    Serial.print(" | state=");
    Serial.print(state == 0 ? "ARMED" : "SECURED");

    Serial.print(" | bucket=");
    Serial.print(bucketLevel);

    Serial.print(" | last msg=");
    Serial.print(msg ? "TRUE" : "FALSE");

    Serial.print(" | radio=");
    Serial.print(radioOK ? "OK" : "NOT OK");

    Serial.print(" | startup=");
    Serial.println(startupGraceActive ? "GRACE" : "NORMAL");
  }
}

void setup() {
  // Très important : commander le relais immédiatement.
  // Rien avant ça.
  pinMode(RELAY_PIN, OUTPUT);
  setRelayRun();

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  startupTime = millis();
  lastLeakTime = millis();

  Serial.begin(115200);
  Serial.println("Receiver debug start");
  Serial.println("Relay forced to RUN immediately");
  Serial.println("Startup grace period active for 3 seconds");

  EEPROM.get(EEPROM_ADDRESS, ChannelNumber);
  if (ChannelNumber < 0 || ChannelNumber > 125) {
    ChannelNumber = 108;
    EEPROM.put(EEPROM_ADDRESS, ChannelNumber);
  }

  Serial.print("EEPROM channel=");
  Serial.println(ChannelNumber);

  // Mode setup canal radio.
  // Le relais reste en RUN pendant ce mode.
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    Serial.println("Mode Setup");
    Serial.println("Please enter the new Channel (0-125): ");

    while (!Serial.available()) {
      // On garde le relais ouvert pendant l'attente setup
      setRelayRun();
    }

    int newChannel = Serial.parseInt();

    while (!(newChannel >= 0 && newChannel <= 125)) {
      Serial.println("Invalid input. Please enter a valid channel (0-125): ");

      while (!Serial.available()) {
        setRelayRun();
      }

      newChannel = Serial.parseInt();
    }

    if (newChannel != ChannelNumber) {
      Serial.print("Default Channel set to: ");
      Serial.println(newChannel);
      EEPROM.put(EEPROM_ADDRESS, newChannel);
      ChannelNumber = newChannel;
    }

    Serial.println("Setup complete. Please release the button to continue.");

    while (digitalRead(RESET_BUTTON_PIN) == LOW) {
      setRelayRun();
      delay(100);
    }
  }

  Serial.println("Starting radio...");

  radioOK = radio.begin();

  if (!radioOK) {
    Serial.println("Radio non detectee au demarrage");
    Serial.println("Receiver will go to STOP after startup grace period");
  } else {
    radio.setPALevel(RF24_PA_MAX);
    radio.setPayloadSize(sizeof(msg));
    radio.setChannel(ChannelNumber);
    radio.setDataRate(RF24_1MBPS);
    radio.openReadingPipe(1, address);
    radio.startListening();

    Serial.println("Radio OK");
  }

  Serial.println("Receiver ready");
}

void loop() {
  printDebugStatus();

  uint8_t pipe;

  // Pendant les 3 premières secondes, on force le relais en RUN.
  if (startupGraceActive) {
    setRelayRun();

    if (millis() - startupTime >= STARTUP_GRACE_TIME) {
      startupGraceActive = false;
      Serial.println("Startup grace period finished");

      // Après les 3 secondes, si la radio n'est pas OK, arrêt immédiat.
      if (!radioOK) {
        triggerAlarm("radio not available after startup");
      }
    }

    return;
  }

  // Handle reset button when system is secured
  if (state == 1) {
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
      if (!buttonPressed) {
        buttonPressed = true;
        buttonPressTime = millis();
        Serial.println("Reset button pressed");
      } else {
        unsigned long heldTime = millis() - buttonPressTime;

        Serial.print("Reset held for ");
        Serial.print(heldTime);
        Serial.println(" ms");

        if (heldTime >= RESET_HOLD_TIME) {
          Serial.println("RESET VALIDATED -> back to ARMED");

          state = 0;
          setRelayRun();

          buttonPressed = false;
          bucketLevel = BUCKET_CAPACITY;
          lastLeakTime = millis();
        }
      }
    } else {
      if (buttonPressed) {
        Serial.println("Reset button released before 3s");
      }
      buttonPressed = false;
    }
  }

  // Si la radio n'est pas OK, arrêt.
  if (!radioOK) {
    if (state == 0) {
      triggerAlarm("radio not available");
    }
    return;
  }

  // Check for incoming radio data
  if (radio.available(&pipe)) {
    Serial.print("Radio message received on pipe ");
    Serial.println(pipe);

    bucketLevel = min(bucketLevel + 1, BUCKET_CAPACITY);
    lastLeakTime = millis();

    radio.read(&msg, sizeof(msg));

    Serial.print("msg=");
    Serial.println(msg ? "ALARM TRUE" : "ALARM FALSE");

    if (msg) {
      triggerAlarm("button alarm received");
    }
  } else {
    if (state == 0) {
      if (millis() - lastLeakTime > LEAK_INTERVAL) {
        bucketLevel = max(bucketLevel - 1, 0);
        lastLeakTime = millis();

        Serial.print("No radio data, bucket=");
        Serial.println(bucketLevel);
      }

      if (bucketLevel == 0) {
        triggerAlarm("signal loss");
      }
    }
  }
}
