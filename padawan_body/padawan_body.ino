// =======================================================================================
// /////////////////////////Padawan Body Code v1.8 ///////////////////////////////////////
// =======================================================================================
//                                 danf
//                        Revised  Date: 03/26/13
//   Designed to be used with a second Arduino running the Padawan Dome code
//              EasyTransfer and PS2X_lib libraries by Bill Porter
//
//      REVISIONS by Dan Kraus for I2C Support
//
//         Set Sabertooth 2x25 Dip Switches 1 and 2 Down, All Others Up
//         For SyRen Simple Serial Set Switches 1 and 2 Down, All Others Up
//         For SyRen Simple Serial Set Switchs 2 & 4 Down, All Others Up
//         Placed a 10K ohm resistor between S1 & GND on the SyRen 10 itself

//************************** Set speed and turn here************************************//

byte drivespeed1 = 50; //set these 3 to whatever speeds work for you. 0-stop, 127-full speed.
byte drivespeed2 = 100;//Recommend beginner: 50 to 75, experienced: 100 to 127, I like 100.
byte drivespeed3 = 127;//Set to 0 if you only want 2 speeds.

byte drivespeed = drivespeed1;  

byte turnspeed = 70;   // the higher this number the faster it will spin in place, lower - easier to controll. 
// Recommend beginner: 40 to 50, experienced: 50 $ up, I like 75

byte domespeed = 80;  // If using a speed controller for the dome, sets the top speed
// Use a number up to 127 for serial

byte ramping = 5;      // Ramping- the lower this number the longer R2 will take to speedup or slow down,
// change this by incriments of 1

byte domecompensation = 10; // For controllers that centering problems, causing slight dome drift in one direction
// use the lowest number with no drift

int domeBaudeRate = 2400; // Set the baude rate for the Syren motor controller
// for packetized options are: 2400, 9600, 19200 and 38400
// for simple use 9600

//#define SYRENSIMPLE   // Comment out for packetized serial connection to Syren - Recomended
// Un-comment for simple serial - do not use in close contact with people.

#define EXTINGUISHERPIN 3

///////include libs and declare variables////////////////////////////
#include <Sabertooth.h>
//#include <SoftwareSerial.h>
#include <SyRenSimplified.h>
#include <PS2X_lib.h>  //for v1.7
#include <Servo.h> 
#include <MP3Trigger.h>
MP3Trigger trigger;
//#include <SoftEasyTransfer.h>
#include <Wire.h>

///DOME SERVO PANELS
#include <Adafruit_PWMServoDriver.h>
Adafruit_PWMServoDriver domeServos = Adafruit_PWMServoDriver(0x40);
int PiePanelClosePositions[4] = {280, 298, 275, 297}; // close spots
int PiePanelOpenPositions[4] = {597, 600, 594, 600}; // open spots
int SidePanelClosePositions[4] = {180, 208, 211, 200}; // close spots
int SidePanelOpenPositions[4] ={600, 600, 600, 600}; // open spots 

boolean isPiePanelOpen[4];
boolean isSidePanelOpen[4];

//////////////////////////////////////////////////////////////////


/*   For Arduino 1.0 and newer, do this:   */
#include <SoftwareSerial.h>
SoftwareSerial domeSerial(2, 8);
SoftwareSerial STSerial(2, 7);
SoftwareSerial SyRSerial(2, 5);
/*   For Arduino 22 and older, do this:   */
//#include <NewSoftSerial.h>
//NewSoftSerial domeSerial(2, 8);//create software serial port
//NewSoftSerial STSerial(2, 7);
//NewSoftSerial SyRSerial(2, 5);

/////////////////////////////////////////////////////////////////
Sabertooth ST(128, STSerial);
#if defined(SYRENSIMPLE)
SyRenSimplified SyR(SyRSerial); // Use SWSerial as the serial port.
#else
Sabertooth SyR(128, SyRSerial);
#endif
//SoftEasyTransfer ET;//create object

struct SEND_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int hpx; // hp movement
  int hpy; // hp movement
  int hpl; // hp light
  int hpa; // hp automation
  int dsp; // 0 = random, 1 = alarm, 5 = leia, 11 = alarm2, 100 = no change
};

