

/*

Edited from the starting code of Dave Bodnar.
http://www.trainelectronics.com/DCC_Arduino/DCC++/IRThrottle/index.htm
http://www.trainelectronics.com/DCC_Arduino/DCC++/IRThrottle/images/DCC__Throttle-P2P-Talking-MP3-Array-v3-3P.ino


  UP = faster - 13 - hold to repeat
  DN = slower - 17 -hold to repeat
   = STOP - 11
  # = Disable / enable DCC (pin 8) -12
  < = - 14
  > = -16
  OK = Menu choices -15
  Power = power off to DCC++ (<0>)
*/
// the following defines the codes for Keyes brand and Sony IR remote controls

#include <SoftwareSerial.h>
#include <IRremote.h>
#include<EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define Keyes0 0xFF50AF
#define Keyes1 0xFFF807
#define Keyes2 0xFFC03F
#define Keyes3 0xFF20DF
#define Keyes4 0xFFA05F
#define Keyes5 0xFF38C7
#define Keyes6 0xFF609F
#define Keyes7 0xFFE01F
#define Keyes8 0xFF10EF
#define Keyes9 0xFFB847
#define KeyesDown 0xFF48B7
#define KeyesLeft 0xFFE817
#define KeyesOK 0xFF18E7
#define KeyesPound 0xFFD02F
#define KeyesRight 0xFF30CF
#define KeyesStar 0xFF906F
#define KeyesUp 0xFF6897
#define KeyesPower 0xFF08F7
#define KeyesMouse 0xFFEA15
#define KeyesMouseLeft 0xFF28D7
#define KeyesMouseRight 0xFFA857

#define Sony0 0x910
#define Sony1 0x10
#define Sony2 0x810
#define Sony3 0x410
#define Sony4 0xC10
#define Sony5 0x210
#define Sony6 0xA10
#define Sony7 0x610
#define Sony8 0xE10
#define Sony9 0x110
#define SonyDown 0x890
#define SonyExit 0xC70
#define SonyLeft 0xC90
#define SonyMenu 0x70
#define SonyMute 0x290
#define SonyPIP 0xDB0
#define SonyPower 0xA90
#define SonyRight 0x490
#define SonyUp 0x90



int irButton;
int IRdelay = 240; // determines speed of repeating button pushes when button held
int LED = 13; // LED to blink when DCC packets are sent in loop
int ReedEnd = 3;  // Reed Switch at each end
int ReedSS = 4;  // Reed Switch for Station Stop

int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;

int LocoAddress[2] = {1830, 3};
int LocoDirection[2] = {1, 1};
int LocoSpeed[2] = {0, 0};

int TurnoutAdd[9] = {1,2,3,4,5,6,7,8,9};
int TurnoutSub[9] = {0,1,2,3,0,1,2,3,0};

byte Fx = 0;
byte LocoFN0to4[2];
byte LocoFN5to8[2];
byte Locofn9to12[2];// 9-12 not yet implemented

int old_speed = 0;

int PTPSpeed = 50; // speed for point to point
int PTPTime = 3; // time after reed switch hit
int PTPFlag = 0; // set to 1 to activate Point to Point
int PTPRandom = 0;  // =0 if normal time, =1 if random (20% + rnd (80%))
int PTPTimeSave = 0;
int ReedCheck = 0;
int ZeroSpeedFlag = 0;
int ActiveAddress = 0; // make address1 active
int irCode = 0;
int inMenu = 0;  // keeps track of being in the menu area 0=out, 1=in
int inMenu2 = 0;  // keeps track of being in the menu area 0=out, 1=in

int digits = 0;
int upFlag = 0;  // trying to get keys to repeat!
int dnFlag = 0;


const unsigned int I2C_ADDR = 0x27; // <<----- Add your address here.  Find it from I2C Scanner
const int BACKLIGHT_PIN = 3;
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

  // LiquidCrystal_I2C  lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);
LiquidCrystal_I2C lcd(I2C_ADDR,16,2);


SoftwareSerial mySerial(5, 2); // RX, TX - note, RX not used
//int MP3file = 0;
int buusyPin = 10;  // sound player busy
int bsy = 0;
int z = 0;

