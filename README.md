# fencingpractice

Target practice device with 5 random targets

Based on the following code: https://www.agurney.com/fencingprojects/all-weapons-target 
Reworked the whole code. The tip of the foil is now connected to an interrupt, so you can only score when the tup
of the foil was pressed. This way it is also possible to score an invalid hit.

## Game play
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

## Logic
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


