#include <Wire.h> 
#include <Adafruit_INA219.h>
#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>
#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>   // to handle this particular modem driver
#include "RF24.h"       // the library which helps us to control the radio modem
#include <Servo.h>
 
 Adafruit_INA219 ina219;
// Initialise Sensors
 
// Initialise Actuators
 
#define RF24_PAYLOAD_SIZE 32
#define AAAD_ARO 4
#define AAAD_MODULE 2
#define I2C_ADDRESS_SOLAR_PANEL 0x40
#define I2C_ADDRESS_BATTERY 0x80
 
Servo myservo;  // create servo object to control a servo 
 
bool vouw_configuratie = false;
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
 
int signaal = 0;
/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10, which are CE & CSN pins  */
RF24 radio(9, 10);
const uint8_t rf24_channel[] = {1,26,51,76,101}; 
// Radio channels set depending on satellite number
const uint64_t addresses[] = { 0x4141414430LL, 0x4141414431LL, 0x4141414432LL, 0x4141414433LL, 0x4141414434LL };  
// Setting radioNumber to 0 configures the transmission (tx) pipe to the address AAAD0, which is equivalent to HEX '4141414430'. 
// This address acts as the destination for the data we are transmitting to the remote device.
// The receive (rx) pipe address represents our local receiving address. The remote device must configure this address as its tx pipe, to ensure successful communication.

uint8_t txData[RF24_PAYLOAD_SIZE];
uint8_t rxData[RF24_PAYLOAD_SIZE];
 
// Timing configuration
unsigned long previousMillis = 0; // will store last time LED was updated
unsigned long currentMillis;
unsigned long sampleTime = 5000; // milliseconds of on-time
 
// int to hex converter
void printHex2(unsigned v) {
    Serial.print("0123456789ABCDEF"[v>>4]);
    Serial.print("0123456789ABCDEF"[v&0xF]);
}
 
void setup() {
  uint32_t currentFrequency;
  Serial.begin(9600);
  Serial.println("nRF24 Application ARO" + String(AAAD_ARO) + ", Module" + String(AAAD_MODULE) + " Started!\n");
 
  // Initialize INA219
   Wire.begin();
 
   myservo.attach(5);  
   // attaches the servo on pin 5 to the servo object
 
  // Activate Radio
  radio.begin();                  // Activate the modem
  radio.setPALevel(RF24_PA_MIN);  // Set the PA Level low to prevent power supply related issues
                                  // since this is a getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setDataRate(RF24_1MBPS);  // choosing 1 Mega bit per second radio frequency data rate
                                  // radio frequency data rate choices are:  //RF24_250KBPS    //RF24_2MBPS  //RF24_1MBPS
  radio.setChannel(rf24_channel[AAAD_ARO]);
  radio.setPayloadSize(RF24_PAYLOAD_SIZE);
  radio.openWritingPipe(addresses[AAAD_MODULE]);
  radio.openReadingPipe(1, addresses[AAAD_MODULE]);
 
  // Start the radio listening for data
  radio.startListening();
  ina219.begin();
}
 
void loop() {
 
// Check for folding or unfolding command being received
 
  // check to see if it's time to change the state of the LED
  currentMillis = millis();
 
  if(currentMillis - previousMillis >= sampleTime) {
 
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
 
  Serial.print("Bus Voltage:      "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage:    "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:     "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:          "); Serial.print(current_mA); Serial.println(" mA");
  Serial.println("");

    uint8_t cursor = 0;
 
  // bereik random van stroom zonnepaneel
    uint8_t min = 0;
    uint8_t maxZonnepaneelStroom = 170;
    uint8_t maxZonnepaneelSpanning = 82;
    uint8_t maxBatterijSpanning = 50;
    uint8_t maxPositie = 100;
 
    // alle randoms moeten uiteindelijk gemeten waarden worden vanuit de sensoren
   
   uint8_t fifteen = 15;
    
    txData[cursor++] = current_mA;
    txData[cursor++] = loadvoltage; //spanning zonnepaneel max 82 wordt omgezet naar 8,2
   
    while (cursor<RF24_PAYLOAD_SIZE) {
      txData[cursor++] = 0;
    }
       
  /****************** Transmit Mode ***************************/
 
   // Print transmit data in Hex format
    Serial.print("txData: ");
    for (size_t i=0; i<cursor; ++i) {
      if (i != 0) Serial.print(" ");
      printHex2(txData[i]);
    }
    Serial.println();
 
    radio.stopListening();  // First, stop listening so we can talk.
 
    // Transmit data to radio
    radio.write(&txData, sizeof(txData));
 
    radio.startListening();  // Now, continue listening
 
    previousMillis = currentMillis;
  }
 
  /****************** Receive Mode ***************************/
 
  if (radio.available()) { //'available' means whether valid bytes have been received and are waiting to be read from the receive buffer
 
    // Receive data from radio
    while (radio.available()) { // While there is data ready
      radio.read(&rxData, sizeof(rxData));  // Get the payload
    }
    // Print received data in Hex format
    Serial.print("rxData: ");
    for (size_t i=0; i<RF24_PAYLOAD_SIZE; ++i) {
      if (i != 0) Serial.print(" ");
      printHex2(rxData[i]);
    }
    Serial.println();
 
    // Switch led on Received command
 
      if(vouw_configuratie == true && rxData[0] == 0x7F)
    {
    vouw_configuratie = false;
myservo.write(150);
delay(26500);

myservo.write(90);
delay(100);

myservo.write(120);
delay(5000);

myservo.write(90);

}

if (vouw_configuratie == false && rxData[0] == 0xFF) {
    vouw_configuratie = true;

    myservo.write(30);
    delay(27500);

    myservo.write(90);
}
  }
  
}