int i = 0;
int irTemp = 0;
int StopButtonPressedOnceFlag = 0; // used to detect 2nd button push within a fraction of a second
unsigned long time;
char VersionNum[] = "4.0"; ///////////////////////////////////////////////////////////////////////////VERSION HERE///////

// 0000000000000000000000000000000000000000000000000000 Void Setup 0000000000000000000000000
void setup() {

  lcd.init();                   // initialize the lcd

  lcd.backlight();


  randomSeed(analogRead(0));
  pinMode(LED, OUTPUT);
  pinMode(ReedEnd,  INPUT_PULLUP);
  pinMode(ReedSS,  INPUT_PULLUP);
  LocoAddress[0] = EEPROM.read(0) * 256;
  LocoAddress[0] = LocoAddress[0] + EEPROM.read(1);
  if (LocoAddress[0] >= 10000) { // set defalut as 3 if not in proper range (0-9999)
    LocoAddress[0] = 3;
  }
  LocoAddress[1] = EEPROM.read(2) * 256;
  LocoAddress[1] = LocoAddress[1] + EEPROM.read(3);
  if (LocoAddress[1] >= 10000) { // set defalut as 3 if not in proper range (0-9999)
    LocoAddress[1] = 3;
  }

  //lcd.begin (16, 2); //  LCD is 16 characters x 2 lines
  //lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  //lcd.setBacklight(HIGH);  // Switch on the backlight
  //lcd.home (); // go home
  Serial.begin (115200);
  lcd.setCursor(0, 0);
  lcd.print("DCC++ Throttle");
  lcd.setCursor(0, 1);
  lcd.print("01-03-19 - v");//3.3p");
  for (int i = 0; i < 4; i++) {
    lcd.print(VersionNum[i]);
    //delay(500);
  }
  irrecv.enableIRIn(); // Start the receiver
  //mySerial.begin(9600);  // for MP3 player
  //mp3_set_serial (mySerial);  //set Serial for DFPlayer-mini //mp3 module
  //mp3_set_volume (30);          // 15 is low for unpowered speaker - 30 good for unpowered speaker - requires power off to reset volume
  Serial.print("01-03-2019  version ");//3.3p");
  for (int i = 0; i < 4; i++) {
    Serial.print(VersionNum[i]);
    //delay(500);
  }
  Serial.println("");
  Serial.print("<0>");// power off to DCC++ unit
  delay(1500);
  lcd.clear();

  //  PTPFlag = 1;  // set to 1 for testing
  //  PTPSpeed = 50; // speed for point to point test
  //  PTPTime = 3 ; // pause time for test

  //  LocoAddress[ActiveAddress] = DCCAddress1; // start out with main DCC address active - secondary is DCCAddress2
}

class Tee : public Print {
  Print &a, &b;
  public:
  Tee(Print &a, Print &b) : a(a), b(b) {}
  virtual size_t write(uint8_t c) {
    return a.write(c) && b.write(c);
  }
  using Print::write;
};

Tee tee(Serial, lcd);

