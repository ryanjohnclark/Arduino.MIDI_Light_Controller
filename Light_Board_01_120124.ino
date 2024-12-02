

#include <EEPROM.h>
#include <FastLED.h>
#include "MIDIUSB.h"
#include "debounce.h"
#include <LiquidCrystal_I2C.h>
#define ENCODER_OPTIMIZE_INTERRUPTS  // must be defined before Encoder.h is included.
#include <Encoder.h>



LiquidCrystal_I2C lcd(0x27, 24, 2);
//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// R O T A R Y    E N C O D E R    S E T T I N G S  ////////////////////////////////////
Encoder encPulse(1,0);  // pins used for encoder turns
#define encButton 19
long knobValCurr;      // previous state of knob
long knobValPrev;      // current state of knob
long pulsePerInc = 4;  // how many pulses equal one increment
int myLoop = 0;
int settingsCount = 0;
int btnCurr = 0;
int knobVal = 1;
int knobMin ;
int knobMax ;

int tempDisplay = 0;                          // timer for temporary display

String sMode = "run";  //sMode: RUN,DIMMER,SETTINGS,setmidi,setColor,SETEXIT,editMidi,editColor,EDITEXIT

int currPress = 0;  // 0 none, 1 short, 2 long
String selNext;


long encPulsePrev;
unsigned long encButtonPressStart = 0;  // Holds the press start time

int longPressLimit = 1000;  // Wait Time before back to Run Mode

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// G E N E R A L    B U T T O N    S E T T I N G S  ////////////////////////////////////

#define btnCount 8              //only loop buttons, not dimmer

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// D I M M E R    B U T T O N    S E T T I N G S  //////////////////////////////////////

int dimmerCCVal = 64;                // Value of CC
unsigned long dimmerPressStart = 0;  // Holds the press start time


//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// S C E N E    L O O P    B U T T O N    S E T T I N G S  /////////////////////////////

int fsPin[btnCount] = { 5, 6, 7, 8, 9, 14,16, 10 };      // Pin
int fsNextMidiCC = 0;
int fsCurrMidiCC = 0;
int writeAddress;

char *fsNames[36] = {"1A", "1B", "1C", "2A", "2B", "2C", "3A", "3B", "3C", "4A", "4B", "4C",   // buttons 1-4 (address 0 - 11)
                     "5A", "5B", "5C", "6A", "6B", "6C", "7A", "7B", "7C", "8A", "8B", "8C",   // buttons 5-8 (address 12 - 23)
                     "1", "2", "3", "4", "5", "6", "7", "8",                                   // colors  (address 24 - 31)
                     "MIDI Ch",                                                                // global MIDI Channel address 32
                     "DIMMER CC#",                                                             // dimmer CC Val address 33
                     "Reset Defaults",                                                         // reset address 34 (0-No, 1-Yes)
                     "Exit" };                                                                 // exit option for settings: not a stored value

int EEPROMAddress[35] = {0,1,2,3,4,5,6,7,8,9,10,11,                                            // buttons 1-4 (address 0 - 11)
                        12,13,14,15,16,17,18,19,20,21,22,23,                                   // buttons 5-8 (address 12 - 23)
                        24,25,26,27,28,29,30,31,                                               // colors  (address 24 - 31)
                        32,                                                                    // global MIDI Channel address 32
                        33,                                                                    // dimmer CC Val address 33
                        34};                                                                   // reset address 34 (0-No, 1-Yes)  

int resetDefaults[35] = {1,2,3,4,5,6,7,8,9,10,11,12,                                           // buttons 1-4 (address 0 - 11)
                        13,14,15,16,17,18,19,20,21,22,23,24,                                   // buttons 5-8 (address 12 - 23)
                        0,1,2,3,4,5,6,7,                                                       // colors  (address 24 - 31)
                        1,                                                                     // global MIDI Channel address 32
                        50,                                                                    // dimmer CC Val address 33
                        0};                                                                    // reset address 34 (0-No, 1-Yes)

int setDisplay = 0;  // 0-Switch CC format, 1-switch color format, 2-global format

String editDisplay = "";

unsigned long fsPressStart[btnCount] = { 0, 0, 0, 0, 0, 0, 0, 0 };

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

