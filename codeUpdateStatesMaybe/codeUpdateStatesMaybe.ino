#include <SimpleDHT.h>
#include <LiquidCrystal.h>
#include <Stepper.h> // Include the header file
#include <RTClib.h>

//steps per revolution
#define STEPS 32

#define RDA 0x80
#define TBE 0x20  

//Pointers for UART
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 //Pointers for ADC
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//dc motor
const int Enable12 = 5;  // PWM pin to L293D's EN12 (pin 1) 
const int Driver1A = 4;  // To L293D's 1A (pin 2)
const int Driver2A = 3;  // To L293D's 2A (pin 7)

volatile unsigned char *port_b = (unsigned char *)0x25;
volatile unsigned char *ddr_b = (unsigned char *)0x24;
volatile unsigned char *pin_b = (unsigned char *)0x23;
volatile unsigned char *port_c = (unsigned char *)0x28;
volatile unsigned char *ddr_c = (unsigned char *)0x27;
volatile unsigned char *pin_c = (unsigned char *)0x26;
volatile unsigned char *port_d = (unsigned char *)0x2B;
volatile unsigned char *ddr_d = (unsigned char *)0x2A;
volatile unsigned char *pin_d = (unsigned char *)0x29;
volatile unsigned char *port_j = (unsigned char *)0x105;
volatile unsigned char *ddr_j = (unsigned char *)0x104;
volatile unsigned char *pin_j = (unsigned char *)0x103;
volatile unsigned char *port_h = (unsigned char *)0x102;
volatile unsigned char *ddr_h = (unsigned char *)0x101;
volatile unsigned char *pin_h = (unsigned char *)0x100;
volatile unsigned char *port_l = (unsigned char *)0x10B;
volatile unsigned char *ddr_l = (unsigned char *)0x10A;
volatile unsigned char *pin_l = (unsigned char *)0x109;

int Pval = 0;

int potentiometerVal = 0;

int adc_id = 0;
int HistoryValue = 0;
char printBuffer[128];

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

Stepper stepMotor(STEPS, 22, 24, 23, 25);


// for DHT11,
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2

int pinDHT11 = 2;
SimpleDHT11 dht11(pinDHT11);

//states 
volatile int state = 0; //starting disabled
volatile int previousState = 0;

//interrupts
volatile unsigned char *mySREG = (unsigned char *)0x5F;
volatile unsigned char *myEICRA = (unsigned char *)0x69;
volatile unsigned char *myEIMSK = (unsigned char *)0x3D;

unsigned int waterLevel = 0;
unsigned int temperature = 0;

RTC_DS1307 rtc; //initialize RTC clock

void setup() {
  lcd.begin(16, 2);
  //lcd.print("hello world");
  Serial.begin(9600);
  
  U0init(9600);
  // setup the ADC
  adc_init();

  stepMotor.setSpeed(800);

      //---set pin direction for DC motor/fan
  pinMode(Enable12,OUTPUT);
  pinMode(Driver1A,OUTPUT);
  pinMode(Driver2A,OUTPUT);
  
  *ddr_l |= 0b00001111;  // set pl0-pl3 as output for led
  *port_l &= 0b11110000; // set leds to be off by default
  *port_l |= 0b00000001; // set except yellow :)

  state = 0;
  previousState = 0;

  *port_d |= 0b00001100; // set d0/d1 to have pull up resistor
  *ddr_d &= 0b11110011;  // set pd0/d1 as input for button

  *myEICRA |= 0b10100000; // falling edge mode for interrupt 2/3
  *myEICRA &= 0b10101111; // falling edge mode for interrupt 2/3
  *myEIMSK |= 0b00001100; // turn on interrupt 2/3
  *mySREG |= 0b10000000;  // turn on global interrupt

  lcd.print("Machine is off. ");

  if(!rtc.begin()){ //check
  char printarray[18] = "Couldn't find RTC";
    for(int i = 0;i < 18;i++)
    {
      U0putchar(printarray[i]);
    }
  }  
}