// 0000000000000000000000000000000000 Void Loop 00000000000000000000000000000000000000000000000000000000000000000000000
void loop() {
  doMainLCD();
  if (PTPFlag == 1) { // if set do PTP by going to PTP speed & looking for ReedCheck to change from 1 to 0
    ReedCheck = digitalRead(ReedEnd);
    if (ReedCheck == 0) { // reverse if in test area at ends
      PTPSpeed = LocoSpeed[ActiveAddress];
      LocoSpeed[ActiveAddress] = 0;// PTPSpeed;  STOP
      doDCC();
      if (LocoDirection[ActiveAddress] == 0) {
        LocoDirection[ActiveAddress] = 1;
      } else LocoDirection[ActiveAddress] = 0;
      doMainLCD();
      if (PTPRandom == 1) {
        PTPTimeSave = PTPTime;// save time for random computation
        Serial.println(PTPTime);
        PTPTime = PTPTime * .8;
        Serial.println(PTPTime);
        PTPTime =   random (0, PTPTime);
        Serial.println(PTPTime);
        PTPTime = PTPTime + PTPTimeSave * .2;
        Serial.println(PTPTime);
      }
      Serial.print(" Delay ");
      for (int xxyyzz = 0; xxyyzz <= PTPTime; xxyyzz++) {
        delay( 1000);
        lcd.setCursor(12, 1);
        lcd.print("   ");
        lcd.setCursor(12, 1);
        lcd.print(PTPTime - xxyyzz);

        Serial.print(PTPTime - xxyyzz);
        Serial.print(",");
      }
      Serial.println(" "); // move to new line
      if (PTPRandom == 1) PTPTime = PTPTimeSave;
      LocoSpeed[ActiveAddress] = PTPSpeed; // back to speed in other direction
      doDCC();
      doMainLCD();
      do {
        delay(2000);
        ReedCheck = digitalRead(ReedEnd);
      } while (ReedCheck == 0);
    }
  }
  if (irrecv.decode(&results))
  {
    translateIR(); //000000000000000000000000000000000000000000000000000000000000000000000000000000 Use translateIR
    irrecv.resume(); // Receive the next value
  }
  if ((irButton > 0 && irButton < 13) | (irButton == 14 | irButton == 16)) {
    upFlag = 0;
    dnFlag = 0;
  }
  if (irButton != 13 && irButton != 17 && irButton != 0) { // set delay to slower speed for non repeating keys
    IRdelay = 240;
  }
  if (irButton >= 1 && irButton < 10) { //Funtions done with numbers 1-9 - clear all with 10.......NUMBERS
    //mp3_play(irButton);
    doDCCturnouts();
    irButton = 0;
    irTemp = 0;
  }
  if (irButton == 15) {  // was 19 (PIP on Sony) changed to 15 (Menu-sony or OK-Keyes...............MENU
    PTPSpeed = LocoSpeed[ActiveAddress];  // save current speed for use in PTP
    ZeroSpeedFlag = 1;
    all2ZeroSpeeed(); // stop all when entering menu
    doOKMENU();
    ZeroSpeedFlag = 0;
    all2ZeroSpeeed(); // restore all when exiting menu
    //    all2SavedSpeed(); // restore speed when done with menu
  }
  if (irButton == 14 ) { //Right volume button - reverse if going forward...........................RIGHT ARROW
    LocoDirection[ActiveAddress] = 0;//!LocoDirection[ActiveAddress] ;
    //mp3_play(12);
//    dlayPrint();
    lcd.setCursor(6, 0);
    lcd.print("<");
    upFlag = 0;
    dnFlag = 0;
    LocoSpeed[ActiveAddress] = constrain(LocoSpeed[ActiveAddress], 0, 126);
    doDCC();
    old_speed = LocoSpeed[ActiveAddress] ;
    irButton = 0;
//    saySpeed();
  }
  if (irButton == 16 ) { //Left volume button - reverse if going backward...........................LEFT ARROW
    LocoDirection[ActiveAddress] = 1;// !LocoDirection[ActiveAddress] ;
    //mp3_play(11);
//    dlayPrint();
    lcd.setCursor(6, 0);
    lcd.print(">");
    upFlag = 0;
    dnFlag = 0;
    LocoSpeed[ActiveAddress] = constrain(LocoSpeed[ActiveAddress], 0, 126);
    doDCC();
    old_speed = LocoSpeed[ActiveAddress] ;
    irButton = 0;
//    saySpeed();
  }
  // get DCC address
  if (irButton == 18 ) //Mute button...............................................................MUTE
  {
    getDCCAddress();
  }
  if (irButton == 13 | (irButton == 99 && upFlag == 1)) // UP and repeat key (99)..................UP
  {
    //Serial.print("locospeed0 ");
    Serial.println(LocoSpeed[ActiveAddress]);
    (LocoSpeed[ActiveAddress])++;
    upFlag = 1;
    dnFlag = 0;
    irButton = 0;
    //Serial.println("found UP 000");
    //Serial.print("locospeed1 ");
    Serial.println(LocoSpeed[ActiveAddress]);
    //bsy = digitalRead(buusyPin);
    //if (bsy == 1) //mp3_play(14);
    IRdelay = 20;
  }

  if (irButton == 12)//...........................................................................POWER
  {
    //   Serial.println("found Power ");
    Serial.print("<0>");
    //mp3_play(13);
    irButton = 0;
  }

  if (irButton == 17 | (irButton == 99 && dnFlag == 1))  // DOWN..................................DOWN
  {
    IRdelay = 20;  // speed up for repeating keys
    //if (LocoSpeed[ActiveAddress] > 0)
    (LocoSpeed[ActiveAddress])--;
    dnFlag = 1;
    upFlag = 0;
//    if (LocoSpeed[ActiveAddress] > 0) {
//      bsy = digitalRead(buusyPin);
//      if (bsy == 1) {}; //mp3_play(15);
//    }
//    if (LocoSpeed[ActiveAddress] == 0) //mp3_play(16);
    irButton = 0;
  }
  if (irButton == 11) //* key  - EXIT does STOP..................................................EXIT (* key)
  {
    if (StopButtonPressedOnceFlag == 1) {
      if (millis() - time <= 1000 )
      {
        Serial.println("double hit on STOP");  // key hit again within 1 second
        Serial.print("<0>");
        delay(100);
        //mp3_play(13);
        irButton = 0;
      }
      StopButtonPressedOnceFlag = 0;
    }
    if (StopButtonPressedOnceFlag == 0) {
      time = millis();
      StopButtonPressedOnceFlag = 1;
    }
    LocoSpeed[ActiveAddress] = 0;
    irButton = 0;
    //mp3_play(16);
  }
  //Functions will not work without this to limit speed command to only new speeds
  if (LocoSpeed[ActiveAddress] != old_speed)
  {
    LocoSpeed[ActiveAddress] = constrain(LocoSpeed[ActiveAddress], 0, 126);
    doDCC();
    old_speed = LocoSpeed[ActiveAddress] ;
    //   Serial.print("LocoSpeed[ActiveAddress] 4 ");
    Serial.println(LocoSpeed[ActiveAddress] , DEC);
  }

}



