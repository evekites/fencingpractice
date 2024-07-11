#include <millisDelay.h>

// #######################################################
// this is the initial target light delay in milliseconds, adjust accordingly
const int TARGET_TIME = 3000;
const int GAME_TIME = 30000;
const int TEST_LED_TIME = 800;
const int END_GAME_LED_TIME = 1200;
//######################################################
const int LIGHT_1 = A0;  // this analogue port will be used as a digital output
const int LIGHT_2 = A1;  // this analogue port will be used as a digital output
const int LIGHT_3 = A2;  // this analogue port will be used as a digital output
const int LIGHT_4 = A3;  // this analogue port will be used as a digital output
                         // A4 A5 reserved for I2C devices (display??)
const int LIGHT_5 = 6;   // A6/A7 didn't work on my Nano, so L5 = 6 (D6)
const int TARGET_1 = 7;  // need to be on adjacent sequential ports!!!, for example 7..11
const int TARGET_2 = 8;
const int TARGET_3 = 9;
const int TARGET_4 = 10;
const int TARGET_5 = 11;
const int LIGHTS[] = {LIGHT_1, LIGHT_2, LIGHT_3, LIGHT_4, LIGHT_5};
const int TARGETS[] = {TARGET_1, TARGET_2, TARGET_3, TARGET_4, TARGET_5};
/*
  Foil:
    pin C (far pin)     Guard connected to GND
    pin B (middle pin)  Weapon connected to an interrupt pin
*/
const int WEAPON = 3;      // Weapon, this must be an interrupt pin (UNO D2 or D3)
const uint8_t BUZZER = 4;  // buzzer pin

// setup pins for scoring box
const int VALID_HIT_LED = 12;    // Valid hit
const int INVALID_HIT_LED = 13;  // Invalid hit

int target;      // which target to be lit/scored
int tipval;      // Tip hit state : 0 is a hit
int targval;     // Target hit state
int prevTarget;  // try to avoid repeating the same target
bool tipTriggered = false;
int totalHits;
int totalValidHits;
int totalMissed;

millisDelay timeToScore;
millisDelay timeToPlay;

int responseTime;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  // initialize the LED pins as output:
  pinMode(INVALID_HIT_LED, OUTPUT);  // on control box and maybe also on target
  pinMode(VALID_HIT_LED, OUTPUT);    // on control box and maybe also on target

  for (int i=0; i<5 ; i++)
  {
        
  }

  pinMode(LIGHT_1, OUTPUT);          // on target 1
  pinMode(LIGHT_2, OUTPUT);          // on target 2
  pinMode(LIGHT_3, OUTPUT);          // on target 3
  pinMode(LIGHT_4, OUTPUT);          // on target 4
  pinMode(LIGHT_5, OUTPUT);          // on target 5
  pinMode(BUZZER, OUTPUT);

  // initialize the input pins. INPUT_PULLUP is used to prevent floating state in rest
  pinMode(TARGET_1, INPUT_PULLUP);  // target 1 Top Left
  pinMode(TARGET_2, INPUT_PULLUP);  // target 2 Top Right
  pinMode(TARGET_3, INPUT_PULLUP);  // target 3 Center
  pinMode(TARGET_4, INPUT_PULLUP);  // target 4 Bottom Left
  pinMode(TARGET_5, INPUT_PULLUP);  // target 5 Bottom Right
  pinMode(WEAPON, INPUT_PULLUP);    // foil: Pin B


  StartNewGame();

  attachInterrupt(digitalPinToInterrupt(WEAPON), ISR_WeaponHit, FALLING);
  /*
    Since the switch in a foil is NC (normaly closed) in rest , the interrupt is triggered on FALLING
  */
}

// ISR handling for a hit, only some variables are set here,
// so an interrupt (pressing tip of foil), interrupting the ISR itself, shouldn't be a very big issue.
void ISR_WeaponHit() {
  tipTriggered = true;
  responseTime = TARGET_TIME - timeToScore.remaining();  // in millisecs
}

void StartNewGame() {
  Serial.println("Press tip of foil to start new game:");
  while (!digitalRead(WEAPON)) {  //tipTriggered is changed through an interrupt!
    delay(50);
    //Serial.println("Press tip of foil to start new game:");
  }
  tipTriggered=false;
  totalHits = 0;
  totalValidHits = 0;
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
  timeToPlay.start(GAME_TIME);
  NextTarget();
}

