#include <EEPROM.h>
#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>

#define CE_PIN 10
#define CSN_PIN 9
#define BUTTON_PIN 2

int EEPROM_ADDRESS = 0;
int ChannelNumber = 0;

int period = 100;

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "1234";

bool msg = false;

unsigned long packetCount = 0;
unsigned long lastDebugPrint = 0;
const unsigned long DEBUG_INTERVAL = 500;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Transmitter debug start");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  EEPROM.get(EEPROM_ADDRESS, ChannelNumber);
  if (ChannelNumber < 0 || ChannelNumber > 125) {
    ChannelNumber = 108;
    EEPROM.put(EEPROM_ADDRESS, ChannelNumber);
  }

  Serial.print("EEPROM channel=");
  Serial.println(ChannelNumber);

  // Check Startup mode
  if (digitalRead(BUTTON_PIN) == HIGH) {
    Serial.println("Mode Setup");
    Serial.println("Please enter the new Channel (0-125): ");

    while (!Serial.available()) {
    }

    int newChannel = Serial.parseInt();

    while (!(newChannel >= 0 && newChannel <= 125)) {
      Serial.println("Invalid input. Please enter a valid channel (0-125): ");

      while (!Serial.available()) {
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

    while (digitalRead(BUTTON_PIN) == HIGH) {
      delay(100);
    }
  }

  Serial.println("Starting radio...");

  while (!radio.begin()) {
    Serial.println("Radio non detectee, retry...");
    delay(500);
  }

  EEPROM.get(EEPROM_ADDRESS, ChannelNumber);

  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(sizeof(msg));
  radio.setChannel(ChannelNumber);
  radio.setDataRate(RF24_1MBPS);
  radio.openWritingPipe(address);
  radio.stopListening();

  Serial.println("Radio OK");
  Serial.println("Transmitter ready");
}

void printDebugStatus(bool sent) {
  if (millis() - lastDebugPrint >= DEBUG_INTERVAL) {
    lastDebugPrint = millis();

    Serial.print("D2 bouton=");
    Serial.print(digitalRead(BUTTON_PIN) == HIGH ? "HIGH" : "LOW");

    Serial.print(" | msg=");
    Serial.print(msg ? "ALARM TRUE" : "ALARM FALSE");

    Serial.print(" | radio.write=");
    Serial.print(sent ? "OK" : "ECHEC");

    Serial.print(" | packets=");
    Serial.print(packetCount);

    Serial.print(" | channel=");
    Serial.println(ChannelNumber);
  }
}

void loop() {
  msg = digitalRead(BUTTON_PIN) == HIGH;

  bool sent = radio.write(&msg, sizeof(msg));
  packetCount++;

  printDebugStatus(sent);

  delay(period);
}
