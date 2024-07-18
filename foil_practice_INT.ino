#include <millisDelay.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define DEBUG
/*
Based on the following code: https://www.agurney.com/fencingprojects/all-weapons-target 
Reworked the whole code. The tip of the foil is connected to an interrupt, so you can only score when the tup
of the foil was pressed. This way it is also possible to score an invalid hit.
- After pressing the foil tip, an interrupt will activate the ISR_WeaponHit() function.
- In this function the boolean tipTriggered will be set to true and the elapsed time will be calculated (nothing more!)
- In the main loop, if tipTriggered = true, the variable totalHits will be incrmented
  and there will be checked whether the correct target was hit
  - If the correct target was hit, the variable totalValidHits will be incremented and the VALID_HIT_LED will be lit
  - If the correct target was not hit, the INVALID_HIT_LED will be lit
- If the tip of the foil was not pressed during a pre-set interval TARGET_TIME, the variable totalMissed will be incremented
  and a new target will be shown
- As soon as the gametime (GAME_TIME) is over the final results will be shown. You can start a new game,
  by pressing the tip of the foil


Game play
- Press the tip of the foil to start a new game
- All lights (5 target lights, INVALID_HIT_LED and VALID_HIT_LED will flash 3 times, to TEST all LED's
- Both INVALID_HIT_LED & VALID_HIT_LED will blink twice
- The first target LED will be lit for a period of GAME_TIME ms and the buzzer will be heard briefly (high pitch)
- Pressing the tip off the foil on the correct target:
  - VALID_HIT_LED wil be lit
  - the buzzer will be heard (high pitch)
  - a new target will shown
- Pressing the tip off the foil somewhere else (including the other targets):
  - INVALID_HIT_LED will be lit
  - the buzzer will be heard (low pitch)
  - a new target will be shown
- If you don't press the tip off the foil within TARGET_TIME ms after the target was shown
  - INVALID_HIT_LED and VALID_HIT_LED and the buzzer (low picht) will blink twice
  - a new target will be shown



*/
// #######################################################
const int TARGET_TIME = 3000;
const int GAME_TIME = 30000;
const int TEST_LED_TIME = 500;
const int END_GAME_LED_TIME = 500;
const int VALID = 1;
const int INVALID = 2;
const int MISSED = 3;
const int NEW = 4;
const int END = 5;
const bool LCD_CLEAR = true;
const bool LCD_PRINT = true;
const bool SERIAL_PRINT = true;
//######################################################
/*
  Foil:
    pin C (far pin)     Guard connected to GND
    pin B (middle pin)  Weapon connected to an interrupt pin
*/
const int WEAPON = 3;      // Weapon, this must be an interrupt pin (Nano/Uno D2 or D3)
const uint8_t BUZZER = 4;  // buzzer pin

const int LIGHT_1 = A0;  // Top Left LED,      this analogue port will be used as a digital output
const int LIGHT_2 = A1;  // Top Right LED      this analogue port will be used as a digital output
const int LIGHT_3 = A2;  // Center LED         this analogue port will be used as a digital output
const int LIGHT_4 = A3;  // Botom Left LED     this analogue port will be used as a digital output
                         //                    A4 A5 reserved for I2C devices (display??)
const int LIGHT_5 = 6;   // Bottom Right LED   A6/A7 didn't work on my Nano, so L5 = 6 (D6)

const int TARGET_1 = 11;  // Top Left Target Target
const int TARGET_2 = 10;  // Top Right Target Target
const int TARGET_3 = 9;   // Center Target Target
const int TARGET_4 = 8;   // Bottom Left Target
const int TARGET_5 = 7;   // Bottom Right Target
const int LIGHTS[] = { LIGHT_1, LIGHT_2, LIGHT_3, LIGHT_4, LIGHT_5 };
const int TARGETS[] = { TARGET_1, TARGET_2, TARGET_3, TARGET_4, TARGET_5 };

// setup pins for scoring box
const int VALID_HIT_LED = 12;    // Valid hit
const int INVALID_HIT_LED = 13;  // Invalid hit

// The following pins are free:
//  D0 & D1 reserved for Serial connection and uploading new firmware
//  D2 (interrupt on Nano/Uno)
//  D5
//  A4 & A5 reserved for I2C devices like a display
//  A6 & A7 (not available on a Uno)


int target;      // which target to be lit/scored
int tipval;      // Tip hit state : 0 is a hit
int targval;     // Target hit state
int prevTarget;  // try to avoid repeating the same target
bool tipTriggered = false;
int totalHits;
int totalValidHits;
int totalInvalidHits;
int totalMissed;

