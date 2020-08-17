/*
	sim800l.cpp

	Copyright (c) 2019 Lagoni
	Not for commercial use
 */ 
#include "SIM800l.h"
#include "Arduino.h" // Needed for Serial.print


SIM800L::SIM800L()
{
}

void SIM800L::init(HardwareSerial  *ptr1, unsigned long baudrate, File *fileptr, File *fileptrRX, int _resetPin)
{
//	memset(&this->inputData.payload, 0x00, MAX_PAYLOAD_LENGTH); // Zero fills the buffer
	this->SIM800Serial = ptr1;
	
	// Set logfile pointer
	this->logFile = fileptr;
	this->rxDataFile = fileptrRX;
			
	// Setup and configure GPS
	this->SIM800Serial->begin(baudrate); // 9600 is default baud after power cycle.. which we don't do here but assumes had been done.	
	resetPin=_resetPin;
	logRXdata("\n\n\n ---------- REBOOT ---------- \n\n\n");
	this->status.readyOk = false;
	this->status.cfunOk = false;
	this->status.pinOk = false;
	this->status.callOk = false;
	this->status.smsOk = false;
	this->status.muxOk = false;	
	this->status.apnOk = false;	
	this->status.gprsEnabled = false;
	this->status.ipOk = false;	
	this->status.GPRSConnected = false;
	this->status.shutdown = false;
	this->ip = ""; 
	this->status.txmsgReady = false;
	this->status.pendingTXdata  = false;	
	this->status.ateSet = false;
	this->rxState = LOOKING_FOR_CR;
	this->linkState = RESET;
	this->status.connection = 0;
	this->status.readyOk = false;		
	this->status.rssiReceived = false;
	this->nextLoopTimeout=millis()+(3*60*1000); // alow 3 minuts to hard boot.
	this->restartCounter=0; // reset hard reset timer counter	
	this->nextRSSIrequest=millis()+30000; // first RSSI after 5 seconds
	this->status.linkReady = false;
	this->status.timeToSend = 0;
	
	for(int a=0;a<SIM800L_DATA_BUFFER_SIZE;a++){
		this->TXbuf1.data[a]=0;
		this->TXbuf2.data[a]=0;
	}
	this->TXbuf1.length = 0;
	this->TXbuf2.length = 0;
	this->bufferPointer = 0;
	this->switchTxBuffer(); // sets the buffer
 }


void SIM800L::restart()
{
	// Pull reset pin low for 
	pinMode(this->resetPin, OUTPUT); // SIM800L reset pin
	digitalWrite(this->resetPin, LOW);
	delay(1000);
	
	while (this->SIM800Serial->available()){
		this->SIM800Serial->read(); // empty rx buffer
	}
	
	digitalWrite(this->resetPin, HIGH);
	delay(2000);

	this->status.readyOk = false;
	this->status.cfunOk = false;
	this->status.pinOk = false;
	this->status.callOk = false;
	this->status.smsOk = false;
	this->status.muxOk = false;	
	this->status.apnOk = false;	
	this->status.gprsEnabled = false;
	this->status.ipOk = false;	
	this->status.GPRSConnected = false;
	this->status.shutdown = false;
	this->ip = "";
	this->status.txmsgReady = false; 
	this->status.pendingTXdata  = false;	
	this->status.ateSet = false;
	this->rxState = LOOKING_FOR_CR;
	this->linkState = RESET;
	this->logRXdata("\n-- Hard restart! --\n");
	this->status.connection = 0;
	this->status.readyOk = false;		
	this->status.rssiReceived = false;
	this->status.error = false;
	this->nextLoopTimeout=millis()+(3*60*1000); // alow 3 minuts to hard boot.
	this->restartCounter=0; // reset hard reset timer counter	
	this->nextRSSIrequest=millis()+30000; // first RSSI after 5 seconds
	this->status.linkReady = false;	
	this->status.timeToSend = 0;	
	
	for(int a=0;a<SIM800L_DATA_BUFFER_SIZE;a++){
		this->TXbuf1.data[a]=0;
		this->TXbuf2.data[a]=0;
	}
	this->TXbuf1.length = 0;
	this->TXbuf2.length = 0;
	this->bufferPointer = 0;
	this->switchTxBuffer(); // sets the buffer
}



void SIM800L::debugPrintTimeStamp(String data)
{
	String dataToPrint = "";
	dataToPrint = "\n" + String(millis()) + ":" + data;
	
	if(SERIAL_ENALBED) 
		Serial.print(dataToPrint);
	
	if(LOG_DATA){
		this->logFile->print(dataToPrint);
		charcount += dataToPrint.length();
		if(charcount > 510){
			//Serial.print("Flushing SD card write buffer\n");
			this->logFile->flush();
			charcount=0;
		}
	}
}

void SIM800L::logRXdata(String data)
{
	if(PRINT_RXDATA)
		Serial.print(data);
	
	if(LOG_DATA){
		this->rxDataFile->print(data);
		charcount2 += data.length();
		if(charcount2 > 510){
			this->rxDataFile->flush();
			charcount2=0;
		}
	}
}


void SIM800L::debugPrint(String data)
{
	if(SERIAL_ENALBED) 
		Serial.print(data);
	
	this->logFile->print(data);
	charcount += data.length();
	if(charcount > 510){
		//Serial.print("Flushing SD card write buffer\n");
		this->logFile->flush();
		charcount=0;
	}
}

void SIM800L::switchTxBuffer(void){
//	this->debugPrint("\n"+ String(millis()) +"SIM800L:Switching Bufffers ");
	if(this->bufferPointer == 0){
		this->bufferPointer = 1;
		
		// make TXbuf ready for new input data.
		this->TXinput=&this->TXbuf1;
		this->TXbuf1.length=0;
		
		// make TXoutput point to the previous input buffer.
		this->TXoutput=&this->TXbuf2;
	}else{
		// make TXbuf ready for new input data.
		this->TXinput=&this->TXbuf2;
		this->TXbuf2.length=0;
		
		// make TXoutput point to the previous input buffer.
		this->TXoutput=&this->TXbuf1;
		
		this->bufferPointer = 0;
	}
}

bool SIM800L::writeUdpData(SerialData_t _msg){ // return true if failed.
//	this->debugPrint("\n"+ String(millis()) +"SIM800L:Got message, TXinput.length=" + String(this->TXinput->length) + " Input buffer is:" + String(this->bufferPointer) + "\n");
	
	if((_msg.totalLength + this->TXinput->length) >= SIM800L_DATA_BUFFER_SIZE){ // check to see if there is room
		//switch to new buffer:
		switchTxBuffer();
		this->status.txmsgReady = true; // force send		
	}
	
	for(int a=0;a<_msg.totalLength;a++){
		this->TXinput->data[this->TXinput->length] = _msg.payload[a];
		this->TXinput->length++;
	}
	return false;
}