// NEW COLOR STUFF
// CHSV( <Hue 0-255> , <Saturation 0-255> , <Brightness 0-255> ) ex. CHSV(0,255,255) is Red
#define clrCount 8
char *clrName[clrCount] = { "RED", "ORANGE", "YELLOW", "GREEN", "CYAN", "BLUE", "VIOLET", "PINK" };
int clrHue[clrCount] = {0,32,64,96,128,160,192,224};
int fsHue[btnCount] = {0,32,64,96,128,160,192,224};                                                // Temporarily set a color for each footswitch until EEPROM.read is in place
int clrBrt[3] = {25, 90, 255};                                                             // These are the ideal brightness setting for the 3 footswitch states
int fsBrt[btnCount * 3] = {clrBrt[1], clrBrt[0], clrBrt[0],            // This stores the brightness of the switch based on the fsStatus.
                          clrBrt[1], clrBrt[0], clrBrt[0],
                          clrBrt[1], clrBrt[0], clrBrt[0],
                          clrBrt[1], clrBrt[0], clrBrt[0],
                          clrBrt[1], clrBrt[0], clrBrt[0],
                          clrBrt[1], clrBrt[0], clrBrt[0],
                          clrBrt[1], clrBrt[0], clrBrt[0],
                          clrBrt[1], clrBrt[0], clrBrt[0]};                                                   
                                                                            
int usedLeds[btnCount * 3] = {14,13,12,                    // Position of used LEDs skipping every 4th because of the pedal layout
                              10,9,8,
                              6,5,4,
                              2,1,0,
                              19,20,21,
                              23,24,25,
                              27,28,29,
                              31,32,33};
int fsStatusOptions[3] = {};                             // 0-Off, 1-Sel, 2-On
int fsStatus[btnCount * 3] = {1,0,0,                     // Real-time condition of each footswitch
                              1,0,0,
                              1,0,0,
                              1,0,0,
                              1,0,0,
                              1,0,0,
                              1,0,0,
                              1,0,0};
                                                                                                   
int fsGroup[btnCount * 3] = {0,0,0,                     // Group of 3 that each step belongs to                      
                             1,1,1,
                             2,2,2,
                             3,3,3,
                             4,4,4,
                             5,5,5,
                             6,6,6,
                             7,7,7};
int fsCurrPress[btnCount] = {};                         // Stores the current press state of each button


int fsEval;

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// M I D I    S E T T I N G S  /////////////////////////////////////////////////////////

byte midiCh = EEPROM.read(32);  //* MIDI channel to be used
byte note = 36;                             //* Lowest note to be used; 36 = C2; 60 = Middle C
byte cc = 1;                                //* Lowest MIDI CC to be used

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// L E D    S E T T I N G S  ///////////////////////////////////////////////////////////

#define ledPin 4             // Where are LEDs connected?
#define ledNum 34  // How many LEDs are there?
CRGB leds[ledNum];

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// S T A R T U P    A C T I O N S  /////////////////////////////////////////////////////
  int readAddress ;
  int loadDflt = 0;
  int myColor =  0;
  int runOnce = 1;

void setup() {


  pinMode(encButton, INPUT_PULLUP);
  Serial.begin(9600);

  lcd.init();
  lcd.clear();
  lcd.backlight();  // Make sure backlight is on


  lcd.setCursor(0, 0);  //Set cursor to character 2 on line 0
  lcd.print("MIDI Light Ctrl");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);  //Set cursor to character 2 on line 0
  lcd.print("Run Mode");
  lcd.print("                ");

  // LED setup
  FastLED.addLeds<WS2812, ledPin, GRB>(leds, ledNum);  //Reorder RGB to get the desired result
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);      // Set Max level of volts and milliamps
  FastLED.clear();
  FastLED.show();

  for (int i = 0; i < btnCount; i++) {
      pinMode(fsPin[i], INPUT_PULLUP);                                        // Set pin resistors
      fsPressStart[i] = 0;                                                    // Reset each Press Start State
  }



  sub_RebuildLeds();

FastLED.show();
  dimmerPressStart = 0;
  encButtonPressStart = 0;
  // encCounter = 0;


}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// M A I N    L O O P  /////////////////////////////////////////////////////////////////

