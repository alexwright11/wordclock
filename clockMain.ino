#include <CapacitiveSensor.h>
#include <LedControl.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>

int dataIn = 17; //A3
int load = 16; //A2
int clock = 15; //A1
int maxInUse = 2;
LedControl lc=LedControl(dataIn,clock,load,maxInUse);
int pHour, pMin;
unsigned long csSum[3];
CapacitiveSensor csS[3] = {CapacitiveSensor(5,8),CapacitiveSensor(11,9),CapacitiveSensor(10,12)};
const int MODEDEFAULT = 0;
const int MODESECONDS = 1;
const int MODETEMPERATURE = 2;
const int MODESET = 3;
int currentMode;
bool forceUpdate;
bool autoBrightness = 1;
int brightness;

void setup(){

   for(int i=0; i<2; i++) {
    lc.shutdown(i,false); //turn on
    lc.setIntensity(i,8); //0 to 15
    lc.clearDisplay(i);
   }

//for(int i=0;i<8;i++) { lc.setRow(1,i,B11111111); }
//for(int i=0;i<8;i++) { lc.setRow(0,i,B11111111); }

  Wire.begin();
//  Serial.begin(9600);
//  setTime(12, 51, 0, 15, 7, 2015);   //set the system time to 23h31m30s on 13Feb2009
//  RTC.set(now());                     //set the RTC from the system time

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
//    if(timeStatus() != timeSet) 
//        Serial.println("Unable to sync with the RTC");
//    else
//        Serial.println("RTC has set the system time");    

    currentMode = MODEDEFAULT;
}


void loop(){
  
  if (currentMode == MODEDEFAULT) defaultMode();
  else if (currentMode == MODESECONDS) secondsMode();
  else if (currentMode == MODETEMPERATURE) temperatureMode();
  else if (currentMode == MODESET) setMode();
  
  int buttonPressed = readButtons();
  
  if (buttonPressed == 0) { //left
    if (currentMode == MODEDEFAULT) currentMode = MODESET; //go to set mode
    else if (currentMode == MODESET) { currentMode = MODEDEFAULT; lc.setLed(0,1,5,0); }
  } 
  else if (buttonPressed == 1) { //middle
    if (currentMode == MODEDEFAULT) { CLEAR(); currentMode = MODESECONDS; }
    else if (currentMode == MODESECONDS) { CLEAR(); currentMode = MODETEMPERATURE; }
    else if (currentMode == MODETEMPERATURE) { CLEAR(); currentMode = MODEDEFAULT; forceUpdate = true;}
    else if (currentMode == MODESET) incrementHours();//increment hour 
  } 
  else if (buttonPressed == 2) { //right
    if (currentMode == MODEDEFAULT) toggleBrightness(); //toggle brightness
    else if (currentMode == MODESET) incrementMinutes(); //increment minutes
  } 
  
  if(autoBrightness) {
    int intensity = analogRead(14)/68;
    lc.setIntensity(0,intensity); //0 to 15
    lc.setIntensity(1,intensity); //0 to 15
  }
}

int readButtons(){
  long cs[] =  {csS[0].capacitiveSensor(30),csS[1].capacitiveSensor(30),csS[2].capacitiveSensor(30)};
  for(int i=0; i<3; i++) {
    if (cs[i] > 80) { //b: Arbitrary number
        csSum[i] += cs[i];
        if (csSum[i] >= 350) { //c: This value is the threshold, a High value means it takes longer to trigger 
          if (csSum[i] > 0) { csSum[i] = 0; } //Reset
          csS[i].reset_CS_AutoCal(); //Stops readings
          return i;
        }
    } else {
      csSum[i] = 0; //Timeout caused by bad readings
    }
  }
  return 3; //no button
}

