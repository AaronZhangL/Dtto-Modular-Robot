/*
 * Código módulo 1 robot modular transformable
 * Alberto Molina Pérez - URV
 * 
 * Serial1 para comunicarse con el PC
 * servo.detach(); para evitar los impulsos de utilizar el softwareSerial. Solo habilitar servos a la hora de escribir
 * 
 * 
 */


 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 //repasar attach detach servos, hay errores y se lia comunicacion con servos en algunas partes
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /*-----( Import needed libraries )-----*/
  
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#include <SoftwareSerial.h>
#include <Servo.h> 
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

/*-----( Declare Constants and Pin Numbers )-----*/
#define PIN 19 //Pin Neopixel

#define CE_PIN   2
#define CSN_PIN 4


/////////////////////////////////////////////////////////NEOPIXEL//////////////////////////////
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

////////////////////////////////////BLUETOOTH///////////////////////////////////////////////////

SoftwareSerial bluetooth(7,8); //RX TX
char rxChar;    // Variable para recibir datos del puerto serie
int ledpin = 13;  // Pin donde se encuentra conectado el led (pin 13)
char rxData[5]; //En el master este es u string, aqui con un array es mas simple
char rxServo;
int rxLength;
int rxAvailable;
int angle;
int w0;
int w1;
int w2;
char i=0;

///////////////////////////////Declaración servos y posicion central
Servo servoM;
int posM = 82;//MIN 001, MAX 172
Servo servoF;
int posF = 83;//MIN002, MAX 174
Servo servoB;
int posB = 82;
Servo servoR;
int posR= 80;
Servo servoL;
int posL = 80;

// NOTE: the "LL" at the end of the constant is "LongLong" type
const uint64_t pipe = 0xE8E8F0F0E1LL; // Define the transmit pipe
char RFData[5];

/*-----( Declare objects )-----*/
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
/*-----( Declare Variables )-----*/
int joystick[2];  // 2 element array holding Joystick readings

/////////////////////////////Programa

int mode =0;
int pos;
int sentido=0;

// Configurar el arduino
void setup()
{
  // Pin 13 como salida
  pinMode(ledpin, OUTPUT);
  // Comunicación serie a 9600 baudios
  bluetooth.begin(9600);
  attachServos();
  //writeServos();
  bluetooth.println("Reset");

   radio.begin();
   radio.openReadingPipe(1,pipe);
   radio.startListening();
  // End of trinket special code
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  /////////////////////////////SerialTestingConfiguration///////////////////////
  Serial.begin(9600);
  delay(1000);
  //////////////////////////////////////////////////////////////////////////////
}
 
// Ciclo infinito, programa principal
void loop()
{ 
  detachServos();
  neopixel();
  listenRF();

  switch (mode)
            
             {
                case 0: //help
                    help();
                  break;
                case 1: //start, initial position
                    start();
                  break;
                case 2://Run, experimental run
                    snake();
                  break;
                case 3://manual movement
                    manual();
                  break;
                case 4:// exit mode
                   
                  break;
                default:
          
                  break;
              }

}

void listenRF()
{

  if ( radio.available() )
  {
    // Read the data payload until we've received everything
    bool done = false;
   while (radio.available())
   {
      // Fetch the data payload
      radio.read( rxData, sizeof(rxData) );
 
    }
    Serial.println(rxData);
  }
  
  

   
  
}

void help()
{
      switch(rxData[0])
   {
              case 'h': //help
               mode=0;
                break;
              case 's': //start
               mode=1;   
                break;
              case 'r': //run
               mode=2; 
                break;
              case 'm': //manual
               mode=3; 
                break;
              case 'e': //Esc
               mode=4; 
                break;
              default:
               mode=0; 
                break;
            }
}

void start()

{
attachServos();
posM = 82;
posF = 83;
posB = 82;
posR= 80;
posL = 80;
writeServos();
delay(250);
detachServos();
listenRF();
  switch(rxData[0])
   {
              case 'h': //help
               bluetooth.println(" DittoV1 is in the standby position. Press 'e.' to return to the menu.\n ");
               rxData[0]=1;
                break;
              case 'e': //Esc
               mode=0; 
               bluetooth.println(" Select a mode or press 'h.' to get some help.\n ");
               rxData[0]=1;
                break;
              default:
               mode=1; 
                break;
            }
}

