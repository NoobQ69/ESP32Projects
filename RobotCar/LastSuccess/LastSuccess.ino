#include <Wire.h>
#include <BluetoothSerial.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_MPU6050.h>
//control pins for left and right motors
const int leftSpeedPin = 27;  //means pin 9 on the Arduino controls the speed of left motor
const int rightSpeedPin = 14;
const int left1 = 32;  //left 1 and left 2 control the direction of rotation of left motor
const int left2 = 33;
const int right1 = 25;
const int right2 = 26;

const int MPU = 0x68;                                            // MPU6050 I2C address
float AccX, AccY, AccZ;                                          //linear acceleration
unsigned char GyroX, GyroY, GyroZ;                               //angular velocity
sensors_event_t a, g, temp;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;  //used in void loop()
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;
int getSerialValue = 0;

const int maxSpeed = 70;  //max PWM value written to motor speed pin. It is typically 255.
const int minSpeed = 30;  //min PWM value at which motor moves
float angle;              //due to how I orientated my MPU6050 on my car, angle = roll
float targetAngle = 0;
int equilibriumSpeed = 100;  //rough estimate of PWM at the speed pin of the stronger motor, while driving straight
//and weaker motor at maxSpeed
int leftSpeedPinVal;
int rightSpeedVal;
bool isDriving = false;     //if the car driving forward OR rotate/stationary
char MoveWay = 0;           // 0: forward, 1: backward
bool prevIsDriving = true;  //equals isDriving in the previous iteration of void loop()
bool paused = false;        //is the program paused

Servo servoMotor1;
Servo servoMotor2;
int servo1Pin = 15;  // Example, need to redefine
int servo2Pin = 2;
int minUs = 500;
int maxUs = 2500;

const int trigger = 4;
const int echo = 13;
unsigned long timeMeasure;
int distance;
int limitDistance = 20;

char Mode = 0;

BluetoothSerial SerialBT;
Adafruit_MPU6050 mpu;

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("A client is connected to ESP32");
  }
}

void measureDistance() {
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);

  timeMeasure = pulseIn(echo, HIGH);
  distance = timeMeasure / 2.0 / 29.412;
}

// void turnSensorLeft()
// {
//   servoMotor1.write(120);              // tell servo to go to position in variable 'pos'
//   delay(1000);
//   measureDistance();
//   servoMotor1.write(90);              // tell servo to go to position in variable 'pos'
// }

// void turnSensorRight()
// {
//   servoMotor1.write(60);              // tell servo to go to position in variable 'pos'
//   delay(1000);
//   measureDistance();
//   servoMotor1.write(90);              // tell servo to go to position in variable 'pos'
// }

void setup() {
  Serial.begin(9600);
  // Wire.begin();                      // Initialize comunication
  // Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
  // Wire.write(0x6B);                  // Talk to the register 6B
  // Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
  // Wire.endTransmission(true);        //end the transmission
  // Call this function if you need to get the IMU error values for your module
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(100);
  calculateError();
  delay(20);

  ledcSetup(0, 1000, 8);
  ledcSetup(1, 1000, 8);
  ledcAttachPin(leftSpeedPin, 0);
  ledcAttachPin(rightSpeedPin, 1);
  // ESP32PWM::allocateTimer(0);
  // ESP32PWM::allocateTimer(1);
  // ESP32PWM::allocateTimer(2);
  // ESP32PWM::allocateTimer(3);
  // servoMotor1.setPeriodHertz(50);      // Standard 50hz servo
  // servoMotor2.setPeriodHertz(50);      // Standard 50hz servo
  // servoMotor1.attach(servo1Pin, minUs, maxUs);
  // servoMotor2.attach(servo2Pin, minUs, maxUs);
  // servoMotor1.write(90);
  // servoMotor2.write(90);

  pinMode(left1, OUTPUT);
  pinMode(left2, OUTPUT);
  pinMode(right1, OUTPUT);
  pinMode(right2, OUTPUT);

  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
  currentTime = micros();

  SerialBT.register_callback(callback);

  if (!SerialBT.begin("ESP32-Bluetooth")) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized");
  }
}