SEND_DATA_STRUCTURE domeData;//give a name to the group of data

PS2X ps2x; // create PS2 Controller Class
int error = 0; // part of the ps2x lib
byte type = 0; // part of the ps2x lib
byte vibrate = 0; // part of the ps2x lib
byte vol = 20; // 0 = full volume, 255 off 
byte drive = 0; // 0 = drive motors off ( right stick disabled )
byte automate = 0;
unsigned long automateMillis = 0;
byte automateDelay = random(5,20);// set this to min and max seconds between sounds
int turnDirection = 20;
byte action = 0;
unsigned long DriveMillis = 0;
int drivenum = 0;
int sticknum = 0;
int domenum = 0;
int turnnum = 0;
//////////////set up run once//////////////////////////////////////////

void setup(){
  pinMode(EXTINGUISHERPIN, OUTPUT);
  digitalWrite(EXTINGUISHERPIN, HIGH); 
  
  SyRSerial.begin(domeBaudeRate);
#if defined(SYRENSIMPLE)
  SyR.motor(0);            
#else
  SyR.autobaud(); 
#endif 
  STSerial.begin(9600);   // 9600 is the default baud rate for Sabertooth packet serial.
  ST.autobaud();          // Send the autobaud command to the Sabertooth controller(s).
  // NOTE: *Not all* Sabertooth controllers need this command.
  //       It doesn't hurt anything, but V2 controllers use an
  //       EEPROM setting (changeable with the function setBaudRate) to set
  //       the baud rate instead of detecting with autobaud.
  //
  //       If you have a 2x12, 2x25 V2, 2x60 or SyRen 50, you can remove
  //       the autobaud line and save yourself two seconds of startup delay.

  ST.setTimeout(950);
  SyR.setTimeout(950);
  
  ST.drive(0); // The Sabertooth won't act on mixed mode packet serial commands until
  ST.turn(0);  // it has received power levels for BOTH throttle and turning, since it
  // mixes the two together to get diff-drive power levels for both motors.
  trigger.setup();

  error = ps2x.config_gamepad(13,11,10,12, true, false);   //setup pins and settings:  GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  trigger.setVolume(vol);//anything with trigger. is for the mp3trigger
  type = ps2x.readType();
  domeSerial.begin(57600);//start the library, pass in the data details and the name of the serial port.
  //ET.begin(details(domeData), &domeSerial); 
  Wire.begin(); // Start I2C Bus as Master
  
  
}

///////Loop run over and over//////////////////////////////////////
//////////////////////////////////////////////////////////////////

