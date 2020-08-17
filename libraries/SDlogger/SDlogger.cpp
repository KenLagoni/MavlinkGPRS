/*
	SDLogger.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "SDLogger.h"


// Constructor
 SDLogger::SDLogger(){
	this->logfileName="";
	this->charCount=0;
 }
  
 bool SDLogger::begin(int csPin, String filePrefix, int logMode){
	// see if the card is present and can be initialized:
	if (!SD.begin(csPin)) {
		// don't do anything more:
		return true;
	}

	switch(logMode)
	{
		case LOG_NEW:
			this->logfileName = findNextLogFile(filePrefix);
		break;
		
		case LOG_APPEND: 	
			this->logfileName = filePrefix;	
		break;

		case LOG_OVERWRITE: 	
			return true; // not implememnted yet!
		break;
		
		default: 
			return true;		
		break;
	}

	this->logFile = SD.open(this->logfileName, FILE_WRITE);
	
	return false;	
 }
 
String SDLogger::getFileName(void){
	return this->logfileName;
}
 
 bool SDLogger::print(String data){
	 
	if (this->logFile) {
		charCount+=data.length();
		logFile.print(data);
		if(charCount>=SD_WRITE_CACHE){
			charCount=0;
			logFile.flush();  
		}
		return false;
	}else{
		return true;
	}	  
 }
 
 void SDLogger::flush(void){
	logFile.flush();  
 }
 
  void SDLogger::end(void){
	logFile.close();  
 }
 
 String SDLogger::findNextLogFile(String filePrefix){
	String logfile = "";

	File dir = SD.open("/");
	uint8_t filecounter = 0;
	//Serial.print("List SD card files:\n");

	while (true) {
		File entry = dir.openNextFile();
		filecounter++;
	//	Serial.print(String(entry.name()) + "\n");
		if (! entry) {  
		  if(filecounter < 10){
			  logfile = filePrefix + "00" + String(filecounter) + ".txt";
		  }else if(filecounter < 100){
			  logfile = filePrefix + "0" + String(filecounter) + ".txt";
		  }else{
			  logfile = filePrefix + String(filecounter) + ".txt";
		  }
		  // no more files
		  break;
		}
		entry.close();
	}

	dir.close();
//	Serial.print("No more files! - New files is:" + logfile + "\n");  
	return logfile;
 }
 