//000000000000000000000000000 Do PTP 0000000000000000000000000000000000000000000000000000000000000000000

void doPTP() {
  inMenu = 1;
  Serial.println("@ PTP  ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Point to Point");
  //mp3_play(19);
  //dlayPrint();
  irButton = 0;
  lcd.clear();
  lcd.print ("1.Time  3.GO");
  lcd.setCursor(0, 1);
  lcd.print ("2.RND   T=");
  lcd.print(PTPTime);
  lcd.print(" sec");
  ReedCheck = digitalRead(ReedEnd);
  do {
    if (irrecv.decode(&results))
    {
      translateIR();
      irrecv.resume(); // Receive the next value
    }
    if (irButton == 15) {

      PTPFlag = 0; // turn off PTP
      inMenu = 0;
      doOKMENU();
    }

    if (irButton == 3) {
      inMenu = 0; // exit on 3
      PTPFlag = 1; // turn on PTP
    }
    if (irButton == 1 || irButton == 2 ) //..................................... 1 set time or random time
    {
      if (irButton == 2) {
        PTPRandom = 1;
      }
      else PTPRandom = 0;
      irButton = 0;
      Serial.print("Random = ");
      Serial.println(PTPRandom);
      lcd.clear();
      lcd.print("Enter 3 dig pause");
      inMenu2 = 1;
      getPTPTime();
      Serial.println("Back @ ptp");
      inMenu = 0;
      lcd.clear();
      doMainLCD();
      irButton = 0;
      inMenu  = 0;
      Serial.print("PTP EXIT ?? ");
      PTPFlag = 1;
    }
  } while (inMenu == 1);
  irButton = 0;
  inMenu  = 0;
  Serial.print("PTP EXIT ?? ");
  //PTPFlag = 1;
}
//0000000000000000000000000000000 OK Menu 00000000000000000000000000000000000000000000000000000000000000
void doOKMENU() {
  inMenu = 1;
  irButton = 0;
  // Serial.println("@ doOKMENU  ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1-Swap    3-PTP");
  lcd.setCursor(0, 1);
  lcd.print("2-ID      4-Exit");
  lcd.setCursor(5, 1);
  lcd.print(LocoAddress[ActiveAddress]);
  //mp3_play(20);
  dlayPrint();
  do {
    if (irrecv.decode(&results))
    {
      translateIR();
      irrecv.resume(); // Receive the next value
      Serial.print("TOP IR Button ");
      Serial.println(irButton);
      if (irButton > 0 && irButton < 5 || irButton == 15) {
        if (irButton != 15) {
          //mp3_play(irButton);
          //dlayPrint();
        }
        if (irButton == 2) {
          getDCCAddress();
        }
        if (irButton == 3) {
          doPTP();
        }
        if (irButton == 1) {
          if (ActiveAddress == 0) {
            //mp3_play(23);
            //dlayPrint();
            ActiveAddress = 1;
          }
          else {
            //mp3_play(22);
            //dlayPrint();
            ActiveAddress = 0;
          }
          lcd.setCursor(4, 1);
          lcd.print("     ");
          lcd.setCursor(5, 1);
          lcd.print(LocoAddress[ActiveAddress]);
        }
        if (irButton == 4 || irButton == 15)  { // button 4 or menu to exit
          inMenu = 0; // exit on 4
          irButton = 0;
        }
      }
    }
    irButton = 0;
  } while ( inMenu == 1);
  lcd.clear();
  doMainLCD();
}

