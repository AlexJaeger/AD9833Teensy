/*
works
 */

#include "SPI.h" // necessary library


#define INPUT_SIZE 50
#define ACK 0x00
#define NACK 0x01

#define FREQMIN 5000
#define FREQMAX 50000

#define NUMCOILS 8

// List of commands goes here.
const char cmdFreq[] = "FREQ";
const char cmdRead[] = "READ";

// Set master clock frequency to 6MHz
float f_MCK = 6000000.0;

// Default frequencies for each coil.
// The number of frequencies should match the number of coils
int coilFreqs[NUMCOILS] ={
	20500, // Coil 1
	21500, // Coil 2
	22500, // Coil 3
	23500, // Coil 4
	24500, // Coil 5
	25500, // Coil 6
	26500, // Coil 7
	27500  // Coil 8
};

// Pin numbers for each coil on Teensy 3.2 
int coilEnable[NUMCOILS] ={
  6, // Coil 1
  5, // Coil 2
  4, // Coil 3
  3, // Coil 4
  10,// Coil 5
  9, // Coil 6
  8, // Coil 7
  7  // Coil 8 
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
    for (int i = 0; i < NUMCOILS; i++){
    	pinMode(coilEnable[i], INPUT);
    }

    // Start Serial and reduce timeout to 100ms
    Serial.begin(115200);
    Serial.setTimeout(100);

    // wake up the SPI bus and set SPI mode and SCLK divisor
    SPI.begin();

    // Allow some time for power-on
    delay(200);

    // Program each AD9833 waveform generator with default frequencies
    sendAllFreqs();

  
}


void loop()
{
	int ack = 0;
	if (Serial.available()){
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
    bitsPerHz=(268435456.0/f_MCK);
    // Total number of bits required for the desired frequency
    totalBits=(freq * bitsPerHz);
    // Round up to the nearest integer; 
  
    freqWord=long(ceil(totalBits));

    return freqWord; 
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
    LSB_part = LSB_part|0b0100000000000000;
    MSB_part = MSB_part|0b0100000000000000;


    SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV128, MSBFIRST, SPI_MODE2));
    pinMode(pinNo, OUTPUT);
    digitalWrite(pinNo,LOW);
    
    
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
	// Send each frequency 10 times to ensure SPI works
    for (int i = 0; i < NUMCOILS; i++)
    {
    	for (int j = 0; j < 3; j++){
    		sendFreq(coilFreqs[i], coilEnable[i]);
    	}
    }

}






int processSerial(char *inputbuf)
{
	char* cmd = strtok(inputbuf, " :");
	if (!strcmp(cmd, cmdFreq)){
		cmd = strtok(NULL, " :");
		return processFreqs(cmd);
	}
	else{
		return NACK;
	}
}



int processFreqs(char *cmd)
{
	char *separator = 0;
	int coilNo = 0;
	int coilFreq = 0;

	while (cmd != NULL){
		separator = strchr(cmd, '@');
		if (separator != NULL){
			*separator = '\0';
			coilNo = atoi(cmd);
			separator++;
			coilFreq = atoi(separator);
			// If both the coil and frequiency are valid then program the new frequency
			if (isCoilValid(coilNo) && isFreqValid(coilFreq)){
				coilFreqs[coilNo-1] = coilFreq;
				sendFreq(coilFreqs[coilNo-1], coilEnable[coilNo-1]);
			}
		}
		cmd = strtok(NULL, " :");
	}
}


int isCoilValid(int coilNo)
{
	for(int i = 0; i < NUMCOILS; i++){
		if(i == coilNo){
			return i + 1; // Add +1 to prevent false return
		}
	}
	return false;
}

bool isFreqValid(int freq)
{
	if(freq >= FREQMIN && freq <= FREQMAX){
		return true;
	}
	else{
		return false;
	}
}