void toggleBrightness(){
  if(autoBrightness) {
    autoBrightness = 0;
    brightness = 0;
    lc.setIntensity(0,brightness); //0 to 15
    lc.setIntensity(1,brightness); //0 to 15
  } else if (brightness == 15) {
    autoBrightness = 1;
    lc.setLed(0,0,5,1);
    delay(200);
    lc.setLed(0,0,5,0);
  } else {
    brightness = brightness+3;
    lc.setIntensity(0,brightness); //0 to 15
    lc.setIntensity(1,brightness); //0 to 15
  }
}

void incrementHours(){
  time_t t = now();
  int cHour = hour(t);
  if (cHour == 23) cHour = 0;
  else cHour = cHour + 1;
  setTime(cHour, minute(t), second(t), day(t), month(t), year(t));   //setTime(hr,min,sec,day,month,yr);
  RTC.set(now());                     //set the RTC from the system time
}

void incrementMinutes(){
  time_t t = now();
  int cMin = minute(t);
  if (cMin == 59) cMin = 0;
  else cMin = cMin+1;
  setTime(hour(t), cMin, 0, day(t), month(t), year(t));   //setTime(hr,min,sec,day,month,yr);
  RTC.set(now());                     //set the RTC from the system time
  if (cMin == 35 || cMin == 0) forceUpdate = true;
}

void setMode(){
  defaultMode();
  lc.setLed(0,1,5,1);
}

void defaultMode(){
   int cHour = hour();
   int cMin = minute();
    
   int tpast5mins = cMin % 5; // remainder
   int t5mins = cMin - tpast5mins;
   int tp5mins = pMin - (pMin % 5);
   int tHour = cHour;
   
   if (tHour > 12) tHour = tHour - 12;
   else if (tHour == 0) tHour = 12;

   //if (forceUpdate) CLEAR();

   if (t5mins > 30) {
    tHour = tHour+1;
    if (tHour > 12) tHour = 1;
   }
   
   if ((tHour == pHour) && (cMin == pMin) && (forceUpdate == false))
    return;
    
   if ((tHour != pHour) || forceUpdate) {
     CLEARHOURS();
     // light up the hour word
     if (tHour == 1) ONE(); else if (tHour == 2) TWO(); else if (tHour == 3) THREE(); else if (tHour == 4) FOUR();
     else if (tHour == 5) FIVE(); else if (tHour == 6) SIX(); else if (tHour == 7) SEVEN(); else if (tHour == 8) EIGHT();
     else if (tHour == 9) NINE(); else if (tHour == 10) TEN(); else if (tHour == 11) ELEVEN(); else if (tHour == 12) TWELVE();
   }
   if ((t5mins != tp5mins) || forceUpdate) { // || (((cHour != pHour)) && (t5mins == tp5mins))
     if(t5mins != 25 && t5mins != 40) CLEARMIN();
     if(t5mins == 40) lc.setRow(0,0,B00000000); //FIVE
     // minute word
     if (t5mins == 5 || t5mins == 55)     FIVE_M();        // 5 past or 5 to..
     else if (t5mins == 10 || t5mins == 50)    TEN_M();        // 10 past or 10 to..
     else if (t5mins == 15 || t5mins == 45)    A_QUARTER();    // etc..
     else if (t5mins == 20 || t5mins == 40)    TWENTY();
     else if (t5mins == 25 || t5mins == 35)  { TWENTY(); FIVE_M(); }
     else if (t5mins == 30)    HALF();
   }
   IT_IS();
   // past or to or o'clock?
   if (t5mins == 0) { 
    if (tp5mins > 30) CLEARMOD();
    OCLOCK(); 
    }
   else if (t5mins > 30) {
    if (tp5mins<=30) CLEARMOD();
    TO(); 
   }
   else {
    if (tp5mins>30 || tp5mins == 0) CLEARMOD();
    PAST();
   }
   
   // light up aux minute LED
   // ugly but quicker 
   if (tpast5mins == 0 ) { P0(); }
   else if (tpast5mins == 1) { P1(); }
   else if (tpast5mins == 2) { P2(); }
   if (tpast5mins == 3) { P3(); }
   if (tpast5mins == 4) { P4(); }

   // save last updated time
   pHour = tHour;
   pMin = cMin;
   forceUpdate = false;
}

