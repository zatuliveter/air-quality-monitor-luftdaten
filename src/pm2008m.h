//cubic_pm2008m-m-ds.h

#include <SoftwareSerial.h>

struct PM2008MData
{
    int PM1, PM25, PM10;
    bool ChecksummError;
};

class PM2008M 
{
public:
    void init(int rx, int tx);
    PM2008MData startAndRead();
    
private:
    SoftwareSerial* _mySerial;    
    unsigned char _receiveBuff[56]; // data buffer

    void sendReadCMD();
    unsigned char checksumCal();
    unsigned long readDataCell(int index);
};
