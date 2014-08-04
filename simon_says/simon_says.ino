#include "LRF.h"
#include "Timer.h"
#include "LRFHardware.h"

Timer gameTimer;
Timer endTimer;

int gameTimerId;

const unsigned char MAX_TAPS = 15;
const unsigned long TAP_SHORT = 200;
const unsigned long TAP_LONG = 700;
const unsigned long FUDGE = 200;

unsigned long touchStartTime;
unsigned long touchEndTime;
boolean touchHolding = false;

// Game variables
boolean gameOn = false;
boolean playing = false;

unsigned long roundTimeMultiplier = 750;
unsigned char gameRound = 1;
unsigned char highScore = 3;
unsigned char roundTapCount;
unsigned long roundTaps[MAX_TAPS];
unsigned char userTapCount;
unsigned long userTaps[MAX_TAPS];

/***********************************/

void startGame() {
  delay(250);
  gameOn = true;
  setupRound();
  startRound();
}

// This exists to be used as a callback to put some space between phantom
// touches and the end of the last level.
void resetGame() {
  //// Serial.println("resetting game");
  delay(10);
  gameOn = false;
  
  if (gameRound > 1) {
    startGame();
  }
}

void quitGame() {
  int duration = min(100, 250 - (gameRound * 20));
  
  for (int i = 1; i < (gameRound * 3); i++) {
    unsigned char color = 255 - (i * 10);
    unsigned int sound = 7000 + (i * 100);
    lrf.setBothLEDs(color, 0, 0);
    lrf.setSpeaker(sound, duration);
  }
  
  lrf.setBothLEDs(0, 0, 0);
  gameRound = 1;
}

void setupRound() {
  // Serial.print("Setting up round ");
  // Serial.println(gameRound);
  
  // Intitialize user taps and round taps to be non-matching
  // values.
  for (int i = 0; i < MAX_TAPS; i++) {
    userTaps[i] = 0;
    roundTaps[i] = 0;
  }
  
  userTapCount = 0;
  setRoundRules();
}

void setRoundRules() {
  roundTapCount = gameRound;
    
  for (int i = 0; i < roundTapCount; i++) {
    lrf.setBothLEDs(0, 255, 0);
    // Random [0,2)
    if (random(0,2) == 1) {
      // Serial.println("adding long tap");
      roundTaps[i] = TAP_LONG;
      lrf.setSpeaker(5000, TAP_LONG);
    } else {
      // Serial.println("adding short tap");
      roundTaps[i] = TAP_SHORT;
      lrf.setSpeaker(5000, TAP_SHORT);
    }
    lrf.setBothLEDs(0, 0, 0);    
    
    delay(200);
  }
}

void startRound() {
  long timeout = 2000 + (gameRound * roundTimeMultiplier);
  // Serial.print("Round started, timer set to: ");
  // Serial.println(timeout);
  gameTimerId = gameTimer.after(timeout, scoreRound);
  playing = true;
}

void addUserTap(unsigned long tapLength) {
  boolean nearLongTap = ((TAP_LONG + FUDGE) > tapLength) && (tapLength > (TAP_LONG - FUDGE));
  boolean nearShortTap = ((TAP_SHORT + FUDGE) > tapLength) && (tapLength > (TAP_SHORT - FUDGE));

  if (nearLongTap && !nearShortTap) {
    userTaps[userTapCount] = TAP_LONG;
  } else if (nearShortTap && !nearLongTap) {
    userTaps[userTapCount] = TAP_SHORT;
  } else if (nearShortTap && nearLongTap) {
    // Tie breaker
    boolean longer = abs(tapLength - TAP_LONG) > abs(tapLength - TAP_SHORT);
    if (longer) {
      userTaps[userTapCount] = TAP_LONG;
    } else {
      userTaps[userTapCount] = TAP_SHORT;
    }
  } else {
    // Tap was neither short nor long, user fails
    userTaps[userTapCount] = 0;
  }
  
  if (userTaps[userTapCount] == TAP_SHORT) {
    // Serial.println("short tap");
  } else if (userTaps[userTapCount] == TAP_LONG) {
    // Serial.println("long tap");
  } else {
    // Serial.println("bad tap");
  }
  
  userTapCount++;
}

void scoreRound() {
  gameTimer.stop(gameTimerId);
  playing = false;
  
  int score = 0;
  boolean won = false;
  
  lrf.setBothLEDs(0, 0, 200);
  for (int i = 0; i < roundTapCount; i++) {
    //// Serial.print(i);
    if (userTaps[i] == roundTaps[i]) {
      score++;
    }
  }
  delay(250);
  lrf.setSpeaker(5000, 1000);
    
  if ((userTapCount == roundTapCount) && (score == roundTapCount)) {
    // Serial.println("You won!");
    won = true;
    if (score > highScore) {
      highScore = score;
      lrf.setBothLEDs(0, 255, 50);
      lrf.setSpeaker(3000, 200);
      lrf.setBothLEDs(0, 255, 255);
      lrf.setSpeaker(2500, 200);
    } else {
      lrf.setBothLEDs(0, 255, 50);
      lrf.setSpeaker(3000, 400);
    }
    gameRound++;
  } else {
    // Serial.println("You lost like a mofo, sukka.");
    lrf.setBothLEDs(255, 0, 50);
    lrf.setSpeaker(7000, 500);
    quitGame();
  }
  
  endTimer.after(500, resetGame);
  
  lrf.setBothLEDs(0, 0, 0);
}

void touchDown() {
  touchStartTime = millis();
  
  if (playing) {
    lrf.setBothLEDs(255, 0, 0);
  } else if (!gameOn) {
    // This means we're about to start a game
    lrf.setBothLEDs(255, 255, 0);
  }
}

void touchPress() {
}

void touchUp() {
  touchEndTime = millis();
  
  lrf.setBothLEDs(0, 0, 0);
  unsigned long tapLength = (touchEndTime - touchStartTime);
    
  if (playing) {
    addUserTap(tapLength);
  } else if (!gameOn) {
    // This means we're about to start a game
    startGame();
  }
}

void doNothing() {
}

void setup(void) {
  lrf.setup();
}

void loop(void) {
  gameTimer.update();
  endTimer.update();
  
  // Process touch events
  if (lrf.readTouch()) {
    // Registering HW touch
    if (!touchHolding) {
      touchDown();
      touchHolding = true;
    } else {
      touchPress();
    }
  } else {
    if (touchHolding) {
      touchUp();
      touchHolding = false;
    }
  }
}