/*
bool SIM800L::writeUdpData(uint8_t *data, uint16_t length){ // return true if failed.
	
	if(this->status.txmsgReady == false){
		
		// copy data to tx buffer.
		for(int a=0;a<length;a++){
			this->TXmsg.data[a] = *data++;
		}
		
		this->TXmsg.length=length;
		
		this->status.txmsgReady = true;		
		return false;
	}else{
	
	}	
	
	return true;
}
*/


	/*
	First output from SIM800L after reset:
	0D 0A 52 44 59 0D 0A  							<CR><LF>RDY<CR><LF>
	0D 0A 2B 43 46 55 4E 3A 20 31 0D 0A				<CR><LF>+CFUN: 1<CR><LF>
	0D 0A 2B 43 50 49 4E 3A 20 52 45 41 44 59 0D 0A <CR><LF>+CPIN: READY<CR><LF>
	0D 0A 43 61 6C 6C 20 52 65 61 64 79 0D 0A 		<CR><LF>Call Ready<CR><LF> 
	0D 0A 53 4D 53 20 52 65 61 64 79 0D 0A 			<CR><LF>SMS Ready<CR><LF>

	Reply to "AT\n"
	41 54 0D 0A 4F 4B 0D 0A							AT<CR><LF>OK<CR><LF>
	
	Reply to "AT+CGATT?":   	                        
	41 54 2B 43 47 41 54 54 3F 0D 0A 2B 43 47 41 54 54 3A 20 30 0D 0A 0D 0A 4F 4B 0D 0A  	AT+CGATT?<CR><LF>+CGATT: 0<CR><LF><CR><LF>OK<CR><LF>

	Reply to "AT+CSQ":  
	41 54 2B 43 53 51 0D 0A 2B 43 53 51 3A 20 32 39 2C 30 0D 0A 0D 0A 4F 4B 0D 0A			AT+CSQ<CR><LF>+CSQ: 29,0<CR><LF><CR><LF>OK<CR><LF>

	Reply to "AT+CIPMUX=1":
	41 54 2B 43 49 50 4D 55 58 3D 31 0D 0A 4F 4B 0D 0A										AT+CIPMUX=1<CR><LR>OK<CR><LF>

	Reply to "AT+CSTT=\"internet\"":
	41 54 2B 43 53 54 54 3D 22 69 6E 74 65 72 6E 65 74 22 0D 0A 4F 4B 0D 0A					AT+CSTT="internet"<CR><LF>OK<CR><LF>
	
	Reply to "AT+CIICR":
	41 54 2B 43 49 49 43 52 0D 0A 4F 4B 0D 0A												AT+CIICR<CR><LF>OK<CR><LF>
	or
	41 54 2B 43 49 49 43 52 0D 0A 2B 50 44 50 3A 20 44 45 41 43 54 0D 0A 0D 0A 45 52 52 4F 52 0D 0A 	AT+CIICR<CR><LF>+PDP: DEACT<CR><LF><CR><LF>ERROR<CR><LF>

	Reply to "AT+CIFSR":							
	41 54 2B 43 49 46 53 52 0D 0A 38 33 2E 37 33 2E 31 31 31 2E 37 33 0D 0A					AT+CIFSR<CR><LF>83.73.111.73<CR><LF>
	
	Reply to "AT+CIPSHUT":							
	guess:																					AT+CIPSHUT<CR><LF>SHUT OK<CR><LF>
	
	Reply to "AT+CIPSTART=0,\"UDP\",\"81.161.156.81\",\"14450\"":
	41 54 2B 43 49 50 53 54 41 52 54 3D 30 2C 22 55 44 50 22 2C 22 38 31 2E 31 36 31 2E 31 35 36 2E 38 31 22 2C 22 31 34 34 35 30 22 0D 0A 4F 4B 0D 0A 0D 0A 30 2C 20 43 4F 4E 4E 45 43 54 20 4F 4B 0D 0A 										 
	AT+CIPSTART=0,"UDP","81.161.156.81","14450"<CR><LF>OK<CR><LF><CR><LF>0, CONNECT OK<CR><LF>
											 
					
	Reply to "AT+CIPSEND=0"
	0d 0a 3e 20 "<CR><LF>> "
	After send is done:
	0d 0a 30 2c 20 53 45 4e 44 20 4f 4b 0d 0a 												<CR><LF>0, SEND OK<CR><LF>							
	or if failed:
	0d 0a 30 2c 20 53 45 4e 44 20 46 41 49 4c 0d 0a 										<CR><LF>0, SEND FAIL<CR><LF>
	
	AT command reply from "AT+CGATT?" : "41 54 2B 43 47 41 54 54 3F 0D 0A 2B 43 47 41 54 54 3A 20 30 0D 0A"  = "AT+CGATT?<CR><LF>+CGATT: 0<CR><LF>"
	AT command reply from success UDP transmission (socket 0) : "20 0D 0A 30 2C 20 53 45 4E 44 20 4F 4B 0D 0A"  = " <CR><LF>0, SEND OK<CR><LF>"
	
	
	AT command reply from "AT+CIPSTATUS\n" :  53 54 41 54 45 3a 20 49 50 20 50 52 4f 43 45 53 53 49 4e 47 0d 0a 0d 0a
										      43 3a 20 30 2c 30 2c 22 55 44 50 22 2c 22 38 31 2e 31 36 31 2e 31 35 36 2e 38 31 22 2c 22 31 34 34 35 30 22 2c 22 43 4f 4e 4e 45 43 54 45 44 22 0d 0a
										      43 3a 20 31 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										      43 3a 20 32 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										      43 3a 20 33 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										      43 3a 20 34 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										      43 3a 20 35 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										 
										 In Chars:
										 
										      STATE: IP PROCESSING<CR><LF>
											  <CR><LF>
										      C: 0,0,"UDP","81.161.156.81","14450","CONNECTED"<CR><LF>
										      C: 1,,"","","","INITIAL"<CR><LF>
										      C: 2,,"","","","INITIAL"<CR><LF>
										      C: 3,,"","","","INITIAL"<CR><LF>
										      C: 4,,"","","","INITIAL"<CR><LF>
										      C: 5,,"","","","INITIAL"<CR><LF>
											  
											  
	AT Command reply from "AT+CSQ\n"		  43 53 51 3a 20 33 31 2c 30 0d 0a 0d 0a 4f 4b 0d 0a 0d 0a 
										In Chars:
											  +CSQ: 31,0<CR><LF>
											  <CR><LF>OK<CR><LF>
											  <CR><LF>
											  
	AT Command reply from "AT\n"			  0d 0a 4f 4b 0d 0a 
										In Chars:
											  <CR><LF>OK<CR><LF>					  
	*/
