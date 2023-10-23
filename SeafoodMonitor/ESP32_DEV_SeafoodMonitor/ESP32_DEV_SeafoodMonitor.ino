#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Tạo struct Time để tạo biến lưu thời gian

typedef struct Time
{
  unsigned char seconds;
  unsigned char minutes;
  unsigned char hours;
} TIME;
// Tạo struct ReadButtons để tạo biến lưu giá trị nút nhấn

typedef struct ReadButtons
{
  unsigned char readMode;
  unsigned char readSet;
  bool readIncrease;
  bool readDecrease;
} READBUTTONS;

// Những biến này sẽ sử dụng để định thời chính xác kết hợp hàm millis
unsigned int oneSecond = 1000; // one second = 1000 milli seconds
unsigned int currentTime, lastTime = 0;
unsigned int numberOfRotation = 5;

TIME timePoint = {0, 0, 0};     // biến để đếm thời gian để kéo nhá lên 
TIME timeSet = {0, 1, 0};       // biến set thời gian kéo nhá lên
READBUTTONS readButtons = {0,0,false, false}; // biến lưu giá trị nút nhấn

// định nghĩa chân cho esp-32
#define MODE_BUTTON          18
#define SET_BUTTON           19
#define INCREASE_BUTTON      15
#define DECREASE_BUTTON       4
#define CAPTURE_IMAGE_BUTTON 33
#define DIRECTION_PIN        13
#define STEP_PIN             12
#define ENABLE_PIN           14