void loop() {
  // === Read accelerometer (on the MPU6050) data === //
  // readAcceleration();
  mpu.getEvent(&a, &g, &temp);
  g.gyro.x /= SENSORS_DPS_TO_RADS;  // deg/s
  g.gyro.y /= SENSORS_DPS_TO_RADS;
  g.gyro.x /= SENSORS_DPS_TO_RADS;
  a.acceleration.x /= SENSORS_GRAVITY_STANDARD;
  a.acceleration.y /= SENSORS_GRAVITY_STANDARD;
  a.acceleration.z /= SENSORS_GRAVITY_STANDARD;
  // Calculating Roll and Pitch from the accelerometer data
  accAngleX = (atan(a.acceleration.y / sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.z, 2))) * 180 / PI) - AccErrorX;  //AccErrorX is calculated in the calculateError() function
  accAngleY = (atan(-1 * a.acceleration.x / sqrt(pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2))) * 180 / PI) - AccErrorY;

  // === Read gyroscope (on the MPU6050) data === //
  previousTime = currentTime;
  currentTime = micros();
  elapsedTime = (currentTime - previousTime) / 1000000;  // Divide by 1000 to get seconds
  //readGyro();
  // Correct the outputs with the calculated error values
  g.gyro.x -= GyroErrorX;  //GyroErrorX is calculated in the calculateError() function
  g.gyro.y -= GyroErrorY;
  g.gyro.z -= GyroErrorZ;
  // Currently the raw values are in degrees per seconds, deg/s, so we need to multiply by sendonds (s) to get the angle in degrees
  gyroAngleX += g.gyro.x * elapsedTime;  // deg/s * s = deg
  gyroAngleY += g.gyro.y * elapsedTime;
  gyroAngleZ += g.gyro.z * elapsedTime;
  yaw += g.gyro.z * elapsedTime;
  //combine accelerometer- and gyro-estimated angle values. 0.96 and 0.04 values are determined through trial and error by other people
  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
  angle = roll;  //if you mounted MPU6050 in a different orientation to me, angle may not = roll. It can roll, pitch, yaw or minus version of the three
  Serial.println(angle);
  // Serial.println(GyroY);
  // Serial.println(GyroZ);
  // angle = roll;
  //for me, turning right reduces angle. Turning left increases angle.

  //measureDistance();
  // Print the values on the serial monitor
  if (Mode == 0)  // Mode = 0: control by app
  {
    if (SerialBT.available() > 0) {
      char getSerialValue = SerialBT.read();

      if (getSerialValue == 'w')  //drive forward
      {
        Serial.println("forward");
        MoveWay = 0;
        isDriving = true;
      }
      if (getSerialValue == 'b')  //drive backward
      {
        Serial.println("backward");
        MoveWay = 1;
        isDriving = true;
      } else if (getSerialValue == 'a')  //turn left
      {
        Serial.println("left");
        targetAngle += 90;
        if (targetAngle > 180) {
          targetAngle -= 360;
        }
        isDriving = false;
      } else if (getSerialValue == 'd')  //turn right
      {
        Serial.println("right");
        targetAngle -= 90;
        if (targetAngle <= -180) {
          targetAngle += 360;
        }
        isDriving = false;
      } else if (getSerialValue == 'q')  //stop or brake
      {
        Serial.println("stop");
        isDriving = false;
        stopCar();
      } else if (getSerialValue == 'p')  //pause the program
      {
        paused = !paused;
        stopCar();
        isDriving = false;
        Serial.println("key p was pressed, which pauses/unpauses the program");
      } else if (getSerialValue == '1')  //stop or brake
      {
        Serial.println("Auto mode activated");
        isDriving = false;
      }
    }
  } else if (Mode == 1) {
    // if(SerialBT.available() > 0)
    // {
    //   char getSerialValue = SerialBT.read();
    //   if (getSerialValue == '0') //stop or brake
    //   {
    //     Serial.println("Manual mode activates");
    //     isDriving = false;
    //   }
    // }
    // Serial.println(distance);
    // if (distance > limitDistance || distance == 0)
    // {
    //   Mode = 0;
    //   isDriving = true;
    //   Serial.println("Go forward");
    // }
    // else
    // {
    //   unsigned int distanceLeft, distanceRight;

    //   stopCar();
    //   delay(300);
    //   turnSensorLeft();
    //   measureDistance();
    //   Serial.println(distance);
    //   distanceLeft = distance;
    //   turnSensorRight();
    //   measureDistance();
    //   Serial.println(distance);
    //   distanceRight = distance;
    //   if (distanceLeft < 15 && distanceRight < 15)
    //   {
    //     backward();
    //     delay(300);
    //     stopCar();
    //     delay(300);
    //     Serial.println("Go backward");
    //   }
    //   else
    //   {
    //     if (distanceRight >= distanceLeft)
    //     {
    //       backward();delay(300);stopCar();delay(300);
    //       right();
    //       Serial.println("Go right");
    //       delay(400);stopCar();delay(300);
    //     }
    //     if (distanceRight < distanceLeft)
    //     {
    //       backward();delay(300);stopCar();delay(300);
    //       left();
    //       Serial.println("Go left");
    //       delay(400);stopCar();delay(300);
    //     }
    //   }
    // }
  }

  static int count;
  static int countStraight;

  if (count < 6) {
    count++;
  } else  //runs once after void loop() runs 7 times. void loop runs about every 2.8ms, so this else condition runs every 19.6ms or 50 times/second
  {
    count = 0;
    if (!paused) {
      if (isDriving != prevIsDriving) {
        leftSpeedPinVal = equilibriumSpeed;
        countStraight = 0;
        Serial.print("mode changed, isDriving: ");
        Serial.println(isDriving);
      }
      if (isDriving) {
        if (abs(targetAngle - angle) < 3) {
          if (countStraight < 20) {
            countStraight++;
          } else {
            countStraight = 0;
            equilibriumSpeed = leftSpeedPinVal;  //to find equilibrium speed, 20 consecutive readings need to indicate car is going straight
            Serial.print("EQUILIBRIUM reached, equilibriumSpeed: ");
            Serial.println(equilibriumSpeed);
          }
        } else {
          countStraight = 0;
        }
        driving();
      } else {
        rotate();
      }
      prevIsDriving = isDriving;
    }
  }
}

