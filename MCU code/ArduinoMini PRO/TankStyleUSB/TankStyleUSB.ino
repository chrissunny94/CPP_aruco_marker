
//#include <Sabertooth.h>
#include <SoftwareSerial.h>
//#include <SabertoothSimplified.h>
#include <USBSabertooth.h>
#include <Servo.h>

const int servoPin = 5;
Servo myServo;

//SoftwareSerial SWSerial(2, 3); // RX on no pin (unused), TX on pin 3   (to S1).
//SabertoothSimplified ST(SWSerial); // Use SWSerial as the serial port.


SoftwareSerial      SWSerial(2,3); // RX on no pin (unused), TX on pin 11 (to S1).
USBSabertoothSerial C(SWSerial);             // Use SWSerial as the serial port.
USBSabertooth       ST(C, 128);              // Use address 128.



 char message[2];


 int FBspeed = 40;
 int Tangle = 0;
 int speed_delta = 5;
char dir ;
                    

void setup()
{
  Serial.begin(9600);
   SWSerial.begin(9600);
   
   ST.setRamping(1940);
  Serial.print("Started");

  Serial.print("Move Forward");
  ST.drive(1000);
  delay(100);

  
  
  Serial.print("Move Right");
  ST.turn(500);  
  delay(100);
  
  Serial.print("Move Left");
  ST.turn(-500);  
  delay(100);

  Serial.print("Move Backward");
  ST.drive(-1000); 
  delay(100);
  

  Serial.println("Ready");
  ST.drive(0);
  ST.turn(0);

  myServo.attach(servoPin);
  
  // mixes the two together to get diff-drive power levels for both motors.
 
}


int angle = 0;


void loop(){

   if(angle < 255)
    angle++;
    else angle = 0;
    
   myServo.write(angle);
   Serial.print("\rAngle:");
   Serial.print(map(angle, 0 ,255 , 0 ,180));
   
   Serial.print("\r  Sharp:");
   Serial.println(analogRead(A0));
   char inByte = ' ';
   char prev = 'A';
   String input;
   int value;
   if (Serial.available())
   {
    
//    while(true){
//      prev = input;
//      input = Serial.read();
//      Serial.print(input);
//      if(input == ':')
//        break;
//      }

      
     input = Serial.readStringUntil(':');
     inByte = input.charAt(0);
     value = Serial.readStringUntil('\n').toInt();
//
//     
//     Serial.println(prev);
//     Serial.println(value);

//     char inByte = input;
//     Serial.println(prev);
     
     
     
     checkcommand(inByte , value);
     message[0]=0;
     message[1]=0;
     delay(5);
   }
} 



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Drive(int power){
  dir='D';
  ST.drive(power);
  FBspeed = power;
  
  }

 void Turn(int angle){
  dir = 'T';
  ST.turn(angle);
  Tangle = angle;
  }

 void Stop()
 {
  dir='S';

  ST.drive(0);
  ST.turn(0);
  ST.setRamping(1940);
  
 }

 void increaseSpeed(){
    if (FBspeed<127)
        FBspeed = FBspeed + speed_delta;
   Serial.println("increase speed");
   Serial.print(FBspeed);
  }

  void decreaseSpeed(){
    if (FBspeed >-127)
   FBspeed = FBspeed - speed_delta;
   Serial.println("decreased  speed");
   Serial.print(FBspeed);
  
  }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void next(){
      if(dir=='D')
          Drive(FBspeed);
       
            
 }



void checkcommand(char cm , int value)
 {
   
   
   switch(cm){
    case 'D': Drive(value); break;
    case 'T': Turn(value);  break;
    case 'S': Stop() ;              break;
    case '+': increaseSpeed(); break;
    case '-': decreaseSpeed(); break;
   }

 }

