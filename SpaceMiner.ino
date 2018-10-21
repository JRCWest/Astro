enum blinkRoles {ASTEROID, SHIP};
byte blinkRole = ASTEROID;
long millisHold = 0;

Timer animTimer;
byte animFrame = 0;

////ASTEROID VARIABLES
byte oreLayout[6];
Color oreColors [5] = {OFF, ORANGE, GREEN, CYAN, YELLOW};
Timer resetTimer;
int resetInterval = 3000;
bool isMinable[6];

////SHIP VARIABLES
byte miningFace = 0;
byte missionCount = 6;
bool missionComplete;
bool gameComplete;
bool isMining;
byte oreTarget;
byte oreCollected;
Timer canMine;
int miningTime = 500;

void setup() {
  // put your setup code here, to run once:
  newAsteroid();
  newMission();
}

void loop() {
  switch (blinkRole) {
    case ASTEROID:
      asteroidLoop();
      asteroidDisplay();
      break;
    case SHIP:
      shipLoop();
      shipDisplay();
      break;
  }
}

void asteroidLoop() {
  if (buttonLongPressed()) {
    blinkRole = SHIP;
    missionCount = 6;
    newMission();
    gameComplete = false;
  }

  //ok, so I'm hanging out, and a blink touches me. is it a ship asking for a thing I have?
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //neighbor!
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getBlinkRole(neighborData) == SHIP) { //a ship!
        resetTimer.set(rand(2000) + resetInterval);
        byte oreRequest = getShipTarget(neighborData);//what does it want?
        FOREACH_FACE(ff) {//do I have one?
          if (oreLayout[ff] == oreRequest) {//yes
            setColorOnFace(WHITE, ff);
            isMinable[f] = true;//that face says "hey, I have one of those. take it"
          }
        }

        //now, if we are minable, check if that ship is currently mining
        if (isMinable[f]) {
          if (getShipMining(neighborData) == 1) { //oh that ship is mining. Our work is done!
            isMinable[f] = false;
            FOREACH_FACE(fff) {
              if (oreLayout[fff] == oreRequest) {
                oreLayout[fff] = 0;//remove that ore
              }
            }
          }
        }
      } else {
        isMinable[f] = false;
      }//end of found ship check
    } else {
      isMinable[f] = false;
    }//end of found neighbor check
  }//end of face check


  //let's check to see if we should renew ourselves!
  if (resetTimer.isExpired()) {
    newAsteroid();
    resetTimer.set(rand(2000) + resetInterval);
  }
  //set up communication
  FOREACH_FACE(f) {
    byte sendData = (blinkRole << 4) + (isMinable[f] << 3);
    setValueSentOnFace(sendData, f);
  }
}

void newAsteroid() {
  //default layout
  oreLayout[0] = 1;
  oreLayout[1] = 2;
  oreLayout[2] = 3;
  oreLayout[3] = 4;
  oreLayout[4] = 0;
  oreLayout[5] = 0;

  //remove one or two of them
  oreLayout[rand(4)] = 0;

  //shuffle array
  for (byte i = 0; i < 10; i++) {
    byte swapA = rand(5);
    byte swapB = rand(5);
    byte temp = oreLayout[swapA];
    oreLayout[swapA] = oreLayout[swapB];
    oreLayout[swapB] = temp;
  }
}

void updateAsteroid(){
  //so we evaluate oreCount
  //then we decide what to do
  //I guess 
}

void shipLoop() {
  if (buttonLongPressed()) {
    blinkRole = ASTEROID;
    newAsteroid();
  }

  //ok, so are we allowed to look around for mining purposes?
  if (canMine.isExpired()) {
    isMining = false;
    //look at neighbors for an asteroid that is minable
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) { //neighbor!
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getBlinkRole(neighborData) == ASTEROID) {//an asteroid!
          if (getAsteroidMinable(neighborData) == 1) {
            //we found one we can mine!
            isMining = true;
            miningFace = f;
            oreCollected++;
            canMine.set(miningTime);
          }
        }
      }
    }
  }

  if (oreCollected >= missionCount) {
    missionComplete = true;
    if (missionCount == 1) {
      gameComplete = true;
    }
  }

  if (buttonDoubleClicked()) {
    if (!gameComplete) { //only works if the game is not complete
      if (missionComplete) {
        missionCount--;
      }
      newMission();
    }
  }

  //set up communication
  byte sendData = (blinkRole << 4) + (isMining << 3) + (oreTarget);
  setValueSentOnAllFaces(sendData);
}

void newMission() {
  missionComplete = false;
  oreTarget = rand(3) + 1;
  oreCollected = 0;
}

void shipDisplay() {
  if (gameComplete) { //big fancy celebration!
    if (resetTimer.isExpired()) {
      //randomly color all faces
      FOREACH_FACE(f) {
        setColorOnFace(oreColors[rand(3) + 1], f);
      }
      resetTimer.set(75);
    }
  } else if (missionComplete) { //small celebration
    setColor(WHITE);
  } else {//just display ore
    FOREACH_FACE(f) {
      if (missionCount > f) {
        if (oreCollected > f) {
          setColorOnFace(oreColors[oreTarget], f);
        } else {
          setColorOnFace(dim(oreColors[oreTarget], 25), f);
        }
      } else {
        setColorOnFace(OFF, f);
      }
    }
    //mining state
    if (!canMine.isExpired()) { //currently mining
      setColorOnFace(WHITE, oreCollected - 1);
    }
  }
}

void asteroidDisplay() {
  FOREACH_FACE(f) {
    Color displayColor = oreColors[oreLayout[f]];
    setColorOnFace(displayColor, f);
  }
}

byte getBlinkRole(byte data) {
  return (data >> 4);//the first two bits
}

byte getShipTarget(byte data) {
  return (data & 7);//the last three bits
}

byte getShipMining(byte data) {
  return ((data >> 3) & 1);//just the third bit
}

byte getAsteroidMinable(byte data) {
  return ((data >> 3) & 1);//just the third bit
}