void loop(){

  if(error == 1) //skip loop if no controller found
 
  return; 
 
  if(ps2x.Analog(PSS_RX) ==255 && ps2x.Analog(PSS_RY) ==255 && ps2x.Analog(PSS_LX)==255 &&ps2x.Analog(PSS_LY)==255)
  { 
    ST.drive(0);
    ST.turn(0);
    SyR.motor(1,0);
    return; 
   }

  ps2x.read_gamepad();   //read controller and set large motor to spin at 'vibrate' speed

    //// enable / disable right stick & play sound 
  if(ps2x.ButtonPressed(PSB_START)) 
  {
    if (drive<1)
    {
      drive = 1; 
      trigger.play(52);
    }
    else {
      drive = 0; 
      trigger.play(53);
    }
  }

  ////turn hp automation or automate on & off      
  if(ps2x.ButtonPressed(PSB_SELECT)) {
    if(ps2x.Button(PSB_R2)){
//      if(domeData.hpa == 1)
//      {
//        domeData.hpa = 0; 
//        domeData.dsp = 100; 
//        ET.sendData();
//      }//if hp automation is off, turn it on
//      else
//      {
//        domeData.hpa = 1; 
//        domeData.dsp = 100; 
//        ET.sendData();
//      }//turn hp automation off
    } 
    /////////////automate
    else{
      if (automate<1){
        automate = 1; 
        trigger.play(52);
      }
      else {
        automate = 0; 
        action = 0; 
        trigger.play(53);
      }
    }
  }

  if (automate == 1){ 
    if (ps2x.Analog(PSS_LX)!=128){ 
      automateMillis = millis();
    }
    
    unsigned long currentMillis = millis();
    
    if (currentMillis - automateMillis > (automateDelay*1000)){
      automateMillis = millis();
      action = random(1,5);
      
      if (action>1){
        (trigger.play(random(32,52)));
      }
      if (action<4){
#if defined(SYRENSIMPLE) 
        SyR.motor(turnDirection);
#else
        SyR.motor(1,turnDirection);
#endif
        delay(750);
#if defined(SYRENSIMPLE) 
        SyR.motor(0);
#else
        SyR.motor(1,0);
#endif
        if (turnDirection>0){
          turnDirection = -45;
        }
        else{
          turnDirection = 45;
        }
      }
      automateDelay = random(5,20);// set this to min and max seconds between sounds
    }
  }
  ////volume controll
  if(ps2x.Button(PSB_PAD_UP)) {
    // volume up
    if(ps2x.Button(PSB_R1)){ 
      if (vol>0){
        vol--;
        trigger.setVolume(vol);
      }
    }  
  }
  if(ps2x.Button(PSB_PAD_DOWN)){
    //volume down
    if(ps2x.Button(PSB_R1)){ 
      if (vol<255){
        vol++;
        trigger.setVolume(vol);
      }
    } 
  }
  
   ////Logic display brightness
   if(ps2x.ButtonPressed(PSB_PAD_UP)) 
   {
    if(ps2x.Button(PSB_L1))
     {
       triggerI2C(10, 24); 
     }  
   }
    if(ps2x.ButtonPressed(PSB_PAD_DOWN)) 
    {
    if(ps2x.Button(PSB_L1))
     {
       triggerI2C(10, 25); 
     } 
   }

  //FIRE EXTINGUISHER
  if(ps2x.ButtonPressed(PSB_PAD_UP)){
    if ((ps2x.Button(PSB_L2))){
      if(isSidePanelOpen[0]){
        //TRIGGER EXTINGUISHER
        digitalWrite(EXTINGUISHERPIN, LOW);
      } else {
        openSidePanel(0);
      }
    }
  }
  //on up release, turn off extinguisher
  if(ps2x.ButtonReleased(PSB_PAD_UP)){
    if ((ps2x.Button(PSB_L2))){
      if(isSidePanelOpen[0]){
        digitalWrite(EXTINGUISHERPIN, HIGH);
      }
    }
  }
  
  if(ps2x.ButtonPressed(PSB_PAD_DOWN)){
    if ((ps2x.Button(PSB_L2))){
      //digitalWrite(EXTINGUISHERPIN, LOW);
    }
  }

  /////hp movement
//  if(ps2x.ButtonPressed(PSB_PAD_UP)){
//    if (!(ps2x.Button(PSB_R1))){
//      domeData.hpy = 30; 
//      domeData.dsp = 100; 
//      //ET.sendData();
//    }
//  }
//  if(ps2x.ButtonPressed(PSB_PAD_DOWN)){
//    if (!(ps2x.Button(PSB_R1))){
//      domeData.hpy = 150; 
//      domeData.dsp = 100; 
//      //ET.sendData();
//    }
//  }
//  if(ps2x.ButtonReleased(PSB_PAD_UP)||ps2x.ButtonReleased(PSB_PAD_DOWN)){
//    domeData.hpy = 0; 
//    domeData.dsp = 100; 
//    //ET.sendData();
//  }
//
//  if(ps2x.ButtonPressed(PSB_PAD_RIGHT)){
//    domeData.hpx = 150; 
//    domeData.dsp = 100; 
//    //ET.sendData();
//  }
//  if(ps2x.ButtonPressed(PSB_PAD_LEFT)){
//    domeData.hpx = 30; 
//    domeData.dsp = 100; 
//    //ET.sendData();
//  }
//  if(ps2x.ButtonReleased(PSB_PAD_RIGHT)||ps2x.ButtonReleased(PSB_PAD_LEFT)){
//    domeData.hpx = 0; 
//    domeData.dsp = 100; 
//    //ET.sendData();
//  }


  /////play sounds and change display
  
  
  //triangle top
  if(ps2x.ButtonPressed(PSB_GREEN)){
    //L1+Triangle
    if(ps2x.Button(PSB_L1)){
      (trigger.play(8)); 
      //logic lights, random
      triggerI2C(10, 0); 
    }
    else if(ps2x.Button(PSB_L2)){
      (trigger.play(2)); 
      //logic lights, random
      triggerI2C(10, 0);
    }
    else if(ps2x.Button(PSB_R1)){
      (trigger.play(9));
      //logic lights, random
      triggerI2C(10, 0);
    } else {
      (trigger.play(random(13,17))); 
      //logic lights, random
      triggerI2C(10, 0);
    }
  }
  
  //x bottom
  if(ps2x.ButtonPressed(PSB_BLUE)){
    if(ps2x.Button(PSB_L1)){
      (trigger.play(6)); 
      //logic lights
      triggerI2C(10, 6);
      // HPEvent 11 - SystemFailure - I2C
      triggerI2C(25, 11); 
      triggerI2C(26, 11);
      triggerI2C(27, 11);
    }
    else if(ps2x.Button(PSB_L2)){
      (trigger.play(1)); 
      //logic lights, alarm
      triggerI2C(10, 1);     
      //  HPEvent 3 - alarm - I2C
      triggerI2C(25, 3);
      triggerI2C(26, 3);
      triggerI2C(27, 3); 
      domeData.dsp = 0;
    } else if(ps2x.Button(PSB_R1)){
      (trigger.play(11)); 
      //logic lights, alarm2Display
      triggerI2C(10, 11);
    } else {
      (trigger.play(random(17,25))); 
      //logic lights, random
      triggerI2C(10, 0);
    }
  }
  
  //circle right
  if(ps2x.ButtonPressed(PSB_RED)) {
    if(ps2x.Button(PSB_L1)){
      (trigger.play(7)); 
      //logic lights, random
      triggerI2C(10, 0);
    } else if(ps2x.Button(PSB_L2)) {
      (trigger.play(3)); 
      //logic lights, random
      triggerI2C(10, 0);
    }
    else if(ps2x.Button(PSB_R1)) {
      (trigger.play(10)); 
      //logic lights, random
      triggerI2C(10, 0);
      // HPEvent 1 - Disco - I2C
      triggerI2C(25, 10);
      triggerI2C(26, 10);
      triggerI2C(27, 10);
    } else {
      (trigger.play(random(32,52))); 
      //logic lights, random
      triggerI2C(10, 0);
    }
  }
  
  //square left
  if(ps2x.ButtonPressed(PSB_PINK)) {
    // leia message
    if(ps2x.Button(PSB_L1)){
      (trigger.play(5)); 
      //logic lights, leia message
      triggerI2C(10, 5);
      // Front HPEvent 1 - HoloMessage - I2C -leia message
      triggerI2C(25, 9);
    } else if(ps2x.Button(PSB_L2)) {
      (trigger.play(4)); 
      //logic lights
      triggerI2C(10, 4);
    }
    else if(ps2x.Button(PSB_R1)){
      (trigger.play(12)); 
      //logic lights, random
      triggerI2C(10, 0);
    } else {
      (trigger.play(random(25,32))); 
      //logic lights, random
      triggerI2C(10, 0);
    }
  }

  ////turn hp light on & off   
//left joystick  
  if(ps2x.ButtonPressed(PSB_L3))  {
    //if hp light is on, turn it off
    if(domeData.hpl == 1){
      domeData.hpl = 0; 
      // Front HPEvent 2 - ledOFF - I2C
      triggerI2C(25, 2);
    } else {
      domeData.hpl = 1; 
      // Front HPEvent 4 - whiteOn - I2C
      triggerI2C(25, 1);
    }//turn hp light on
  } 

 ////Change drivespeed
 //right joystick
  if(ps2x.ButtonPressed(PSB_R3)) {
    //if in lowest speed
    if(drivespeed == drivespeed1){
      //change to medium speed and play sound 3-tone
      drivespeed = drivespeed2; trigger.play(53);
      triggerI2C(10, 22);
    } else if(drivespeed == drivespeed2 && (drivespeed3!=0)) {
      //change to high speed and play sound scream
      drivespeed = drivespeed3; trigger.play(1);
      triggerI2C(10, 23);
   } else{
      //we must be in high speed
      //change to low speed and play sound 2-tone
      drivespeed = drivespeed1; trigger.play(52);
      triggerI2C(10, 21);
    }
  }   

  /////foot drives 
  /////////////////new stuff//////////////////
  sticknum = (map(ps2x.Analog(PSS_RY), 0, 255, -drivespeed, drivespeed));

  if (drivenum < sticknum){
    if (sticknum-drivenum<(ramping+1)){
      drivenum+=ramping;
    } else {
      drivenum = sticknum;
    }

  }

  else if (drivenum > sticknum)
  {
    if (drivenum-sticknum<(ramping+1))
      drivenum-=ramping;
    else
      drivenum = sticknum;

  }


  turnnum = (ps2x.Analog(PSS_RX));   
  if (turnnum <= 200 && turnnum >= 54)
    turnnum = (map(ps2x.Analog(PSS_RX), 54, 200, -(turnspeed/3), (turnspeed/3)));
  else if (turnnum > 200)
    turnnum = (map(ps2x.Analog(PSS_RX), 201, 255, turnspeed/3, turnspeed));
  else if (turnnum < 54)
    turnnum = (map(ps2x.Analog(PSS_RX), 0, 53, -turnspeed, -(turnspeed/3))); 

  ////////////////////////////////// 
  if (drive == 1)// right stick (drive)
  {  
    ST.turn(turnnum);
    ST.drive(drivenum);
  }

  /////dome drive
  domenum = (map(ps2x.Analog(PSS_LX), 0, 255, -domespeed, domespeed));
  if (domenum > -domecompensation && domenum < domecompensation)
    domenum = 0;
#if defined(SYRENSIMPLE)  
  SyR.motor(domenum);
#else
  SyR.motor(1,domenum);
#endif

  ////////////
  delay(50);
}