millisDelay timePerTarget;
millisDelay timePerGame;

int responseTime;
int totalTime = 0;
int avgTime;
char buff[16];

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  lcd.init();
  lcd.backlight();
  // initialize the LED pins as output:
  pinMode(BUZZER, OUTPUT);
  pinMode(INVALID_HIT_LED, OUTPUT);  // on control box and maybe also on target
  pinMode(VALID_HIT_LED, OUTPUT);    // on control box and maybe also on target
  // initialize the light pins in a loop
  for (int i = 0; i < 5; i++) {
    pinMode(LIGHTS[i], OUTPUT);
  }

  // initialize the input pins. INPUT_PULLUP is used to prevent floating state in rest
  pinMode(WEAPON, INPUT_PULLUP);  // foil: Pin B
  // initialize the target pins in a loop
  for (int i = 0; i < 5; i++) {
    pinMode(TARGETS[i], INPUT_PULLUP);  // target 1 Top Left
  }

  attachInterrupt(digitalPinToInterrupt(WEAPON), ISR_WeaponHit, FALLING);
  /*
    Since the switch in a foil is NC (normaly closed) in rest , the interrupt is triggered on FALLING
  */
  StartNewGame();
}

// ISR handling for a hit, only some variables are set here,
// so an interrupt (pressing tip of foil), interrupting the ISR itself, shouldn't be a very big issue.
void ISR_WeaponHit() {
  tipTriggered = true;
  responseTime = TARGET_TIME - timePerTarget.remaining();  // in millisecs
}

// Function to play a tone, but not during DEBUG (the buzzer is anyoing)
void playTone(int buzzerPin, unsigned int frequency) {
#ifndef DEBUG
  tone(buzzerPin, frequency);
#endif
}

// Function to print an array of 16 chars to both 16x2 LCD and Serial!
void printLCDandSerial(bool clear, int x, int y, bool lcdprint, bool serialprint, char message[16]) {
  if (clear) {
    if (lcdprint) {
      lcd.clear();
    }
    if (serialprint) {
      Serial.println();
    }
  }
  if (lcdprint) {
    lcd.setCursor(x, y);
    lcd.print(message);
  }
  if (serialprint) {
    Serial.println(message);
  }
}

// Function to start a new game
// start the timePerGame session
// reset some variables
// flash all LED's and sound the buzzer
// blink the hit LED's and sound the buzzer
// and finaly activate nextTarget()
void StartNewGame() {
  printLCDandSerial(LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "Press foil tip");
  printLCDandSerial(!LCD_CLEAR, 0, 1, LCD_PRINT, SERIAL_PRINT, "to start game!");
  while (!digitalRead(WEAPON)) {
    delay(50);
  }
  tipTriggered = false;
  totalHits = 0;
  totalValidHits = 0;
  totalInvalidHits = 0;
  totalMissed = 0;  // no valid nor invalid hit within TARGET_TIME ms
  totalTime = 0;
  avgTime = 0;
  printLCDandSerial(LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "Testing all LEDs");
  printLCDandSerial(!LCD_CLEAR, 0, 1, LCD_PRINT, SERIAL_PRINT, "PRETS?");
  // loop three times to flash lights to confirm all are OK
  for (int i = 1; i <= 3; i++) {
    FlashLEDs(TEST_LED_TIME);
  }
  //delay(2000);  // 2 seconds delay before start
  BlinkHitLEDs();
  timePerGame.start(GAME_TIME);
  printLCDandSerial(LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "ALLEZ");
  NextTarget();
}

// Function to end the current game
// Printing HALT
// Printing avgTime 
// Other statistics are allready visible (Touche, Incorrect & Missed)
// flash all LED's and sound the buzzer
// Waits for pressing on the foil tip and then activate StartNewGame()
void EndGame() {
  printLCDandSerial(!LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "HALT    ");
  sprintf(buff, "%4d ms", avgTime);
  printLCDandSerial(!LCD_CLEAR, 0, 1, LCD_PRINT, SERIAL_PRINT, buff);
  for (int i = 0; i < 3; i++) {
    FlashLEDs(END_GAME_LED_TIME);
  }
  while (!digitalRead(WEAPON)) {
    delay(50);
  }
  delay(500);
  StartNewGame();
}

// Function to turn on all LED's (target LED's and hit LED's) and sound buzzer
void TurnAllLEDsOn() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LIGHTS[i], HIGH);
  }
  digitalWrite(VALID_HIT_LED, HIGH);
  digitalWrite(INVALID_HIT_LED, HIGH);
  playTone(BUZZER, 500);
}

