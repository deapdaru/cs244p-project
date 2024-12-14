// #include <Arduino.h>
// #include <s3servo.h>

// s3servo myservo;

// const int servoPin = 44;

// void setup() {
//   Serial.begin(9600);
//   myservo.attach(servoPin);
  
//   Serial.println("Servo test starting...");
// }

// void moveServoToAngle(int targetAngle) {
//   // Break down the target angle into steps of 180° or less
//   static int currentAngle = 0;

//   if (targetAngle > currentAngle) {
//     while (currentAngle < targetAngle) {
//       currentAngle = min(currentAngle + 180, targetAngle);
//       Serial.print("Moving to: ");
//       Serial.println(currentAngle);
//       myservo.write(currentAngle);
//       delay(1000); // Wait 1 second for the servo to reach the position
//     }
//   } else if (targetAngle < currentAngle) {
//     while (currentAngle > targetAngle) {
//       currentAngle = max(currentAngle - 180, targetAngle);
//       Serial.print("Moving to: ");
//       Serial.println(currentAngle);
//       myservo.write(currentAngle);
//       delay(1000); // Wait 1 second for the servo to reach the position
//     }
//   }
// }

// void loop() {
//     delay(1000);
//   // Define the angles to test
//   int angles[] = {90, 180, 270, 360, 480};
//   static int currentIndex = 0;

//   if (currentIndex < sizeof(angles) / sizeof(angles[0])) {
//     int targetAngle = angles[currentIndex];
//     Serial.print("Target angle: ");
//     Serial.println(targetAngle);

//     // Move servo to the target angle in staggered steps
//     moveServoToAngle(targetAngle);
    
//     delay(2000); // Pause 2 seconds before the next angle
//     currentIndex++;
//   } else {
//     Serial.println("Test complete.");
//     while (true); // Stop further execution
//   }
// }
