#define ENCODER_A 3
#define ENCODER_B 4
#define PWM_PIN 9
#define DIR_PIN 8
#define OPEN_BUTTON 5
#define CLOSE_BUTTON 6 
#define home 2

volatile int encoderCount = 0;
const int maxPosition = 1000;
const int baseCloseDelay = 5000;

bool reached = false;
bool isOpening = false;
bool isClosing = false;
unsigned long closeStartTime = 0;
unsigned long closeDelay = baseCloseDelay;

int currentSpeed = 0;

void encoderISR() {
  if (digitalRead(ENCODER_A) > digitalRead(ENCODER_B)) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

int calculateSpeed(int position) {
  int minSpeed = 80;
  int maxSpeed = 180;
  int slowZone = maxPosition * 0.2;
  int mediumZoneEnd = maxPosition * 0.8;
  
  int targetSpeed = (position < slowZone || position > mediumZoneEnd) ? minSpeed : maxSpeed;

  int speedStep = 5;
  if (currentSpeed < targetSpeed) {
    currentSpeed = min(currentSpeed + speedStep, targetSpeed);
  } else if (currentSpeed > targetSpeed) {
    currentSpeed = max(currentSpeed - speedStep, targetSpeed);
  }
  
  return currentSpeed;
}

void moveMotor(int direction) {
  digitalWrite(DIR_PIN, direction);
  analogWrite(PWM_PIN, currentSpeed);
}

void stopMotor() {
  analogWrite(PWM_PIN, 0);
  currentSpeed = 0;
}

void setup() {
  Serial.begin(9600);
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(home, INPUT_PULLUP);
  pinMode(OPEN_BUTTON, INPUT_PULLUP);
  pinMode(CLOSE_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderISR, RISING);

  // Ensure motor moves indefinitely until homing is complete
  while (digitalRead(home) == HIGH) {
    digitalWrite(DIR_PIN, HIGH);
    analogWrite(PWM_PIN, 80);
  }
  stopMotor();
  reached = true;  // Homing complete
  Serial.println("Homing Complete. Ready for operations.");
}

void loop() {
  if (!reached) return;  // Prevent logic from running until homing is done

  if (digitalRead(OPEN_BUTTON) == LOW) {
    if (isClosing) {
      // Emergency stop when closing
      stopMotor();
      isClosing = false;
      isOpening = false;
      Serial.println("Emergency Stop!");
    } else {
      if (!isOpening) {
        isOpening = true;
        Serial.println("Opening...");
      } else {
        closeDelay += 5000;
        Serial.print("Extending close delay: ");
        Serial.println(closeDelay);
      }
    }
  }

  if (digitalRead(CLOSE_BUTTON) == LOW) {
    if (!isClosing) {
      isClosing = true;
      Serial.println("Closing...");
    } // No action needed if already closing
  }

  if (isOpening) {
    int speed = calculateSpeed(abs(encoderCount));
    moveMotor(HIGH);
    if (abs(encoderCount) >= maxPosition) {
      stopMotor();
      isOpening = false;
      closeStartTime = millis();
      Serial.println("Door Fully Open");
    }
    Serial.print("Opening, Encoder: ");
    Serial.println(encoderCount);
  }

  if (!isOpening && !isClosing && millis() - closeStartTime > closeDelay) {
    isClosing = true;
    Serial.println("Auto-Closing...");
  }

  if (isClosing) {
    int speed = calculateSpeed(abs(encoderCount));
    moveMotor(LOW);
    if (encoderCount <= 0) {
      stopMotor();
      isClosing = false;
      closeDelay = baseCloseDelay; // Reset delay after full close
      Serial.println("Door Fully Closed");
    }
    Serial.print("Closing, Encoder: ");
    Serial.println(encoderCount);
  }
}
