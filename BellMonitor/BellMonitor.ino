#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>

#define RELAY1_PIN 13
#define RELAY2_PIN 14
#define RELAY3_PIN 15
#define RELAY4_PIN 16

#define BUTTON1 33
#define BUTTON2 25
#define BUTTON3 26
#define BUTTON4 27
#define BUTTON_ALL 32

String ClientRequest, myresultat, DataBuffer;
WiFiServer server(80);
WiFiClient client;

LiquidCrystal_I2C lcd(0x27, 16, 2);

String ReadIncomingRequest()
{
  while(client.available())
  {
    ClientRequest = (client.readStringUntil('\r'));
    
    if ((ClientRequest.indexOf("HTTP/1.1")>0)&&(ClientRequest.indexOf("/favicon.ico")<0))
    {
      myresultat = ClientRequest;
    }
  }
  return myresultat;
}



void serialFlush()
{
  while(Serial.available() > 0)
  {
    char t = Serial.read();
  }
}

void ringBell(unsigned char relay, char * msg)
{
    Serial.println(msg);
    lcd.setCursor(0,1);
    lcd.print(msg);
    digitalWrite(relay, HIGH);
    delay(1000);
    digitalWrite(relay, LOW);
    lcd.setCursor(0,1);
    lcd.print("               ");
}

void readButtons()
{
  if (digitalRead(BUTTON1) == LOW)
  {
    ringBell(RELAY1_PIN, "Chuong 4 bat");
  }
  else if (digitalRead(BUTTON2) == LOW)
  {
    ringBell(RELAY2_PIN, "Chuong 3 bat");
  }
  else if (digitalRead(BUTTON3) == LOW)
  {
    ringBell(RELAY3_PIN, "Chuong 2 bat");
  }
  else if (digitalRead(BUTTON4) == LOW)
  {
    ringBell(RELAY4_PIN, "Chuong 1 bat");
  }
  else if (digitalRead(BUTTON_ALL) == LOW)
  {
    lcd.setCursor(0,1);
    lcd.print("All chuong bat");
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
    digitalWrite(RELAY3_PIN, HIGH);
    digitalWrite(RELAY4_PIN, HIGH);
    while (digitalRead(BUTTON_ALL) == LOW)
    {
      delay(80);
    }
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY3_PIN, LOW);
    digitalWrite(RELAY4_PIN, LOW);
    lcd.setCursor(0,1);
    lcd.print("                 ");
  }
  delay(20);
}

void setup()
{
  ClientRequest = "";

  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();

  Serial.begin(9600);

  WiFi.disconnect();
  delay(3000);
  Serial.println("START");
  WiFi.begin("B8-06","DHCT9760");
  while ((!(WiFi.status() == WL_CONNECTED))){
    delay(300);
    Serial.print("..");
  }
  Serial.println("Connected");
  Serial.println("Your IP is");
  Serial.println((WiFi.localIP()));

  server.begin();
    // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print message
  lcd.print("IP:");
  lcd.print(WiFi.localIP());
  delay(1000);
  // clears the display to print new message
  //lcd.clear();

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
  pinMode(BUTTON4, INPUT_PULLUP);
  pinMode(BUTTON_ALL, INPUT_PULLUP);
}

void loop()
{
    readButtons();
    client = server.available();
    if (!client) { return; }
    while(!client.available()) {  delay(1); }
    ClientRequest = (ReadIncomingRequest());
    Serial.println(ClientRequest);
    ClientRequest.remove(0, 5);
    ClientRequest.remove(ClientRequest.length()-9,9);

    Serial.println("Get:");
    Serial.println(ClientRequest);
    if (ClientRequest == "F1ON")
    {
      ringBell(RELAY1_PIN, "Chuong 4 bat");
    }
    else if (ClientRequest == "F2ON")
    {
      ringBell(RELAY2_PIN, "Chuong 3 bat");
    }
    else if (ClientRequest == "F3ON")
    {
      ringBell(RELAY3_PIN, "Chuong 2 bat");
    }
    else if (ClientRequest == "F4ON")
    {
      ringBell(RELAY4_PIN, "Chuong 1 bat");
    }
    else if (ClientRequest == "ALLON")
    {
      lcd.setCursor(0,1);
      lcd.print("Tat ca chuong on");
      digitalWrite(RELAY1_PIN, HIGH);
      digitalWrite(RELAY2_PIN, HIGH);
      digitalWrite(RELAY3_PIN, HIGH);
      digitalWrite(RELAY4_PIN, HIGH);
    }
    else if (ClientRequest == "ALLOFF")
    {
      digitalWrite(RELAY1_PIN, LOW);
      digitalWrite(RELAY2_PIN, LOW);
      digitalWrite(RELAY3_PIN, LOW);
      digitalWrite(RELAY4_PIN, LOW);
      lcd.setCursor(0,1);
      lcd.print("                 ");
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("");
    client.print(ClientRequest);
    client.stop();
    delay(1);
    client.flush();
}

