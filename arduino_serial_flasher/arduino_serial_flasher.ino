#define RESET_PIN 2
#define CLK_PIN 3
#define SERIAL_PIN 4
#define nENABLE_PIN 5
#define LSB_PIN 13
#define MSB_PIN 6

const int chunkSize = 8;
const char startOfMessage = '<';
const char endOfMessage = '>';

bool receivedMessage = false;
String message = "";
byte chunkBuffer[chunkSize];

// === === === === === === === FUNCTIONS === === === === === === === ===
void resetPin(int pin, bool type, bool level) {
  pinMode(pin, type);
  digitalWrite(pin, level);
}

void setAddress(int address) {
  // reset the serial shifter (active low)
  digitalWrite(RESET_PIN, LOW);
  delay(1);
  digitalWrite(RESET_PIN, HIGH);

  // shift in the address
  shiftOut(SERIAL_PIN, CLK_PIN, LSBFIRST, (byte)address);
  shiftOut(SERIAL_PIN, CLK_PIN, LSBFIRST, address >> 8);
}

void waitForSerialMessage(void) {
  char currentChar = ' ';
  receivedMessage = false;

  while (!receivedMessage) {
    while (Serial.available() > 0) {
      currentChar = Serial.read();
      if (currentChar == startOfMessage) {
        message = "";
      } else if (currentChar == endOfMessage) {
        receivedMessage = true;
      } else {
        message += String(currentChar);
      }
    }
    delay(1);
  }
}

void changeAccessMode(String mode) {
  if (mode == "read") {
    for (int pin = MSB_PIN; pin <= LSB_PIN; pin++) {
      pinMode(pin, INPUT);
    }
  } else if (mode == "write") {
    for (int pin = MSB_PIN; pin <= LSB_PIN; pin++) {
      pinMode(pin, OUTPUT);
    }
  }
}

// ****** READ ******
byte readDataByte(int address) {
  setAddress(address);
  byte data = 0;
  for (int pin = MSB_PIN; pin <= LSB_PIN; pin++) {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}

void sendMemoryContents() {
  // prepare EPROM for read access
  digitalWrite(nENABLE_PIN, LOW);
  changeAccessMode("read");

  Serial.print(startOfMessage);
  for (int adr = 0; adr < 256; adr ++) {
    byte b = readDataByte(adr);
    Serial.println(b);
  }
  Serial.print(endOfMessage);
  
  digitalWrite(nENABLE_PIN, HIGH);
}

// ***** WRITE ******
void writeDataByte(int address, byte data) {
  setAddress(address);

  for(int pin=LSB_PIN; pin>=MSB_PIN; pin--){
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }

  delay(1);
  digitalWrite(nENABLE_PIN, LOW);
  delay(50);
  digitalWrite(nENABLE_PIN, HIGH);
  delay(1);
}

void writeProgramChunk(int startIndex, int chunkSize) {
  for (uint8_t chunkAdr = 0; chunkAdr < chunkSize; chunkAdr++) {
    writeDataByte(startIndex + chunkAdr, chunkBuffer[chunkAdr]);
  }
}

// === === === === === === === SETUP === === === === === === === ===

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000);

  resetPin(CLK_PIN, OUTPUT, LOW);
  resetPin(RESET_PIN, OUTPUT, HIGH);
  resetPin(SERIAL_PIN, OUTPUT, LOW);
  resetPin(nENABLE_PIN, OUTPUT, LOW);

  while (!Serial) {
    delay(10);
  }
  Serial.println("<READY>");
}


// === === === === === === === LOOP === === === === === === === ===

void loop() {
  waitForSerialMessage();

  if (message.equals("rm"))
  {
    sendMemoryContents();
  }
  else if (message.equals("wm"))
  {
    // prepare the chip for write procedure
    changeAccessMode("write");
    digitalWrite(nENABLE_PIN, HIGH);
    delay(1);
    
    bool transferIsDone = false;
    int chunkIndex = 0;
    Serial.println("<WRITE-READY>");
    while (!transferIsDone) {
      int rlen = Serial.readBytes(chunkBuffer, chunkSize);
      writeProgramChunk(chunkIndex, rlen);
      if (rlen < chunkSize) {
        transferIsDone = true;
      }
      chunkIndex += chunkSize;
      Serial.println("<CONTINUE>");
    }
    Serial.println("<DONE>");
  }

}


// EOF