// 000000000000000000000000000000000000 Do Main LCD 00000000000000000000000000000000000000000000000000000000000000
void doMainLCD() {
  lcd.setCursor(0, 0);
  lcd.print("S=");
  lcd.print(LocoSpeed[ActiveAddress], DEC);
  lcd.print("  ");
  lcd.setCursor(6, 0);
  if (LocoDirection[ActiveAddress] == 1 ) {
    lcd.print(">");
  }
  else {
    lcd.print("<");
  }
  lcd.setCursor(8, 0);
  lcd.print("A=");
  if (LocoAddress[ActiveAddress] <= 9) {
    lcd.print("0");  // add leading zero for single digit addresses
  }
  lcd.print(LocoAddress[ActiveAddress] , DEC);
  lcd.setCursor(14, 0);
  lcd.print("#");
  lcd.setCursor (15, 0);       // go to end of 1st line
  lcd.print(ActiveAddress + 1);
  lcd.setCursor(1, 1); // start of 2nd line

  

//  char TestData;        //0000000000000000000000000000000000 My LCD Serial Monitor 00000000000000000
//  if(Serial.availableForWrite() ) {
//    delay(100);
////    while (Serial.availableForWrite() > 0) {
//    TestData = Serial.availableForWrite();
//    lcd.setCursor(0, 1);
//    lcd.print (TestData); // echo the incoming character to the LCD }
////    }
//  }



  // String temp = "0000" + String(LocoFN0to4[ActiveAddress], BIN);  // pad with leading zeros
  // int tlen = temp.length() - 5;
  // lcd.print(temp.substring(tlen));
  // temp = "000" + String(LocoFN5to8[ActiveAddress], BIN);
  // tlen = temp.length() - 4;
  // lcd.setCursor(0, 1); // start of 2nd line
  // lcd.print(temp.substring(tlen));

  if (PTPFlag == 1) {
    lcd.setCursor(11, 1);
    lcd.print("P");
    if (PTPRandom == 1) {
      lcd.print("R");
    }
  }
}
//0000000000000000000000000000000000000000000 Say Speed 0000000000000000000000000000000000000000000000000
void saySpeed() {
  int tempSpeed = LocoSpeed[ActiveAddress] ;
  int digHundreds = tempSpeed / 100;
  if (digHundreds >= 1) {
    //mp3_play(digHundreds);
    dlayPrint();
    tempSpeed = tempSpeed - digHundreds * 100;
  }
  int digTens = tempSpeed / 10;
  if (digHundreds >= 1 || digTens >= 1 ) {
    if (digTens == 0) {
      //mp3_play(10);
    } else {
      //mp3_play (digTens);
    }
    dlayPrint();
  }
  tempSpeed = tempSpeed - digTens * 10;
  if (tempSpeed == 0) {
    //mp3_play(10);
  } else {
    //mp3_play (tempSpeed);
  }
  dlayPrint();
}
//0000000000000000000000000000000000000000000 IR TRANSLATE 00000000000000000000000000000000000000000000000000000

