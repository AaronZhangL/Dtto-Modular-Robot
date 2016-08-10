/*
 * Código módulos Dtto:  robot modular transformable
 * Alberto Molina Pérez - URV
 * */


/////LIBRERIAS/////

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#include <SoftwareSerial.h>
#include <Servo.h> 
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

/////DEFINICIONES/////

#define DttoType 1//0 for MASTER; 1 for SLAVE
#define METALGEAR 0 //0 for NO, 1 for YES

#define PIN 19 //Pin Neopixel

#define CE_PIN 2  //RF pin
#define CSN_PIN 4  //RF pin


int SCenterM =83;
int SCenterF =83;
#define SCenterB 83
#define SMinB 5
#define SMaxB 150

/////CONTROL SINUSOIDAL /////


#define vel 0.002 //Velocidad (w)
//
int ampl = 40; //Amplitud
float desfase = 3.14; // Desfase entre modulos(motores*2)
float desfaseint = 3.14;
unsigned long t = 0;
unsigned long t2 = 0;
unsigned long t_start=0;
byte reset=1;



/////INICIALIZACIONES/////

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);  //number of pix, arduino PIN, pixel type flags

SoftwareSerial bluetooth(7,8); //RX TX

const uint64_t pipes[2] = {0xE8E8F0F0E1LL,0xF0F0F0F0D2LL}; // Define the transmit pipe rf
char RFData[6];
char addmodRF[2]={'a','+'};
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

/////VARIABLES PROGRAMA/////

//bt
char rxChar;    // Variable para recibir datos del puerto serie
int ledpin = 13;  // Pin donde se encuentra conectado el led (pin 13)

String rxDataBT; //Datos recibidos por BT
char rxDataBT2[6]; //Datos recibidos por BT convertidos a char array
char rxDataRF[6]; //Data recibida por RF, valorar si es para ese modulo
char rxData[6]; //Data internal para el modulo
char rxServo;
int angle;
int w0;
int w1;
int w2;
int i=0;
int j=0;

int modnum=1;
int modtotal=1;

//SERVOS
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

///// VARIABLES MOVIMIENTO CIRCULO

#define vel2 0.001 //Velocidad (w)
byte stepc = 0;
byte inicio=0;
byte next = 0;

  unsigned long started_at = millis();
  bool timeout2 = true; 
  int sync_ok=0;
  int sync_data[2];

  
void setup() 
{

    if (METALGEAR)
    {
      SCenterM =90;
      SCenterF =90;
    }
  // put your setup code here, to run once:
    Serial.begin(9600);
    delay(3000);
    
    radio.begin();
    //radio.setRetries(15,5); //Intentos de reenvio en caso de mala comunicacion
    
    if (DttoType==0)
    {
      bluetooth.begin(9600);
      bluetooth.println("Reset");
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1,pipes[1]);
      //radio.openWritingPipie(pipes[2]);
      radio.startListening();
    }
    else
    {
      radio.openWritingPipe(pipes[1]);
      radio.openReadingPipe(1,pipes[0]);
      //radio.openReadingPipe(2,pipes[2]);  
    }
  
    if (DttoType==1) getmodulenum(); //Asignar un numero al modulo esclavo conectado

    strip.begin();
    strip.setBrightness(255);
    strip.setPixelColor(0,0,100,0);
    strip.show();
        
        
       


}


void loop() 
{  
   
  if(DttoType==0)
  {
    assignmodulenum();
    listenBluetooth();
  }
  else 
  {
    listenRF();
  }
      //rxData[1]='r';
      //rxData[2]='m';
      switch (rxData[1])
                
                 {
                    case 's': //start, initial position
                        start();
                      break;
                    case 'r': //Run, sinusoidal control
                        snake();
                      break;
                    case 'e'://Escape
                        escape();
                      break;
                    case 'm'://manual movement
                        manual();
                      break;
                    case 'g'://attach module
                        gancho();
                      break;
                    case 't'://test communications
                        test_comm();
                      break;
                    case 'a'://modificar Amplitud
                        amplitud();
                      break;
                    case 'f'://modificar Fase
                        fase();
                      break;
                    case '+':
                        modtotal++;
                        rxData[1]='e';
                        Serial.println(modtotal);
                        break;
                    case 'h'://test communications
                        modtotal=1;
                        rxData[1]='e';
                      //break;
                    default:
                      break;
                  }
  }


void amplitud()
{
      w0=(rxData[2]-'0')*100;
      w1=(rxData[3]-'0')*10;
      w2=rxData[4]-'0';
      ampl=w0+w1+w2;
      rxData[1]='e';
}

void fase()
{
      w0=(rxData[2]-'0')*100;
      w1=(rxData[3]-'0')*10;
      w2=rxData[4]-'0';
      desfaseint=w0+w1+w2;
      desfaseint=(desfaseint*3.1415)/180;
      desfase=desfaseint;
      rxData[1]='e';
}