void secondsMode(){
  displayDigits(second(),1);
}

void temperatureMode(){
  //Serial.println(RTC.temperature()/4);
  int temperatureC = RTC.temperature()/4;
  displayDigits(temperatureC,0);
  DEGREE();
}

void displayDigits(int number, bool shift){
  if (number/10 == 0) L0(shift);
  else if (number/10 == 1) L1(shift);
  else if (number/10 == 2) L2(shift);
  else if (number/10 == 3) L3(shift);
  else if (number/10 == 4) L4(shift);
  else if (number/10 == 5) L5(shift);

  if (number%10 == 0) R0(shift);
  else if (number%10 == 1) R1(shift);
  else if (number%10 == 2) R2(shift);
  else if (number%10 == 3) R3(shift);
  else if (number%10 == 4) R4(shift);
  else if (number%10 == 5) R5(shift);
  else if (number%10 == 6) R6(shift);
  else if (number%10 == 7) R7(shift);
  else if (number%10 == 8) R8(shift);
  else if (number%10 == 9) R9(shift);
}

void CLEAR(){ 
    lc.clearDisplay(0);
    lc.clearDisplay(1);
}

void CLEARHOURS(){
  //lc.setRow(0,2,B00000000); //NINE
  //for(int i=3; i<8; i++) { lc.setRow(0,i,B00000000); }
  //for(int i=5; i<8; i++) { lc.setRow(1,i,B00000000); }
  for(int row=3; row<7; row++) {
    for(int column=0; column < 6; column++) {
      lc.setLed(1,row,column,0); }}
  for(int column=1; column<6; column++) { lc.setLed(1,7,column,0); }
  for(int row=2; row<7; row++) {
    for(int column=0; column < 5; column++) {
      lc.setLed(0,row,column,0); } }
  //lc.setRow(1,3,B00000010);
  //lc.setRow(1,4,B00000010);
}
void CLEARMIN(){
  lc.setRow(1,0,B00000010); 
  lc.setRow(0,0,B00000000);
  lc.setColumn(1,7,B00000000);
  lc.setColumn(0,6,B00000000); 
  lc.setRow(1,1,B00000010);
  //lc.setRow(0,1,B00000000); 
  lc.setLed(0,1,3,0);
  lc.setLed(0,1,4,0);
}

void CLEARMOD(){
  lc.setLed(0,1,1,0);
  lc.setLed(0,1,0,0);
  lc.setLed(1,2,5,0);
  lc.setLed(1,2,4,0);
  lc.setLed(1,2,3,0);
  lc.setLed(1,2,2,0);
  lc.setLed(1,7,0,0); //O'
  lc.setRow(0,7,B0000000); //  CLOCK
}

void IT_IS(){
  lc.setColumn(1,6,B11011000); //IT IS
}

void A_QUARTER(){
  lc.setColumn(1,7,B10111111); //A QUAR
  lc.setColumn(0,6,B00111000); //      TER
}

void TWENTY(){
  lc.setRow(1,0,B11111100); //TWENTY
}

void FIVE_M(){
  lc.setRow(0,0,B01111000); //FIVE
}

void HALF(){
  lc.setRow(1,1,B00111100); //HALF
}

void TEN_M(){
  lc.setRow(1,1,B10000000); //T
  lc.setRow(0,1,B00011000); // EN
}

void TO(){
  //lc.setRow(0,1,B11000000); //TO
  lc.setLed(0,1,1,1);
  lc.setLed(0,1,0,1);
}

void PAST(){ 
  //lc.setRow(1,2,B00111100); //PAST
  lc.setLed(1,2,5,1);
  lc.setLed(1,2,4,1);
  lc.setLed(1,2,3,1);
  lc.setLed(1,2,2,1);
} 

void OCLOCK(){
  lc.setLed(1,7,0,1);
  //lc.setRow(1,7,B10000000); //O'
  lc.setRow(0,7,B11111000); //  CLOCK
}

