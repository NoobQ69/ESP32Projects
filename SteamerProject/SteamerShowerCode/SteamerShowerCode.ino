// thư viện MODBUS 
#include <ModbusMaster.h>

uint16_t data[6];

#define MODBUS_BAUD_RATE 19200

#if MODBUS_BAUD_RATE > 19200
        uint32_t t3_5 = 1750;
#else
        uint32_t t3_5 = (1000000 * 39) / MODBUS_BAUD_RATE + 500; // +500us : to be safe
#endif

#define MAX485_DE_RE      3
// instantiate ModbusMaster object
ModbusMaster node;
bool state = true;

void preTransmission()
{
  delayMicroseconds(t3_5);

  digitalWrite(MAX485_DE_RE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_DE_RE, 0);
}

/// ------------ KHAI BÁO CHÂN CHO KHỐI LED 7 -------------
// Chân hiển thị giá trị led nhiệt độ và thời gian
#define SCLK 4
#define RCLK 3
#define DIO 2

// Chân điều khiển led thời gian (phút)
#define TIME1 5 
#define TIME2 6
// Chân điều khiển led nhiệt độ (độ C)
#define TEMP1 7
#define TEMP2 8
#define TEMP3 9
// thời gian 1 led sáng (ms)
#define LED7_TIME_ON 1

unsigned char Code7SegSource[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77/*A*/, 0x7C/*B*/, 0x39/*C*/, 0x5E/*D*/, 0x79/*E*/, 0x71/*F*/, 0x00/* */};

/// ------------ KHAI BÁO CHÂN CHO KHỐI LED ĐƠN -------------
// Chân hiển thị led đơn sử dụng 74hc595
#define SCLK2 12
#define RCLK2 11
#define DIO2 13
/* 
 TIME led   <-- QB -[``\_/``]- VCC
 OFF led    <-- QC -[       ]- QA --> TEMP led
 START led  <-- QD -[       ]- DIO
 STOP led   <-- QE -[  74HC ]- OE --> GND
 ON_OFF led <-- QF -[  595  ]- RCLK
                   -[       ]- SCLK
                   -[       ]- RESET --> 5v
               GND -[_______]-
    
*/
#define TEMP_LED                  0x01  //00000001b
#define TIME_LED                  0x02  //00000010b
#define OFF_LED                   0x04  //00000100b
#define RESET_MEMORY_SETTING_LED  0xF8  //11111000b
//____________________________________  // logic tương tự như trên 
#define START_LED                 0x08
#define STOP_LED                  0x10
#define RESET_STEAM_LED           0xE7
//____________________________________
#define ON_OFF_LED                0x20

/// ------------ KHAI BÁO CHÂN CHO KHỐI NÚT NHẤN -------------
#define ROW1_PIN                  A0    
#define ROW2_PIN                  A1    
#define ROW3_PIN                  A2    
#define ROW4_PIN                  A3    

#define COL1_PIN                  A4    
#define COL2_PIN                  A5    

/// ------------ KHAI BÁO CHÂN CHO KHỐI NÚT NHẤN -------------
#define BUZZER_PIN 10

unsigned char Time = 10, ModeLED = OFF_LED+STOP_LED+ON_OFF_LED;
unsigned char MonitorMode = 1;      // 1: Mode ON, 0: Mode OFF
unsigned char MemorSettingMode = 0; // 0: Mode TIME, 1: Mode TIME, 2: Mode Temp
unsigned char SteamMode = 0; // 0: Mode Stop, 1: Mode Start
unsigned int Temperature = 100;
unsigned char KeyBuffer[3] = {0, 0, 0}; // 0: Mode Stop, 1: Mode Start

unsigned int PreviousTime = 0, CurrentTime, TimeInterval = 10; // 30ms

// Khai biến xử lí khi nhấn giữ nút tằng giảm
unsigned char EnableKeyRepeat = 0; // 0: Mode Stop, >= 1: Mode Start
unsigned int RepeatTime = 0;


