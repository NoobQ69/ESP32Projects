#include <BluetoothSerial.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVOMIN  80 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define MAX_POSITION_CAPACITY 10 // Analog servos run at ~50 Hz updates

BluetoothSerial SerialBT;

enum {
  SERVO1,
  SERVO2,
  SERVO3,
  SERVO4,
  SERVO5,
  SERVO6
};

typedef struct {
  String command;
  String value;
} DATA_PACKAGE;

DATA_PACKAGE DataPackage = {"", ""};

unsigned char DegreePositionBefore[6] = {0, 120, 144, 100, 31, 110};
unsigned char DegreePositionAfter[MAX_POSITION_CAPACITY][6] = {{120, 120, 160, 20, 20, 180}};
char ServoOrder[MAX_POSITION_CAPACITY][6] = { {SERVO1, SERVO2, SERVO3, SERVO4, SERVO5, SERVO6} };
unsigned char IndexSaved = 2;

String Dataln = "";

int SpeedDelay = 20;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("A client is connected to ESP32");
  }
}

void setup() {
  Serial.begin(9600);

  SerialBT.register_callback(callback);
  if(!SerialBT.begin("ESP32-Bluetooth"))
  {
    Serial.println("An error occurred initializing Bluetooth");
  }
  else{
    Serial.println("Bluetooth initialized");
  }
  pwm.begin();
  
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  setDegreeServo(SERVO1, DegreePositionBefore[0]); delay(20);
  setDegreeServo(SERVO2, DegreePositionBefore[1]); delay(20);
  setDegreeServo(SERVO3, DegreePositionBefore[2]); delay(20);
  setDegreeServo(SERVO4, DegreePositionBefore[3]); delay(20);
  setDegreeServo(SERVO5, DegreePositionBefore[4]); delay(20);
  setDegreeServo(SERVO6, DegreePositionBefore[5]); delay(20);
}

void setDegreeServo(int servo,int positionDegree)
{
  int pulse = map(positionDegree, 0, 180, SERVOMIN, SERVOMAX);

  pwm.setPWM(servo, 0, pulse);
}

void runOneServo(int servo, int beforeDegree, int afterDegree)
{
  while (beforeDegree != afterDegree)
  {
    setDegreeServo(servo, beforeDegree);
    if (beforeDegree > afterDegree)
    {
      beforeDegree--;
    }
    else
    {
      beforeDegree++;
    }
    delay(SpeedDelay);
  }
}

void runServos(DATA_PACKAGE *dataPackage, unsigned char current[], unsigned char nextPosition[][6], char order[][6])
{
  while(true)
  {
    for (int i = 0; i <= IndexSaved; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        runOneServo(order[i][j], current[order[i][j]], nextPosition[i][order[i][j]]);
        current[order[i][j]] = nextPosition[i][order[i][j]];
        
        if (SerialBT.available() > 0)
        {
          handleDataFromBluetooth(dataPackage);
          String temp = dataPackage->command;

          if (temp.startsWith("Pause"))
          {
            return;
          }
          else if (temp.startsWith("V"))
          {
            SpeedDelay = 110 - dataPackage->value.toInt();
          }
        }
      } // end loop 2
    } // end loop 1
  } // end infinity loop
}

int savePositionServo(DATA_PACKAGE *dataPackage, unsigned char current[], unsigned char nextPosition[][6], char order[][6])
{
  if (IndexSaved < MAX_POSITION_CAPACITY)
  {
    for (int i = 0; i < 6; i++)
    {
      nextPosition[IndexSaved][i] = current[i];
    }
    order[IndexSaved][0] = dataPackage->value.substring(1,2).toInt()-1;
    order[IndexSaved][1] = dataPackage->value.substring(3,4).toInt()-1;
    order[IndexSaved][2] = dataPackage->value.substring(5,6).toInt()-1;
    order[IndexSaved][3] = dataPackage->value.substring(7,8).toInt()-1;
    order[IndexSaved][4] = dataPackage->value.substring(9,10).toInt()-1;
    order[IndexSaved][5] = dataPackage->value.substring(11,12).toInt()-1;
    IndexSaved++;

    // for (int i = 0; i < 6; i++)
    // {
    //   Serial.println((int)order[IndexSaved-1][i]);
    // }
    return 0;
  }
  return -1;
}