void NINE(){
  //lc.setRow(0,2,B11110000); //NINE
  for(int column=0; column<4; column++) { lc.setLed(0,2,column,1); }
}

void ONE(){
  //lc.setRow(1,3,B00011110); //ONE
  for(int column=3; column<6; column++) { lc.setLed(1,3,column,1); }
}

void SIX(){
  //lc.setRow(1,3,B11100010); //SIX
  for(int column=0; column<3; column++) { lc.setLed(1,3,column,1); }
}

void THREE(){
  //lc.setRow(0,3,B11111000); //THREE
  for(int column=0; column<5; column++) { lc.setLed(0,3,column,1); }
}

void FOUR(){
  //lc.setRow(1,4,B00111110); //FOUR
  for(int column=2; column<6; column++) { lc.setLed(1,4,column,1); }
}

void FIVE(){
  //lc.setRow(1,4,B11000010); //FI
  for(int column=0; column<2; column++) { lc.setLed(1,4,column,1); }
  //lc.setRow(0,4,B00011000); //  VE
  for(int column=3; column<5; column++) { lc.setLed(0,4,column,1); }
} 

void TWO(){
  //lc.setRow(0,4,B11100000); //TWO
  for(int column=0; column<3; column++) { lc.setLed(0,4,column,1); }
}

void EIGHT(){
  //lc.setRow(1,5,B01111100); //EIGHT
  for(int column=1; column<6; column++) { lc.setLed(1,5,column,1); }
}

void ELEVEN(){
  //lc.setRow(1,5,B10000000); //E
  lc.setLed(1,5,0,1);
  //lc.setRow(0,5,B11111100); // LEVEN
  for(int column=0; column<6; column++) { lc.setLed(0,5,column,1); }
}

void SEVEN(){
  //lc.setRow(1,6,B01111100); //SEVEN
  for(int column=1; column<6; column++) { lc.setLed(1,6,column,1); }
}

void TWELVE(){
  //lc.setRow(1,6,B10000000); //T
  lc.setLed(1,6,0,1);
  //lc.setRow(0,6,B11111000); // WELVE
  for(int column=0; column<5; column++) { lc.setLed(0,6,column,1); }
}

void TEN(){
  //lc.setRow(1,7,B00011100); //TEN
  for(int column=3; column<6; column++) { lc.setLed(1,7,column,1); }
}

void P0(){
  lc.setColumn(0,7,B00000000); //+0
}

void P1(){
  lc.setColumn(0,7,B00001000); //+1
}

void P2(){
  lc.setColumn(0,7,B00001100); //+2
}

void P3(){
  lc.setColumn(0,7,B00001110); //+3
}

void P4(){
  lc.setColumn(0,7,B00001111); //+4
}

void L0(bool b){
  
  lc.setColumn(1,5-b,B00111110); //L0
  lc.setColumn(1,4-b,B01000001);
  lc.setColumn(1,3-b,B01000001);
  lc.setColumn(1,2-b,B00111110);
}