void SIM800L::printStringAsHEXBytes(String data){

	for(int a=0; a<data.length(); a++){
		if(data.charAt(a) < 16){
		//Serial.print("0");
			this->debugPrint("0");
		}
		this->debugPrint(String(data.charAt(a),HEX));
		this->debugPrint(" ");	
	}

}
void SIM800L::serviceRXdata()
{
	uint16_t bytesToRead = (uint16_t)this->SIM800Serial->available();
	this->debugPrint(".");	
	while (bytesToRead-- > 0)	
	{
		if(bytesToRead > 150)
			this->debugPrint("\n----Buffer:" + String(bytesToRead) + ":\n");	
		
		uint8_t readByte=this->SIM800Serial->read();
		
		if(DEBUG_SIM800L){
			if(readByte < 16){
				this->logRXdata("0");
			}	
			this->logRXdata(String(readByte,HEX));
			this->logRXdata(" ");
		}
			
		switch (rxState)
		{
			case LOOKING_FOR_CR: // look for <CR> or 0x0D 
				if(readByte == 0x0D)
				{
					logRXdata("\n" + String(millis()) + ": <0x0D> Found expecting <0x0A>, go to READ_PREFIX: ");
					rxState = READ_PREFIX;
				}else{
					this->msgreply.prefix += (char)readByte;
				}
			break;
			
			case READ_PREFIX: // this readbyte mustbe <LF> 0x0A, else error adn restart.				
				if(readByte == 0x0A)
				{
//					logRXdata("\n" + String(millis()) + ": <0x0A> Found (Prefix):");
					// Read the 
					if(msgreply.prefix == ""){
						//empty string, normally ignore, but it could also be start of UDP transmission, SIM800L will only reply with "<CR><LF>> " (0x0d 0x0a 0x3e 0x20)
						logRXdata("\n" + String(millis()) + ": <0x0A> Found (Prefix Empty) go to Wait_FOR_0x3E: ");
						rxState = WAIT_FOR_0x3E; 
						break;
					}else if(msgreply.prefix == "ERROR" || msgreply.prefix == "ERROR"){
						logRXdata("\n" + String(millis()) + ": <0x0A> Found Prefix is \"ERROR\", go to LOOKING_FOR_CR: ");	
						this->status.error = true; // indicate errror to retry, so we don't have to wait for timeout. This is the case if AT+CIPSEND is too high 
					}else if(msgreply.prefix == "OK"){
						logRXdata("\n" + String(millis()) + ": <0x0A> Found Prefix: is \"OK\", go to LOOKING_FOR_CR: ");						
					}else if(msgreply.prefix == "0, SEND OK"){
						logRXdata("\n" + String(millis()) + ": <0x0A> Found (Prefix: \"0, SEND OK\"), go to LOOKING_FOR_CR: ");
						this->status.pendingTXdata = true;
					}else if(msgreply.prefix == "0, SEND FAIL"){
						logRXdata("\n" + String(millis()) + ": <0x0A> Found (Prefix: \"0, SEND FAIL\"), go to WAIT_TO_SEND: ");
						this->debugPrintTimeStamp("Sending Error!\n");
						linkState=WAIT_TO_SEND;
						this->status.txmsgReady = false;
						break;
					}else if(msgreply.prefix.substring(0,8) == "+RECEIVE"){
						this->debugPrintTimeStamp("Incomming UDP package!:" + msgreply.prefix + ": Prefix length:"+String(msgreply.prefix.length())+":\n");
						if(msgreply.prefix.length() == 13){ // length is 1 char
							this->status.incommingPackageLength = (((uint8_t)msgreply.prefix.charAt(11))-48);
						}else if(msgreply.prefix.length() == 14){ // length is 2 char
							this->status.incommingPackageLength = (((uint8_t)msgreply.prefix.charAt(11))-48)*10 + (((uint8_t)msgreply.prefix.charAt(12))-48);
						}else if(msgreply.prefix.length() == 15){ // length is 3 char
							this->status.incommingPackageLength = (((uint8_t)msgreply.prefix.charAt(11))-48)*100 + ((((uint8_t)msgreply.prefix.charAt(12))-48)*10) + (((uint8_t)msgreply.prefix.charAt(13))-48);						
						}else if(msgreply.prefix.length() == 16){ // length is 4 char (maximum i hope) think it is 1400 or so
							this->status.incommingPackageLength = (((uint8_t)msgreply.prefix.charAt(11))-48)*1000 + ((((uint8_t)msgreply.prefix.charAt(12))-48)*100) + ((((uint8_t)msgreply.prefix.charAt(13))-48)*10) + (((uint8_t)msgreply.prefix.charAt(14))-48);							
						}else{
							this->debugPrintTimeStamp("Error reading incomming UDP length!\n");
						}
						logRXdata("\n" + String(millis()) + ": <0x0A> Found (Prefix: \"+RECEIVE\"), go to READ_INCOMMING_DATA: ");						
						this->debugPrintTimeStamp("Reading " + String(this->status.incommingPackageLength)  + " bytes of data from SIM800L:");
						rxState=READ_INCOMMING_DATA;
						break;
					}else if(msgreply.prefix == "RDY"){
						//Serial.print("READY!!!\n");
						logRXdata("\n" + String(millis()) + ": <0x0A> Found (Prefix: \"RDY\"), go to LOOKING_FOR_CR: ");		
						this->debugPrintTimeStamp("READY!!!\n");
						this->status.readyOk = true;			
					}else if(msgreply.prefix.substring(0,5) == "+CFUN"){
						logRXdata("\n" + String(millis()) + ": <0x0A> Found (Prefix: \"+CFUN=" + String(msgreply.prefix.charAt(7)) + "\"), go to LOOKING_FOR_CR: ");	
						this->debugPrintTimeStamp("+CFUN ");
						if(msgreply.prefix.charAt(7) == '1'){
							this->debugPrintTimeStamp("OK, CFUN=1\n");
							this->status.cfunOk = true;
						}else if(msgreply.prefix.charAt(7) == '0'){
							this->debugPrintTimeStamp("Error, CFUN=0\n");			
						}else if(msgreply.prefix.charAt(7) == '0'){
							this->debugPrintTimeStamp("Error, CFUN=4\n");								
						}else{
							this->debugPrintTimeStamp("Error, CFUN reply unknown:" + msgreply.prefix + ":\n");								
						}
					}else if(msgreply.prefix.substring(0,5) == "+CPIN"){
						this->debugPrintTimeStamp("+CPIN ");
						if(msgreply.prefix.substring(7,12) == "READY"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found (Prefix: \"OK, CPIN READY\"), go to LOOKING_FOR_CR: ");
							this->debugPrintTimeStamp("OK, CPIN READY\n");
							this->status.pinOk = true;	
						}else{
							logRXdata("\n" + String(millis()) + ": <0x0A> Found (CPIN Unkown), go to LOOKING_FOR_CR: ");
							this->debugPrintTimeStamp("Error, CPIN reply unknown:" + msgreply.prefix + ":\n");	
							this->debugPrintTimeStamp("Looking for:" + msgreply.prefix.substring(7,12) + ":\n");
						}
					}else if(msgreply.prefix == "Call Ready"){
						logRXdata("\n" + String(millis()) + ": <0x0A> Found (Call Ready), go to LOOKING_FOR_CR: ");
						this->debugPrintTimeStamp("Call Ready\n");
						this->status.callOk = true;
					}else if(msgreply.prefix == "SMS Ready"){
						logRXdata("\n" + String(millis()) + ": <0x0A> Found (SMS Ready), go to LOOKING_FOR_CR: ");
						this->debugPrintTimeStamp("SMS Ready\n");
						this->status.smsOk = true;
						
						// List all known commands here:
					}else if(msgreply.prefix == "UNDER-VOLTAGE WARNNING"){
						this->debugPrintTimeStamp("Under Voltage Detected!\n");	
						logRXdata("\n" + String(millis()) + ": Under Voltage Detected!\n");
					}	
					//}else if(msgreply.prefix == "AT+CGATT?" || msgreply.prefix == "AT+CIPMUX"){
					//	rxState = READ_RESPONSE1;
					//	break;
					else if(msgreply.prefix == "AT+CIPMUX=1" ||
							msgreply.prefix == "AT+CSTT=\"internet\"" ||
							msgreply.prefix == "AT+CIICR" ||
							msgreply.prefix == "AT+CIPSHUT" ||
							msgreply.prefix == "AT+CIFSR" ||
							msgreply.prefix.substring(0,13) == "AT+CIPSTART=0" ||
							msgreply.prefix == "ATE0" ||
							msgreply.prefix.substring(0,5) == "+CSQ:" ||
							msgreply.prefix.substring(0,8) == "+RECEIVE" ||
							msgreply.prefix.substring(0,6) == "STATE:"
							){
						// go to read response:
						// Perrhaps we should only do this if we know the PREFIX and know it has a response???:
						// AT+CIPMUX=1
						// AT+CSTT="internet"
						// AT+CIICR
						// AT+CIPSHUT
						// AT+CIFSR
						// AT+CIPSTART=0,"UDP","81.161.156.81","14450"
						// ATE0
						// +CSQ
						// +RECEIVE
						// STATE:

						logRXdata("\n" + String(millis()) + ": <0x0A> Found, Know prefix" + msgreply.prefix + "go to READ_RESPONSE1: ");
						
						rxState = READ_RESPONSE1;
						break;
					}else{
						logRXdata("\n" + String(millis()) + ": <0x0A> Found, but prefix is unknown:" + msgreply.prefix + ":, go to LOOKING_FOR_CR: ");
					}				
				}else{
					logRXdata("\n" + String(millis()) + ": Missing <0x0A> - restart statmachine, go to LOOKING_FOR_CR: ");
					this->debugPrintTimeStamp("Error! - no LF after CR. - Restart statemachine!\n");
				}
				resetRXdata();
			break;
			
			case READ_RESPONSE1:
				if(readByte == 0x0D)
				{
					logRXdata("\n" + String(millis()) + ": <0x0D> Found expecting <0x0A>, go to HANDLE_RESPONSE1: ");
					rxState = HANDLE_RESPONSE1;
				}else{
					this->msgreply.response1 += (char)readByte;
				}			
			break;
			
			case HANDLE_RESPONSE1:
				if(readByte == 0x0A)
				{
					// Read the response1.
					if(msgreply.response1 == "" || msgreply.response1 == "error" || msgreply.response1 == "ERROR"){						
						// check if this is a "STATE:" reply, because then response1 will be empty, but prefix is starts with "STATE:"
						if(msgreply.prefix.substring(0,6) == "STATE:"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found and prefix is \"STATE:\", go to READ_RESPONSE2: ");
							rxState = READ_RESPONSE2; // go to response 2, but keep response 1 blank.
							msgreply.response1="";
							break;
						}else if(msgreply.prefix == "OK"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found and Prefix is \"OK\", go to LOOKING_FOR_CR: ");					
						
						}else if(msgreply.prefix.substring(0,6) == "+CSQ: "){
							// We have a RSSI response.
							rxState = READ_RESPONSE2; // go to response 2, but keep response 1 blank.
							msgreply.response1="";
							break;				
						}else{
							logRXdata("\n" + String(millis()) + ": <0x0A> Found But response 1 is empty or error, go to LOOKING_FOR_CR: ");
							this->debugPrintTimeStamp("Response 1 Error prefix:" +  msgreply.prefix + ": response1:" + msgreply.response1 + ": response2:N/A:\n");
						}
					}else if(msgreply.response1 == "ok" || msgreply.response1 == "OK"){
						// enter here if response 1 is ok.
						if(msgreply.prefix == "AT+CIPMUX=1"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 is \"AT+CIPMUX=1\", go to LOOKING_FOR_CR: ");
							this->status.muxOk = true;	
						}else if(msgreply.prefix.substring(0,7) == "AT+CSTT"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 is \"AT+CSTT\", go to LOOKING_FOR_CR: ");
							this->status.apnOk = true;	
						}else if(msgreply.prefix == "AT+CIICR"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 is \"AT+CIICR\", go to LOOKING_FOR_CR: ");
							this->status.gprsEnabled = true;	
						}else if(msgreply.prefix.substring(0,11) == "AT+CIPSTART"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 is \"AT+CIPSTART\", go to READ_RESPONSE2: ");
							rxState = READ_RESPONSE2;
							break;
						}else if(msgreply.prefix == "ATE0"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 is \"ATE0\", go to LOOKING_FOR_CR: ");
							this->status.ateSet = true;
						}
						else{
							logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 is unknown, go to LOOKING_FOR_CR: ");
							this->debugPrintTimeStamp("Unknown prefix:" +  msgreply.prefix + ": response1:" + msgreply.response1 + ": response2:N/A:\n");
						}							
					}else if(msgreply.prefix == "AT+CGATT?"){		
							logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 is \"AT+CGATT?\", go to READ_RESPONSE2: ");
							rxState = READ_RESPONSE2; 
							break;
					}else if(msgreply.prefix == "AT+CIICR"){ // If command doesn't reply with ok, then probberly +PDP: DEACT error, but we need to read next response.
							logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 \"AT+CIICR\" did not reply with ok, go to READ_RESPONSE2: ");
							rxState = READ_RESPONSE2; 
							this->debugPrintTimeStamp("AT+CIICR response is not ok\n");
							break;					
					}else if(msgreply.prefix == "AT+CIFSR"){ // no OK from this command, just the IP.
						logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 \"AT+CIFSR\", go to LOOKING_FOR_CR: ");
						this->status.ip = msgreply.response1;	
						this->status.ipOk = true;						
					}else if(msgreply.prefix == "AT+CIPSHUT" && msgreply.response1 == "SHUT OK" ){ 
						logRXdata("\n" + String(millis()) + ": <0x0A> Found Response1 \"SHUT OK\", go to LOOKING_FOR_CR: ");
						this->status.shutdown = false;					
					}
					else{
						logRXdata("\n" + String(millis()) + ": <0x0A> Found But response not valid or known, LinkState=RESET, go to LOOKING_FOR_CR: ");
						this->debugPrintTimeStamp("Response 1 Error prefix only:" + msgreply.prefix + ":\n");
						this->debugPrintTimeStamp("prefix again:");
						this->printStringAsHEXBytes(msgreply.prefix);
						this->debugPrint(": response1:");
						this->printStringAsHEXBytes(msgreply.response1);
						this->debugPrint(": response2:N/A:\n");
						this->debugPrint("Current linkState:" + String(linkState) + ":\n");
						this->status.txmsgReady = false; 
					}		
				}else{
					logRXdata("\n" + String(millis()) + ": No <0x0A> after <0x0D> this is an error!, go to LOOKING_FOR_CR: ");
					this->debugPrintTimeStamp("Error! - no LF after CR. - Restart statemachine!\n");
				}
				resetRXdata();
			break;
			
			case READ_RESPONSE2:
				if(readByte == 0x0D)
				{
					logRXdata("\n" + String(millis()) + ": <0x0D> Found expecting <0x0A>, go to HANDLE_RESPONSE2: ");
					rxState = HANDLE_RESPONSE2;
				}else{
					this->msgreply.response2 += (char)readByte;
				}			
			break;
			
			case HANDLE_RESPONSE2:			
				if(readByte == 0x0A)
				{
					//this->debugPrint("Handle response 2: prefix:" + msgreply.prefix + ": response1:" + msgreply.response1 + ": response2:" + msgreply.response2 + ":\n");	
					// Read the response1.
					if(msgreply.response2 == ""){ // skip if blank.
						logRXdata("\n" + String(millis()) + ": <0x0A> Found, response is empty, try again, go to READ_RESPONSE2: ");
						rxState = READ_RESPONSE2;
						this->debugPrintTimeStamp("Empty string at response 2 - retry\n");
						break;
					}else if(msgreply.response2 == "error" || msgreply.response2 == "ERROR"){	
						this->debugPrintTimeStamp("AT command ERROR: prefix:" + msgreply.prefix + ": response1:" + msgreply.response1 + ": response2:" + msgreply.response2 + ":\n");
						if(msgreply.prefix == "AT+CIICR" && msgreply.response1 == "+PDP: DEACT"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found, response is \"+PDP: DEACT\" (error) set linkState=PDP_DEACT, go to LOOKING_FOR_CR: ");					
							linkState=PDP_DEACT;
						}else{
							logRXdata("\n" + String(millis()) + ": <0x0A> Found, response is \"error\", go to LOOKING_FOR_CR: ");	
						}
					}else if(msgreply.response2 == "ok" || msgreply.response2 == "OK"){
						if(msgreply.prefix == "AT+CGATT?"){			
							logRXdata("\n" + String(millis()) + ": <0x0A> Found, response is \"ok\" ");							
							if(msgreply.response1.charAt(8) == '0'){
								this->status.GPRSConnected = false;
								logRXdata("\"AT+CGATT=0\" (GPRS not connected), go to LOOKING_FOR_CR: ");		
								this->debugPrintTimeStamp("GPRS is not connected prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ":\n");
							}else if(msgreply.response1.charAt(8) == '1'){
								logRXdata("\"AT+CGATT=1\" (GPRS is connected), go to LOOKING_FOR_CR: ");
								this->status.GPRSConnected = true;	
								this->debugPrintTimeStamp("GPRS is connected prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ":\n");
							}else{
								logRXdata("\"AT+CGATT\" is unknown, go to LOOKING_FOR_CR: ");
								this->debugPrintTimeStamp("GPRS status unknown prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ":\n");
							}
						}else if(msgreply.prefix.substring(0,6) == "+CSQ: "){
							
							// We have a RSSI response.
							logRXdata("\n" + String(millis()) + ": <0x0A> Found and this is RSSI and BER:");			
							if(msgreply.prefix.charAt(7) == ','){ // then signal is only one digit.
								this->rssi = (uint8_t)(msgreply.prefix.charAt(6)-48); // ascii to int.
								
								if(msgreply.prefix.length() == 9){ // get bit error one digit (ber)
									this->ber = (uint8_t)(msgreply.prefix.charAt(8)-48);
								}else if(msgreply.prefix.length() == 10){
									this->ber = (uint8_t)((msgreply.prefix.charAt(8)-48)*10 + (msgreply.prefix.charAt(9)-48)); // ascii to int.
								}else{
									logRXdata(" Bit Error Not Found! ");	
								}
							}else if(msgreply.prefix.charAt(8) == ','){ // then signal is two digit.
								this->rssi = (uint8_t)((msgreply.prefix.charAt(6)-48)*10 + (msgreply.prefix.charAt(7)-48)); // ascii to int.
								
								if(msgreply.prefix.length() == 10){ // get bit error one digit (ber)
									this->ber = (uint8_t)(msgreply.prefix.charAt(9)-48);
								}else if(msgreply.prefix.length() == 11){
									this->ber = (uint8_t)((msgreply.prefix.charAt(9)-48)*10 + (msgreply.prefix.charAt(10)-48)); // ascii to int.
								}else{
									logRXdata(" Bit Error Not Found! ");	
								}
							}else{
								logRXdata(" Rssi Not Found! ");			
							}
							this->status.rssiReceived = true;
						    logRXdata("RSSI:" + String(this->rssi) + ": BER:" + String(this->ber) + ", go to LOOKING_FOR_CR: ");	
							
						}else{
							logRXdata("\n" + String(millis()) + ": <0x0A> Found, response is ok, but unkown, go to LOOKING_FOR_CR: ");
	                        this->debugPrintTimeStamp("Reply OK, but unknown prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ":\n");
						}
					}else if(msgreply.prefix.substring(0,11) == "AT+CIPSTART"){
						if(msgreply.response2 == "0, CONNECT OK"){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found, response is AT+CIPSTART is CONNECTED OK, go to LOOKING_FOR_CR: ");
							this->status.GPRSConnected = true;	
							this->debugPrintTimeStamp("GPRS connected succesfully\n");
						}else{
							logRXdata("\n" + String(millis()) + ": <0x0A> Found, response is AT+CIPSTART but it is not connected, perhaps error, go to LOOKING_FOR_CR: ");
							this->debugPrintTimeStamp("Error on AT+CIPSTART prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ": response2:" + msgreply.response2 + ":\n");					
						}
					}else if(msgreply.prefix.substring(0,6) == "STATE:"){ // this is a resply in AT+CPSTATUS (connection status).
					
						// Response is in ressponse2:C: 0,0,"UDP","81.161.156.81","14450","CONNECTED":
						logRXdata("\n" + String(millis()) + ": Response is \"" + msgreply.response2 + "\" HANDLE_RESPONSE2: ");
						if(msgreply.response2.charAt(3) == '0'){ // we are only using connection 0, thus read this status and skip the rest.
							logRXdata("\n" + String(millis()) + ": <0x0A> Found, Now analysing connection, go to READ_RESPONSE2: ");
							
							// Check if we are connected:
							uint16_t replyLength = msgreply.response2.length();
							conStatus = "";
							uint16_t index = replyLength-1;
							do{
								//this->debugPrintTimeStamp("\nconStatus [" + String(index) + "] :" + msgreply.response2.charAt(index-1) + ":\n");	
								if(msgreply.response2.charAt(index-1) == '"'){
									conStatus=msgreply.response2.substring(index,replyLength-1);
									replyLength=0; // stop loop
								}
								index--;
							}while(replyLength > 0);
							
							this->debugPrintTimeStamp(" conStatus:" + conStatus + ":\n");				

							msgreply.response2=""; //reset string
							rxState = READ_RESPONSE2;
							break;	
						}else if(msgreply.response2.charAt(3) == '5'){
							logRXdata("\n" + String(millis()) + ": <0x0A> Found, Done reading connection status status, go to LOOKING_FOR_CR: ");
							
							if(conStatus == "CONNECTED"){
								this->status.connection=1;
							}else if(conStatus == "CLOSED" || conStatus == "CLOSING" || conStatus == "INITIAL" ){
								this->status.connection=2; // indicate we are not connected.
							}else{
								this->status.connection=2; // indicate we are not connected.
							}								
						}else{
							msgreply.response2=""; //reset string
							rxState = READ_RESPONSE2;
							this->debugPrintTimeStamp("Empty string at response 2 - retry\n");
							break;	
						}

						/*
						////////////////// DEBUG TO SE IFF THIS COMMAND IS ENDED WITH OK<CR><LF>
						this->debugPrint("\n While(1):");
						do{
							if(this->SIM800Serial->available()){
								this->debugPrint("\n");		
								readByte=this->SIM800Serial->read();
								if(readByte < 16){
									this->debugPrint("0");
								}	
								this->debugPrint(String(readByte,HEX));
								this->debugPrint(" ");								
							}else{
								this->debugPrint(".");								
							}							
						}while(1);
						*/
						
					}else{
						logRXdata("\n" + String(millis()) + ": <0x0A> Found, but response is not known or handled!, go to LOOKING_FOR_CR: ");
						this->debugPrintTimeStamp("Reply Error prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ": response2:" + msgreply.response2 + ":\n");
					}										
				}else{
					logRXdata("\n" + String(millis()) + ": No <0x0A> after <0x0D> this is an error!, go to LOOKING_FOR_CR: ");
					this->debugPrintTimeStamp("Error! - no LF after CR. - Restart statemachine!\n");
				}
				resetRXdata();
			break;
			
			case WAIT_FOR_0x3E:
				if(readByte == 0x3E){ // 0x3E after <CR><LF> is pobberly because we started UDP transmission and are waiting for "> " (0x3E 0x20)
					logRXdata("\n" + String(millis()) + ": <0x3E> Found expecting <0x20>, go to WAIT_FOR_0x20: ");
					rxState=WAIT_FOR_0x20; 
				}else{
					//this->debugPrint("Empty string\n"); // string was empty, restart but keep this read byte:
					logRXdata("\n" + String(millis()) + ": <0x3E> Not Found, include this byte, go to LOOKING_FOR_CR : ");
					resetRXdata();//Restart
					this->msgreply.prefix = (char)readByte; // keep the byte we just read.
				}
			break;
			
			case WAIT_FOR_0x20:
				if(readByte == 0x20){
					logRXdata("\n" + String(millis()) + ": <0x20> Found expecting, Now ready for data, go to LOOKING_FOR_CR: ");
					this->status.pendingTXdata=true;
				}else{
					logRXdata("\n" + String(millis()) + ": <0x20 >Not Found!, go to LOOKING_FOR_CR: ");
					this->debugPrintTimeStamp("Error, expecting 0x20 but got:" + String(readByte, HEX) + ":\n");
				}
				resetRXdata();//Restart
			break;
			
			case READ_INCOMMING_DATA:
				if(this->status.incommingPackageLength >= 0){
					this->status.incommingPackageLength--;
					if(this->status.incommingPackageLength == 0){
						resetRXdata();//Restart
						this->debugPrintTimeStamp("Read complete!\n");
					}
				}
			break;
			
			default:
				logRXdata("\n" + String(millis()) + ": StateMachine Default state, this should not happend!!, LinkState=RESET, go to LOOKING_FOR_CR: \n\n");
				this->debugPrintTimeStamp("Error! - Default\n");
				resetRXdata();//Restart
				linkState=RESET;
			break;
		}
	}	
}	

void SIM800L::serviceGPRSLink(void)
{
	// timout check.
	uint32_t now = millis();
	if(now >= nextLoopTimeout || this->status.error == true){
		nextLoopTimeout = now+SOFT_TIMEOUT_RECOVER;
		// We are hanging, write AT<CR><LF> 20 times
		this->status.error = false;
		
		if(restartCounter++ > HARD_TIMEOUT_COUNT){
			this->debugPrint("\n\n\n\n**************HARD Timeout! time:"+ String(now) + " Limit:" + String(nextLoopTimeout-SOFT_TIMEOUT_RECOVER) +  "**************\n\n\n\n");
			//hard restart
			linkState = RESET;
		}else{
			//soft restart
			this->debugPrint("\n\n\n\n**************Soft Timeout! time:"+ String(now) + " Limit:" + String(nextLoopTimeout-SOFT_TIMEOUT_RECOVER) +  "**************\n\n\n\n");
			this->debugPrintTimeStamp("SIM800L State: [" + String(linkState) + "]->CONNECTION_STATUS\n");
			
			for(int a = 0;a<50;a++)
				this->SIM800Serial->print("T");
			this->SIM800Serial->print("AT\n"); 
			resetRXdata();
			this->SIM800Serial->flush(); // wait for tx buffer to be empty.
			this->SIM800Serial->print("AT+CIPSTATUS\n"); // Get connection status.
			this->status.connection=0; // no reply
			this->status.txmsgReady = false; // only send next timne a new msg has been recieved.
			linkState=CONNECTION_STATUS;			
		}
	}
	
	if(now > this->status.timeToSend){
		this->status.timeToSend = now + SEND_WINDOW_TIMEFREQ; // set next Send time.
		this->switchTxBuffer();
		
		if(this->TXoutput->length > 0){
			if(this->status.txmsgReady){
				this->debugPrint("Time to send allready true... overflow!\n");
			}else{				
				this->status.txmsgReady = true;	
			}
		}else{
			//this->debugPrint("No data to send\n");
		}
	}
	
	switch (linkState)
	{
		case RESET: // Reset SIM800l
			this->restart();
			this->debugPrintTimeStamp("SIM800L State: RESET->SETUP_MUX\n");
			linkState=SETUP_MUX;
		break;
		
		case SETUP_MUX: // wait for init to complete.
			if(this->status.readyOk == true && this->status.cfunOk == true && this->status.pinOk == true && this->status.callOk == true && this->status.smsOk == true){
				this->debugPrintTimeStamp("Init complete!\n");
				// Init complete.
				this->SIM800Serial->print("AT+CIPMUX=1\n"); // set multi connection,
				linkState=IP_INITIAL;
				this->debugPrintTimeStamp("SIM800L State: SETUP_MUX->IP_INITIAL\n");
			}
			/*
			if(millis()>timetoprint){
				timetoprint=millis()+1000;
			this->debugPrint("\n");
			if(this->status.readyOk == true)
				this->debugPrint("1");		
			else
				this->debugPrint("0");		
			
			if(this->status.cfunOk == true)
				this->debugPrint("1");		
			else
				this->debugPrint("0");		
			
			if(this->status.pinOk == true)
				this->debugPrint("1");		
			else
				this->debugPrint("0");		
			
			if(this->status.callOk == true)
				this->debugPrint("1");		
			else
				this->debugPrint("0");	
			
			if(this->status.smsOk == true)
				this->debugPrint("1");		
			else
				this->debugPrint("0");
			this->debugPrint("\n");
			}*/
		break;
		
		case IP_INITIAL: // wait for init to complete.
			if(this->status.muxOk == true && this->status.shutdown == false){
				// Init complete.
				this->SIM800Serial->print("AT+CSTT=\"internet\"\n"); // APNset
				linkState=IP_START;
				this->debugPrintTimeStamp("SIM800L State: IP_INITIAL->IP_START\n");
			}
		break;

		case IP_START:
			if(this->status.apnOk == true){
				// Init complete.
				this->SIM800Serial->print("AT+CIICR\n"); // Connect to GPRS network
				linkState=IP_CONFIG;
				this->debugPrintTimeStamp("SIM800L State: IP_START->IP_CONFIG\n");
			}				
		break;
		
		case IP_CONFIG:
			if(this->status.gprsEnabled == true){
				// Init complete.
				this->SIM800Serial->print("AT+CIFSR\n"); // Get IP
				linkState=IP_GPRSACT;
				this->debugPrintTimeStamp("SIM800L State: IP_CONFIG->IP_GPRSACT\n");
			}				
		break;
		
		case IP_GPRSACT:
			if(this->status.ipOk == true){
				linkState=IP_STATUS;
				this->debugPrintTimeStamp("SIM800L State: IP_GPRSACT->IP_STATUS\n");
				this->debugPrintTimeStamp("SIM800L GPRS ready! IP:" + this->status.ip + "\n");
			}				
		break;		
		
		case IP_STATUS:
			if(this->status.GPRSConnected == false){
				this->SIM800Serial->print("AT+CIPSTART=0,\"UDP\",\"81.161.156.81\",\"14450\"\n"); // Connect UDP socket				
				this->debugPrintTimeStamp("SIM800L Starting UDP socket to lagoni.org\n");
				this->debugPrintTimeStamp("SIM800L State: IP_STATUS->IP_PROCESSING\n");
				linkState=IP_PROCESSING;
			}
		break;	
		
		case IP_PROCESSING:
			if(this->status.GPRSConnected == true){ // will be set when GPRS is connected.
				if(this->status.ateSet == false){
					this->SIM800Serial->print("ATE0\n"); // set no echo back.
					this->debugPrintTimeStamp("SIM800L State: IP_PROCESSING->SET_ATE\n");
					linkState=SET_ATE;
				}else{
					linkState=WAIT_TO_SEND;
				}
			}				
		break;	
		
		case SET_ATE:
			if(this->status.ateSet == true){
				this->debugPrintTimeStamp("SIM800L State: SET_ATE->WAIT_TO_SEND\n");
				this->status.txmsgReady = false; // will not send what we have but wait for new fresh msg.
				this->status.linkReady = true;
				linkState=WAIT_TO_SEND;
			}
		break;	

		case WAIT_TO_SEND:
			this->clearTimeout();
			// Module is ready here, lets first see if it is time to send RSSI requrst:
			if( millis() > this->nextRSSIrequest){
				this->nextRSSIrequest = millis() + RSSI_REQURST;
				this->SIM800Serial->print("AT+CSQ\n"); // Ask for RSSI.
				//this->SIM800Serial->print("AT+CIPSTATUS\n"); // For debug, force a connection test.
				this->status.rssiReceived = false;
				this->debugPrintTimeStamp("SIM800L State: WAIT_TO_SEND->GET_RSSI\n");
				linkState=GET_RSSI;
			}else{
				// check if there is any data ready to be send:			
				if(this->status.txmsgReady == true){
					//this->sendOnlyOnceDebug = false; // only send one mavlink to wake up the Qstation and read the send back.
					this->debugPrintTimeStamp("AT+CIPSEND=0," + String(this->TXoutput->length) + "\n");
					this->SIM800Serial->print("AT+CIPSEND=0," + String(this->TXoutput->length) + "\n");
					this->status.pendingTXdata  = false;
					this->debugPrintTimeStamp("SIM800L State: WAIT_TO_SEND->START_SEND\n");
					linkState=START_SEND;
				}
			}
			
		break;	
		
		case START_SEND:
			if(this->status.pendingTXdata == true){
				this->debugPrintTimeStamp("Writing: " + String(this->TXoutput->length) + "bytes: ");
				for(uint16_t index=0; index < this->TXoutput->length; index++){
					this->SIM800Serial->write(this->TXoutput->data[index]);
					if(this->TXoutput->data[index] < 16){
						this->debugPrint(" 0" + String(this->TXoutput->data[index], HEX));
					}else{
						this->debugPrint(" " + String(this->TXoutput->data[index], HEX));
					}					
				}
				this->debugPrint(" Done writing data to SIM800L");
				this->debugPrintTimeStamp("SIM800L State: START_SEND->SENDING\n");
				this->status.pendingTXdata = false;	
				this->clearTimeout();		
				linkState=SENDING;
			}		
		break;	

		case SENDING:
			if(this->status.pendingTXdata == true){	
				this->clearTimeout();			
				this->status.txmsgReady = false;
				this->debugPrintTimeStamp("SIM800L State: SENDING->WAIT_TO_SEND\n");
				linkState=WAIT_TO_SEND;
			}			
		break;	
		

		case PDP_DEACT:
			this->SIM800Serial->print("AT+CIPSHUT\n");
			this->debugPrintTimeStamp("PDP deactived, restarting IP \n");
			this->debugPrintTimeStamp("SIM800L State: PDP_DEACT->IP_INITIAL\n");
			this->status.apnOk = false;	
			this->status.gprsEnabled = false;
			this->status.ipOk = false;	
			this->status.GPRSConnected = false;
			this->status.shutdown = true;
			this->status.ateSet = false;
			this->status.linkReady = false;
			ip = "";			
			linkState=IP_INITIAL;
		break;			
		
		case CONNECTION_STATUS:
			if(this->status.connection == 1){
				// Connection is fine and we can continue:
				this->clearTimeout();
				this->debugPrintTimeStamp("Link is OK, we can continue\n");
				this->debugPrintTimeStamp("SIM800L State: CONNECTION_STATUS->WAIT_TO_SEND\n");
				linkState=WAIT_TO_SEND;
			}else if(this->status.connection == 2){
				// Connection is lost, restart connection:
				clearTimeout();
				this->debugPrintTimeStamp("Link is Lost, we can continue\n");
				this->debugPrintTimeStamp("SIM800L State: CONNECTION_STATUS->RESET\n");
				linkState=RESET;
			}
		break;
		
		case GET_RSSI:
			if(this->status.rssiReceived == true){
				this->clearTimeout();
				this->debugPrintTimeStamp("SIM800L State: GET_RSSI->WAIT_TO_SEND\n");
				linkState=WAIT_TO_SEND;
			}
		break;
		
		default:
			this->debugPrintTimeStamp("Error! - GPRS State Default\n");
		break;
	}
}	
	
void SIM800L::service(void) 
{		
	serviceRXdata(); 
	serviceGPRSLink();
}

void SIM800L::resetRXdata(void){
	this->msgreply.prefix = "";
	this->msgreply.response1 = "";
	this->msgreply.response2 = "";
	rxState=LOOKING_FOR_CR;
	logRXdata("\n"); // make blank line each time we start over.
}

void SIM800L::clearTimeout(void){
	
	this->nextLoopTimeout=millis()+SOFT_TIMEOUT; // we must be back here before 100ms has passed.
//	this->debugPrintTimeStamp("Setting next timeout to:" + String(this->nextLoopTimeout) + ":\n");
	this->restartCounter=0; // reset soft to hard reset counter.
}

uint8_t SIM800L::getRSSI(void){
	// RSSI from SIM800L is:
	// 0 -> -115dBm
	// 1 -> -111dbm
	// 2-30 -> -110 to -54dbm
	// 31 -> -52dBm or greater
	// 99 Unknown
	
	/* This will be translated to RSSI in % for Mavlink:
	RSSI value:31 Gives:254 Which is 100%
	RSSI value:30 Gives:246 Which is 97%
	RSSI value:29 Gives:239 Which is 94%
	RSSI value:28 Gives:231 Which is 91%
	RSSI value:27 Gives:224 Which is 88%
	RSSI value:26 Gives:217 Which is 85%
	RSSI value:25 Gives:209 Which is 82%
	RSSI value:24 Gives:202 Which is 79%
	RSSI value:23 Gives:195 Which is 76%
	RSSI value:22 Gives:187 Which is 73%
	RSSI value:21 Gives:180 Which is 70%
	RSSI value:20 Gives:172 Which is 68%
	RSSI value:19 Gives:165 Which is 65%
	RSSI value:18 Gives:158 Which is 62%
	RSSI value:17 Gives:150 Which is 59%
	RSSI value:16 Gives:143 Which is 56%
	RSSI value:15 Gives:136 Which is 53%
	RSSI value:14 Gives:128 Which is 50%
	RSSI value:13 Gives:121 Which is 47%
	RSSI value:12 Gives:113 Which is 44%
	RSSI value:11 Gives:106 Which is 41%
	RSSI value:10 Gives:99 Which is 39%
	RSSI value:9 Gives:91 Which is 36%
	RSSI value:8 Gives:84 Which is 33%
	RSSI value:7 Gives:77 Which is 30%
	RSSI value:6 Gives:69 Which is 27%
	RSSI value:5 Gives:62 Which is 24%
	RSSI value:4 Gives:54 Which is 21%
	RSSI value:3 Gives:47 Which is 18%
	RSSI value:2 Gives:40 Which is 15%
	RSSI value:1 Gives:32 Which is 12%
	RSSI value:0 Gives:25 Which is 10%
	*/
	float scaledRSSI = 0;
	
	if(this->rssi == 99){
		return 255; // invalid/unknown
	}
	/*
	for(uint8_t a=31; a>0 ; a--){
		scaledRSSI = ((((float)(a))/31*90+10)/100)*254; 
		Serial.print("RSSI value:" + String(a) + " Gives:" + String((uint8_t)scaledRSSI) + " Which is " + String((uint8_t)((scaledRSSI*100.0)/254.0)) + "%\n");
	}
	*/
	scaledRSSI = ((((float)(this->rssi))/31*90+10)/100)*254;
	
	return (uint8_t)scaledRSSI;
}

bool SIM800L::isReadyToSend(void){
	if(this->status.txmsgReady == false && this->status.linkReady == true)
		return true;
	
	return false;
}