void getmodulenum()
{
  int peticion=1234;
  detachServos();
  while (modnum==1)
  { 
      radio.stopListening();
      radio.write(&peticion,sizeof(int));
      radio.startListening();
      unsigned long started_waiting_at = millis();
      bool timeout = false;
      while ( ! radio.available() && ! timeout )       // Esperamos 200ms
      {
          if (millis() - started_waiting_at > 500 )timeout = true;
      }
      if ( ! timeout )
      {   // Leemos el mensaje recibido
           int got_num;
           radio.read(&got_num, sizeof(int) );
           modnum=got_num;
           modtotal=modnum-1;
      }
  }
}


void assignmodulenum()
{
  
      //detachServos();
      if ( radio.available())
      {   // Leemos el mensaje recibido
           int got_peticion;
           int addmod;
           radio.read(&got_peticion, sizeof(int) );
           if (got_peticion==1234)
            {
              radio.stopListening();
              modtotal++;
              radio.write(&modtotal,sizeof(int));              
              radio.write(&addmodRF,6); 
              radio.startListening();
              
            }
      }
}

void listenBluetooth()
{
  
  if( bluetooth.available()>1 ) //Aqui hay num de bytes disponibles up to 64 bytes
  {  
    //  Detach servos para que no crear interferencias con softwareSerial.
      //radio.stopListening();
      detachServos();
      rxDataBT= bluetooth.readStringUntil('\0'); 
      //radio.startListening();
      rxDataBT.toCharArray( rxDataBT2,6 );      
       //Check DATA syntax?? 
      if(rxDataBT2[0]=='1')
        {
            for(i=0;i<6;i++)
            {
              rxData[i]=rxDataBT2[i];
            }
                
        }
      else if (rxDataBT2[0]=='a')
        {
            for(i=0;i<6;i++)
            {
              RFData[i]=rxDataBT2[i];
              rxData[i]=rxDataBT2[i];
            }
            radio.stopListening();
            radio.write( RFData, 6 );
            radio.startListening();
        }
      else
        {
          for(i=0;i<6;i++)
            {
              RFData[i]=rxDataBT2[i];
            }
            radio.stopListening();
            radio.write( RFData, 6 );
            radio.startListening();
        }
  }
  //
}


void syncMaster()
{
            sync_data[0]='S';
            delay(1);
            radio.stopListening();
            radio.write( &sync_data, sizeof(int) );
            radio.startListening();

}

void listenRF()
{
  if ( radio.available() )
  {
    // Read the data payload until we've received everything
   detachServos();
   while (radio.available())
   {
      // Fetch the data payload
      radio.read( rxDataRF, 6 );
    }
      
   if((rxDataRF[0]== (modnum+48))||(rxDataRF[0]=='a') )
   {
    for(i=0;i<6;i++)
            {
              rxData[i]=rxDataRF[i];
            }
   }
   //attachServos();  
  }
}

void gancho()
{
  int rxData2OLD;
  int rxData3OLD;
  
  if ((rxData[2]!=rxData2OLD)||(rxData[3]!=rxData3OLD))
  {  
    switch(rxData[2])
    {
      case 'b':
        servoB.attach(10);
        if (rxData[3]=='0')posB=SCenterB;
        else if (rxData[3]=='1')posB=SMinB;
        else if (rxData[3]=='2')posB=SMaxB;
        servoB.write(posB);      
        break;
      case 'r': 
        servoR.attach(3); 
        if (rxData[3]=='0')posR=SCenterB;
        else if (rxData[3]=='1')posR=SMinB;
        else if (rxData[3]=='2')posR=SMaxB;
        servoR.write(posR);      
        break;
      case 'l':  
        servoL.attach(9);
        if (rxData[3]=='0')posL=SCenterB;
        else if (rxData[3]=='1')posL=SMinB;
        else if (rxData[3]=='2')posL=SMaxB;
        servoL.write(posL);      
        break;
      default:
        break;
    }
    rxData2OLD=rxData[2];
    rxData3OLD=rxData[3];
    delay(150);
    
  }
  detachServos();
}

void start()

{
  attachServos();
  posM = SCenterM;
  posF = SCenterF;
  posB = SCenterB;
  posR = SCenterB;
  posL = SCenterB;
  writeServos();
  delay(250);
  detachServos();

}