void driving()  //called by void loop(), which isDriving = true
{
  int deltaAngle = round(targetAngle - angle);  //rounding is neccessary, since you never get exact values in reality
  if (MoveWay == 0) {
    forward();
  } else {
    backward();
  }

  if (deltaAngle != 0) {
    controlSpeed();
    rightSpeedVal = maxSpeed;
    ledcWrite(1, rightSpeedVal);    // rightSpeedPin
    ledcWrite(0, leftSpeedPinVal);  // leftSpeedPin
  }
}

void controlSpeed()  //this function is called by driving ()
{
  int deltaAngle = round(targetAngle - angle);
  int targetGyroX;

  //setting up propoertional control, see Step 3 on the website
  if (deltaAngle > 30) {
    targetGyroX = 60;
  } else if (deltaAngle < -30) {
    targetGyroX = -60;
  } else {
    targetGyroX = 2 * deltaAngle;
  }

  if (round(targetGyroX - GyroX) == 0) {
    ;
  } else if (targetGyroX > GyroX) {
    leftSpeedPinVal = changeSpeed(leftSpeedPinVal, -1);  //would increase GyroX
  } else {
    leftSpeedPinVal = changeSpeed(leftSpeedPinVal, +1);
  }
}

void rotate()  //called by void loop(), which isDriving = false
{
  int deltaAngle = round(targetAngle - angle);
  int targetGyroX;

  if (abs(deltaAngle) <= 1) {
    stopCar();
  } else {
    if (angle > targetAngle) {  //turn left
      left();
    } else if (angle < targetAngle) {  //turn right
      right();
    }

    //setting up propoertional control, see Step 3 on the website
    if (abs(deltaAngle) > 30) {
      targetGyroX = 60;
    } else {
      targetGyroX = 2 * abs(deltaAngle);
    }

    if (round(targetGyroX - abs(GyroX)) == 0) {
      ;
    } else if (targetGyroX > abs(GyroX)) {
      leftSpeedPinVal = changeSpeed(leftSpeedPinVal, +1);  //would increase abs(GyroX)
    } else {
      leftSpeedPinVal = changeSpeed(leftSpeedPinVal, -1);
    }
    rightSpeedVal = leftSpeedPinVal;
    ledcWrite(1, rightSpeedVal);    // rightSpeedPin
    ledcWrite(0, leftSpeedPinVal);  // leftSpeedPin
  }
}

