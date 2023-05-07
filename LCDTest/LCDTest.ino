
#include <SimpleDHT.h>
#include <LiquidCrystal.h>
#include <Stepper.h> // Include the header file



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

void setup() {
  lcd.begin(16, 2);
  //lcd.print("hello world");
  Serial.begin(9600);
  
  U0init(9600);
  // setup the ADC
  adc_init();

  stepMotor.setSpeed(300);

      //---set pin direction for DC motor/fan
  pinMode(Enable12,OUTPUT);
  pinMode(Driver1A,OUTPUT);
  pinMode(Driver2A,OUTPUT);
 
  
}

void loop() {
  
 //     adc_init();
    int waterLevel = adc_read(8); 
    //Serial.println(var);

    //testing motor
      if (waterLevel > 200){
          motorCTRL(255, HIGH, LOW);

        }
        
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

  
  //calculate potentiometerVal 
  potentiometerVal = map(adc_read(0),0,1024,0,500);
  if( potentiometerVal != Pval ){
    
    if (potentiometerVal > Pval){
        stepMotor.step(20);
          Serial.println(Pval); //for debugging
      }
    
    if (potentiometerVal < Pval){
      
      stepMotor.step(-20);
        Serial.println(Pval); //for debugging
      }
   
    Pval = potentiometerVal;
  }
  
  
  Serial.println(Pval); //for debugging
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