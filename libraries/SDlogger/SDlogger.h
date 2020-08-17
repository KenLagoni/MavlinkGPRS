/*
	SDlogger.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef SDLOGGER_H_
#define SDLOGGER_H_

/*
#if defined (__AVR__) || (__avr__)	
	#include "mavlinkArduino.h"
#elif defined (ARDUINO_SAMD_MKRZERO)  // Arduino MKR Zero
	#include "mavlinkArduino.h"
#else
	#include "c_library_v1-master/common/mavlink.h" 
	#include "c_library_v1-master/ardupilotmega/mavlink.h" 
#endif
*/
#include <SPI.h>
#include <SD.h>

#define LOG_NEW		  1		//Will take the filename and add 0,1...N to each new file.
#define LOG_APPEND 	  2		//Will append to file.
#define LOG_OVERWRITE 3	    //Will clear the file each time. // not implememnted yet!

class SDLogger
{
	// Public functions to be used on all Messages
	public:
		SDLogger(); 
		bool begin(int csPin, String filePrefix, int logMode); // returns true if error (unable to initilize SD card)
		bool print(String data); // returns true if error.
		void flush(void); // only need to forece flush. else class will do this every time SD_WRITE_CACHE limit is reached.
		void end(); // closing the file.
		String getFileName(void);
	
	// General helper functions and varibels only used by inherited classes.
	protected:
	
	// Parameters only used on mother class.
	private:
		const static int SD_WRITE_CACHE = 512; // chars write to SD card before flushing the SD card.
		uint16_t charCount; // keeping track when it is time to flush.
		String findNextLogFile(String filePrefix);
		File logFile;
		String logfileName;
};

#endif /* SDLOGGER_H_ */





  
  
  