void setup() {
  // put your setup code here, to run once:
  lcd.begin();
  lcd.backlight();
  
  Serial.begin(9600);
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(SET_BUTTON, INPUT_PULLUP);
  pinMode(INCREASE_BUTTON, INPUT_PULLUP);
  pinMode(DECREASE_BUTTON, INPUT_PULLUP);
  pinMode(CAPTURE_IMAGE_BUTTON, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  digitalWrite(CAPTURE_IMAGE_BUTTON, HIGH); // chân này dùng để báo cho esp-cam biết khi nào sẽ chụp và gửi ảnh
}

// HÀM RUN STEPPER MOTOR
void RunStepperMotor(int dir, unsigned int speed)
{
  digitalWrite(DIRECTION_PIN, dir);

  for (int i = 0; i < numberOfRotation*200; i++)
  {
    digitalWrite(STEP_PIN, HIGH);
    delay(speed);
    digitalWrite(STEP_PIN, LOW);
    delay(speed);
  }
}

// HÀM ĐẾM THỜI GIAN 
void countTime()
{
  timePoint.seconds++;
  if (timePoint.seconds > 59)
  {
    timePoint.seconds = 0;
    timePoint.minutes++;
    if (timePoint.minutes > 59)
    {
      timePoint.minutes = 0;
      timePoint.hours++;
      if (timePoint.hours > 24)
      {
        timePoint.hours = 0;
      }
    }
  }
}

// HÀM ĐỂ ĐỌC NÚT NHẤN
void readButtonsFunction()
{
  if (digitalRead(MODE_BUTTON) == LOW)
  {
    delay(10);
    readButtons.readMode = (readButtons.readMode + 1) % 3;  // readButtons.readMode nằm trong phạm vi 0, 1, 2 (3 chế độ run, chỉnh thời gian và chỉnh số vòng motor) 
    while (digitalRead(MODE_BUTTON) == LOW) { delay(20); }
  } 
  else if (digitalRead(SET_BUTTON) == LOW)
  {
    delay(10);
    readButtons.readSet = (readButtons.readSet + 1) % 2;   // readButtons.readSet nằm trong phạm vi 0, 1 (tương ứng set giờ, phút)
    while (digitalRead(SET_BUTTON) == LOW) { delay(20); }
  }
  else if (digitalRead(INCREASE_BUTTON) == LOW)           // nếu đọc thấy nút tăng nhấn rồi
  {
    delay(10); // chống dội
    readButtons.readIncrease = true;                      // gán bằng true cho biết nhấn rồi   
    while (digitalRead(INCREASE_BUTTON) == LOW) { delay(20); } // chống lặp
  }
  else if (digitalRead(DECREASE_BUTTON) == LOW)
  {
    delay(10);
    readButtons.readDecrease = true;   
    while (digitalRead(DECREASE_BUTTON) == LOW) { delay(20); }
  }
}

// RESET THỜI GIAN CHẠY 
void resetTimePoint()
{
  timePoint.seconds = 0;
  timePoint.minutes = 0;
  timePoint.hours   = 0;  
}

// HÀM SO SÁNH THỜI GIAN CHẠY VỚI THỜI GIAN SET UP
void compareTime()
{
  if (timePoint.minutes == timeSet.minutes && timePoint.hours == timeSet.hours) // nếu giờ và phút hiện bằng giờ phút thiết lập
  {
    resetTimePoint(); 
    // lcd in ra màn hình
    lcd.clear();        
    lcd.setCursor(0, 0);
    lcd.printstr("Capture ON");
    // code step motor kéo nhá lên
    RunStepperMotor(1, 1000);
    // esp-cam  xuất mức logic low cho esp-cam 500ms
    digitalWrite(CAPTURE_IMAGE_BUTTON, LOW);
    delay(500);
    digitalWrite(CAPTURE_IMAGE_BUTTON, HIGH);    
    RunStepperMotor(0, 1000);
  }
}

// HÀM XỬ LÍ CHẾ ĐỘ HOẠT ĐỘNG
void OperatingMode()
{
  currentTime = millis(); // (xem thêm hàm millis) hàm tương tự systick timer, cứ 1 ms tăng lên 1 đơn vị
  if (currentTime - lastTime > oneSecond) // nếu đêm đủ 1 giây
  {
    lastTime = millis(); 
    countTime();        // set thời gian tăng lên 1 giây cho biến timeSet
    
    // hiển thị ra lcd thời gian mình thiết lập
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printstr("Set ");
    lcd.setCursor(5, 0);
    lcd.printstr("   ");
    lcd.setCursor(5, 0);
    lcd.printstr(String(timePoint.hours).c_str());
    lcd.printstr("h");
    lcd.setCursor(9, 0);
    lcd.printstr("    ");
    lcd.setCursor(9, 0);
    lcd.printstr(String(timePoint.minutes).c_str());
    lcd.printstr("m");
    lcd.setCursor(13, 0);
    lcd.printstr("    ");
    lcd.setCursor(13, 0);
    lcd.printstr(String(timePoint.seconds).c_str());
    lcd.printstr("s");

    // hiển thị ra lcd thời gian đang chạy
    lcd.setCursor(0, 1);
    lcd.printstr("Run ");
    lcd.setCursor(5, 1);
    lcd.printstr("   ");
    lcd.setCursor(5, 1);
    lcd.printstr(String(timeSet.hours).c_str());
    lcd.printstr("h");
    lcd.setCursor(9, 1);
    lcd.printstr("    ");
    lcd.setCursor(9, 1);
    lcd.printstr(String(timeSet.minutes).c_str());
    lcd.printstr("m");
    // cuối cùng so sánh thời gian thiết lập và thời gian chạy
    compareTime();        
  }
}

// HÀM DÙNG ĐỂ ĐIỀU CHỈNH THỜI GIAN GIỜ PHÚT KHI SANG CHẾ ĐỘ CHỈNH GIỜ
void SetTime()
{
  if (readButtons.readSet == 0)
  {
    lcd.setCursor(0, 1);
    lcd.printstr("HOURS  ");
    
    if (readButtons.readIncrease == true)
    {
      readButtons.readIncrease = false; // để ý mấy chỗ gán về false, khi đọc nút nhấn tăng hoặc giảm thì mấy cái biến đã set true rồi, nên nếu
                                        // không gán về false thì cứ mỗi lần nhấn nút set thì thời gian cứ tăng hoặc giảm vì biến cứ luôn được 
                                        // set giá trị true
      timeSet.hours = (timeSet.hours + 1) % 24;            
    }
    else if (readButtons.readDecrease == true)
    {
      readButtons.readDecrease = false;
      timeSet.hours == 0 ? timeSet.hours = 23 : timeSet.hours--; 
    }
  }
  else if(readButtons.readSet == 1)
  {
    lcd.setCursor(0, 1);
    lcd.printstr("MINUTES"); 
    
    if (readButtons.readIncrease == true)
    {
      readButtons.readIncrease = false;
      timeSet.minutes = (timeSet.minutes + 1) % 60;            
    }
    else if (readButtons.readDecrease == true)
    {
      readButtons.readDecrease = false;
      timeSet.minutes == 0 ? timeSet.minutes = 59 : timeSet.minutes--; 
    }
  }
}

// HÀM THIẾT LẬP SỐ VÒNG QUAY CHO MOTOR
void SetMotor()
{
  if (readButtons.readIncrease == true)
  {
    readButtons.readIncrease = false;
    numberOfRotation = numberOfRotation + 1;           
  }
  else if (readButtons.readDecrease == true)
  {
    readButtons.readDecrease = false;
    numberOfRotation == 0 ? numberOfRotation = 0 : numberOfRotation--; 
  }
}

// HÀM THIẾT LẬP MODE
void SetMode()
{
  while (readButtons.readMode != 0) // ở trong while này cho tới khi nào biến readButtons.readMode khác 0 (bằng 1 cho set thời gian, bằng 2 set vòng quay motor)
  {
    readButtonsFunction();          // đọc nút nhấn trước
    
    switch (readButtons.readMode)   // sau đó xem giá trị biến readButtons.readMode 
    {
      case 1: // set thời gian
      {
        lcd.setCursor(8, 1);
        lcd.printstr("   ");
        lcd.setCursor(8, 1);
        lcd.printstr(String(timeSet.hours).c_str());
        lcd.printstr("h");
        lcd.setCursor(11, 1);
        lcd.printstr("    ");
        lcd.setCursor(11, 1);
        lcd.printstr(String(timeSet.minutes).c_str());
        lcd.printstr("m");

        SetTime();            // vô đây set thời gian            
        break;
      }
      case 2:
      {
        lcd.setCursor(0, 1);
        lcd.printstr("                ");
        lcd.setCursor(0, 1);
        lcd.printstr("N.o loop:");
        lcd.setCursor(11, 1);
        lcd.printstr(String(numberOfRotation).c_str());
        
        SetMotor(); // vô đây set số vòng motor
        break;
      }
      default:
      {
        // để trống vì chưa nghĩ ra code handle lỗi:D
        break;
      }
    }    
  }
}

void loop() 
{
  // put your main code here, to run repeatedly:
  readButtonsFunction(); // Đầu tiên đọc nút nhấn
  
  if (readButtons.readMode != 0) // nếu biến readButtons.readMode bằng 1 
  {
    // in lcd mode 1
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printstr("Set Mode"); 
    SetMode(); // vào set mode (set mode gồm 2 phần : chỉnh time và chỉnh số vòng quay)
    resetTimePoint();
  }
  else // không thì vào mode 0
  {
    OperatingMode();
  }
  delay(10);
}
