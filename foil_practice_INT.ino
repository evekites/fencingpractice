#include <millisDelay.h>
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
// this is the initial target light delay in milliseconds, adjust accordingly
const int TARGET_TIME = 3000;
const int GAME_TIME = 30000;
const int TEST_LED_TIME = 800;
const int END_GAME_LED_TIME = 1200;
const int VALID = 1;
const int INVALID = 2;
const int MISSED = 3;
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

const int TARGET_1 = 7;   // Top Left Target Target
const int TARGET_2 = 8;   // Top Right Target Target
const int TARGET_3 = 9;   // Center Target Target
const int TARGET_4 = 10;  // Bottom Left Target
const int TARGET_5 = 11;  // Bottom Right Target
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

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

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

void StartNewGame() {
  Serial.println("Press tip of foil to start new game:");
  while (!digitalRead(WEAPON)) {
    delay(50);
    //Serial.println("Press tip of foil to start new game:");
  }
  tipTriggered = false;
  totalHits = 0;
  totalValidHits = 0;
  totalInvalidHits = 0;
  totalMissed = 0;  // timeout passed

  // write to serial console
  Serial.println("------------------------");
  Serial.println("----- Foil target ------");
  Serial.println("------- New game ------");
  Serial.println("------------------------");
  // loop three times to flash lights to confirm all are OK
  for (int i = 1; i <= 3; i++) {
    FlashLEDs(TEST_LED_TIME);
  }
  //delay(2000);  // 2 seconds delay before start
  BlinkHitLEDs();
  timePerGame.start(GAME_TIME);
  NextTarget();
}

void EndGame() {
  FlashLEDs(END_GAME_LED_TIME);
  Serial.println("------------------------");
  Serial.println("----- Game Result ------");
  Serial.println("------------------------");
  Serial.println("Score:");
  Serial.print(totalValidHits);
  Serial.println(" valid hits");
  Serial.print(totalInvalidHits);
  Serial.println(" invalid hits");
  Serial.print(totalMissed);
  Serial.println(" missed: ");
  Serial.println("------------------------");
  Serial.println("------- Game End -------");
  Serial.println("------------------------");
  StartNewGame();
}

void TurnAllLEDsOn() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LIGHTS[i], HIGH);
  }
  digitalWrite(VALID_HIT_LED, HIGH);
  digitalWrite(INVALID_HIT_LED, HIGH);
}

void TurnAllLEDsOff() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LIGHTS[i], LOW);
  }
  digitalWrite(VALID_HIT_LED, LOW);
  digitalWrite(INVALID_HIT_LED, LOW);
}

void FlashLEDs(int pause) {
  TurnAllLEDsOn();
  // Pause before clearing the lights
  delay(pause);
  TurnAllLEDsOff();
  delay(pause / 2);
}


void BlinkHitLEDs() {
  // Blink the eyes
  digitalWrite(INVALID_HIT_LED, HIGH);
  digitalWrite(VALID_HIT_LED, HIGH);
  tone(BUZZER, 2000);
  delay(200);

  digitalWrite(INVALID_HIT_LED, LOW);
  digitalWrite(VALID_HIT_LED, LOW);
  noTone(BUZZER);
  delay(200);

  digitalWrite(INVALID_HIT_LED, HIGH);
  digitalWrite(VALID_HIT_LED, HIGH);
  tone(BUZZER, 2000);
  delay(200);

  digitalWrite(INVALID_HIT_LED, LOW);
  digitalWrite(VALID_HIT_LED, LOW);
  noTone(BUZZER);
  delay(500);
}

void NextTarget() {
  // lights off
  TurnAllLEDsOff();
  target = random(5) + 1;
  if (!tipTriggered) {
    BlinkHitLEDs();
  }
  tipTriggered = false;
  //turn selected target on
  digitalWrite(LIGHTS[target - 1], HIGH);  // target_1 is at position 0 in array, so [target -1]
  tone(BUZZER, 2000);
  delay(100);
  noTone(BUZZER);
  Serial.println("Alez!");
  timePerTarget.start(TARGET_TIME);
}

void PrintStatus(bool status) {
  if (status == VALID) {
    Serial.print("VALID Score: ");
  } else if (status == INVALID) {
    Serial.print("INVALID Score: ");
  } else if (status == MISSED) {
    Serial.print("MISSED Score: ");
  }
  Serial.print("valid hits:");
  Serial.print(totalValidHits);
  Serial.print(", invalid hits:");
  Serial.print(totalInvalidHits);
  Serial.print(", missed:");
  Serial.print(totalMissed);
  Serial.print(", response: ");
  Serial.print(responseTime);
  Serial.print("ms, game time left: ");
  Serial.println(timePerGame.remaining() / 1000);
}

void ValidHit() {
  totalValidHits++;
  PrintStatus(VALID);
  digitalWrite(VALID_HIT_LED, HIGH);
  tone(BUZZER, 1000);
  delay(1000);
  noTone(BUZZER);
  digitalWrite(VALID_HIT_LED, LOW);
}

void InvalidHit() {
  totalInvalidHits++;
  PrintStatus(INVALID);
  digitalWrite(INVALID_HIT_LED, HIGH);
  tone(BUZZER, 500);
  delay(1000);
  noTone(BUZZER);
  digitalWrite(INVALID_HIT_LED, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (timePerTarget.remaining() == 0) {
    totalMissed++;
    PrintStatus(MISSED);
    NextTarget();
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