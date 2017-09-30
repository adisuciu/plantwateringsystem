#include <stdlib.h>

#ifdef DEBUG
  #define debug(x) debugFkt(x)
#else
  #define debug(x)
#endif

#define DEBUG_MILLISECONDS

unsigned long debug_day = 86400000; // 86400000 milliseconds in a day
unsigned long debug_hour = 3600000; // 3600000 milliseconds in an hour
unsigned long debug_minute = 60000; // 60000 milliseconds in a minute
unsigned long debug_second =  1000; // 1000 milliseconds in a second

char debugBuffer[20];
HardwareSerial *debHardwareSerial = &Serial;

void setDebugPort(HardwareSerial *ser)
{
	debHardwareSerial = ser;
}
const char* debugTimestamp()
{
	unsigned long debugNow = millis();
	#ifdef MILLISECONDS
		sprintf(debugBuffer,"%02lu:%02lu:%02lu.%04lu",debugNow/debug_hour,debugNow/debug_minute,debugNow/debug_second,debugNow%debug_second);
	#else
		sprintf(debugBuffer,"%02lu:%02lu:%02lu",debugNow/debug_hour,debugNow/debug_minute,debugNow/debug_second);
	#endif
	return debugBuffer;

}

void debugFkt(const char* buf)
{  
  
  debHardwareSerial->print("<DEB|");
  debHardwareSerial->print(debugTimestamp());
  debHardwareSerial->print("> ");
  debHardwareSerial->println(buf);
}

void debugFkt(int buf)
{
  debHardwareSerial->print("DEB: ");
  debHardwareSerial->println(buf);
}

void debugFkt(long buf)
{
  debHardwareSerial->print("DEB: ");
  debHardwareSerial->println(buf);
}

void debugFkt(char buf)
{
  debHardwareSerial->print("DEB: ");
  debHardwareSerial->println(buf);
}