void R0(bool b){
  lc.setColumn(1-b,4*b,B00111110); //R0
  lc.setColumn(0,4-b,B01000001);
  lc.setColumn(0,3-b,B01000001);
  lc.setColumn(0,2-b,B00111110);
}
  
 void L1(bool b){ 
  lc.setColumn(1,5-b,B00010000); //L1
  lc.setColumn(1,4-b,B00100001);
  lc.setColumn(1,3-b,B01111111);
  lc.setColumn(1,2-b,B00000001);
 }

 void R1(bool b){
  lc.setColumn(1-b,4*b,B00010000); //R1
  lc.setColumn(0,4-b,B00100001);
  lc.setColumn(0,3-b,B01111111);
  lc.setColumn(0,2-b,B00000001);
 }

 void L2(bool b){
  lc.setColumn(1,5-b,B00100011); //L2
  lc.setColumn(1,4-b,B01000101);
  lc.setColumn(1,3-b,B01001001);
  lc.setColumn(1,2-b,B00110001);
 }

 void R2(bool b){
  lc.setColumn(1-b,4*b,B00100011); //R2
  lc.setColumn(0,4-b,B01000101);
  lc.setColumn(0,3-b,B01001001);
  lc.setColumn(0,2-b,B00110001);
 }

 void L3(bool b){
  lc.setColumn(1,5-b,B00100010); //L3
  lc.setColumn(1,4-b,B01001001);
  lc.setColumn(1,3-b,B01001001);
  lc.setColumn(1,2-b,B00111110);
 }

 void R3(bool b){
  lc.setColumn(1-b,4*b,B00100010); //R3
  lc.setColumn(0,4-b,B01001001);
  lc.setColumn(0,3-b,B01001001);
  lc.setColumn(0,2-b,B00111110);
 }

 void L4(bool b){
  lc.setColumn(1,5-b,B01111000); //L4
  lc.setColumn(1,4-b,B00001000);
  lc.setColumn(1,3-b,B00111111);
  lc.setColumn(1,2-b,B00001000);
 }

 void R4(bool b){
  lc.setColumn(1-b,4*b,B01111000); //R4
  lc.setColumn(0,4-b,B00001000);
  lc.setColumn(0,3-b,B00111111);
  lc.setColumn(0,2-b,B00001000);
 }

 void L5(bool b){
  lc.setColumn(1,5-b,B01111010); //L5
  lc.setColumn(1,4-b,B01001001);
  lc.setColumn(1,3-b,B01001001);
  lc.setColumn(1,2-b,B01000110);
 }

 void R5(bool b){
  lc.setColumn(1-b,4*b,B01111010); //R5
  lc.setColumn(0,4-b,B01001001);
  lc.setColumn(0,3-b,B01001001);
  lc.setColumn(0,2-b,B01000110);
 }

 void L6(bool b){
  lc.setColumn(1,5-b,B00111110); //L6
  lc.setColumn(1,4-b,B01001001);
  lc.setColumn(1,3-b,B01001001);
  lc.setColumn(1,2-b,B00100110);
 }

 void R6(bool b){
  lc.setColumn(1-b,4*b,B00111110); //R6
  lc.setColumn(0,4-b,B01001001);
  lc.setColumn(0,3-b,B01001001);
  lc.setColumn(0,2-b,B00100110);
 }

 void L7(bool b){
  lc.setColumn(1,5-b,B01000000); //L7
  lc.setColumn(1,4-b,B01000111);
  lc.setColumn(1,3-b,B01011000);
  lc.setColumn(1,2-b,B01100000);
 }

 void R7(bool b){
  lc.setColumn(1-b,4*b,B01000000); //R7
  lc.setColumn(0,4-b,B01000111);
  lc.setColumn(0,3-b,B01011000);
  lc.setColumn(0,2-b,B01100000);
 }

 void L8(bool b){
  lc.setColumn(1,5-b,B00110110); //L8
  lc.setColumn(1,4-b,B01001001);
  lc.setColumn(1,3-b,B01001001);
  lc.setColumn(1,2-b,B00110110);
 }

 void R8(bool b){
  lc.setColumn(1-b,4*b,B00110110); //R8
  lc.setColumn(0,4-b,B01001001);
  lc.setColumn(0,3-b,B01001001);
  lc.setColumn(0,2-b,B00110110);
 }

 void L9(bool b){
  lc.setColumn(1,5-b,B00110010); //L9
  lc.setColumn(1,4-b,B01001001);
  lc.setColumn(1,3-b,B01001001);
  lc.setColumn(1,2-b,B00111110);
 }

 void R9(bool b){
  lc.setColumn(1-b,4*b,B00110010); //R9
  lc.setColumn(0,4-b,B01001001);
  lc.setColumn(0,3-b,B01001001);
  lc.setColumn(0,2-b,B00111110);
 }

void DEGREE(){
  lc.setLed(0,1,0,1); //degree simple
//lc.setColumn(0,0,B01100000); //degree
//lc.setColumn(0,1,B01100000);
}




