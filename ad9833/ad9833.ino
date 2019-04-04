/*
  works
*/

#include "SPI.h" // necessary library


#define INPUT_SIZE 256
#define ACK 0x00
#define NACK 0x01

#define FREQMIN 1000
#define FREQMAX 100000

#define NUMCOILS 8

// List of commands goes here.
const char cmdFreq[] = "FREQ";
const char cmdFreqReset[] = "FREQR";
const char cmdAlive[] = "ALIVE";
const char cmdGetFreq[] = "COIL";
const char cmdRead[] = "READ";
const char cmdRst[]  = "RESET";

// Set master clock frequency to 6MHz
float f_MCK = 6000000.0;


// Frequencies to be programmed on system boot (Hz)
int coilFreqs[NUMCOILS] = {
  20000, // Coil 1
  22000, // Coil 2
  24000, // Coil 3
  26000, // Coil 4
  28000, // Coil 5
  30000, // Coil 6
  32000, // Coil 7
  34000  // Coil 8
};

// Enable pin fpr for each coil on Teensy 3.2
int coilEnable[NUMCOILS] = {
  6, // Coil 1
  5, // Coil 2
  4, // Coil 3
  3, // Coil 4
  10,// Coil 5
  9, // Coil 6
  8, // Coil 7
  7  // Coil 8
};

// Teensy pin number for SPI chip select lines
int coilSelect[NUMCOILS] = {
  15, // Coil 1
  16, // Coil 2
  17, // Coil 3
  19, // Coil 4
  20, // Coil 5
  21, // Coil 6
  22, // Coil 7
  23  // Coil 8
};

// Input serial buffer and size holder
char input[INPUT_SIZE + 1];
byte cmdsize = 0;


// Function declarations
int processSerial(char* input);
int processFreqs(char* cmd);
int isCoilValid(int coil);
bool isFreqValid(int freq);

void setup()
{
  // Set states for the SPI enable pins
  for (int i = 0; i < NUMCOILS; i++) {
    pinMode(coilEnable[i], INPUT);
    pinMode(coilSelect[i], OUTPUT);
    digitalWrite(coilSelect[i], LOW);
  }

  // Start Serial and reduce timeout to 100ms
  Serial.begin(115200);
  Serial.setTimeout(10);

  // wake up the SPI bus and set SPI mode and SCLK divisor
  SPI.begin();
  // Set SCK = 14 (PTD1) drive drive strength enabled. This makes SPI more reliable
  // LED current draw on Pin13 (default) results in SCK error on wavegens.
  CORE_PIN14_CONFIG = PORT_PCR_MUX(2) | PORT_PCR_DSE ; 

  // Allow some time for power-on
  delay(200);

  // Program each AD9833 waveform generator with default frequencies
  sendAllFreqs();


}


void loop()
{
  int ack = 0;
  if (Serial.available()) {
    cmdsize = Serial.readBytes(input, INPUT_SIZE);
    input[cmdsize] = '\0';
    ack = processSerial(input);
  }

  //Spin MCU indefinitely

  delay(50);

}


// Calculate the word required for programming the AD9833 to frequency 'freq'
long calcAD9833FreqReg(float freq)
{
  // Declare variables for calculations
  long freqWord;
  float bitsPerHz;
  float totalBits;

  // Bits per unit Hz with respect to the frequency of the master clock MCLK
  bitsPerHz = (268435456.0 / f_MCK);
  // Total number of bits required for the desired frequency
  totalBits = (freq * bitsPerHz);
  // Round up to the nearest integer;

  freqWord = long(ceil(totalBits));

  return freqWord;
}


void resetAll()
{
  for (int i = 0; i < NUMCOILS; i++)
  {
    reset(coilEnable[i]);
  }
}


void reset(int pinNo)
{
  SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV128, MSBFIRST, SPI_MODE2));
  pinMode(pinNo, OUTPUT);
  digitalWrite(pinNo, LOW);

  delayMicroseconds(25);
  SPI.transfer(highByte(0x2100));
  SPI.transfer(lowByte(0x2100));

  delayMicroseconds(25);
  pinMode(pinNo, INPUT);
  SPI.endTransaction();
  delay(10);
}


