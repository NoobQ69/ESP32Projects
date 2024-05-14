
enum notify_type_msg 
{
  ERROR_TYPE_MSG,
  NOTIFY_TYPE_MSG
};

void update_tank_menu(int no_tank)
{
  display.clearDisplay();
  oled128x32_drawBitmap(bitmap_border_rec, 0, 21, 128, 20, false);
  oled128x32_drawBitmap(bitmap_border_rec, 0, 42, 128, 20, false);

  oled128x32_printText("n.o tank:", 15, 5, 1);
  oled128x32_printText(String(no_tank).c_str(), 90, 5, 2);
  
  for (int i = 0; i < 4; i++)
  {
    if (register_tank[i+1])
      oled128x32_printText(String(i+1).c_str(), 20+(i*26), 26, 1);
  }
  for (int i = 0; i < 4; i++)
  {
    if (register_tank[i+5])
      oled128x32_printText(String(i+5).c_str(), 20+(i*26), 47, 1);
  }
  // oled128x32_printText("Set time", 5, 26, 1);
  // oled128x32_printText("Info", 5, 47, 1);
  display.clearDisplay();
}

void print_notify_message(const char*msg, int type)
{
  if (type == ERROR_TYPE_MSG)
  {
    oled128x32_printText("ERROR", 10, 10, 2);
    oled128x32_printText(msg, 5, 40, 1);
  }
  if (type == NOTIFY_TYPE_MSG)
  {
    oled128x32_printText("NOTE", 10, 10, 2);
    oled128x32_printText(msg, 5, 40, 1);
  }
}
