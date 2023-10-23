#include "WiFi.h"
#include <WiFiClient.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

Servo servoMotor1;
Servo servoMotor2;

int servo1Pin = 18; // Example, need to redefine
int servo2Pin = 19;

LiquidCrystal_I2C lcd(0x27, 16, 2);
// Use only  core 1 for demo purpose
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 0;
#endif

#define IR_Sensor 14                    /*D14 IR pin defined*/
#define BUTTON_STATE1  
#define BUTTON_STATE2  
#define BUTTON_STATE_ALL_OPEN
#define BUTTON_STATE_ALL_CLOSE

WiFiClient client;

const char *ssid = "NAME";
const char *password = "PASSWORD";
const char *host = "192.168.1.20";
const int port = 5000;

unsigned int IR = 0;
unsigned long Count = 0;

unsigned char Gate1 = 0;
unsigned char Gate2 = 0;

String messageSending = "";           // create String instant that contains the object type String
String messageReceiving = ""; 

// *************************************************  SUB TASKs  ****************************************************

void displayTask(void *parameter)
{
  while (1)
  {
    lcd.setCursor(0, 0);
    lcd.printstr("Count:");
    lcd.write(Count);

    lcd.setCursor(0, 1);    
    lcd.printstr("G 1:");
    if (Gate1 == 1)
      lcd.printstr("ON");      
    else
      lcd.printstr("OFF");

    lcd.setCursor(8, 1);    
    lcd.printstr("G 2:");
    if (Gate2 == 1)
      lcd.printstr("ON");      
    else
      lcd.printstr("OFF");   
    vTaskDelay(1000 / portTICK_PERIOD_MS);  
    lcd.clear();       
  }
}
// *****************************************************************************************************************

void isrState1() {
  Gate1 = (Gate1+1) % 2;
}

void isrState2() {
  Gate2 = (Gate2+1) % 2;
}

void isrStateAllOpen() {
  Gate1 = 0;
  Gate1 = 0;
}

void isrStateAllClose() {
  Gate1 = 1;
  Gate1 = 1;  
}

void resetValues()
{
  Count = 0;
  IR = 0;
  Gate1 = 0;
  Gate2 = 0;
}

// ***********************************  FUNCTION HANDLES CONNECTION TCP/ IP ****************************************
void receiveFromServer()
{
  if (client.available())
  {
    while (client.available())
    {
      char c = (char)client.read();
      messageReceiving += c;
      delay(1);
    }
    if (messageReceiving.length() > 0)
    {
      Serial.println(messageReceiving);
      messageReceiving = "";
    }
  }
}

void sendToServer()
{
  // CHANGE CODE BELOW TO FIT THE PROJECT!
  while (Serial.available())
  {
    char c = (char)Serial.read();
    messageSending += c;
    if (c == '\n')
    {
      client.write(messageSending.c_str());
      messageSending = "";
    }
  }
  // -------------------------------------
}

// **************************************************************************************************************

// ************************************************  SET UP  *******************************************************
void setup() 
{
  Serial.begin(115200); // baudrate 115200 bit/sec
  
  pinMode(IR_Sensor, INPUT);   /*IR Pin D14 defined as Input*/

  servo1.attach(servo1Pin);
  servo1.attach(servo2Pin);

  pinMode(BUTTON_STATE1, INPUT_PULLUP); // for gate 1  
  pinMode(BUTTON_STATE2, INPUT_PULLUP);  // for gate 2
  pinMode(BUTTON_STATE_ALL_OPEN, INPUT_PULLUP);  // for all gate
  pinMode(BUTTON_STATE_ALL_CLOSE, INPUT_PULLUP);  // for all gate
  attachInterrupt(BUTTON_DIR, isrState1, FALLING);  
  attachInterrupt(BUTTON_MODE, isrState2, FALLING);  
  attachInterrupt(BUTTON_MODE, isrStateAllOpen, FALLING);  
  attachInterrupt(BUTTON_MODE, isrStateAllClose, FALLING);  

  // For LCD I2C
  lcd.begin();
  lcd.backlight();
  
  Serial.print("Connecting to ");   // Connect to Wi-Fi network with SSID and password  
  Serial.println("ssid");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }  

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("Wifi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  while (!client.connect(host, port))
  {
    delay(500);
  }
  Serial.print("Connected to Server: ");
  Serial.println(host);
  
  // Task to run forever
  xTaskCreatePinnedToCore(  //
                          displayTask,                // Function to be called
                          "display Task",             // Name of task
                          1024,                   // Stack size (bytes in ESP32, word in FreeRTOS)
                          NULL,                   // parameter to pass function
                          1,                      // Task priority ( 0 to configMAX_PRIORITIES - 1)
                          NULL,                   // Task handle
                          app_cpu);               // Run on one core for demo purposes (ESP32 only)
}

// ***********************************  MAIN LOOP ****************************************
void loop() {
  // put your main code here, to run repeatedly:
  IR=digitalRead(IR_Sensor);  /*digital read function to check IR pin status*/
  if(IR==LOW)
  {               /*If sensor detect any reflected ray*/
    count++;     
  }
  if (Gate1 == 1)
  {
    servoMotor1.write(180);
  }
  else
  {
    servoMotor1.write(0);
  }
  if (Gate2 == 1)
  {
    servoMotor2.write(180);
  }
  else
  {
    servoMotor2.write(0);
  }
  // CHANGE CODE HERE
  receiveFromServer();
}
