const int ledPin =  LED_BUILTIN; // the number of the onboard LED pin
const int readyPin =  10;        // Standby received
const int wakePin =  11;         // Wake word received
const int idlePin =  12;         // Waiting for wake word
const int armPin =  9;           // Throwing arm relay
const int wabblePin =  8;           // Throwing arm relay

int ledState = LOW;                   // ledState used to set the LED
unsigned long previousMillis = 0;     // will store last time LED was updated
unsigned long throwStart = 0;         // will store last time clay throwing arm is activated
unsigned long nextClay = 0;           // will store the next clay launch time;
unsigned long throwDelay = 1000;      // time to complete one throw action
unsigned long throwTimeout = 30000;   // timeout

int b = 0; // for incoming serial data
int stateReady = 0;
int wabbling = 0;
int randomClay = 0;

String commands[] = {
  "Shooter Ready!",
  "Standby!",
  "Pull!",
  "Stop!",
  "Random"
};

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(readyPin, OUTPUT);
  pinMode(wakePin, OUTPUT);
  pinMode(idlePin, OUTPUT);
  pinMode(armPin, OUTPUT);
  pinMode(wabblePin, OUTPUT);
  randomSeed(analogRead(0));
  
  Serial.begin(9600); // opens serial port, sets data rate to 9600 bps
  delay (2000);       // allow voice command board to initialize
  Serial.write(0xAA); // force compact mode
  Serial.write(0x37);
  delay (1000);
  Serial.write(0xAA); // load voice command group 1
  Serial.write(0x21); // 0x21 Group 1 0x22 - Group 2
  Serial.println ("Initialized");
}

void execute (int c){
    Serial.print ("Voice Command received: ");
    Serial.println (commands[c]);
    switch (c) {
      case 0:
        stateReady = 1;
        break;
      case 1:
        if (stateReady == 1) {
          stateReady = 2;
        }
        else {
            Serial.println ("ignored");
        }
        break;
      case 2:
        if (stateReady == 2) {
          throwClay (1);
        }
        else {
            Serial.println ("ignored");
        }
        break;
      case 4:
        if (stateReady == 2) {
          randomStart ();
        }
        else {
            Serial.println ("ignored");
        }
        break;
      case 3:
      default:
        idle ();
        break;
    }
    updateDisplay ();
}

void idle () {
  stateReady = 0;
  throwStart = 0;
}

void randomStart () {
    Serial.println (wabbling == 0 ? "Start wabbling" : "Stop wabbling");
    wabbling = 1 - wabbling;
    digitalWrite(wabblePin, HIGH);
    delay(200);
    digitalWrite(wabblePin, LOW);
    randomClay = random (1, 7);
    Serial.println ("Throwing " + String(randomClay) + " clays");
    delay(200);
    throwClay (1);
}

void throwClay (int start) {
    if (start == 1) {
      stateReady = 3;
      Serial.println ("Starting arm...");
      throwStart = millis();
    }
    else if (start == 0) {
      if (stateReady == 3) {
        Serial.println ("Stopping arm...");
        stateReady = 2;
      }
    }
    else {
      Serial.println ("Timeout...");
      idle ();
    }
}

void updateDisplay () {
  switch (stateReady) {
    case 0:
        Serial.println ("Waiting for wake word");
        break;
    case 1:
        Serial.println ("Waiting for standby");
        break;
    case 2:
        Serial.println ("Ready to throw");
        break;
    case 3:
        Serial.println ("Throwing clay");
        break;
    default:
        Serial.println ("Unexpected");
        break;
  }
}

void showLED () {
    unsigned long currentMillis = millis();
    int interval = 1000 - stateReady * 300;
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        if (ledState == LOW) {
          ledState = HIGH;
        } else {
          ledState = LOW;
        }
    }
    digitalWrite(ledPin, ledState);
    digitalWrite(idlePin, ledState);
    digitalWrite(wakePin, (stateReady == 1 || randomClay > 0) ? HIGH : LOW);
    digitalWrite(readyPin, stateReady == 2 ? HIGH : LOW);
    digitalWrite(armPin, stateReady == 3 ? HIGH : LOW);
}

void clayTimer () {
    unsigned long currentMillis = millis();
    if (throwStart > 0 && nextClay == 0 && currentMillis - throwStart > throwTimeout) {
      throwClay (-1);
    }
    if (stateReady == 3 && throwStart > 0 && currentMillis - throwStart > throwDelay) {
      throwClay (0);
      if (randomClay > 0) {
        Serial.println ("Clay #" + String (randomClay));
        --randomClay;
        if (randomClay > 0) {
          unsigned long d = throwDelay / 2 * random (1, 8) ;
          Serial.println ("Delay = " + String(d));
          nextClay = currentMillis + d;
          throwStart = currentMillis;
        }
      }
    }
    if (nextClay > 0 && currentMillis > nextClay) {
      nextClay = 0;
      throwClay (1);
    }
}

void loop() {
  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    b = Serial.read();

    // say what you got:
    if (b == 0xCC) {
      Serial.println ("OK");
    }
    else if (b == 0xE0) {
      Serial.println ("ERROR");
    }
    else if (b > 0x10 && b < (0x11 + sizeof(commands))) {
      execute (b - 0x11);
    }
    else {
      Serial.println (b, HEX);
    }
  }
  showLED ();
  clayTimer ();
}