int translateIR() // takes action based on IR code received
// describing KEYES Remote IR codes (first) and Sony IR codes (second)
{
  //  Serial.print("value - hex ");
  // Serial.println(results.value, HEX);
  switch (results.value)
  {
    case KeyesUp:   //Keyes remote code for UP
    case SonyUp: //Sony remote code for UP
      //Serial.println(" UP");
      irButton = 13;
      break;
    case KeyesLeft:
    case SonyLeft:
      //Serial.println(" LEFT");
      irButton = 14;
      break;
    case KeyesOK:
    case SonyMenu:
      Serial.println(" - MENU - ");
      irButton = 15;
      break;
    case KeyesRight:
    case SonyRight:
      //Serial.println(" RIGHT");
      irButton = 16;
      break;
    case KeyesDown:
    case SonyDown:
      //Serial.println(" DOWN");
      irButton = 17;
      break;
    case Keyes1:
    case Sony1:
      //Serial.println(" 1");
      irButton = 1;
      break;
    case Keyes2:
    case Sony2:
      //Serial.println(" 2");
      irButton = 2;
      break;
    case Keyes3:
    case Sony3:
      //Serial.println(" 3");
      irButton = 3;
      break;
    case Keyes4:
    case Sony4:
      //Serial.println(" 4");
      irButton = 4;
      break;
    case Keyes5:
    case Sony5:
      //Serial.println(" 5");
      irButton = 5;
      break;
    case Keyes6:
    case Sony6:
      //Serial.println(" 6");
      irButton = 6;
      break;
    case Keyes7:
    case Sony7:
      //Serial.println(" 7");
      irButton = 7;
      break;
    case Keyes8:
    case Sony8:
      //Serial.println(" 8");
      irButton = 8;
      break;
    case Keyes9:
    case Sony9:
      //Serial.println(" 9");
      irButton = 9;
      break;
    case Keyes0:
    case Sony0:
      //Serial.println(" 0");
      irButton = 10;
      break;
    case KeyesStar:
    case KeyesMouse:
    case KeyesMouseLeft:
    case KeyesMouseRight:
      //Serial.println(" *");
      irButton = 11;
      break;
    case SonyExit:  //EXIT
      //Serial.println(" *");
      irButton = 11;
      break;
    case KeyesPound:
      //Serial.println(" #");
      irButton = 12;
      break;
    case KeyesPower:
    case SonyPower:  //POWER
      //Serial.println(" #");
      irButton = 12;
      break;
    case SonyMute:  //Mute
      //Serial.println(" #");
      irButton = 18;
      break;
    case SonyPIP:
      //Serial.println(" PIP");
      irButton = 19;
      break;
    case 0xFFFFFFFF:
      //Serial.println(" REPEAT");
      irButton = 99;
      break;
    default:
      irButton = 99;
  }// End Case
  delay(IRdelay); // Determines auto repeat of IR buttons
}
//000000000000000000000000000 END translateIR 00000000000000000000000000000000000000000000

//00000000000000000 START DO FUNCTION BUTTONS 000000000000000000000000000000000000000000000
int doFunction() {
  irTemp = irButton - 1;
  lcd.setCursor (14, 1);       // go to end of 2nd line
  ///  lcd.print("FN code ");
  lcd.print(irButton, DEC);
  // Serial.print("got a keypad button ");
  // Serial.println(irButton, DEC);
  if (irTemp <= 4) {
    if (bitRead(LocoFN0to4[ActiveAddress], irTemp) == 0 ) {
      bitWrite(LocoFN0to4[ActiveAddress], irTemp, 1);
    }
    else {
      if (bitRead(LocoFN0to4[ActiveAddress], irTemp) == 1 ) {
        bitWrite(LocoFN0to4[ActiveAddress], irTemp, 0);
      }
    }
    doDCCfunction04();
    Serial.print(LocoFN0to4[ActiveAddress], BIN);
    Serial.println(" LocoFN0to4[ActiveAddress] d ");
    Serial.print(LocoFN0to4[ActiveAddress], DEC);
    Serial.println(" LocoFN0to4[ActiveAddress]");
  }
  if (irTemp >= 5 && irTemp <= 8) {
    irTemp = irTemp - 5;
    if (bitRead(LocoFN5to8[ActiveAddress], irTemp) == 0 ) {
      bitWrite(LocoFN5to8[ActiveAddress], irTemp, 1);
    }
    else {
      if (bitRead(LocoFN5to8[ActiveAddress], irTemp) == 1 ) {
        bitWrite(LocoFN5to8[ActiveAddress], irTemp, 0);
      }
    }
    doDCCfunction58();
    Serial.print(LocoFN5to8[ActiveAddress], BIN);
    Serial.println(" LocoFN5to8[ActiveAddress] d ");
    Serial.print(LocoFN5to8[ActiveAddress], DEC);
    Serial.println(" LocoFN5to8[ActiveAddress]");
  }
  if (irButton == 10)
  {
    lcd.setCursor (14, 1);       // go to end of 2nd line
    ///    lcd.print("FN code ");
    lcd.print(irButton, DEC);
    irButton = 0;
    LocoFN0to4[ActiveAddress] = B00000000; //clear variables for which functions are set
    LocoFN5to8[ActiveAddress] = B00000000;
    doDCCfunction04();
    doDCCfunction58();
    delay(500);
    irButton = 0;
  }
  irButton = 0;
  delay(500);
}