void sendFreq(float freq, int pinNo)
{

  delay(10);
  long freq_reg_word;
  word LSB_part;
  word MSB_part;

  freq_reg_word = calcAD9833FreqReg(freq);
  LSB_part = freq_reg_word & 0b0000000000000011111111111111;
  MSB_part = freq_reg_word >> 14;
  LSB_part = LSB_part | 0b0100000000000000;
  MSB_part = MSB_part | 0b0100000000000000;


  SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV128, MSBFIRST, SPI_MODE2));
  pinMode(pinNo, OUTPUT);
  digitalWrite(pinNo, LOW);


  delayMicroseconds(25);
  SPI.transfer(highByte(0x2100));
  SPI.transfer(lowByte(0x2100));
  SPI.transfer(highByte(LSB_part));
  SPI.transfer(lowByte(LSB_part));
  SPI.transfer(highByte(MSB_part));
  SPI.transfer(lowByte(MSB_part));
  SPI.transfer(highByte(0xC000));
  SPI.transfer(lowByte(0xC000));
  SPI.transfer(highByte(0x2000));
  SPI.transfer(lowByte(0x2000));
  delayMicroseconds(25);
  pinMode(pinNo, INPUT);
  SPI.endTransaction();
  delay(10);
}



void sendAllFreqs()
{
  // Send each frequency 3 times to ensure SPI works
  for (int i = 0; i < NUMCOILS; i++)
  {
    for (int j = 0; j < 3; j++) {
      sendFreq(coilFreqs[i], coilEnable[i]);
    }
  }

}






int processSerial(char *inputbuf)
{
  char* cmd = strtok(inputbuf, " :");
  if (!strcmp(cmd, cmdFreq)) {
    cmd = strtok(NULL, " :");
    return processFreqs(cmd);
  }
  else if (!strcmp(cmd, cmdRst)) {
    resetAll();
  }
  else if (!strcmp(cmd, cmdFreqReset)) {
    processFreqsReset(cmd);
  }
  else if (!strcmp(cmd, cmdAlive)) {
    return ACK;
  }
  else if(!strcmp(cmd, cmdRead)){
    Serial.write(readCoils());
    return ACK;
  }
  else {
    return NACK;
  }
}



int processFreqs(char *cmd)
{
  char *separator = 0;
  int coilNo = 0;
  int coilFreq = 0;

  while (cmd != NULL) {
    separator = strchr(cmd, '@');
    if (separator != NULL) {
      *separator = '\0';
      coilNo = atoi(cmd);
      separator++;
      coilFreq = atoi(separator);
      // If both the coil and frequiency are valid then program the new frequency
      if (isCoilValid(coilNo) && isFreqValid(coilFreq)) {
        coilFreqs[coilNo - 1] = coilFreq;
        sendFreq(coilFreqs[coilNo - 1], coilEnable[coilNo - 1]);
      }
    }
    cmd = strtok(NULL, " :");
  }
}

int processFreqsReset(char *cmd)
{
  resetAll();
  char *separator = 0;
  int coilNo = 0;
  int coilFreq = 0;

  while (cmd != NULL) {
    separator = strchr(cmd, '@');
    if (separator != NULL) {
      *separator = '\0';
      coilNo = atoi(cmd);
      separator++;
      coilFreq = atoi(separator);
      // If both the coil and frequiency are valid then program the new frequency
      if (isCoilValid(coilNo) && isFreqValid(coilFreq)) {
        selectCoil(coilNo - 1);
        coilFreqs[coilNo - 1] = coilFreq;
        sendFreq(coilFreqs[coilNo - 1], coilEnable[coilNo - 1]);
      }
    }
    cmd = strtok(NULL, " :");
  }
}


void selectCoil(int coilNo)
{
  for (int i = 0; i < NUMCOILS; i++) {
    if (i == coilNo) {
      digitalWrite(coilSelect[i], LOW); // Set to transmit
    }
    else {
      digitalWrite(coilSelect[i], HIGH); // Set to receive
    }
  }
}



int isCoilValid(int coilNo)
{
  for (int i = 1; i <= NUMCOILS; i++) {
    if (i == coilNo) {
      return i ; // Add +1 to prevent false return
    }
  }
  return false;
}

bool isFreqValid(int freq)
{
  if (freq >= FREQMIN && freq <= FREQMAX) {
    return true;
  }
  else {
    return false;
  }
}

char* readCoils(){
    char str[128];
    int i=0;
    int index = 0;
    for (i=0; i<NUMCOILS; i++) {
                index += sprintf(&str[index], "%d \n", coilFreqs[i]);
    }
    char *coilstr = (char*)malloc(128);
    strcpy(coilstr, str);
    return coilstr;
}