void setup() {
  // put your setup code here, to run once:
  pinMode(MAX485_DE_RE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_DE_RE, 0);
  Serial.begin(MODBUS_BAUD_RATE, SERIAL_8E1);

  // communicate with Modbus slave ID 1 over Serial (port 0)
  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  pinMode(SCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(DIO, OUTPUT);
  pinMode(TIME1, OUTPUT);
  pinMode(TIME2, OUTPUT);
  pinMode(TEMP1, OUTPUT);
  pinMode(TEMP2, OUTPUT);
  pinMode(TEMP3, OUTPUT);

  pinMode(SCLK2, OUTPUT);
  pinMode(RCLK2, OUTPUT);
  pinMode(DIO2, OUTPUT);
  pinMode(ROW1_PIN, INPUT_PULLUP);
  pinMode(ROW2_PIN, INPUT_PULLUP);
  pinMode(ROW3_PIN, INPUT_PULLUP);
  pinMode(ROW4_PIN, INPUT_PULLUP);
  pinMode(COL1_PIN, OUTPUT);
  pinMode(COL2_PIN, OUTPUT);

  digitalWrite(TIME1, HIGH);
  digitalWrite(TIME2, HIGH);
  digitalWrite(TEMP1, HIGH);
  digitalWrite(TEMP2, HIGH);
  digitalWrite(TEMP3, HIGH);

  PrintModeLED(ON_OFF_LED);
  node.setTransmitBuffer(0, Temperature);
  node.setTransmitBuffer(1, Time);

  node.writeSingleCoil(0x0A, 1);
  node.writeSingleCoil(0x00, 1);
  node.writeSingleCoil(0x01, 1);
  node.writeSingleCoil(0x02, 1);


  node.writeMultipleRegisters(200, 2);
  node.writeSingleCoil(0x0A, 0);
}

// ------------------------- PHẦN HIỂN THỊ LED 7 ĐOẠN -----------------------------
void PrintLED7(unsigned char number, unsigned char pin)
{
  shiftOut(DIO, SCLK, MSBFIRST, Code7SegSource[number]);
  digitalWrite(RCLK,HIGH);
  delay(2);
  digitalWrite(RCLK,LOW);

  digitalWrite(pin, HIGH);
  delay(LED7_TIME_ON);
  digitalWrite(pin, LOW);
}

void PrintTemperature()
{
  if (MonitorMode == 1)
  {
    PrintLED7(Temperature%1000/100, TEMP1);
    PrintLED7(Temperature%100/10, TEMP2);
    PrintLED7(Temperature%10, TEMP3);
    return;
  }
  PrintLED7(16, TEMP1);
  PrintLED7(16, TEMP2);
  PrintLED7(16, TEMP3);
}

void PrintTime()
{
  if (MonitorMode == 1)
  {
    PrintLED7(Time%100/10, TIME1);
    PrintLED7(Time%10, TIME2);
    return;
  }
  PrintLED7(16, TIME1);
  PrintLED7(16, TIME2);
}

// ------------------------- PHẦN HIỂN THỊ LED ĐƠN ------------------------------

void PrintModeLED(unsigned char led)
{
  if (led <= 0x04) // nếu led nằm trong Memory Setting
  {
    ModeLED &= RESET_MEMORY_SETTING_LED;
  }
  else if (led <= 0x10)
  {
    ModeLED &= RESET_STEAM_LED;
  }
  else
  {
    if (MonitorMode == 0)
    {
      shiftOut(DIO2, SCLK2, MSBFIRST, 0x00);
      digitalWrite(RCLK2,LOW);
      delay(2);
      digitalWrite(RCLK2,HIGH);
      return;
    }
  }

  ModeLED |= led;
  shiftOut(DIO2, SCLK2, MSBFIRST, ModeLED);
  digitalWrite(RCLK2,LOW);
  delay(2);
  digitalWrite(RCLK2,HIGH);
}


// ------------------------- PHẦN ĐỌC NÚT NHẤN ------------------------------

void determineRow(unsigned char *key, unsigned char number)
{
  if (digitalRead(ROW1_PIN) == 0)  
  {
    *key = number+1;
  }
  if (digitalRead(ROW2_PIN) == 0)
  {
   *key = number+2;  
  } 
  if (digitalRead(ROW3_PIN) == 0)
  {
   *key = number+3;
  } 
  if (digitalRead(ROW4_PIN) == 0)
  {
   *key = number+4;
  } 
}
/*
1: Mode ON OFF
2: Memory setting
3: Steam 
4: UP time
5: DOW time
6: UP temp
7: DOW temp
*/
unsigned char readBtns() // Hàm đọc tất cả nút nhấn
{
  unsigned char key = 0;

  digitalWrite(COL1_PIN, LOW);
  determineRow(&key, 0);
  digitalWrite(COL1_PIN, HIGH);

  digitalWrite(COL2_PIN, LOW);
  determineRow(&key, 4);
  digitalWrite(COL2_PIN, HIGH);

  return key;
} 


uint8_t ReadButtons(void){

  unsigned char result = 0; //no key pressed

	unsigned char key = readBtns(); 

	KeyBuffer[0] = KeyBuffer[1]; //Store data

  KeyBuffer[1] = KeyBuffer[2];

	KeyBuffer[2] = key;

  RepeatTime++;
  if (RepeatTime > 30)
  {
    EnableKeyRepeat = KeyBuffer[0];
  }

	if ((KeyBuffer[0] == KeyBuffer[1])&&(KeyBuffer[2] == 0))
	{
    RepeatTime = 0;
    EnableKeyRepeat = 0;
		result = KeyBuffer[0];
	}

	return result;
}

void HandleReadRepeatBtns()
{
  if (RepeatTime > 36)
  {
    RepeatTime = 31;
    if (EnableKeyRepeat == 4){
      Time += 1;
    }
    else if (EnableKeyRepeat == 5){
      if (Time > 0)
        Time -= 1;
    }
    else if (EnableKeyRepeat == 6){
      Temperature += 1;
    }
    else if (EnableKeyRepeat == 7){
      if (Temperature > 0)
        Temperature -= 1;
    }
  }
}



void loop() {
  // put your main code here, to run repeatedly:
  CurrentTime = millis();
  if (CurrentTime - PreviousTime >= TimeInterval) // cứ 30ms thì chương trình sẽ vào lệnh if này
  {
    PreviousTime = CurrentTime;
    unsigned char readButtons = ReadButtons();
    switch(readButtons)                           // Đọc khi giữ nút nhấn tăng giảm
    {
      case 0: // Xử lí khi có nút nhấn giữ (nút tăng giảm)
      {
        if (MemorSettingMode >= 1)
        {
          HandleReadRepeatBtns();
        }
        break;
      }
      case 1:
      {
        //tone(BUZZER_PIN, 1000, 1000);
        MonitorMode = (MonitorMode + 1) % 2;
        if (MonitorMode == 1)
        {
          // Send data to actuator
          node.setTransmitBuffer(0, Temperature);
          node.setTransmitBuffer(1, Time);

          node.writeSingleCoil(0x0A, 1);
          node.writeSingleCoil(0x00, 1);
          node.writeSingleCoil(0x01, 1);
          node.writeSingleCoil(0x02, 1);

          node.writeMultipleRegisters(200, 2);
          node.writeSingleCoil(0x0A, 0);
        }
        PrintModeLED(ON_OFF_LED);
        break;
      }
      case 2:
      {
        // tone(BUZZER_PIN, 1000, 200);
        if (SteamMode == 0)
        {
          MemorSettingMode = (MemorSettingMode + 1) % 3;
          if (MemorSettingMode == 0) PrintModeLED(OFF_LED);
          else if (MemorSettingMode == 1) PrintModeLED(TIME_LED);
          else if (MemorSettingMode == 2) PrintModeLED(TEMP_LED);
        }
        break;
      }
      case 3:
      {
        if (MemorSettingMode == 0)
        {
          // tone(BUZZER_PIN, 1000, 200);
          SteamMode = (SteamMode + 1) % 2;
          if (SteamMode == 1) 
          {
            PrintModeLED(START_LED);
            
            node.setTransmitBuffer(0, Temperature);
            node.setTransmitBuffer(1, Time);

            node.writeSingleCoil(0x0A, 1);
            node.writeSingleCoil(0x00, 1);
            node.writeSingleCoil(0x01, 1);
            node.writeSingleCoil(0x02, 1);

            node.writeMultipleRegisters(200, 2);
            node.writeSingleCoil(0x0A, 0);
          }
          else if (SteamMode == 0) 
          {
            PrintModeLED(STOP_LED);
          }

          // Send data to actuator
          // ....
        }
        break;
      }
      case 4:
      {
        if (MemorSettingMode == 1)
        {
          // tone(BUZZER_PIN, 1000, 200);
          if (Time > 99)
          {
            Time = 0;
          }
          Time += 1;
        }
        break;
      }
      case 5:
      {
        if (MemorSettingMode == 1)
        {
          // tone(BUZZER_PIN, 1000, 200);
          if (Time > 0)
          {
            Time -= 1;
          }
        }
        break;
      }
      case 6:
      {
        if (MemorSettingMode == 2)
        {
          // tone(BUZZER_PIN, 1000, 200);
          Temperature += 1;
        }
        break;
      }
      case 7:
      {
        if (MemorSettingMode == 2)
        {
          // tone(BUZZER_PIN, 1000, 200);
          if (Temperature > 0)
          {
            Temperature -= 1;
          }
        }
        break;
      }
    }

    PrintTemperature();
    PrintTime();
  }
}