void loop() {

sub_introLeds ();

// Reset defaults if triggered
  if (EEPROM.read(34) == 1 || loadDflt == 1) {
    for (int i = 0; i < 150; i++) {
      EEPROM.write(i, 0);
    }
    for (int i = 0; i < 34; i++) {
      EEPROM.write(i, resetDefaults[i]);
    }
    EEPROM.write(34,0);
    lcd.setCursor(0,0);
    lcd.print("DEFAULTS RESET! ");
    delay(1500);
  }


  // evaluate moving encoder
  knobValCurr = encPulse.read() / pulsePerInc;  // read current state of encoder

  if (knobValCurr != knobValPrev) {                // if there is a change
    if (sMode == "run") {                          // if in run mode
      sMode = "dimmer";                            // then set to dimmer mode
      dimmerPressStart = millis();                // then start dimmer time-out
      lcd.clear();                                 // then clear lcd
      lcd.setCursor(0, 0);                         // then place lcd cursor position
      lcd.print("DIMMER MODE");                    // then add lcd text
      lcd.print("                ");               // then overwrite anything left on line
      encPulse.write(dimmerCCVal * pulsePerInc);  // then set state to match the current Dimmer setting
    }
    if (sMode == "dimmer") {         // if in dimmer mode
      dimmerPressStart = millis();  // then reset dimmer time-out
    }
    knobValPrev = knobValCurr;  // update the knob state
  }

  // Evaluate Idle Encoder
  if (knobValPrev == knobValCurr && millis() - dimmerPressStart > longPressLimit) {  // If no turn and time-out exceeded
    if (sMode == "dimmer") {                                                            // if in dimmer mode
      sMode = "run";                                                                    // then set to Run Mode
      lcd.clear();                                                                      // then clear lcd
      lcd.setCursor(0, 0);                                                              // then place cursor
      lcd.print("Run Mode");
      lcd.print("                ");
      // sub_print_tmpStrX();  // blank out rest of line
    }
  }

  sub_chkEncBtn(); // run sub to see if button push is long or short

  // Handle Long Btn Press
  if (currPress == 2) {
    if (sMode == "run" || sMode == "edit") {
      sub_goToSetMenu();
    } else if (sMode == "setMenu" || sMode == "setMidi" || sMode == "setColor" || sMode == "chgVal") {
      sub_goToRunMode();
      currPress = 0;
    }
  }


  // Handle Short Btn Press
  if (currPress == 1) {
    if (sMode == "run") {
      //Do Nothing
    }
    if (sMode == "dimmer") {
      sub_goToRunMode();
    }
    if (sMode == "setMenu") {
      if (selNext == "editExitSettings") {  // if the button is pressed when in setMenu and selNext is exit then...
        sub_goToRunMode();          // run exit macro
      }
      if (selNext == "editMidiCh" || selNext == "editMidiCC" || selNext == "editColor" || selNext == "editResetAll" || selNext == "editDimmerCC") {
        sMode = "edit";
        knobVal = EEPROM.read(knobVal);
        // sceneVal = 1;
      }
    }
  }
  if (sMode == "run") {
    sub_fsPressNew();  // Run Run Macro

  }
  if (sMode == "dimmer") {
    sub_encDimmer();  // Run Dimmer Macro
  }
  if (sMode == "setMenu") {
    sub_setMenu();  // Run General Settings Macro
  }
  if (sMode == "editMidi") {
    //sub_editMidi();  //
  }
  if (sMode == "editColor") {
  //   sub_editColor();  //
  }
  if (sMode == "edit") {
    if (selNext == "editMidiCC") {
      sub_writeEEPROM();
    }
    if (selNext == "editColor") {
      sub_writeEEPROM();
    }
    if (selNext == "editResetAll") {
      sub_writeEEPROM();
    }
    if (selNext == "editMidiCh") {
      sub_writeEEPROM();
    }
    if (selNext == "editDimmerCC") {
      sub_writeEEPROM();
    }
  }
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_introLeds() {
    if (runOnce == 1) {
    for (int k = 0; k < 2; k++) {
    for (int i = 0; i < btnCount * 3; i++) {
      leds[usedLeds[i]] = CHSV(myColor, 255, 255);     // hue, saturation, value 
      FastLED.show();
      leds[usedLeds[btnCount * 3 - i]] = CHSV(myColor, 255, 255);     // hue, saturation, value 
      FastLED.show();
      delay(30);
      leds[usedLeds[i]] = CHSV(myColor, 255, 0);     // hue, saturation, value 
      leds[usedLeds[btnCount * 3 - i]] = CHSV(myColor, 255, 0);     // hue, saturation, value 
      FastLED.show();
      myColor = myColor + 10;
    }
  }
    runOnce = 0;
  }
}

void sub_reloadRow2 () {
  lcd.setCursor(0,1);
  lcd.print("MIDI CC#: ");
  lcd.print(fsCurrMidiCC);
  lcd.print(" ON");
  lcd.print("          "); 
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_goToRunMode() {
  sMode = "run";
  selNext = "stndby";
  lcd.clear();
  lcd.setCursor(0, 0);  //Set cursor to character 2 on line 0
  lcd.print("Run Mode");
  lcd.print("                ");
  lcd.setCursor(0, 1);
  sub_reloadRow2 ();
  currPress = 0;
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_goToSetMenu() {
  sMode = "setMenu";  // Set to General Settings Mode
  //selNext = "editGblMidi";
  lcd.clear();                 // clear lcd
  lcd.setCursor(0, 0);         // place cursor
  lcd.print("Settings Menu");  // set to run mode
  lcd.print("                ");
  encPulse.write(0);  // reset encoder
  encPulsePrev = 0;
  knobVal = 0;
  currPress = 0;
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_RebuildLeds() {

   for (int i = 0; i < btnCount * 3; i++) {                                    // Reset each Press Start State
      leds[usedLeds[i]] = CHSV(clrHue[EEPROM.read(EEPROMAddress[fsGroup[i]] + 24)], 255, fsBrt[i]);     // hue, saturation, value 
      Serial.print("LED-"); 
      Serial.print(i);
      Serial.print(" ADDRESS-");
      Serial.print(EEPROMAddress[fsGroup[i]] + 24);
      Serial.print(" VALUE-");
      Serial.println(EEPROM.read(EEPROMAddress[fsGroup[i]] + 24));
   }
   FastLED.show();
}   

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_fsPressNew() {
      int fsGroupOn = 0;                                               // position of evaluated
      int fsStepOn = 0;

  for (int i = 0; i < btnCount; i++) {
      int eval1 = i * 3;                                              //Get position of 1st state in foootswitch group
      int eval2 = (i * 3) + 1;                                        //Get position of 2nd state in foootswitch group
      int eval3 = (i * 3) + 2;                                        //Get position of 3rd state in foootswitch group

      if (digitalRead(fsPin[i]) == LOW && fsPressStart[i] == 0) {     // When button pressed and not in timing state
        fsPressStart[i] = millis();                                   //start timer
      }
      if (digitalRead(fsPin[i]) == LOW && fsPressStart[i] > 1) {      // When button pressed and in timing state, watch for duration to exceed threshold
        if (millis() - fsPressStart[i] > longPressLimit) {            // When the long press state is exceeded, toggle the state
        // actions for long press footswitch state
          Serial.print(i);
          Serial.println(" - Long Press");
          for (int j = 0; j < btnCount * 3; j++) {                    // Turn all 2-On off before proceeding...
            if (fsStatus[j] == 2) {fsStatus[j] = 1;}
          }
          Serial.print(fsStatus[(i * 3)]);
          Serial.print(" - ");
          Serial.print(fsStatus[(i * 3) + 1]);
          Serial.print(" - ");
          Serial.println(fsStatus[(i * 3) + 2]);

          if(fsStatus[(i * 3)] == 1) {                                // if footswitch is in 1-Sel state...
              fsStatus[(i * 3)] = 2;                                  // set footswitch to 2-On and...
              fsStatus[(i * 3) + 1] = 0;                              // set footswitch to 0-Off and...
              fsStatus[(i * 3) + 2] = 0;                              // set footswitch to 0-Off and...
              fsCurrMidiCC = EEPROM.read((i * 3));                // load the Nest Up MIDI CC to send if long pressed...
              Serial.println("Turn On 1")   ;         
          } else if(fsStatus[(i * 3) + 1] == 1) {                     // else if footswitch is in 1-Sel state...
              fsStatus[(i * 3)] = 0;                                  // set footswitch to 0-Off and...
              fsStatus[(i * 3) + 1] = 2;                              // set footswitch to 0-Off and...
              fsStatus[(i * 3) + 2] = 0;                              // set footswitch to 2-On and...
              fsCurrMidiCC = EEPROM.read((i * 3) + 1);                // load the Nest Up MIDI CC to send if long pressed...
              Serial.println("Turn On 2")   ;   
          } else if(fsStatus[(i * 3) + 2] == 1) {                     // else if footswitch is in 1-Sel state...
              fsStatus[(i * 3)] = 0;                                  // set footswitch to 2-On and...
              fsStatus[(i * 3) + 1] = 0;                              // set footswitch to 0-Off and...
              fsStatus[(i * 3) + 2] = 2;                              // set footswitch to 0-Off and...
              fsCurrMidiCC = EEPROM.read((i * 3) + 2);                // load the Nest Up MIDI CC to send if long pressed...
              Serial.println("Turn On 3")   ;   
          }
          sub_controlChange(0, fsCurrMidiCC, 127);                    // Send the MIDI CC
          MidiUSB.flush();                                            // flush MIDI
          sub_reloadRow2 ();
          fsPressStart[i] = 1;                                          // Stop timer, but don't let it start timing again until the HIGH is read
        }
      }
       if (digitalRead(fsPin[i]) == HIGH) {                             // When button pressed and in timing state, watch for duration to exceed threshold
        if (millis() - fsPressStart[i] <= longPressLimit) {             // If Press Start is recognized as being a long press already...
          fsPressStart[i] = 0;                                          // reset counting
          // actions for short press footswitch state
          Serial.print(i);
          Serial.println(" - Short Press");
            if(fsStatus[(i * 3)] == 1) {                                // if footswitch is in 1-Sel state...
                fsStatus[(i * 3)] = 0;                                  // set footswitch to 0-Off and...
                fsStatus[(i * 3) + 1] = 1;                              // set next footswitch to 1-Sel and...
                fsStatus[(i * 3) + 2] = 0;                              // set footswitch to 0-Off and...
                fsNextMidiCC = EEPROM.read((i * 3) + 1);                // load the Nest Up MIDI CC to send if long pressed...
                fsPressStart[i] = 0;                                    // Stop timer and ready it for next LOW read  
                tempDisplay = 0;                                        // ???
            } else if(fsStatus[(i * 3) + 1] == 1) {                     // else if footswitch is in 1-Sel state...
                fsStatus[(i * 3)] = 0;                                  // set footswitch to 0-Off and...
                fsStatus[(i * 3) + 1] = 0;                              // set footswitch to 0-Off and...
                fsStatus[(i * 3) + 2] = 1;                              // set next footswitch to 1-Sel and...
                fsNextMidiCC = EEPROM.read((i * 3) + 2);                // load the Nest Up MIDI CC to send if long pressed...
                fsPressStart[i] = 0;                                    // Stop timer and ready it for next LOW read  
                tempDisplay = 0;                                        // ???
            } else if(fsStatus[(i * 3) + 2] == 1) {                     // else if footswitch is in 1-Sel state...
                fsStatus[(i * 3)] = 1;                                  // set next footswitch to 1-Sel and...
                fsStatus[(i * 3) + 1] = 0;                              // set footswitch to 0-Off and...
                fsStatus[(i * 3) + 2] = 0;                              // set footswitch to 0-Off and...
                fsNextMidiCC = EEPROM.read((i * 3));                    // load the Nest Up MIDI CC to send if long pressed...
                fsPressStart[i] = 0;                                    // Stop timer and ready it for next LOW read  
                tempDisplay = 0;                                        // ???
            }
          lcd.setCursor(0,1);
          lcd.print("MIDI CC#: ");
          lcd.print(fsNextMidiCC);
          lcd.print("          ");
          tempDisplay = millis();                         // start the timing of the temp display duration  
  
        } else if (fsPressStart[i] == 1){
          Serial.println("Reset");
          fsPressStart[i] = 0;                                // Stop timer and ready it for next LOW read    
        }
    }
  fsBrt[i * 3] = clrBrt[fsStatus[i * 3]];                     // update brightness of group/sel
  fsBrt[(i * 3)  + 1] = clrBrt[fsStatus[(i * 3)  + 1]];       // update brightness of group/sel
  fsBrt[(i * 3)  + 2] = clrBrt[fsStatus[(i * 3)  + 2]];       // update brightness of group/sel
  }
  sub_RebuildLeds();                                          // call sub
  delay(20);                                                  // skip double taps  
  if (millis() - tempDisplay > 1500) {                        // if the display duration has exceeded the limit...        
    tempDisplay = 0;                                          // reset the duration
    sub_reloadRow2 ();                                        // call sub  
  } 
  Serial.println(tempDisplay);
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//
//////////////////////////////////// E N C O D E R  //////////////////////////////////////////////////////////////////////

void sub_encDimmer() {
  int myCC = EEPROM.read(33);
  if (knobValCurr != dimmerCCVal) {  // If the previous and the current state of the outputA are different, that means a Pulse has occured
    encPulse.write(max(min(encPulse.read(), 127 * pulsePerInc), 0));
    knobValCurr = encPulse.read() / pulsePerInc;
    dimmerCCVal = knobValCurr;
    sub_controlChange(0, myCC, dimmerCCVal);
    MidiUSB.flush();
    lcd.setCursor(0, 0);  //Set cursor to character 2 on line 0
    lcd.print("Dimmer Mode:     ");
    lcd.setCursor(0, 1);  //Set cursor to character 2 on line 0
    lcd.print("MIDI:");
    lcd.print(myCC);
    lcd.print(" Val:");
    lcd.print(dimmerCCVal);
    lcd.print("                ");
  }
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_setMenu() {
  settingsCount = 34;                           // 
  if (encPulse.read() != encPulsePrev) {        // if the encoder changes...
      lcd.setCursor(0, 1);
      lcd.print("                ");
    if (encPulse.read() >= encPulsePrev + 4) {  // every 4 reads is one inc, so when + 4 are read...
      if (knobVal + 1 > settingsCount) {
        knobVal = 0;
      } else knobVal = knobVal + 1;  // increase the knob, but don't exceed the settings count
      encPulsePrev = encPulse.read();
    }  // update the previous enc value

    if (encPulse.read() <= encPulsePrev - 4) {  // every 4 reads is one inc, so when  - 4 are read...
      if (knobVal - 1 < 0) {
        knobVal = settingsCount;
      } else knobVal = knobVal - 1;                                         // increase the knob, but don't exceed the settings count
      encPulsePrev = encPulse.read();
    }

    if (knobVal > -1 && knobVal < 24) {
      setDisplay = 0;                                                       // Midi CCs
    } else if (knobVal > 23 && knobVal < 32) {                              
      setDisplay = 1;                                                       // color
    } else if (knobVal == 32) {
      setDisplay = 2;                                                      // Midi Channel
    } else if (knobVal == 33) {
      setDisplay = 3;                                                    // Dimmer CC
    } else if (knobVal == 34) {
      setDisplay = 4;                                                    // Reset Defaults
    } else if (knobVal == 35) {
      setDisplay = 5;                                                   // Exit Settings
    }
  }
  switch (setDisplay) {
    case 0:
      editDisplay = "FS ";
      editDisplay = editDisplay + fsNames[knobVal];
      editDisplay = editDisplay +  " CC# ";
      editDisplay = editDisplay + EEPROM.read(knobVal);
      selNext = "editMidiCC";
      break;
    case 1:
      editDisplay = "FS ";
      editDisplay = editDisplay + fsNames[knobVal];
      editDisplay = editDisplay +  " CLR ";
      editDisplay = editDisplay + clrName[EEPROM.read(knobVal)];
      selNext = "editColor";
      break;
    case 2:
      editDisplay = "MIDI Channel ";
      editDisplay = editDisplay + EEPROM.read(32);
      selNext = "editMidiCh";
      break;
    case 3:
      editDisplay = "Dimmer CC ";
      editDisplay = editDisplay + EEPROM.read(33);
      selNext = "editDimmerCC";
      break;
    case 4:
      editDisplay = "Reset Defaults ";
      editDisplay = editDisplay + EEPROM.read(34);
      selNext = "editResetAll";
      break;
    case 5:
      editDisplay = "Exit Settings";
      selNext = "editExitSettings";
      break;
  }
    lcd.setCursor(0, 1);
    lcd.print(editDisplay);
    writeAddress = knobVal;
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_editExit() {
  sMode = "run";                  // update set mode
  selNext = "";                   // clear what happens next
  lcd.setCursor(0, 0);            // place lcd cursor
  lcd.print("Run Mode");          // enter text
  lcd.print("                ");  // overwrite any left over text from line
  lcd.setCursor(0, 1);            // place lcd cursor
  sub_reloadRow2 ();
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_writeEEPROM() {

  if (selNext == "editMidiCC" || selNext == "editDimmerCC") {
    knobMin = 1;
    knobMax = 128;
  }
  if (selNext == "editColor") {
    knobMin = 0;
    knobMax = 9;
  }
  if (selNext == "editResetAll") {
    knobMin = 0;
    knobMax = 1;
  }
  if (selNext == "editMidiCh") {
    knobMin = 1;
    knobMax = 16;
  }
  lcd.setCursor(0, 0);                          // place lcd cursor
  lcd.print(editDisplay);                       // enter text
  lcd.print("                ");                // overwrite any left over text from line
  lcd.setCursor(0, 1);                          // place lcd cursor
  lcd.print(knobVal); 
  if (selNext == "editColor") {
    lcd.print(" ");                // overwrite any left over text from line
    lcd.print(clrName[knobVal]);   
  }                          // enter text
  lcd.print("                ");                // overwrite any left over text from line
  if (encPulse.read() != encPulsePrev) {        // if the encoder changes...
      lcd.setCursor(0, 1);
      lcd.print("                ");
    if (encPulse.read() >= encPulsePrev + 4) {  // every 4 reads is one inc, so when + 4 are read...
      if (knobVal + 1 > knobMax) {
        knobVal = knobMin;
      } else knobVal = knobVal + 1;  // increase the knob, but don't exceed the settings count
      encPulsePrev = encPulse.read();
    }  // update the previous enc value

    if (encPulse.read() <= encPulsePrev - 4) {  // every 4 reads is one inc, so when  - 4 are read...
      if (knobVal - 1 < knobMin) {
        knobVal = knobMax;
      } else knobVal = knobVal - 1;                                         // increase the knob, but don't exceed the settings count
      encPulsePrev = encPulse.read();
    }
  }
  sub_chkEncBtn();                                                          // call sub
  if (currPress == 1) {
    EEPROM.update(writeAddress, knobVal);
    lcd.setCursor(0, 0);
    FastLED.show();
    lcd.print("SAVED!");
    lcd.print("                ");
    delay(1500);
    sub_RebuildLeds();
    sub_goToSetMenu();
  }
  if (currPress == 2) {
    sub_goToSetMenu();

  }
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_chkEncBtn() {
  // Handle Btn Presses
  currPress = 0;
  if (digitalRead(encButton) == LOW && encButtonPressStart == 0) {  // if encoder pressed and no time stamp
    encButtonPressStart = millis();                                 // time stamp
  }
  if (digitalRead(encButton) == LOW && encButtonPressStart > 0) {  // if button pressed and time stamp
    if (millis() - encButtonPressStart > longPressLimit) {       // if exceeded the threshold
      myLoop = myLoop + 1;                                         // then add 1 to the loop counter
    }
    if (myLoop == 1) {
      currPress = 2;  // set to long press if pressed for long enough
      encButtonPressStart = 1;
    }
  }
  if (digitalRead(encButton) == HIGH && encButtonPressStart > 0) {
    if (encButtonPressStart > 1 && millis() - encButtonPressStart <= longPressLimit) {
      currPress = 1;  // set to short press if depressed quickly enough
      encButtonPressStart = 0;
      myLoop = 0;
    } else {
      encButtonPressStart = 0;
      myLoop = 0;
    }
  }
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//

void sub_controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}

//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//----\\----//