//000000000000000000000000 Do DCC Turnouts 00000000000000000000000000000000000000000000000000
// <a ADDRESS SUBADDRESS ACTIVE>
void doDCCturnouts() {
  irTemp = irButton - 1;
//  Serial.write("<a ");
//  Serial.print(TurnoutAdd[irTemp]);
//  Serial.print(" ");
//  Serial.printc;
//  Serial.println(" 1>");

  String outText = (String) "<a " + TurnoutAdd[irTemp] + " " + TurnoutAdd[irTemp] + " 1>";
  lcd.setCursor (1, 1);
  tee.print (outText);
  lcd.setCursor (14, 1);       // go to end of 2nd line
  lcd.print ("T");
  lcd.print(irButton, DEC);
   
 }








//0000000000000000000000000 Do DCC Functions 0000000000000000000000000000000000000000000000

void doDCCfunction04() {
  Serial.write("<f ");
  Serial.print(LocoAddress[ActiveAddress] );
  Serial.print(" ");
  int fx = LocoFN0to4[ActiveAddress] + 128;
  Serial.print(fx);
  Serial.print(" >");
}

void doDCCfunction58() {
  Serial.write("<f ");
  Serial.print(LocoAddress[ActiveAddress] );
  Serial.print(" ");
  int fx = LocoFN5to8[ActiveAddress] + 176;
  Serial.print(fx);
  Serial.print(" >");
}

// 0000000000000000000000000000000000000000000 Dlay Print 0000000000000000000000000000000000000000000000
//routine to stay here till busy pin goes low once then goes high after speech item completes
void dlayPrint()
{
  delay(50);// was 100
  int bsyflag = 0;
  Serial.println(" ");
  // Serial.print("buusypin ");
  for ( z = 0; z <= 60 ; z++) { // was 300
    bsy = digitalRead(buusyPin);
    Serial.print(bsy);
    delay(20);
    if (bsyflag == 1 && bsy == 1) {
      break;
    }
    if (bsy == 0) {
      bsyflag = 1;
    }
  }
  Serial.println(" ");
  Serial.println("done");
}



// 0000000000000000000000000000000000000000000 Do DCC 000000000000000000000000000000000000000000
void doDCC() {
//  Serial.print("d = ");
//  Serial.println(LocoDirection[ActiveAddress] );
  Serial.print("<1>");
  Serial.print("<t1 ");
  Serial.print(LocoAddress[ActiveAddress] );//locoID);
  Serial.print(" ");
  Serial.print(LocoSpeed[ActiveAddress] );
  Serial.print(" ");
  Serial.print(LocoDirection[ActiveAddress] );
  Serial.write(">");
}


