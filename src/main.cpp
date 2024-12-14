#include <Arduino.h>
#include <s3servo.h>
#include <WiFi.h>
#include <HttpClient.h>

// ---------------------------------------------
// User Configuration and Constants
// ---------------------------------------------

// Pins for PIR sensor and ultrasonic sensor
const int PIR_SENSOR_OUTPUT_PIN = 18;
const int trigPin = 44;
const int echoPin = 43;

// Servo pin
const int servoPin = 21;

// Distance threshold for detection
#define SOUND_SPEED 0.034
#define DISTANCE_THRESHOLD 15.0

// Cooldown period after one cycle completes (in ms)
const unsigned long cooldownPeriod = 5000;

// Additional timing constants
const unsigned long extraPaperCheckDelay = 3000; // 3 seconds wait before checking if user still needs more paper
const unsigned long restartDelay = 5000;         // 5 seconds after all steps, ready for next user

// Buttons for user review (just pick some pins)
const int button_satisfactory_pin = 2; // "good"
const int button_less_pin = 3;         // "less"
const int button_more_pin = 10;         // "more"

// Wi-Fi and server configuration (dummy placeholders)
const char* ssid = "Joe_Mama";
const char* password = "paanidarang123";
const char server[] = "3.147.237.91"; 
const int port = 5000; 
const char endpoint[] = "/review"; 

// ---------------------------------------------
// State Definitions
// ---------------------------------------------
enum State {
  IDLE,
  PALM_DETECTED,
  MOTION_STARTED
};

State currentState = IDLE;

// ---------------------------------------------
// Global Variables
// ---------------------------------------------
unsigned long ultrasonic_start_time = 0;
unsigned long pir_start_time = 0;
unsigned long pir_end_time = 0;
bool detectionEnabled = true;
unsigned long detectionDisabledAt = 0;

// Servo object
s3servo myservo;

// ---------------------------------------------
// Function Prototypes
// ---------------------------------------------
float getUltrasonicDistance();
float calculatePaperLength(float pirDuration);
void dispensePaper(float lengthInches);
void dispenseExtraPaperIfNeeded();
String getUserReview();
void sendDataToServer(unsigned long pirTime, const String &review);

// ---------------------------------------------
// Setup
// ---------------------------------------------
void setup() {
  Serial.begin(9600);
  delay(3000);  // Allow time for serial monitor to initialize

  pinMode(PIR_SENSOR_OUTPUT_PIN, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Buttons for user feedback
  pinMode(button_satisfactory_pin, INPUT_PULLUP);
  pinMode(button_less_pin, INPUT_PULLUP);
  pinMode(button_more_pin, INPUT_PULLUP);

  // Attach servo
  myservo.attach(servoPin);

  // Connect to Wi-Fi (for sending data)
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("System Ready!");
}

// ---------------------------------------------
// Main Loop
// ---------------------------------------------
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

          // 1. Calculate paper length needed
          float paperLengthInches = calculatePaperLength(pir_duration);
          Serial.print("Paper Length Required: ");
          Serial.print(paperLengthInches);
          Serial.println(" inches");

          // 2. Rotate servo according to required paper length
          dispensePaper(paperLengthInches);

          // 3. Wait 3 seconds and check if palm still under ultrasonic sensor. If yes, dispense more paper slowly
          dispenseExtraPaperIfNeeded();

          // 4. Collect user review and send to server
          String userReview = getUserReview();
          sendDataToServer((unsigned long)(pir_duration * 1000), userReview);

          // Disable detection for cooldown period
          detectionEnabled = false;
          detectionDisabledAt = millis();
          Serial.println("Detection disabled for 5 seconds.");

          // Reset state
          currentState = IDLE;

          // 5. Wait for 5 seconds before next user
          delay(restartDelay);
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

// ---------------------------------------------
// Function Definitions
// ---------------------------------------------

float getUltrasonicDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);  
  float distance = (duration * SOUND_SPEED) / 2.0;
  return distance;
}

// paper length (in inches) = PIR Motion Duration
float calculatePaperLength(float pirDuration) {
  return pirDuration;
}

// Dispense initial paper based on length (inches)
void dispensePaper(float lengthInches) {
  // Calculate servo angle: 1 inch = 100 degrees
  float angle = lengthInches * 100.0;
  
  // Cap the angle at 180 degrees
  if (angle > 180.0) {
    angle = 180.0;
  }
  
  Serial.print("Rotating servo to dispense paper: ");
  Serial.print(angle);
  Serial.println(" degrees");
  
  myservo.write((int)angle);
  delay(1000); // Wait for the servo to reach the position
  myservo.write(0); // Reset servo to initial position
}

// Check after 3 seconds if palm is still present, then dispense extra paper slowly
void dispenseExtraPaperIfNeeded() {
  Serial.println("Waiting 3 seconds before checking for additional paper requirement...");
  delay(extraPaperCheckDelay); // 3 seconds
  
  // Re-check distance
  float distanceCm = getUltrasonicDistance();
  if (distanceCm < DISTANCE_THRESHOLD) {
    Serial.println("Palm still detected. Dispensing extra paper slowly...");
    
    // Rotate servo in 10-degree increments up to 180 degrees
    for (int angle = 0; angle <= 180; angle += 10) {
      myservo.write(angle);
      delay(500); // Adjust delay for slower rotation
      
      // Check if palm is still present
      distanceCm = getUltrasonicDistance();
      if (distanceCm >= DISTANCE_THRESHOLD) {
        Serial.println("Palm removed during extra dispensing.");
        break;
      }
    }
    
    // Reset servo to initial position
    myservo.write(0);
    Serial.println("Extra paper dispensing completed.");
  } else {
    Serial.println("No additional paper required.");
  }
}

// Get user review by checking which button is pressed
String getUserReview() {
  Serial.println("Please provide feedback (press one button): ");
  Serial.println("Satisfactory (blue), Less (yellow), More (red)");

  // Wait until a button is pressed
  while (true) {
    if (digitalRead(button_satisfactory_pin) == LOW) {
      Serial.println("User chose: satisfactory");
      return "satisfactory";
    } else if (digitalRead(button_less_pin) == LOW) {
      Serial.println("User chose: less");
      return "less";
    } else if (digitalRead(button_more_pin) == LOW) {
      Serial.println("User chose: more");
      return "more";
    }
    delay(100);
  }
}

// Send data to server: {pir_time: int, user_review: string}
void sendDataToServer(unsigned long pirTime, const String &review) {
  // Create JSON
  String json = "{ \"pir_time\": " + String(pirTime) + ", \"user_review\": \"" + review + "\" }";
  Serial.print("JSON to send: ");
  Serial.println(json);

  // We assume WiFi is connected, send via HTTP
  WiFiClient client;
  HttpClient http(client);
  http.beginRequest();
  http.post(server, port, endpoint);
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", json.length());
  http.print(json);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  Serial.print("Response Code: ");
  Serial.println(statusCode);

  // Print server response
  while (http.available()) {
    char c = http.read();
    Serial.print(c);
  }
  Serial.println();
  http.stop();
  Serial.println("Data sent to server.");
}
