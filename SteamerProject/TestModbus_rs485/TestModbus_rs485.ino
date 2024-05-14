/*
  Library:: ModbusMaster
  Author:: Doc Walker <4-20ma@wvfans.net>

  Copyright:: 2009-2016 Doc Walker

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

*/

#include <ModbusMaster.h>
#include <SoftwareSerial.h>

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
SoftwareSerial MySerial(10, 11); // rx, tx

void setup()
{
  pinMode(MAX485_DE_RE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_DE_RE, 0);
  pinMode(7,  OUTPUT);
  // use Serial (port 0); initialize Modbus communication baud rate
  Serial.begin(MODBUS_BAUD_RATE, SERIAL_8E1);
  MySerial.begin(9600);

  // communicate with Modbus slave ID 2 over Serial (port 0)
  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

void loop()
{
  static uint16_t i = 100;
  int j, result;
  uint16_t data[6];
  
  i++;
  
  // set word 0 of TX buffer to least-significant word of counter (bits 15..0)
  node.setTransmitBuffer(0, i);
  node.setTransmitBuffer(1, i+1);
  
  // set word 1 of TX buffer to most-significant word of counter (bits 31..16)
  node.writeSingleCoil(0x00, state);
  node.writeSingleCoil(0x01, state);
  node.writeSingleCoil(0x02, state);
  state = !state;
  // slave: write TX buffer to 1 16-bit registers starting at register 0
  result = node.writeMultipleRegisters(200, 2);
  MySerial.print("Result 1:");
  MySerial.println(result);
  MySerial.println();
  // slave: read 1 16-bit registers starting at register 100 to RX buffer
  result = node.readHoldingRegisters(200, 1);
  delay(2000);
  // do something with data if read is successful
  if (result == node.ku8MBSuccess)
  {
    MySerial.println("Success");
    digitalWrite(7, HIGH);
    delay(1000);
    digitalWrite(7, LOW);

    data[0] = node.getResponseBuffer(0);
  }

  MySerial.print("Result:");
  MySerial.println(result);
  MySerial.print("Value:");
  MySerial.println(data[0]);
  delay(1000);
}
