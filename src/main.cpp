#include <Arduino.h>
#include <s3servo.h>
#include <WiFi.h>
#include <HttpClient.h>

const int PIR_SENSOR_OUTPUT_PIN = 18;
const int trigPin = 44;
const int echoPin = 43;

const int servoPin = 21;

#define SOUND_SPEED 0.034
#define DISTANCE_THRESHOLD 15.0

const unsigned long cooldownPeriod = 5000;

const unsigned long extraPaperCheckDelay = 3000;
const unsigned long restartDelay = 5000;        

const int button_satisfactory_pin = 2; 
const int button_less_pin = 3;         
const int button_more_pin = 10;        

const char* ssid = "Joe_Mama";
const char* password = "paanidarang123";
const char server[] = "3.147.237.91"; 
const int port = 5000; 
const char endpoint[] = "/review"; 

enum State {
  IDLE,
  PALM_DETECTED,
  MOTION_STARTED
};

State currentState = IDLE;

unsigned long ultrasonic_start_time = 0;
unsigned long pir_start_time = 0;
unsigned long pir_end_time = 0;
bool detectionEnabled = true;
unsigned long detectionDisabledAt = 0;

s3servo myservo;

float getUltrasonicDistance();
float calculatePaperLength(float pirDuration);
void dispensePaper(float lengthInches);
void dispenseExtraPaperIfNeeded();
String getUserReview();
void sendDataToServer(unsigned long pirTime, const String &review);

void setup() {
  Serial.begin(9600);
  delay(3000);  

  pinMode(PIR_SENSOR_OUTPUT_PIN, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(button_satisfactory_pin, INPUT_PULLUP);
  pinMode(button_less_pin, INPUT_PULLUP);
  pinMode(button_more_pin, INPUT_PULLUP);

  myservo.attach(servoPin);

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
          float pir_duration = (pir_end_time - pir_start_time) / 1000.0;  
          float motion_start = (pir_start_time - ultrasonic_start_time);
          Serial.print("Motion Detection Start Time: ");
          Serial.print(motion_start);
          Serial.print(" ms, PIR Motion Duration: ");
          Serial.print(pir_duration);
          Serial.println(" seconds");

          float paperLengthInches = calculatePaperLength(pir_duration);
          Serial.print("Paper Length Required: ");
          Serial.print(paperLengthInches);
          Serial.println(" inches");

          dispensePaper(paperLengthInches);

          dispenseExtraPaperIfNeeded();

          String userReview = getUserReview();
          sendDataToServer((unsigned long)(pir_duration * 1000), userReview);

          detectionEnabled = false;
          detectionDisabledAt = millis();
          Serial.println("Detection disabled for 5 seconds.");

          currentState = IDLE;

          delay(restartDelay);
        }
        break;
    }
  } else {
    if (millis() - detectionDisabledAt >= cooldownPeriod) {
      detectionEnabled = true;
      Serial.println("Detection re-enabled.");
    }
  }

  delay(100);
}

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

float calculatePaperLength(float pirDuration) {
  return pirDuration;
}

void dispensePaper(float lengthInches) {
  float angle = lengthInches * 100.0;
  
  if (angle > 180.0) {
    angle = 180.0;
  }
  
  Serial.print("Rotating servo to dispense paper: ");
  Serial.print(angle);
  Serial.println(" degrees");
  
  myservo.write((int)angle);
  delay(1000); 
  myservo.write(0);
}

void dispenseExtraPaperIfNeeded() {
  Serial.println("Waiting 3 seconds before checking for additional paper requirement...");
  delay(extraPaperCheckDelay); 
  
  float distanceCm = getUltrasonicDistance();
  if (distanceCm < DISTANCE_THRESHOLD) {
    Serial.println("Palm still detected. Dispensing extra paper slowly...");
    
    for (int angle = 0; angle <= 180; angle += 10) {
      myservo.write(angle);
      delay(500); 
      
      distanceCm = getUltrasonicDistance();
      if (distanceCm >= DISTANCE_THRESHOLD) {
        Serial.println("Palm removed during extra dispensing.");
        break;
      }
    }
    
    myservo.write(0);
    Serial.println("Extra paper dispensing completed.");
  } else {
    Serial.println("No additional paper required.");
  }
}

String getUserReview() {
  Serial.println("Please provide feedback (press one button): ");
  Serial.println("Satisfactory (blue), Less (yellow), More (red)");

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

void sendDataToServer(unsigned long pirTime, const String &review) {
  String json = "{ \"pir_time\": " + String(pirTime) + ", \"user_review\": \"" + review + "\" }";
  Serial.print("JSON to send: ");
  Serial.println(json);

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

  while (http.available()) {
    char c = http.read();
    Serial.print(c);
  }
  Serial.println();
  http.stop();
  Serial.println("Data sent to server.");
}