void EndGame() {
  FlashLEDs(END_GAME_LED_TIME);
  Serial.println("------------------------");
  Serial.println("----- Game Result ------");
  Serial.println("------------------------");
  Serial.println("Score:");
  Serial.print(totalHits);
  Serial.println(" valid hits");
  Serial.print(totalValidHits);
  Serial.println(" invalid hits");
  Serial.print(totalMissed);
  Serial.println(" missed: ");
  Serial.println("------------------------");
  Serial.println("------- Game End -------");
  Serial.println("------------------------");
  StartNewGame();
}
void FlashLEDs(int pause) {
  // flash all target LEDs
  // turn all the target LEDs ON
  digitalWrite(LIGHT_1, HIGH);
  digitalWrite(LIGHT_2, HIGH);
  digitalWrite(LIGHT_3, HIGH);
  digitalWrite(LIGHT_4, HIGH);
  digitalWrite(LIGHT_5, HIGH);
  digitalWrite(VALID_HIT_LED, HIGH);
  digitalWrite(INVALID_HIT_LED, HIGH);

  // Pause before clearing the lights
  delay(pause);

  digitalWrite(LIGHT_1, LOW);
  digitalWrite(LIGHT_2, LOW);
  digitalWrite(LIGHT_3, LOW);
  digitalWrite(LIGHT_4, LOW);
  digitalWrite(LIGHT_5, LOW);
  digitalWrite(VALID_HIT_LED, LOW);
  digitalWrite(INVALID_HIT_LED, LOW);

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
  delay(200);
}

void NextTarget() {
  // lights off
  digitalWrite(INVALID_HIT_LED, LOW);
  digitalWrite(VALID_HIT_LED, LOW);
  digitalWrite(LIGHT_1, LOW);
  digitalWrite(LIGHT_2, LOW);
  digitalWrite(LIGHT_3, LOW);
  digitalWrite(LIGHT_4, LOW);
  digitalWrite(LIGHT_5, LOW);

  // prevTarget = target;

  // // pick a random target, but not the current one
  // do {
  //   target = random(5) + 1;
  // } while (target == prevTarget);
  target = random(5) + 1;
  if (!tipTriggered) {
    BlinkHitLEDs();
  }
  tipTriggered = false;
  //turn selected target on
  switch (target) {
    case 1:
      digitalWrite(LIGHT_1, HIGH);
      break;
    case 2:
      digitalWrite(LIGHT_2, HIGH);
      break;
    case 3:
      digitalWrite(LIGHT_3, HIGH);
      break;
    case 4:
      digitalWrite(LIGHT_4, HIGH);
      break;
    case 5:
      digitalWrite(LIGHT_5, HIGH);
      break;
  }
  Serial.println(timeToPlay.remaining() / 1000);
  Serial.print("Target: ");
  Serial.println(target);
  Serial.println("Alez!");
  timeToScore.start(TARGET_TIME);
}

void PrintStatus(bool valid) {
  if (valid) {
    Serial.print("Valid hit: ");
  } else {
    Serial.print("Invalid hit: ");
  }
  Serial.print(totalValidHits);
  Serial.print("/");
  Serial.print(totalHits);
  Serial.print(", ");
  Serial.print(responseTime);
  Serial.print("ms, game time left: ");
  Serial.println(timeToPlay.remaining() / 1000);
}

void ValidHit() {
  totalValidHits++;
  PrintStatus(true);
  digitalWrite(VALID_HIT_LED, HIGH);
  tone(BUZZER, 1000);
  delay(1000);
  noTone(BUZZER);
  digitalWrite(VALID_HIT_LED, LOW);
}

void InvalidHit() {
  PrintStatus(true);
  digitalWrite(INVALID_HIT_LED, HIGH);
  tone(BUZZER, 500);
  delay(1000);
  noTone(BUZZER);
  digitalWrite(INVALID_HIT_LED, LOW);
}



void loop() {
  // put your main code here, to run repeatedly:
  if (timeToScore.remaining() == 0) {
    totalMissed++;
    NextTarget();
  }
  if (timeToPlay.remaining() == 0) {
    EndGame();
  }
  if (tipTriggered) {
    totalHits++;
    switch (target) {
      case 1:
        targval = !digitalRead(TARGET_1);  // pullup, so LOW when hit, HIGH in rest
        break;
      case 2:
        targval = !digitalRead(TARGET_2);  // pullup, so LOW when hit, HIGH in rest, so targvalue is the opposite
        break;
      case 3:
        targval = !digitalRead(TARGET_3);  // pullup, so LOW when hit, HIGH in rest, so targvalue is the opposite
        break;
      case 4:
        targval = !digitalRead(TARGET_4);  // pullup, so LOW when hit, HIGH in rest, so targvalue is the opposite
        break;
      case 5:
        targval = !digitalRead(TARGET_5);  // pullup, so LOW when hit, HIGH in rest, so targvalue is the opposite
        break;
    }
    if (targval) {
      ValidHit();
    } else {
      InvalidHit();
    }
    NextTarget();
  }
}