//000000000000000000000000000000000000000000 get PTP Time 00000000000000000000000000000000000000000000000000
void getPTPTime() {
  i = 0;
  int xxx = 1;
  do {
    if (irrecv.decode(&results))
    {
      translateIR();
      irrecv.resume(); // Receive the next value
      //mp3_play(irButton);
    }
    if (irButton == 15) {
      //      LocoAddress[ActiveAddress] = DCCAddress1; // restore first address since it was set to 0
      inMenu2 = 0;
    }
    if (irButton > 0 && irButton < 11) {
      if (irButton == 10) irButton = 0;
      xxx = i * 10;
      if (i == 0) xxx = 0;
      if (i >= 1) xxx = 10;
      PTPTime = PTPTime  * xxx;
      PTPTime = PTPTime + irButton;
      irButton = 0;
      ++i;
      lcd.setCursor(0, 1);
      lcd.print("               ");
      if (PTPTime == 0) {
        lcd.setCursor(10 + i, 1);
      } else lcd.setCursor(10, 1);
      lcd.print(PTPTime );
      if (i == 3) inMenu2 = 0; // exit after 3 digits entered
    }
    if (inMenu2 == 0) {
      inMenu2 = 0;
      irButton = 0;
      digits = 0;
      lcd.clear();
      break;
    }
    irButton = 0;
  } while ( inMenu2 = 1);
  IRdelay = 20; //speed up auto repeat
  Serial.println("Exiting getPTPTime");
}
// 000000000000000000000000000000 Set DCC Address and save to memory 000000000000000000000000000000000000000000000000000000000
void getDCCAddress() {
  //mp3_play(18);
  delay(200);
  //dlayPrint();
  delay(500);
  inMenu = 1;
  lcd.clear(); // blank screen
  lcd.setCursor(0, 0);
  lcd.print("Loco Address ");
  if (ActiveAddress == 0) {
    lcd.print("1");
  } else lcd.print("2");
  lcd.setCursor(0, 1);
  lcd.print("Enter 4 digits");
  irButton = 0;
  LocoAddress[ActiveAddress] = 0;
  i = 0;
  int xxx = 1;
  do {
    if (irrecv.decode(&results))
    {
      translateIR();
      irrecv.resume(); // Receive the next value
      //mp3_play(irButton);
    }
    if (irButton == 15) {
      //      LocoAddress[ActiveAddress] = DCCAddress1; // restore first address since it was set to 0
      inMenu = 0;
    }
    if (irButton > 0 && irButton < 11) {
      if (irButton == 10) irButton = 0;
      xxx = i * 10;
      if (i == 0) xxx = 0;
      if (i >= 1) xxx = 10;
      LocoAddress[ActiveAddress] = LocoAddress[ActiveAddress]  * xxx;
      LocoAddress[ActiveAddress] = LocoAddress[ActiveAddress] + irButton;
      irButton = 0;
      ++i;
      lcd.setCursor(0, 1);
      lcd.print("               ");
      if (LocoAddress[ActiveAddress] == 0) {
        lcd.setCursor(10 + i, 1);
      } else lcd.setCursor(10, 1);
      lcd.print(LocoAddress[ActiveAddress] );
      if (i == 4) inMenu = 0; // exit after 4 digits entered
    }
    if (inMenu == 0) {
      inMenu = 0;
      irButton = 0;
      digits = 0;
      lcd.clear();
      break;
    }
    irButton = 0;
  } while ( inMenu = 1);
  IRdelay = 20; //speed up auto repeat
  xxx = LocoAddress[0] / 256;
  EEPROM.write(0, xxx);
  xxx = LocoAddress[0] - (xxx * 256);
  EEPROM.write(1, xxx);

  xxx = LocoAddress[1] / 256;
  EEPROM.write(2, xxx);
  xxx = LocoAddress[1] - (xxx * 256);
  EEPROM.write(3, xxx);
}
// 0000000000000000000000000000000 All to Zero 0000000000000000000000000000000000000000000000000000
void all2ZeroSpeeed() {  // set flag to 1 to stop, set to 0 to restore
  for (int tempx = 0; tempx <= 1; tempx++) {
    Serial.print("<t1 ");
    Serial.print(LocoAddress[tempx] );//locoID);
    Serial.print(" ");
    if (ZeroSpeedFlag == 1) {
      Serial.print(0);//LocoSpeed[0] );
    }
    else Serial.print(LocoSpeed[0] );
    Serial.print(" ");
    Serial.print(LocoDirection[1] );
    Serial.write(">");
  }
}
