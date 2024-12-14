/* #include <Arduino.h>

// Define pins for PIR sensor and ultrasonic sensor
const int PIR_SENSOR_OUTPUT_PIN = 18;
const int trigPin = 44;
const int echoPin = 43;

// Constants for ultrasonic sensor
#define SOUND_SPEED 0.034  // Speed of sound in cm/us
#define DISTANCE_THRESHOLD 15.0  // Distance threshold in cm

// State definitions
enum State {
  IDLE,
  PALM_DETECTED,
  MOTION_STARTED
};

State currentState = IDLE;

// Variables for timing
unsigned long ultrasonic_start_time = 0;
unsigned long pir_start_time = 0;
unsigned long pir_end_time = 0;

// Detection cooldown variables
bool detectionEnabled = true;
unsigned long detectionDisabledAt = 0;
const unsigned long cooldownPeriod = 5000;  // 20 seconds

// Function to get distance from ultrasonic sensor
float getUltrasonicDistance() {
  // Clear the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Set the trigPin on HIGH state for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH, 30000);  // Timeout after 30ms

  // Calculate the distance in cm
  float distance = (duration * SOUND_SPEED) / 2.0;

  // Optional: Print distance for debugging
  // Serial.print("Distance: ");
  // Serial.print(distance);
  // Serial.println(" cm");

  return distance;
}

void setup() {
  Serial.begin(9600);
  delay(3000);  // Allow time for serial monitor to initialize

  // Initialize sensor pins
  pinMode(PIR_SENSOR_OUTPUT_PIN, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.println("System Ready!");
}

void loop() {
  if (detectionEnabled) {
    float distanceCm = getUltrasonicDistance();

    switch (currentState) {
      case IDLE:
        if (distanceCm < DISTANCE_THRESHOLD) {
          ultrasonic_start_time = millis();
          Serial.println("Palm detected by ultrasonic sensor.");
          currentState = PALM_DETECTED;
        }
        break;

      case PALM_DETECTED:
        if (digitalRead(PIR_SENSOR_OUTPUT_PIN) == HIGH) {
          pir_start_time = millis();
          Serial.println("Motion detected by PIR sensor. Starting timer.");
          currentState = MOTION_STARTED;
        }
        break;

      case MOTION_STARTED:
        if (digitalRead(PIR_SENSOR_OUTPUT_PIN) == LOW) {
          pir_end_time = millis();
          float pir_duration = (pir_end_time - pir_start_time) / 1000.0;  // Duration in seconds
          float motion_start = (pir_start_time - ultrasonic_start_time);
          Serial.print("Motion Detection Start Time: ");
          Serial.print(motion_start);
          Serial.print(" ms, PIR Motion Duration: ");
          Serial.print(pir_duration);
          Serial.println(" seconds");

          // Disable detection for cooldown period
          detectionEnabled = false;
          detectionDisabledAt = millis();
          Serial.println("Detection disabled for 5 seconds.");
          
          // Reset state
          currentState = IDLE;
        }
        break;
    }
  } else {
    // Check if cooldown period has passed
    if (millis() - detectionDisabledAt >= cooldownPeriod) {
      detectionEnabled = true;
      Serial.println("Detection re-enabled.");
    }
  }

  delay(100);  // Short delay to prevent rapid looping
}
 */