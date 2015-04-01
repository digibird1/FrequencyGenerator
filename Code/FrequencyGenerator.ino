//(c) by D.P. 2015

#include <SPI.h>

const int slaveSelectPin = 10;

const int SegDisp1Pin=2;
const int ClkPin1=3;
const int SegDisp2Pin=4;
const int ClkPin2=5;

const int PushButton1Pin=6;
const int PushButton2Pin=7;

const long MaxFreq=68000;
const long MinFreq=1;

int FirstDigit=-1;
int SecondDigit=-1;

long value=1000;//in kHz
boolean FirstLoop=true;

int LastButtonState=LOW;
long ButtonPushCount=0;


void setup()
{
  pinMode(SegDisp1Pin, OUTPUT);      // Pin of the Segemt Display 1
  pinMode(ClkPin1, OUTPUT); //Clock Port
  pinMode(SegDisp2Pin, OUTPUT);      // Pin of the Segemt Display 2
  pinMode(ClkPin2, OUTPUT); //Clock Port

  pinMode(PushButton1Pin, INPUT); //PushButton1
  pinMode(PushButton2Pin, INPUT); //PushButton2

  //SPI
  // set the slaveSelectPin as an output:
  pinMode (slaveSelectPin, OUTPUT);
  // initialize SPI:
  SPI.setBitOrder(MSBFIRST);
  SPI.begin();


  Serial.begin(115200);        // connect to the serial port
  Serial.println("Welcome Frequency Generator\n\n");
  Serial.println("Enter Frequency  in kHz: ");
}


//Table to translate to 7 Segment display
byte getCodedNumber(int i){
  byte b =B00000000;

  switch (i) {

  case -2:
    b = B11111011;//"0." Zero with a dot
    break;
  case -1:
    b = B00000000;//all segemnts off
    break;
  case 0 : 
    b = B11111010;//0 MSB first
    break;
  case 1 : 
    b = B00100010;//1
    break;
  case 2 :
    b = B01111100;//2 
    break;  
  case 3 :
    b = B01101110;//3 
    break; 
  case 4 : 
    b = B10100110;//4
    break; 
  case 5 : 
    b = B11001110;//5
    break; 
  case 6 : 
    b = B11011110;//6
    break; 
  case 7 : 
    b = B01100010;//7
    break; 
  case 8 : 
    b = B11111110;//8
    break; 
  case 9 : 
    b = B11101110;//9
    break;     
  default : 
    b = B00000000;//
  }  
  return b;
}

void fullShiftReg(int Pin,int ClkPin, int value){
  byte b=getCodedNumber(value);

  for (int i = 0; i < 8; ++i) {
    digitalWrite(ClkPin, LOW);

    if (b & (1 << i)) {
      digitalWrite(Pin, LOW);//Low let the LED turn on
    }
    else {
      digitalWrite(Pin, HIGH);
    }    
    //delay(100);
    digitalWrite(ClkPin, HIGH);
    //delay(100);
  }
}

boolean ButtonIsPushed(int ButtonPin){
  if(digitalRead(ButtonPin)==HIGH){
    if(LastButtonState==LOW){//Check if we released the button in the meantime or push for longer time
      LastButtonState=HIGH;
      delay(100);
      return true;
    }
    else{
      ButtonPushCount++;//count how long the button is pressed
      delay(100);
      if(ButtonPushCount>20000){
        return true;
      }  
      return false; 
    }
  }
  else{//Button is not pressed
    LastButtonState=LOW;
    ButtonPushCount=0;
    return false;
  }
}

//Wite frequency to the SPI bus
void SPIWrite(byte Byte1, byte Byte2) {
  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPin,LOW);
  //send the 16 bits in two bytes
  SPI.transfer(Byte1);
  SPI.transfer(Byte2);
  // take the SS pin high to de-select the chip:
  digitalWrite(slaveSelectPin,HIGH);
}

//Generate the bit pattern for the frequency setting
void calculateFreqSet(long value){
  //We need to calculate in Hz here

    int myOCT=(int)((double)3.322*log10(value*1000./1039.));

  //Serial.print("OCT is:");
  //Serial.println(myOCT);

  int DAC=(int)round(2048-(2078*pow(2,(10+myOCT)))/(value*1000));
  //Serial.print("DAC is:");
  //Serial.println(DAC);

  //Check boundaries
  if(DAC<0) DAC=0;
  if(myOCT<0) myOCT=0;
  if(DAC>1023) DAC=1023;
  if(myOCT>15) myOCT=15;

  //Define the conf bits (last two bits)
  byte CNF =B00000000;

  //int is 16 bits
  //Shift the bits to the right position
  unsigned int BitMap=(myOCT<<12)|(DAC<<2)|CNF;
  //Serial.print("BitMap");
  //Serial.println(BitMap);
  
  //convert into two bytes
  byte Byte1=(byte)(BitMap>>8);
  byte Byte2=(byte)BitMap;
  
  //Serial.println(Byte1);
  //Serial.println(Byte2);
  
  
  SPIWrite(Byte1, Byte2);

}

void loop()
{


  long NewValue=value;

  if(Serial.available () > 0){
    NewValue=Serial.parseInt();//Read in the value
    while (Serial.available() > 0) {
      Serial.read();
    }
  }  


  if(ButtonIsPushed(PushButton1Pin)){//up count
    if(value<=900){//All values below one Mhz are incresed with the pushbutton by 100kHz to full x*100kHz values
      NewValue=(value/100)*100+100;//here we make it a full 100kHz value and add 100
    }
    else if(value<=67000){
      NewValue=(value/1000)*1000+1000;//here we make it a full 1MHz value and add 1MHz
    }
  }

  if(ButtonIsPushed(PushButton2Pin)){//down count
    if(value<=1000&&value>100){//All values up to one Mhz are decresed with the pushbutton by 100kHz to full x*100kHz values
      NewValue=(value/100)*100-100;//here we make it a full 100kHz value and subtract 100
    }
    else if(value<=68000&&value>1000){
      NewValue=(value/1000)*1000-1000;//here we make it a full 1MHz value and subtract 1MHz
    }
  }

  if(NewValue!=value || FirstLoop==true){//value changed or first loop
    FirstLoop=false;

    if(NewValue>MaxFreq || NewValue<MinFreq){
      Serial.println("Frequency out of range!");
    }
    else{
      value=NewValue;
      Serial.print("Frequency set: ");
      Serial.print(value);
      Serial.println("kHz");

      //Serial.print("Frequency set: ");
      //Serial.print(value/1000);
      //Serial.println("MHz");

      //Serial.println((value/1000)/10);

      //Set the oscillator
      calculateFreqSet(value);

      if(value<1000){//100Khz
        FirstDigit=-2;
        SecondDigit=value/100;
      }
      else if(value>=1000 && value<10000){//Mhz
        FirstDigit=-1;
        SecondDigit=(value/1000)%10;
      } 
      else if(value>=10000){//10Mhz
        FirstDigit=(value/1000)/10;
        SecondDigit=(value/1000)%10;   
      }

      fullShiftReg(SegDisp1Pin,ClkPin1, FirstDigit);//Fill second digit
      fullShiftReg(SegDisp2Pin,ClkPin2, SecondDigit);//Fill first digit 
      delay(100);

      //      fullShiftReg(SegDisp1Pin,ClkPin1, (value/1000)/10);//Fill second digit
      //      fullShiftReg(SegDisp2Pin,ClkPin2, (value/1000)%10);//Fill first digit    
    }



    Serial.println("Enter Frequency in kHz: ");
  }
  else {
    delay(100); 
  }


  //SPI buss

}