int changeSpeed(int motorSpeed, int increment) {
  motorSpeed += increment;

  if (motorSpeed > maxSpeed)  //to prevent motorSpeed from exceeding 255, which is a problem when using analogWrite
  {
    motorSpeed = maxSpeed;
  } else if (motorSpeed < minSpeed) {
    motorSpeed = minSpeed;
  }
  return motorSpeed;
}

void calculateError()  // When this function is called, ensure the car is stationary. See Step 2 for more info
{
  // Read accelerometer values 200 times
  getSerialValue = 0;
  while (getSerialValue < 200) {
    // readAcceleration();
    mpu.getEvent(&a, &g, &temp);
    g.gyro.x /= SENSORS_DPS_TO_RADS;  // deg/s
    g.gyro.y /= SENSORS_DPS_TO_RADS;
    g.gyro.x /= SENSORS_DPS_TO_RADS;
    a.acceleration.x /= SENSORS_GRAVITY_STANDARD;
    a.acceleration.y /= SENSORS_GRAVITY_STANDARD;
    a.acceleration.z /= SENSORS_GRAVITY_STANDARD;


    // Sum all readings
    AccErrorX += (atan((a.acceleration.y) / sqrt(pow((a.acceleration.x), 2) + pow((a.acceleration.z), 2))) * 180 / PI);
    AccErrorY += (atan(-1 * (a.acceleration.x) / sqrt(pow((a.acceleration.y), 2) + pow((a.acceleration.z), 2))) * 180 / PI);

    GyroErrorX += g.gyro.x;
    GyroErrorY += g.gyro.y;
    GyroErrorZ += g.gyro.z;

    getSerialValue++;
  }
  //Divide the sum by 200 to get the error value, since expected value of reading is zero
  AccErrorX = AccErrorX / 200;
  AccErrorY = AccErrorY / 200;

  //Divide the sum by 200 to get the error value
  GyroErrorX = GyroErrorX / 200;
  GyroErrorY = GyroErrorY / 200;
  GyroErrorZ = GyroErrorZ / 200;
  Serial.println("The the gryoscope setting in MPU6050 has been calibrated");
}

void readGyro() {
  Wire.beginTransmission(MPU);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  GyroX = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;
}

void stopCar() {
  digitalWrite(right1, LOW);
  digitalWrite(right2, LOW);
  digitalWrite(left1, LOW);
  digitalWrite(left2, LOW);
  ledcWrite(1, 0);  // rightSpeedPin
  ledcWrite(0, 0);  // leftSpeedPin
}

void forward() {               //drives the car forward, assuming leftSpeedPinVal and rightSpeedVal are set high enough
  digitalWrite(right1, HIGH);  //the right motor rotates FORWARDS when right1 is HIGH and right2 is LOW
  digitalWrite(right2, LOW);
  digitalWrite(left1, HIGH);
  digitalWrite(left2, LOW);
}

void backward() {             //drives the car forward, assuming leftSpeedPinVal and rightSpeedVal are set high enough
  digitalWrite(right1, LOW);  //the right motor rotates FORWARDS when right1 is HIGH and right2 is LOW
  digitalWrite(right2, HIGH);
  digitalWrite(left1, LOW);
  digitalWrite(left2, HIGH);
}

void left() {  //rotates the car left, assuming speed leftSpeedPinVal and rightSpeedVal are set high enough
  digitalWrite(right1, LOW);
  digitalWrite(right2, HIGH);
  digitalWrite(left1, HIGH);
  digitalWrite(left2, LOW);
}

void right() {
  digitalWrite(right1, HIGH);
  digitalWrite(right2, LOW);
  digitalWrite(left1, LOW);
  digitalWrite(left2, HIGH);
}