void snake()
{ 
  attachServos();
  if(reset) t_start=millis();
  t=millis()- t_start;
  reset=0;
  //Serial.println(posM);
  //Serial.println(posF);
  switch(rxData[2])
  {
    case 'm':
      posM=SCenterM+ampl*sin((vel*t)+(modnum*desfase));   
      posF=SCenterF+ampl*sin((vel*t)+((modnum*desfase)+((desfase)/2))); 
      servoM.write(posM);
      servoF.write(posF);      
      break;
    case 'f':
      posM=SCenterM+ampl*sin((vel*t)-(modnum*desfase));   
      posF=SCenterF+ampl*sin((vel*t)-((modnum*desfase)+((desfase)/2))); 
      servoM.write(posM);
      servoF.write(posF);
      break;
    case 'c':
        switch(modtotal)
            {
            case 3:
              if (stepc == 0) 
              {
                if (modnum==1) 
                {
                  servoB.write(5);
                  delay(1000);
                  stepc=1;
                  delay(1000);
                  servoM.write(0);
                  servoF.write(178);
                  delay(1000);
                }
                else if (modnum==2) 
                {
                  servoB.write(5);
                  delay(1000);
                  stepc=3;
                  delay(400);
                  servoM.write(0);
                  servoF.write(85);
                  delay(600);
                  delay(1000);
                }
                else if (modnum==3) 
                {
                  delay(1000);
                  stepc=2;
                  servoM.write(85);
                  servoF.write(178);
                  delay(1000);
                  delay(1000);
                  servoB.write(5);
                }
                delay(1000);
                t_start=millis();
                next=0;
                
              }
              if(next) 
              {
                stepc++;
                if (stepc ==4) stepc =1;
                delay(500);
                t_start=millis();   
                next=0;    
              }
                t=millis()- t_start;
             
              switch(stepc)
              {
                case 1:
                  posM=85*sin((vel2*t)); 
                  if (posM>83) next++;  
                  servoM.write(posM);
                  servoF.write(178);
                  break;
                case 2:
                  posM=85-85*sin((vel2*t));
                  if(posM<1)posM=0;
                  posF=178-93*sin((vel2*t));
                  if(posF<87)next++;
                  servoM.write(posM);
                  servoF.write(posF);
                  
                  break;
                case 3:
                  posF=85+93*sin((vel2*t));
                  if(posF>176)next++;
                  servoM.write(0);
                  servoF.write(posF);
                  
                  break;
                default:
                break;  
              }
              break;
            break;
            case 4:
              if (stepc == 0) 
              {
                if (modnum==1) 
                {
                stepc=1;
                servoB.write(5);
                delay(2500);
                servoF.write(178);
                servoM.write(2);
                delay(1000);
                }
                else if (modnum==2) 
                {
                  stepc=3;
                  servoB.write(5);
                  delay(3500);
                }
                else if (modnum==3) 
                {
                  stepc=1;
                  servoB.write(5);
                  delay(500);
                  servoM.writeMicroseconds(700);
                  delay(1000);
                  servoF.writeMicroseconds(2288); 
                  delay(2000);
                }
                else if (modnum==4) 
                {
                  stepc=3;
                  servoF.write(178);
                  delay(1500);
                  servoF.write(85);
                  delay(2000);
                  servoB.write(5);

                }
                delay(1000);
                t_start=millis();
                next=0;
                
              }
              if(next) 
              {
                stepc++;
                if (stepc ==5) stepc =1;
                delay(400);
                t_start=millis();   
                next=0;    
              }
                t=millis()- t_start;
             
              switch(stepc)
              {
                case 1:
                  posM=85*sin((vel2*t)); 
                  if (posM>83) next++;  
                  servoM.write(posM);
                  servoF.write(178);
                  break;
                case 2:
                  posF=178-93*sin((vel2*t));
                  if(posF<87)next++;
                  servoM.write(86);
                  servoF.write(posF);
                  
                  break;
                case 3:
                  posM=85-85*sin((vel2*t));
                  if(posM<1)next++;        
                  servoM.write(posM);
                  servoF.write(87);       
                  break;
                case 4:
                  posF=85+93*sin((vel2*t));
                  if(posF>176)next++;
                  servoM.write(1);
                  servoF.write(posF); 
                  break;
                default:
                break;  
              }
              break;
            break;
            }
    default:
      break;
  }
  //detachServos();
}

void manual()
{
    //Lectura angulo introducido
      w0=(rxData[3]-'0')*100;
      w1=(rxData[4]-'0')*10;
      w2=rxData[5]-'0';
      angle=w0+w1+w2;
      if ((angle<=180)&&(angle>=0))
      {
           switch (rxData[2])         
           {
              case 'm':
                  servoM.write(angle);
                break;
              case 'f':
                  servoF.write(angle);
                break;
              default:
        
                break;
            }
      }
}
void escape()
{
  t=0;
  detachServos();
  strip.setPixelColor(0,0,100,0);
  strip.show();
  reset=0;
  stepc=0;
  next=0;
  inicio=0;
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

void test_comm()
{
      
      if((millis()- started_at)>500) strip.setPixelColor(0,0,0,100);
      else strip.setPixelColor(0,100,0,0);
      if((millis()- started_at)>1000) started_at=millis();
      strip.show();
      
}
