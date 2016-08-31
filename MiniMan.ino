#include <i2c_t3.h>
#include <IRremote.h>

// http://wiki.seeedstudio.com/wiki/Raspberry_Pi_Relay_Board_v1.0
// http://henrysbench.capnfatz.com/henrys-bench/arduino-sensors-and-input/arduino-xinda-keyes-infrared-remote-tutorial/#attachment wp-att-2065/0/
// https://www.pjrc.com/teensy/td_libs_Wire.html
// https://github.com/nox771/i2c_t3
/* Selenoid MAP
 *  4 -> ARM control
 *  1 -- right arm ( from mini's perspective )
 *  2 -- head
 *  3 -- left arm ( form mini's perspective )
 */
int IR_PIN = 23;
IRrecv irDetect(IR_PIN);
decode_results irIn;




uint relayBankSize = 3;// but 4 is reserved for arms

uint lastCode = 0x0;
ulong lastPress = 0;
bool HOLDING = false;
// This needs to be tuned for the IR system. 
ulong holdTimeout = 120;// amount of ms before we care about it being off or not. aka minum burst
int stateMap = 0x0000;

struct KEYMAP {
  int UP = 0xFF629D;
  int DOWN = 0xFFA857;
  int LEFT = 0xFF22DD;
  int RIGHT = 0xFFC23D;
  int OK = 0xFF02FD;
  int ONE =  0xFF6897;
  int TWO = 0xFF9867;
  int THREE = 0xFFB04F;
  int FOUR = 0xFF30CF;
  int FIVE = 0xFF18E7;
  int SIX = 0xFF7A85;
  int SEVEN = 0xFF10EF;
  int EIGHT = 0xFF38C7;
  int NINE = 0xFF5AA5;
  int STAR = 0xFF42BD;
  int ZERO =  0xFF4AB5;
  int POUND = 0xFF52AD;
};

// RELAY CONFIG
const int DEVICE_ADDRESS = 0x20; // 7 bit address (will be left shifted to add the read write bit)
const int DEVICE_REG_MODE1 = 0x06;
int DEVICE_REG_DATA = 0xff;

const int time = 300;

// RELAY HELPERS
void onData (int address, int base) {
    DEVICE_REG_DATA &= ~(base << address);
}

void offData (int address, int base) {
    DEVICE_REG_DATA |= (base << address);
}

void toggle(int address, bool mode, int base) {
  Wire.beginTransmission(DEVICE_ADDRESS);
    if(mode) {
      stateMap |= address;
      onData(address, base);
    } else {
      stateMap &= ~(address);
      offData(address, base);
    }
    Wire.write(DEVICE_REG_MODE1);
    Wire.write(DEVICE_REG_DATA);
  Wire.endTransmission();
}

void allOn() {
  //toggle(0, true, 0xf);
  on(0);
  on(1);
  on(2);
}

void allOff() {
//  toggle(0, false, 0xf);
  off(0);
  off(1);
  off(2);
}

void on (int address) {
  toggle(address, true, 0x1);
}

void off (int address) {
  toggle(address, false, 0x1);
}

//IR MAPPERS
void decodeIR() {
  lastPress = millis();
  HOLDING = false;

  if (irIn.value != 0xFFFFFFFF) {
    lastCode = irIn.value;
  }

  switch(irIn.value) {
    case 0xFF629D:
      Serial.println("Up Arrow");
      on(3);
      break;
    case 0xFF22DD:
      Serial.println("Left Arrow");
      on(0);
      break;
    case 0xFF02FD:
      Serial.println("OK");
      on(1);
      break;
    case 0xFFC23D:
      on(2);
      Serial.println("Right Arrow");
      break;
    case 0xFFA857:
      Serial.println("Down Arrow");
      off(3);
      break;
    case 0xFF6897:
      Serial.println("1");
      on(0);
      break;
    case 0xFF9867:
      Serial.println("2");
      on(1);
      break;
    case 0xFFB04F:
      Serial.println("3");
      on(2);
      break;
    case 0xFF30CF:
      Serial.println("4");
      break;
    case 0xFF18E7:
      Serial.println("5");
      break;
    case 0xFF7A85:
      Serial.println("6");
      break;
    case 0xFF10EF:
      Serial.println("7");
      break;
    case 0xFF38C7:
      Serial.println("8");
      break;
    case 0xFF5AA5:
      Serial.println("9");
      break;
    case 0xFF42BD:
      Serial.println("* -- ALL ON");
      allOn();
      break;
    case 0xFF4AB5:
      Serial.println("0");
      break;
    case 0xFF52AD:
      Serial.println("# -- ALL OFF");
      allOff();
      break;
    case 0xFFFFFFFF:
      HOLDING = true;
      Serial.print("HOLDING LAST:");
      Serial.println(lastCode, HEX);
      break;
    default:
     break;
  }
  irDetect.resume();
}

void closeAllSelenoids() {
  // Loop 0 - 2, omit 3 so arms stay up.
  for (int i = 0; i <= relayBankSize - 1; i++) {
    if ((stateMap & i) == i
        && !HOLDING
       ) {
      off(i);
    }
  }
}

void setup() {
  irDetect.enableIRIn(); // Start the Receiver
  pinMode(LED_BUILTIN,OUTPUT);    // LED
  digitalWrite(LED_BUILTIN,LOW);  // LED off
  // Setup for Master mode, pins 18/19, internal pullups, 400kHz
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  // Tell RELAY board what's up.
  Wire.beginTransmission(DEVICE_ADDRESS);
    Wire.write(DEVICE_REG_MODE1);
    Wire.write(DEVICE_REG_DATA);
    Serial.println(DEVICE_REG_DATA, HEX);
  Wire.endTransmission();
}

void loop() {
  if (irDetect.decode(&irIn)) {
    decodeIR();
  } else {
    if (((millis() - lastPress) > holdTimeout)) { // DONE HOLDING
      HOLDING = false;
      closeAllSelenoids();
    }
  }
}
