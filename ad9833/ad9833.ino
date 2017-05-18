/*
works
 */

#include "SPI.h" // necessary library

// Set master clock frequency to 6MHz
float f_MCK=6000000;

// Default frequencies for each coil.
float coilfreq1=10500;
float coilfreq2=11500;
float coilfreq3=12500;
float coilfreq4=13500;
float coilfreq5=14500;
float coilfreq6=15500;
float coilfreq7=16500;
float coilfreq8=17500;

const int coilen1 = 3;
const int coilen2 = 4;
const int coilen3 = 5;
const int coilen4 = 6;
const int coilen5 = 7;
const int coilen6 = 8;
const int coilen7 = 9;
const int coilen8 = 10;


void setup()
{

    pinMode(coilen1, INPUT);
    pinMode(coilen2, INPUT);
    pinMode(coilen3, INPUT);
    pinMode(coilen4, INPUT);
    pinMode(coilen5, INPUT);
    pinMode(coilen6, INPUT);
    pinMode(coilen7, INPUT);
    pinMode(coilen8, INPUT);

    digitalWrite(coilen1, HIGH);
    digitalWrite(coilen2, HIGH);
    digitalWrite(coilen3, HIGH);
    digitalWrite(coilen4, HIGH);
    digitalWrite(coilen5, HIGH);
    digitalWrite(coilen6, HIGH);
    digitalWrite(coilen7, HIGH);
    digitalWrite(coilen8, HIGH);

    // wake up the SPI bus and set SPI mode and SCLK divisor.
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV128);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE2);
  
}


void loop()
{

    // Allow some time for power-on
    delay(3000);

    // Program each AD9833 waveform generator
    sendAllFreqs();


    //Spin MCU indefinitely
    while(1){
    delay(200);
    //sendFreq(coilfreq1,coilen1);
    }
}


// Calculate the word required for programming the AD9833 to frequency 'freq'
long calc_freq_reg(float freq)
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





void sendFreq(float freq, int pin_no)
{

    delay(100);
    long freq_reg_word;
    word LSB_part;
    word MSB_part;

    freq_reg_word=calc_freq_reg(freq);
    LSB_part=freq_reg_word&0b0000000000000011111111111111;
    MSB_part=freq_reg_word >> 14;
    LSB_part=LSB_part|0b0100000000000000;
    MSB_part=MSB_part|0b0100000000000000;

    
    digitalWrite(pin_no,LOW);
    pinMode(pin_no, OUTPUT);
    
    delayMicroseconds(100);
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
    pinMode(pin_no, INPUT);

    delay(100);
}



void sendAllFreqs()
{
    int i = 0;

    for (i=0;i<10;i++)
    {
        sendFreq(coilfreq1, coilen1);
        sendFreq(coilfreq2, coilen2);
        sendFreq(coilfreq3, coilen3);
        sendFreq(coilfreq4, coilen4);
        sendFreq(coilfreq5, coilen5);
        sendFreq(coilfreq6, coilen6);
        sendFreq(coilfreq7, coilen7);
        sendFreq(coilfreq8, coilen8);
    }

}