void triggerI2C(int deviceID, int eventID){
  Wire.beginTransmission(deviceID);
  Wire.write(eventID);
  Wire.endTransmission();    
}
/////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

void  closeAllPanels(){
  closePiePanel(0);
  closePiePanel(1);
  closePiePanel(2);
  closePiePanel(3);
  
  closeSidePanel(0);
  closeSidePanel(1);
  closeSidePanel(2);
  closeSidePanel(3);
}

void  openAllPanels(){
  openPiePanel(0);
  openPiePanel(1);
  openPiePanel(2);
  openPiePanel(3);
  
  openSidePanel(0);
  openSidePanel(1);
  openSidePanel(2);
  openSidePanel(3);  
}

void  closeAllPiePanels(){
  closePiePanel(0);
  closePiePanel(1);
  closePiePanel(2);
  closePiePanel(3);
}

void  openAllPiePanels(){
  openPiePanel(0);
  openPiePanel(1);
  openPiePanel(2);
  openPiePanel(3);
}

void  closeAllSidePanels(){ 
  closeSidePanel(0);
  closeSidePanel(1);
  closeSidePanel(2);
  closeSidePanel(3);
}

void  openAllSidePanels(){
  openSidePanel(0);
  openSidePanel(1);
  openSidePanel(2);
  openSidePanel(3);  
}

void openPiePanel(int num){  
  //Pie Panels occupy Servos 0,1,2,3
  domeServos.setPWM(num, 0, PiePanelOpenPositions[num]);
  isPiePanelOpen[num] = true;
}

void openSidePanel(int num){  
  //Sidepanels occupy Servos Number 4,5,6,7
  //add 4 to num for making servo position call
  domeServos.setPWM(num+4, 0, SidePanelOpenPositions[num]);
  isSidePanelOpen[num] = true; 
}

void closePiePanel(int num){
  //Pie Panels occupy Servos 0,1,2,3  
  domeServos.setPWM(num, 0, PiePanelClosePositions[num]);
  isPiePanelOpen[num] = false;
}

void closeSidePanel(int num){  
  //Sidepanels occupy Servos Number 4,5,6,7
  //add 4 to num for making servo position call
  domeServos.setPWM(num+4, 0, SidePanelClosePositions[num]);
  isSidePanelOpen[num] = false;
}