void snake()
{
    while (mode==2)
    {
        listenRF();
        switch(rxData[0])
         {
              case 'h': //help
                 rxData[0]=1;
                 break;
              case 'e': //Esc
                 mode=0; 
                 rxData[0]=1;
                 break;
              case 'm': //Male dirige el rumbo
                sentido=1;
                break;
              case 'f': //Female dirige el rumbo
                sentido=2;
                break;
              case 'p': //Pause motion
                sentido=0;
                break;
              default:
               mode=2; 
                break;
            }

            if(sentido==1) //Los sentidos estan desfasados 180º en esta versión
            {
                  attachServos();

                          for(pos = 10; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
                        {                                  // in steps of 1 degree 
                          servoM.write(100-pos); 
                          servoF.write(pos);// tell servo to go to position in variable 'pos' 
                          //if(bluetooth.available()) pos=90;
                          delay(7);                       // waits 15ms for the servo to reach the position 
                        } 
                          for(pos = 10; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
                        {                                  // in steps of 1 degree 
                          servoM.write(pos); 
                          servoF.write(80+pos);// tell servo to go to position in variable 'pos' 
                          //if(bluetooth.available()) pos=90;
                          delay(7);                       // waits 15ms for the servo to reach the position 
                        } 
                          for(pos = 10; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
                        {                                  // in steps of 1 degree 
                          servoM.write(80+pos); 
                          servoF.write(180-pos);// tell servo to go to position in variable 'pos' 
                          //if(bluetooth.available()) pos=90;
                          delay(7);                       // waits 15ms for the servo to reach the position 
                        } 
                        for(pos = 10; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
                        {                                  // in steps of 1 degree 
                          servoM.write(180-pos); 
                          servoF.write(100-pos);// tell servo to go to position in variable 'pos' 
                          //if(bluetooth.available()) pos=90;
                          delay(7);                       // waits 15ms for the servo to reach the position 
                        } 
                        detachServos();
            }
            if(sentido==2)
            {
              attachServos();

                  for(pos = 10; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
                {                                  // in steps of 1 degree 
                  servoF.write(100-pos); 
                  servoM.write(pos);// tell servo to go to position in variable 'pos' 
                  //if(bluetooth.available()) pos=90;
                  delay(7);                       // waits 15ms for the servo to reach the position 
                } 
                  for(pos = 10; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
                {                                  // in steps of 1 degree 
                  servoF.write(pos); 
                  servoM.write(80+pos);// tell servo to go to position in variable 'pos' 
                  //if(bluetooth.available()) pos=90;
                  delay(7);                       // waits 15ms for the servo to reach the position 
                } 
                 for(pos = 10; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
                {                                  // in steps of 1 degree 
                  servoF.write(80+pos); 
                  servoM.write(180-pos);// tell servo to go to position in variable 'pos' 
                  //if(bluetooth.available()) pos=90;
                  delay(7);                       // waits 15ms for the servo to reach the position 
                } 
                for(pos = 10; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
                {                                  // in steps of 1 degree 
                  servoF.write(180-pos); 
                  servoM.write(100-pos);// tell servo to go to position in variable 'pos' 
                  //if(bluetooth.available()) pos=90;
                  delay(7);                       // waits 15ms for the servo to reach the position 
                }                 
                detachServos();
            }
        }     
      }
  





void manual()
{
  
listenRF();
      rxServo = rxData[0];
  
    //Lectura angulo introducido
      w0=(rxData[1]-'0')*100;
      w1=(rxData[2]-'0')*10;
      w2=rxData[3]-'0';
      angle=w0+w1+w2;

    //Attach servos para realizar movimientos.
      attachServos();
      
      //if (rxLength == 4) No le llega rxLength de la misma manera
      
            switch (rxServo)
          
           {
              case 'M':
                    while(posM<angle)
                      {
                        servoM.write(posM++);
                        delay(2);
                      }
                    while(posM>angle)
                      {
                        servoM.write(posM--);
                        delay(2);
                      }  
                  posM=angle;
                  
                  servoM.write(posM);
                break;
              case 'F':
                
                    while(posF<angle)
                      {
                        servoF.write(posF++);
                        delay(2);
                      }
                    while(posF>angle)
                      {
                        servoF.write(posF--);
                        delay(2);
                      }                      
                  posF=angle;
                  servoF.write(posF);
                break;
              case 'B':
                  posB=angle;  
                   
                  servoB.write(posB);
                break;
              case 'R':
                  posR=angle;  
                   
                  servoR.write(posR);
                break;
              case 'L':
                  posL=angle;   
                   
                  servoL.write(posL);
                break;
              default:
        
                break;
            }
            rxServo=0;
      
  
 
  // Podemos hacer otras cosas aquí
  delay(50);
  detachServos();
  switch(rxData[0])
   {
              case 'h': //help
               rxData[0]=1;
                break;
              case 'e': //Esc
               mode=0; 
               rxData[0]=1;
                break;
              default:
               mode=3; 
                break;
            }
}

void attachServos()
{
  servoM.attach(6);
  servoF.attach(5);
  servoB.attach(10);
  servoR.attach(3);
  servoL.attach(9);
}
  
void writeServos()
{
  servoM.write(posM);
  servoF.write(posF);
  servoB.write(posB);
  servoR.write(posR);
  servoL.write(posL);
}

void detachServos()
{
  servoM.detach();
  servoF.detach();
  servoB.detach();
  servoR.detach();
  servoL.detach();
}

void neopixel()
{
  strip.setPixelColor(0,0,100,0);
  strip.show();
  
}