void resetPositionServo()
{
  IndexSaved = 0;
}

String getLatestBluetoothValue(String dataln)
{
  int i = dataln.length()-1;

  while (true)
  {
    if (!(dataln[i] >= '0' && dataln[i] <= '9')) break;
    i--;
  }
  return dataln.substring(i, dataln.length());
}

void handleDataFromBluetooth(DATA_PACKAGE *dataPackage)
{
  String dataln = SerialBT.readString();
  
  if (dataln.startsWith("Run"))
  {
    dataPackage->command = "Run";
  }
  else if (dataln.startsWith("Pause"))
  {
    dataPackage->command = "Pause";
  }
  else if (dataln.startsWith("Reset"))
  {
    dataPackage->command = "Reset";
  }
  else if (dataln.startsWith("SP"))
  {
    dataPackage->command = "SP"; // Set position
    dataPackage->value = dataln.substring(2, dataln.length());
  }
  else if (dataln.startsWith("V")) // Velocity
  {
    String temp = getLatestBluetoothValue(dataln);

    dataPackage->command = "V";
    dataPackage->value = dataln.substring(1, dataln.length());
  }
  else
  {
    String temp = getLatestBluetoothValue(dataln);

    dataPackage->command = temp.substring(0, 2);
    dataPackage->value = temp.substring(2, temp.length());
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (SerialBT.available() > 0)
  {
    handleDataFromBluetooth(&DataPackage);

    if (DataPackage.command.startsWith("Run"))
    {
      runServos(&DataPackage, DegreePositionBefore, DegreePositionAfter, ServoOrder);
    }
    else if (DataPackage.command.startsWith("Reset"))
    {
      Serial.println("Get reset");
      resetPositionServo();
    }
    else if (DataPackage.command.startsWith("SP"))
    {
      Serial.println("Get save");
      savePositionServo(&DataPackage, DegreePositionBefore, DegreePositionAfter, ServoOrder);
    }
    else if (DataPackage.command.startsWith("V"))
    {
      Serial.println("Get speed");
      SpeedDelay = 110 - DataPackage.value.toInt();
      Serial.println(SpeedDelay);
    }
    else
    {
      Serial.println("Get servo");
      
      int currentDegree = DataPackage.value.toInt();
      
      if (DataPackage.command.startsWith("S1"))
      {
        runOneServo(SERVO1, DegreePositionBefore[SERVO1], currentDegree);
        DegreePositionBefore[SERVO1] = currentDegree;
      }
      else if (DataPackage.command.startsWith("S2"))
      {
        runOneServo(SERVO2, DegreePositionBefore[SERVO2], currentDegree);
        DegreePositionBefore[SERVO2] = currentDegree;
      }
      else if (DataPackage.command.startsWith("S3"))
      {
        runOneServo(SERVO3,DegreePositionBefore[SERVO3], currentDegree);
        DegreePositionBefore[SERVO3] = currentDegree;
      }
      else if (DataPackage.command.startsWith("S4"))
      {
        runOneServo(SERVO4, DegreePositionBefore[SERVO4], currentDegree);
        DegreePositionBefore[SERVO4] = currentDegree;
      }
      else if (DataPackage.command.startsWith("S5"))
      {
        runOneServo(SERVO5, DegreePositionBefore[SERVO5], currentDegree);
        DegreePositionBefore[SERVO5] = currentDegree;
      }
      else if (DataPackage.command.startsWith("S6"))
      {
        runOneServo(SERVO6, DegreePositionBefore[SERVO6], currentDegree);
        DegreePositionBefore[SERVO6] = currentDegree;
      }
    } // end else
  }
  delay(10);
}