// Function to turn off all LED's (target LED's and hit LED's) and turn off sound buzzer
void TurnAllLEDsOff() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LIGHTS[i], LOW);
  }
  digitalWrite(VALID_HIT_LED, LOW);
  digitalWrite(INVALID_HIT_LED, LOW);
  noTone(BUZZER);
}

// Function to flash all LED's during a specified duration
void FlashLEDs(int pause) {
  TurnAllLEDsOn();
  // Pause before clearing the lights
  delay(pause);
  TurnAllLEDsOff();
  delay(pause / 2);
}

// Function to blink the hit LED's and a buzzer twice
void BlinkHitLEDs() {
  // Blink the eyes
  digitalWrite(INVALID_HIT_LED, HIGH);
  digitalWrite(VALID_HIT_LED, HIGH);
  playTone(BUZZER, 500);
  delay(200);

  digitalWrite(INVALID_HIT_LED, LOW);
  digitalWrite(VALID_HIT_LED, LOW);
  noTone(BUZZER);
  delay(200);

  digitalWrite(INVALID_HIT_LED, HIGH);
  digitalWrite(VALID_HIT_LED, HIGH);
  playTone(BUZZER, 500);
  delay(200);

  digitalWrite(INVALID_HIT_LED, LOW);
  digitalWrite(VALID_HIT_LED, LOW);
  noTone(BUZZER);
  delay(500);
}

// Function to select a new target
void NextTarget() {
  // lights off
  TurnAllLEDsOff();
  target = random(5) + 1;
  if (!tipTriggered) {
    BlinkHitLEDs(); // I don't know anymore in which situation this will be executed!!
  }
  tipTriggered = false; // tipTriggerd is reset to restore from a double hit!
  //turn selected target on
  digitalWrite(LIGHTS[target - 1], HIGH);  // target_1 is at position 0 in array, so [target -1]
  playTone(BUZZER, 2000);
  delay(100);
  noTone(BUZZER);
  printLCDandSerial(!LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "ALLEZ   ");
  timePerTarget.start(TARGET_TIME);
}

void PrintStatus(int status) {
  printLCDandSerial(LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "");
  if (status == VALID) {
    printLCDandSerial(!LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "TOUCHE");
  } else if (status == INVALID) {
    printLCDandSerial(!LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "INCOR.");
  } else if (status == MISSED) {
    printLCDandSerial(!LCD_CLEAR, 0, 0, LCD_PRINT, SERIAL_PRINT, "NO HIT");
    responseTime=TARGET_TIME;
  }
  sprintf(buff, "%2d/%2d/%2d", totalValidHits, totalInvalidHits, totalMissed);
  printLCDandSerial(!LCD_CLEAR, 8, 0, LCD_PRINT, SERIAL_PRINT, buff);

  sprintf(buff, "%4d ms", responseTime); // responseTime is the time for the previous hit VALID/INVALID/MISSED
  printLCDandSerial(!LCD_CLEAR, 0, 1, LCD_PRINT, SERIAL_PRINT, buff);

  printLCDandSerial(!LCD_CLEAR, 8, 1, LCD_PRINT, !SERIAL_PRINT, "To/In/No");
}

void ValidHit() {
  totalValidHits++;
  totalTime += responseTime;
  avgTime = totalTime / totalValidHits; // only calculate avgTime for valid hits

  PrintStatus(VALID);
  digitalWrite(VALID_HIT_LED, HIGH);
  playTone(BUZZER, 1000);
  delay(1000);
  noTone(BUZZER);
  digitalWrite(VALID_HIT_LED, LOW);
}

void InvalidHit() {
  totalInvalidHits++;
  PrintStatus(INVALID);
  digitalWrite(INVALID_HIT_LED, HIGH);
  playTone(BUZZER, 500);
  delay(1000);
  noTone(BUZZER);
  digitalWrite(INVALID_HIT_LED, LOW);
}

void Missed() {
  totalMissed++;
  PrintStatus(MISSED);
  NextTarget();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (timePerTarget.remaining() == 0) {
    Missed();
  }
  if (timePerGame.remaining() == 0) {
    EndGame();
  }

  if (tipTriggered) {
    totalHits++;

    targval = !digitalRead(TARGETS[target - 1]);  // pullup, so LOW when hit, HIGH in rest, so targvalue is the opposite
    if (targval) {
      ValidHit();
    } else {
      InvalidHit();
    }
    NextTarget();
  }
}