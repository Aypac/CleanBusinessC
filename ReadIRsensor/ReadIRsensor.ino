// constants won't change. They're used here to set pin numbers:
#define NUM_SENS 3
#define PRESENT 0
#define ABSENT 1
#define INIT 2

// put the following values from top to bottom sensor.
int RO_pins[] = {16, 5, 4}; //{10, 9, 8};
int OU_pins[] = {0, 2, 14}; //{11, 7, 6};
int num_rolls[NUM_SENS] = {3, 2, 1};

// Always initalize as many sensors as you have
int sens_vals[NUM_SENS] = {};
int prev_sens_vals[NUM_SENS] = {};

// variables will change:
int prevState = 0;    // variable for the last number of rolls.
int newState = 0;
bool wifiOn = false;


// Turn the Wifi off (if it isn't yet)
void turnWIFIoff() {
  if (wifiOn) {
    Serial.println("Turning off WIFI");
  }
  wifiOn = false;
}

// Turn the Wifi on (if it isn't yet)
void turnWIFIon() {
  if (not wifiOn) {
    Serial.println("Turning on WIFI");
  }
  wifiOn = true;
}

bool measure(bool full=false) {
  bool changed = false;
  for (int i = 0; i < NUM_SENS; i++) {
    digitalWrite(OU_pins[i], HIGH);
    delay(100);
    sens_vals[i] = digitalRead(RO_pins[i]);
    digitalWrite(OU_pins[i], LOW);
    
    // Check if the value has changed from prev measurement
    changed = changed or (prev_sens_vals[i] != sens_vals[i]);
    // if the result matches, we don't need to waste
    // energy to measure the rolls below.
    if (not full and not changed) {
      break;
    }
    // The new value is now old.
    prev_sens_vals[i] = sens_vals[i];
  }
  return changed;
}

void sendMessage(const char* message) {
  turnWIFIon();
  Serial.println(message);
}


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Init...");
  
  // initialize the OUTPUT and input pins
  for (int i = 0; i < NUM_SENS; i++) {
    pinMode(OU_pins[i], OUTPUT);
    pinMode(RO_pins[i], INPUT);
    sens_vals[i] = INIT;
    prev_sens_vals[i] = INIT;
  }

  for (int i = 0; i < 100; i++) {
    // We need to do a full measurement once when we start!
    measure(true);
    delay(1000);
    Serial.print("iter ");
    Serial.println(i);
    Serial.print("Vals ");
    for (int j = 0; j < NUM_SENS; j++) {
      Serial.print(sens_vals[j]);
    }
    Serial.println("");
  }
  Serial.println("Done with init");
}

void loop() {
  // Keep measuring in 1mins intervals until the same result is achieved twice.
  //  only then digest the result.
  //    This prevents messages to be sent while refilling or taking several rolls in quick succession
  bool changed = true;
  while (changed) {
    changed = measure();
    if (changed) {
      delay(60000); //1 min
    }
  }

  // Evaluate measurements
  bool found = false;
  for (int i = 0; i < NUM_SENS; i++) {
    // If there is a roll
    if (not(found) and sens_vals[i] == PRESENT) {
      newState = i; //having num_rolls[i] rolls
      found = true;
      // TODO: only send messages at reasonable hours!
      // TODO: Send out message if different from previous measurement,
      //       or if level deemed critical and last message long ago.
      if (newState != prevState) {
        sendMessage(char(num_rolls[newState]) + " rolls of TP available");
      }
    } else if (found and (sens_vals[i] == PRESENT)) {
      // Only send this once (per sensor)!
      // Send broken sensor message
      if (newState != prevState) {
        char* msg = "";
        sprintf(msg, "Sensor %d (at roll %d) seems to be broken, please check :)", i, num_rolls[i]);
        sendMessage(msg);
      }
    }
  }


  // the new new is now old.
  prevState = newState;
  
  turnWIFIoff();
  // TODO: Go to deep sleep and wake up after an hour or so
  delay(2000);
}
