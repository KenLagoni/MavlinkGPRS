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

void SIM800L::init(HardwareSerial  *ptr1, unsigned long baudrate, File *fileptr, int _resetPin)
{
//	memset(&this->inputData.payload, 0x00, MAX_PAYLOAD_LENGTH); // Zero fills the buffer
	this->SIM800Serial = ptr1;
	
	// Set logfile pointer
	this->logFile = fileptr;
			
	// Setup and configure GPS
	this->SIM800Serial->begin(baudrate); // 9600 is default baud after power cycle.. which we don't do here but assumes had been done.	
	resetPin=_resetPin;
	this->restart();
	/*
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
	String ip = "";
	this->status.txmsgReady == false;
	this->status.pendingTXdata  = false;*/
}


void SIM800L::restart()
{
	// Pull reset pin low for 
	pinMode(this->resetPin, OUTPUT); // SIM800L reset pin
	digitalWrite(this->resetPin, LOW);
	delay(1000);
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
	this->status.txmsgReady == false;
	this->status.pendingTXdata  = false;	
	this->rxState = LOOKING_FOR_CR;
	this->linkState = RESET;
}



void SIM800L::debugPrintTimeStamp(String data)
{
	String dataToPrint = "";
	dataToPrint = String(millis()) + ":" + data;
	Serial.print(dataToPrint);
	this->logFile->print(dataToPrint);
	charcount += dataToPrint.length();
	if(charcount > 510){
		//Serial.print("Flushing SD card write buffer\n");
		this->logFile->flush();
		charcount=0;
	}
}

void SIM800L::debugPrint(String data)
{
	Serial.print(data);
	this->logFile->print(data);
	charcount += data.length();
	if(charcount > 510){
		//Serial.print("Flushing SD card write buffer\n");
		this->logFile->flush();
		charcount=0;
	}
}

void SIM800L::startUdpSocket(String ip, String port, String apn)
{

}

void SIM800L::writeUdpData(uint8_t data[], uint16_t length)
{
	
}


