#include "pm2008m.h"
#include <SoftwareSerial.h>

void PM2008M::init(int rx, int tx) 
{  
  pinMode(rx, INPUT);
  pinMode(tx, OUTPUT);
  _mySerial = new SoftwareSerial(rx, tx);
  _mySerial->begin(9600);
  while (!&_mySerial)
  {
    Serial.println("Connecting to PM2008M...");
    delay(10);
  }  
}

void PM2008M::sendReadCMD()
{
  Serial.println("sendReadCMD");
  unsigned char sendData[5] = {0x11, 0x02, 0x0B, 0x07, 0xDB}; //read data command
  unsigned char i;
  for(i=0; i<5; i++)
  {
    _mySerial->write(sendData[i]);
    delay(1);      // Don't delete this line !!
  }
  Serial.println("sendReadCMD - Done.");
}

unsigned char PM2008M::checksumCal()                          // CHECKSUM 
{
  unsigned char count, SUM=0;
  for(count=0; count<55; count++)
  {
     SUM += _receiveBuff[count];
  }
  return 256-SUM;
}

unsigned long PM2008M::readDataCell(int index)
{
  return (unsigned long)_receiveBuff[index + 0] << 24 
       | (unsigned long)_receiveBuff[index + 1] << 16 
       | (unsigned long)_receiveBuff[index + 2] << 8
       | (unsigned long)_receiveBuff[index + 3]; 
}
        
PM2008MData PM2008M::startAndRead()
{
  sendReadCMD();  // Send Read Command
  
  Serial.println("startAndRead - reading data");
  unsigned char recv_cnt = 0;
  while(1)
  {
    if(_mySerial->available())
    { 
       _receiveBuff[recv_cnt++] = _mySerial->read();
       if(recv_cnt ==56) { 
        recv_cnt = 0; 
        break; 
       }
    }    
  }
  Serial.println("startAndRead - data was read.");
  
  PM2008MData data;    
  if(checksumCal() == _receiveBuff[55])  
  {
    data.PM1  = readDataCell(3);
    data.PM25 = readDataCell(7);
    data.PM10 = readDataCell(11);
    data.ChecksummError = false;  
  }
  else
  {    
    data.ChecksummError = true;
  }

  return data;
}
 