void loop() {

  waterLevel = adc_read(8);
  //when not in error - constantly running
  if (state != 3) {
    
    potentiometerVal = map(adc_read(0),0,1024,0,500);
    if( potentiometerVal != Pval ){
      
      if (potentiometerVal > Pval){
          stepMotor.step(20);
           // Serial.println(Pval); //for debugging
        }
      
      if (potentiometerVal < Pval){
        
        stepMotor.step(-20);
          //Serial.println(Pval); //for debugging
        }
     
      Pval = potentiometerVal;
    } 
  }

  if (state != 0)
  {
    //not disabled
    *port_b |= 0b00000011; //enables sensors

    //add if function to update every 60 seconds?
    displayTempHumidity();
  }  
  
  *mySREG &= 0b01111111; // turn off global interrupt
  delay(100);
  *mySREG |= 0b10000000;  // turn on global interrupt

  //when states happen
  if (waterLevel < 50 && state != 0 && state != 3) // water level low, not disabeled, not in error
  {
    state = 3;
  }

  if (temperature >= 23 && state == 1) // if temp is high and state is idle
  {
    state = 2; // set state to enabled
  }

  if (temperature < 23 && state == 2) // if temp is low and state is enabled
  {
    state = 1; // set state to idle
  }
  //state switches
  switch (state)
  {
    case 0: //disabled
      //machine off
      if (previousState != 0) 
      {

        previousState = 0;
        char printarray[23] = "Machine turned off at ";
        
        for (int i = 0; i < 23; i++)
        {
          U0putchar(printarray[i]);
        }
        RTCtime();
        U0putchar('.');
        U0putchar('\n');  

        //turn on yellow LEDs and turn off others
        //*port_b &= 0b11111100; //sensors off
        *port_l &= 0b11110001; // set leds to be off by default
        *port_l |= 0b00000001; // set except yellow :)
  

        lcd.setCursor(0, 0);
        lcd.print("Machine is off. ");
        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
      break;
    case 1: //idle
      if (previousState != 1)
      {
        previousState = 1;
        char printarray[18] = "Machine idled at ";

        for (int i = 0; i < 18; i++)
        {
          U0putchar(printarray[i]);
        }
        RTCtime();
        U0putchar('.');
        U0putchar('\n');

        //turn on green led and turn off others
        *port_l &= 0b11110010; //others off
        *port_l |= 0b00000010; //green on
        
        //turn off fan
      }
      //call LCD function to display temp and humdity
      break;
    case 2: //running
      //fan on + blue light
      if (previousState != 2)
      {
        previousState = 2;
        char printarray[20] = "Machine enabled at ";

        for (int i = 0; i < 20; i++)
        {
          U0putchar(printarray[i]);
        }
        RTCtime();
        U0putchar('.');
        U0putchar('\n');

        //turn on fan
        //turn on blue light and others off
        *port_l &= 0b11110100; //others off
         *port_l |= 0b00000100;//Blue on
  
      }

      //call LCD function
      break;
    case 3: //error
      if (previousState != 3)
      {
        previousState = 3;
        char printarray[18] = "Machine error at ";
        for (int i = 0; i < 18; i++)
        {
          U0putchar(printarray[i]);
        }
        RTCtime();
        U0putchar('.');
        U0putchar('\n');

        //turn off fan
        //turn on red light and others off

      *port_l &= 0b11111000; //others off    
      *port_l |= 0b00001000; //red on
      Serial.println("Red on");

      }

      lcd.setCursor(0, 0);
      lcd.print("Water level low.");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      break;
  }

  *mySREG &= 0b01111111; // turn off global interrupt
  delay(100);
  *mySREG |= 0b10000000;  // turn on global interrupt 

  //if water level changes -> print
    if(((HistoryValue>= waterLevel ) && ((HistoryValue - waterLevel) > 10)) || ((HistoryValue<waterLevel) && ((waterLevel - HistoryValue) > 10))){
      sprintf(printBuffer,"ADC%d level is %d\n",adc_id, waterLevel);
      Serial.print(printBuffer);
      HistoryValue = waterLevel;

    }
  
//adc_init();
  //start fan (using pinmode rn)
  //motorCTRL(255, HIGH, LOW);

  //temperature and humidity for lcd
  // start working...
//  Serial.println("=================================");
//  Serial.println("Sample DHT11...");
//
//
//  lcd.setCursor(0, 1);
//  //  lcd.print(millis()/1000);
//
//  
//  // read without samples.
//  byte temperature = 0;
//  byte humidity = 0;
//  
//  int err = SimpleDHTErrSuccess;
//  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
//    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
//    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
//    //lcd.print("Read DHT11 failed, err="); lcd.print(SimpleDHTErrCode(err));
//    //Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
//
//    return;
//
//  }
//
//  Serial.print("Sample OK: ");
//  Serial.print((int)temperature); Serial.print(" *C, ");
//  Serial.print((int)humidity); Serial.println(" H");
//
//
//  lcd.print((int)temperature); lcd.print(" C*, ");
//  lcd.print((int)humidity); lcd.print("% H");

  
  //calculate potentiometerVal 
//  potentiometerVal = map(adc_read(0),0,1024,0,500);
//  if( potentiometerVal != Pval ){
//    
//    if (potentiometerVal > Pval){
//        stepMotor.step(20);
//          Serial.println(Pval); //for debugging
//      }
//    
//    if (potentiometerVal < Pval){
//      
//      stepMotor.step(-20);
//        Serial.println(Pval); //for debugging
//      }
//   
//    Pval = potentiometerVal;
//  }
  
  
  //Serial.println(Pval); //for debugging
  // DHT11 sampling rate is 1HZ.
  //delay(1500);
}


void adc_init(){
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num){
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7){
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}


void U0init(int U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit(){
  return *myUCSR0A & RDA;
}
unsigned char U0getchar(){
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata){
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

//motorCTRL(255, HIGH, LOW);
void motorCTRL(byte speed, bool D1A, bool D2A){
    /*  motorCTRL controls the DC motor
     *    speed: any value between 0-255, used as PWM
     *             0 - off
     *           255 - maximum
     *      D1A: Input 1 or 1A, boolean value of HIGH or LOW          
     *      D2A: Input 2 or 2A, boolean value of HIGH or LOW
     */
  analogWrite(Enable12,speed);  // PWM
  digitalWrite(Driver1A,D1A);   // Boolean
  digitalWrite(Driver2A,D2A);   // Boolean 
}

void displayTempHumidity(){
    Serial.println("=================================");
    Serial.println("Sample DHT11...");
  
  
    lcd.setCursor(0, 1);
    //  lcd.print(millis()/1000);
  
    // read without samples.
    byte temperature = 0;
    byte humidity = 0;
    
    int err = SimpleDHTErrSuccess;
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
      Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
      //lcd.print("Read DHT11 failed, err="); lcd.print(SimpleDHTErrCode(err));
      //Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
  
      return;
  
    }
  
    Serial.print("Sample OK: ");
    Serial.print((int)temperature); Serial.print(" *C, ");
    Serial.print((int)humidity); Serial.println(" H");
  
  
    lcd.print((int)temperature); lcd.print(" C*, ");
    lcd.print((int)humidity); lcd.print("% H");
  }

  void RTCtime(){
  DateTime now = rtc.now();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();
  char time[24] = {
    'a',
    't',
    ' ',
    hour / 10 + '0',
    hour % 10 + '0',
    ':',
    minute / 10 + '0',
    minute % 10 + '0',
    ':',
    second / 10 + '0',
    second % 10 + '0',
    'o',
    'n',
    month / 10 + '0',
    month % 10 + '0',
    '/',
    day / 10 + '0',
    day % 10 + '0',
    '/',
    (year / 1000) + '0',
    (year % 1000 / 100) + '0',
    (year % 100 / 10) + '0',
    (year % 10) + '0',
  };
  for (int i = 0; i < 23; i++)
  {
    U0putchar(time[i]);
  }
}