void SIM800L::writeUdpData(SerialData_t msg)
{
	if(this->status.txmsgReady == false){
		this->msg = msg;
		this->status.txmsgReady = true;		
		sendErrorCount=0;
	}else{
		sendErrorCount++;
		if(sendErrorCount > 2500){ // SIM800 must be in a weird state, so we should restart:
			this->debugPrintTimeStamp("Sending Error > 2500, restart SIM800\n");
			this->restart();
		}
		//this->debugPrint("Incomming Mavlink package MSG#" + String(msg.msgID) + " discarded, tx buffer full!\n");
	}	
}


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
	if(this->SIM800Serial->available())
	{
		uint8_t readByte=this->SIM800Serial->read();
			
		if(DEBUG_SIM800L){
			if(readByte < 16){
				//Serial.print("0");
				this->debugPrint("0");
			}
//			Serial.print(readByte,HEX);
//			Serial.print(" ");			
			this->debugPrint(String(readByte,HEX));
			this->debugPrint(" ");
		}
			
		switch (rxState)
		{
			case LOOKING_FOR_CR: // look for <CR> or 0x0D 
				if(readByte == 0x0D)
				{
					rxState = READ_PREFIX;
				}else{
					this->msgreply.prefix += (char)readByte;
				}
			break;
			
			case READ_PREFIX: // this readbyte mustbe <LF> 0x0A, else error adn restart.
				if(DEBUG_SIM800L){ this->debugPrint("\n");}// Serial.print("\n"); }
					
				if(readByte == 0x0A)
				{
					// Read the 
					if(msgreply.prefix == ""){
						//empty string, normally ignore, but it could also be start of UDP transmission, SIM800L will only reply with "<CR><LF>> " (0x0d 0x0a 0x3e 0x20)
						rxState = WAIT_FOR_0x3E; 
						break;
					}else if(msgreply.prefix == "0, SEND OK"){
						this->status.pendingTXdata = true;
					}else if(msgreply.prefix == "0, SEND FAIL"){
						this->debugPrintTimeStamp("Sending Error!\n");
						linkState=WAIT_TO_SEND;
						this->status.txmsgReady = false;
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
						this->debugPrintTimeStamp("Reading " + String(this->status.incommingPackageLength)  + " bytes of data from SIM800L:");
						rxState=READ_INCOMMING_DATA;
						break;
					}else if(msgreply.prefix == "RDY"){
						//Serial.print("READY!!!\n");
						this->debugPrintTimeStamp("READY!!!\n");
						this->status.readyOk = true;
					}else if(msgreply.prefix.substring(0,5) == "+CFUN"){
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
							this->debugPrintTimeStamp("OK, CPIN READY\n");
							this->status.pinOk = true;	
						}else{
							this->debugPrintTimeStamp("Error, CPIN reply unknown:" + msgreply.prefix + ":\n");	
							this->debugPrintTimeStamp("Looking for:" + msgreply.prefix.substring(7,12) + ":\n");
						}
					}else if(msgreply.prefix == "Call Ready"){
						this->debugPrintTimeStamp("Call Ready\n");
						this->status.callOk = true;
					}else if(msgreply.prefix == "SMS Ready"){
						this->debugPrintTimeStamp("SMS Ready\n");
						this->status.smsOk = true;
						
						// List all known commands here:
					} //else if(msgreply.prefix == "AT+CGATT?" || msgreply.prefix == "AT+CIPMUX"){
					//	rxState = READ_RESPONSE1;
					//	break;
					else{
						// unknown:
//						this->debugPrint("Unknown prefix:" + msgreply.prefix + "\n");
						rxState = READ_RESPONSE1;
						break;
					}					
				}else{
					this->debugPrintTimeStamp("Error! - no LF after CR. - Restart statemachine!\n");
				}
				resetRXdata();
			break;
			
			case READ_RESPONSE1:
				if(readByte == 0x0D)
				{
					rxState = HANDLE_RESPONSE1;
				}else{
					this->msgreply.response1 += (char)readByte;
				}			
			break;
			
			case HANDLE_RESPONSE1:
				if(DEBUG_SIM800L){ this->debugPrint("\n"); }
					
				if(readByte == 0x0A)
				{
					// Read the response1.
					if(msgreply.response1 == "" || msgreply.response1 == "error" || msgreply.response1 == "ERROR"){
						this->debugPrintTimeStamp("Response 1 Error prefix:" +  msgreply.prefix + ": response1:" + msgreply.response1 + ": response2:N/A:\n");
					}else if(msgreply.response1 == "ok" || msgreply.response1 == "OK"){
						// enter here if response 1 is ok.
						if(msgreply.prefix == "AT+CIPMUX=1"){
							this->status.muxOk = true;	
						}else if(msgreply.prefix.substring(0,7) == "AT+CSTT"){
							this->status.apnOk = true;	
						}else if(msgreply.prefix == "AT+CIICR"){
							this->status.gprsEnabled = true;	
						}else if(msgreply.prefix.substring(0,11) == "AT+CIPSTART"){
							rxState = READ_RESPONSE2;
							break;
						}else if(msgreply.prefix == "ATE0"){
							this->status.ateSet = true;
						}
						else{
							this->debugPrintTimeStamp("Unknown prefix:" +  msgreply.prefix + ": response1:" + msgreply.response1 + ": response2:N/A:\n");
							linkState=RESET;
						}							
					}else if(msgreply.prefix == "AT+CGATT?"){		
							rxState = READ_RESPONSE2; 
							break;
					}else if(msgreply.prefix == "AT+CIICR"){ // If command doesn't reply with ok, then probberly +PDP: DEACT error, but we need to read next response.
							rxState = READ_RESPONSE2; 
							this->debugPrintTimeStamp("AT+CIICR response is not ok\n");
							break;					
					}else if(msgreply.prefix == "AT+CIFSR"){ // no OK from this command, just the IP.
						this->status.ip = msgreply.response1;	
						this->status.ipOk = true;						
					}else if(msgreply.prefix == "AT+CIPSHUT" && msgreply.response1 == "SHUT OK" ){ 
						this->status.shutdown = false;					
					}
					else{
						this->debugPrintTimeStamp("Response 1 Error prefix only:" + msgreply.prefix + ":\n");
						this->debugPrintTimeStamp("prefix again:");
						this->printStringAsHEXBytes(msgreply.prefix);
						this->debugPrint(": response1:");
						this->printStringAsHEXBytes(msgreply.response1);
						this->debugPrint(": response2:N/A:\n");
						this->debugPrint("Current linkState:" + String(linkState) + ":\n");
						linkState=RESET;
						this->status.txmsgReady == false; 
					}		
				}else{
					this->debugPrintTimeStamp("Error! - no LF after CR. - Restart statemachine!\n");
				}
				resetRXdata();
			break;
			
			case READ_RESPONSE2:
				if(readByte == 0x0D)
				{
					rxState = HANDLE_RESPONSE2;
				}else{
					this->msgreply.response2 += (char)readByte;
				}			
			break;
			
			case HANDLE_RESPONSE2:
				if(DEBUG_SIM800L){ this->debugPrint("\n"); }
				
				if(readByte == 0x0A)
				{
					//this->debugPrint("Handle response 2: prefix:" + msgreply.prefix + ": response1:" + msgreply.response1 + ": response2:" + msgreply.response2 + ":\n");	
					// Read the response1.
					if(msgreply.response2 == ""){ // skip if blank.
						rxState = READ_RESPONSE2;
						this->debugPrintTimeStamp("Empty string at response 2 - retry\n");
						break;
					}else if(msgreply.response2 == "error" || msgreply.response2 == "ERROR"){		
						this->debugPrintTimeStamp("AT command ERROR: prefix:" + msgreply.prefix + ": response1:" + msgreply.response1 + ": response2:" + msgreply.response2 + ":\n");
						if(msgreply.prefix == "AT+CIICR" && msgreply.response1 == "+PDP: DEACT"){
							linkState=PDP_DEACT;
						}
					}else if(msgreply.response2 == "ok" || msgreply.response2 == "OK"){
						if(msgreply.prefix == "AT+CGATT?"){							
							if(msgreply.response1.charAt(8) == '0'){
								this->status.GPRSConnected = false;	
								this->debugPrintTimeStamp("GPRS is not connected prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ":\n");
							}else if(msgreply.response1.charAt(8) == '1'){
								this->status.GPRSConnected = true;	
								this->debugPrintTimeStamp("GPRS is connected prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ":\n");
							}else{
								this->debugPrintTimeStamp("GPRS status unknown prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ":\n");
							}
						}
						else{
	                        this->debugPrintTimeStamp("Reply OK, but unknown prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ":\n");
						}
					}else if(msgreply.prefix.substring(0,11) == "AT+CIPSTART"){
						if(msgreply.response2 == "0, CONNECT OK"){
							this->status.GPRSConnected = true;	
							this->debugPrintTimeStamp("GPRS connected succesfully\n");
						}else{
							this->debugPrintTimeStamp("Error on AT+CIPSTART prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ": response2:" + msgreply.response2 + ":\n");					
						}
					}else{
						this->debugPrintTimeStamp("Reply Error prefix:" + msgreply.prefix + ": response:" + msgreply.response1 + ": response2:" + msgreply.response2 + ":\n");
					}										
				}else{
					this->debugPrintTimeStamp("Error! - no LF after CR. - Restart statemachine!\n");
				}
				resetRXdata();
			break;
			
			case WAIT_FOR_0x3E:
				if(readByte == 0x3E){ // 0x3E after <CR><LF> is pobberly because we started UDP transmission and are waiting for "> " (0x3E 0x20)
					rxState=WAIT_FOR_0x20; 
				}else{
					//this->debugPrint("Empty string\n"); // string was empty, restart but keep this read byte:
					resetRXdata();//Restart
					this->msgreply.prefix = (char)readByte; // keep the byte we just read.
				}
			break;
			
			case WAIT_FOR_0x20:
				if(readByte == 0x20){
					this->status.pendingTXdata=true;
				}else{
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
				this->debugPrintTimeStamp("Error! - Default\n");
				resetRXdata();//Restart
				linkState=RESET;
			break;
		}
	}	
}	

void SIM800L::serviceGPRSLink(void)
{
	switch (linkState)
	{
		case RESET: // Reset SIM800l
			this->debugPrintTimeStamp("SIM800L State: RESET->SETUP_MUX\n");
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
			this->status.ateSet = false;
			this->status.txmsgReady = false;
			ip = "";
			this->status.pendingTXdata  = false;
			
			// HARD reset SIM8800L
			digitalWrite(3, LOW);
			delay(2000);
			digitalWrite(3, HIGH);
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
			if(this->status.GPRSConnected == true){
				if(this->status.ateSet == false){
					this->SIM800Serial->print("ATE0\n"); // set no echo back.
					this->debugPrintTimeStamp("SIM800L State: IP_PROCESSING->WAIT_TO_SEND\n");
					linkState=WAIT_TO_SEND;
				}
			}				
		break;	

		case WAIT_TO_SEND:
			if(this->status.ateSet == true && this->status.txmsgReady == true && this->sendOnlyOnceDebug == true){
				//this->sendOnlyOnceDebug = false; // only send one mavlink to wake up the Qstation and read the send back.
				this->debugPrintTimeStamp("AT+CIPSEND=0," + String(this->msg.totalLength) + "\n");
				this->SIM800Serial->print("AT+CIPSEND=0," + String(this->msg.totalLength) + "\n");
				this->status.pendingTXdata  = false;
				this->debugPrintTimeStamp("SIM800L State: WAIT_TO_SEND->START_SEND\n");
				linkState=START_SEND;
			}
			
		break;	
		
		case START_SEND:
			if(this->status.pendingTXdata == true){
				this->debugPrintTimeStamp("Writing: " + String(this->msg.totalLength) + "bytes: ");
				for(uint16_t index=0; index < this->msg.totalLength; index++){
					this->SIM800Serial->write(this->msg.payload[index]);
					if(this->msg.payload[index] < 16){
						this->debugPrint(" 0" + String(this->msg.payload[index], HEX));
					}else{
						this->debugPrint(" " + String(this->msg.payload[index], HEX));
					}					
				}
				this->debugPrint(" Done writing data to SIM800L\n");
				this->debugPrintTimeStamp("SIM800L State: START_SEND->SENDING\n");
				this->status.pendingTXdata = false;				
				linkState=SENDING;
			}		
		break;	

		case SENDING:
			if(this->status.pendingTXdata == true){			
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
			ip = "";			
			linkState=IP_INITIAL;
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
}


ATReply_t SIM800L::writeData(uint8_t data[], uint16_t length, uint16_t timeout)
{
	if(DEBUG_SIM800L){
		Serial.print("\nWriting Data to SIM800L:     ");
	}
	
	for(uint16_t a=0; a<length; a++)
	{
		this->SIM800Serial->write(data[a]);	
		
		if(DEBUG_SIM800L)
		{		
			if(data[a] < 16){
				Serial.print("0");
			}
			Serial.print(data[a],HEX);
			Serial.print(" "); 
		}
	}
	/*
	if(timeout == 0){
		return true;
	}
	*/
	this->waitForReply(timeout);
	return this->msgreply;
}

ATReply_t SIM800L::sendCommand(String data, uint16_t timeout)
{
	this->SIM800Serial->print(data);
	this->waitForReply(timeout);
	return this->msgreply;
}
	
	
void SIM800L::waitForReply(uint16_t timeout)
{
	if(DEBUG_SIM800L){Serial.print("\nRX data from SIM800L: ");}
	
	this->msgreply.prefix = "";
	this->msgreply.response1 = "";
	this->msgreply.response2 = "";
	this->msgreply.error = true;
	state=LOOKING_FOR_START; 
	
	bool completed = false;
	uint32_t startTime=millis();
	uint8_t fastOK =0;
	
	while(!completed)	
	{
		if(millis() > startTime + timeout){
			completed=true;
			this->msgreply.response2 = "TIMEOUT";
			this->msgreply.error = true;
			break;
			/*
			Serial.print("\nSIM800L TIMEOUT!\n");
			Serial.print("\nPREFIX:\"" + this->msgreply.prefix  + "\"\n");
			Serial.print("RESPONSE1:\"" + this->msgreply.reponse1 + "\"\n");
			Serial.print("RESPONSE2:\"" + this->msgreply.reponse2 + "\"\n");*/
		}
			
		if(this->SIM800Serial->available())
		{
			readByte=this->SIM800Serial->read();
			
			if(DEBUG_SIM800L){
				if(readByte < 16){
					Serial.print("0");
				}
				Serial.print(readByte,HEX);
				Serial.print(" ");			
			}
			
			if(readByte == 0x3E){ // done if '>' (3E) is found.
				completed=true;
				this->msgreply.response2 = ">";
				this->msgreply.error = false;
				break;
			}
			/*   			O                                       K                                       <CR>  									  <CR>
			if(    ((readByte == 0x4F) && (fastOK == 0)) || ( (readByte == 0x4B) && (fastOK == 1) ) || ( (readByte == 0x0D) && (fastOK == 2) ) || ( (readByte == 0x0A) && (fastOK == 3) ) ){ // fast ok from UDP
				fastOK++;
				if(fastOK == 4){ // OK<CR><LF> found
					this->msgreply.response2 = "OK";
					completed=true;	
					this->msgreply.error = false;
					break;
				}
			}else{
				fastOK=0; // reset
			}
			*/
			
			switch (state)
			{
				case LOOKING_FOR_START: // look for at || AT start or 0 (for send ok)
				if(readByte == 0x41 || readByte == 0x61 || readByte == 0x20)
				{
					this->msgreply.prefix = "";
					this->msgreply.response1 = "";
					this->msgreply.response2 = "";
					this->msgreply.prefix += (char)readByte;
					if(readByte == 0x20){
						state=SEND_RESPONSE; // Start of Send reply
					}else{
						state=PREFIX; // Start of string found!							
					}
				}
				break;
				
				case PREFIX: // Read until the first <CR>
				if(readByte == 0x0D)
				{
  					state=LINEFEED1; // Start of string found!
				}else
				{
					this->msgreply.prefix += (char)readByte;
				}
				break;		

				case LINEFEED1: // Next char must be linefeed or else?
				if(readByte == 0x0A)
				{
					state=RESPONSE1; // Start of string found!
				}else
				{
					Serial.print("KRISE!");
					state=LOOKING_FOR_START; //REstart
				}
				break;		
				
				case RESPONSE1: // Read until next <CR>
				if(readByte == 0x0D)
				{
  					state=LINEFEED2; 
				}else
				{
					this->msgreply.response1 += (char)readByte;
				}
				break;	
				
				case LINEFEED2: // Next char must be linefeed or else?
				if(readByte == 0x0A)
				{
					if(this->msgreply.response1 == "OK" || this->msgreply.response1 == "ok") // Done no more commands
					{
						/*
						Serial.print("\nPREFIX:\"" + this->msgreply.prefix = ""; + "\"\n");
						Serial.print("RESPONSE1:\"" + this->msgreply.response1 + "\"\n");
						*/
						this->msgreply.error = false;
						completed=true;
						state=LOOKING_FOR_START; //Restart
					}else if (this->msgreply.response1 == "ERROR" || this->msgreply.response1 == "error" ){
						/*
						Serial.print("\n---- Error ----\n");
						Serial.print("\nPREFIX:\"" + this->msgreply.prefix = ""; + "\"\n");
						Serial.print("RESPONSE1:\"" + this->msgreply.response1 + "\"\n");
						*/
						completed=true;
						state=LOOKING_FOR_START; //REstart
					}else if (this->msgreply.response1 == "0, SEND OK" || this->msgreply.response1 == "0, send ok" ){
						completed=true;
						this->msgreply.error = false;
						state=LOOKING_FOR_START; //Restart
					}else if(this->msgreply.response1 == "+CGATT: 0"){
						completed=true;
						this->msgreply.error = false;
						state=LOOKING_FOR_START; //Restart
					}else if(this->msgreply.response1.substring(0,5) == "+CSQ:"){
						completed=true;
						this->msgreply.error = false;
	//					Serial.print("Substring!");
						state=LOOKING_FOR_START; //Restart
					}else if(this->msgreply.response1.substring(0,8) == "+RECEIVE"){
						completed=true;
						this->msgreply.error = false;
	//					Serial.print("Substring!");
						state=LOOKING_FOR_START; //Restart
					}else
					{
						//state=RESPONSE2; // Start of string found!	//ERROR!
						completed=true;
						state=LOOKING_FOR_START; //Restart
					}					
				}else
				{
					Serial.print("KRISE2!");
					state=LOOKING_FOR_START; //Restart
				}
				break;	
				
				case RESPONSE2: // Read until next <CR>
				if(readByte == 0x0D)
				{
					if(this->msgreply.response2 == "")
					{
						state=LINEFEED2; 	
					}else{
						state=END; 	
					}
  					
				}else
				{
					this->msgreply.response2 += (char)readByte;
				}
				break;	
				
				case SEND_RESPONSE: // Read until next <CR>
				if(readByte == 0x0D)
				{
					state=LINEFEED1; 						
				}else
				{
					this->msgreply.prefix += (char)readByte;
				}
				break;	
				
				case END: // Read until next <CR>
				if(readByte == 0x0A)
				{
					if(this->msgreply.response2 == "OK" || this->msgreply.response2 == "ok") // Done no more commands
					{
						/*
						Serial.print("\nPREFIX:\"" + this->msgreply.prefix = ""; + "\"\n");
						Serial.print("RESPONSE1:\"" + this->msgreply.response1 + "\"\n");
						*/
						this->msgreply.error = false;
						completed=true;
					}
					completed=true;
					state=LOOKING_FOR_START; //REstart
				}else
				{
					Serial.print("KRISE3!");
					state=LOOKING_FOR_START; //REstart
				}
				break;	
				
				default:
					Serial.print("Default");
					state=LOOKING_FOR_START; //REstart
				break;
			}
		}
	}
	if(DEBUG_SIM800L){Serial.print(" Prefix:" + this->msgreply.prefix + ": Response1:" + this->msgreply.response1 + ": Response2:" + this->msgreply.response2 + ":");}
}

/*
void SIM800L::sendData(uint8_t *data, uint16_t length)
{

}
*/
String SIM800L::base64_encode(uint8_t bytes_to_encode[], int in_len)
{
  String ret = "";
  int i = 0;
  int j = 0;
  uint8_t char_array_3[3];
  uint8_t char_array_4[4];
  int place = 0;

  while (in_len-- > 0) {
    char_array_3[i++] = bytes_to_encode[place++];
    if (i == 3) {
      char_array_4[0] = (uint8_t)((char_array_3[0] & 0xfc) >> 2);
      char_array_4[1] = (uint8_t)(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
      char_array_4[2] = (uint8_t)(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
      char_array_4[3] = (uint8_t)(char_array_3[2] & 0x3f);

      for(i = 0; (i<4) ; i++)
      ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i > 0) {
    for(j = i; j< 3; j++)
      char_array_3[j] = 0;

    char_array_4[0] = (uint8_t)(( char_array_3[0] & 0xfc) >> 2);
    char_array_4[1] = (uint8_t)(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
    char_array_4[2] = (uint8_t)(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));

    for (j = 0; (j<i + 1); j++)
      ret += base64_chars[char_array_4[j]];   

    while((i++ < 3))
      ret += '=';
  }

  return ret;
}



/*
String sim800l::getUDPData(void){
		return this->outputData;
}
*/
