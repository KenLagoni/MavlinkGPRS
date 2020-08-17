/*
	sim800l.h

	Copyright (c) 2019 Lagoni
	Not for commercial use
 */ 

#ifndef SIM800L_h
#define SIM800L_h

#include "Arduino.h" // Needed for Strings and Serial
//#include "MavlinkData.h"
#include <SD.h> 	 // for File
#include "MavlinkLite.h" // for SerialData struct

#define DEBUG_SIM800L 1
#define SERIAL_ENALBED 1
#define PRINT_RXDATA 1
#define LOG_DATA 0

#define SOFT_TIMEOUT          500  //  500ms
#define SOFT_TIMEOUT_RECOVER 3000  // 3000ms (Time to recover from soft reset)
#define HARD_TIMEOUT_COUNT     10  //  Number of softresets in a row to trigger hard reset.

#define RSSI_REQURST         1000  // 1000ms

#define SIM800L_DATA_BUFFER_SIZE      512
#define SEND_WINDOW_TIMEFREQ	100 // every 100ms data must be send, if any.	

struct ATReply_t
{
	String prefix;
	String response1;
	String response2;
	bool error;
};

struct ATdataBuffer_t
{
	uint8_t data[SIM800L_DATA_BUFFER_SIZE];
	uint16_t length;
};

struct SIM800LStatus_t
{
	bool readyOk;
	bool cfunOk;
	bool pinOk;
	bool callOk;
	bool smsOk;
	bool muxOk;
	bool apnOk;	
	bool gprsEnabled;
	bool ipOk;
	bool GPRSConnected;
	bool shutdown;
	bool ateSet;
	String ip;
	bool txmsgReady;
	bool pendingTXdata;
	uint16_t incommingPackageLength;
	uint8_t connection = 0;
	bool rssiReceived;
	bool error = false;
	bool linkReady;
	uint32_t timeToSend;
};
	
class SIM800L
{
  public:
    SIM800L();
	void init(HardwareSerial *ptr1, unsigned long baudrate, File *fileptr, File *fileptrRX, int _resetPin);
	void service();
	ATReply_t writeData(uint8_t data[], uint16_t length, uint16_t timeout);

	// new:
	void startUdpSocket(String ip, String port, String apn);
	bool writeUdpData(SerialData_t msg); // true if msg is dischared.
//	bool writeUdpData(uint8_t *data, uint16_t length); // true if msg is dischared.
	void logRXdata(String data);
	uint8_t getRSSI(void);
	bool isReadyToSend();
	//readUdpData(void);

  private:
  
  void switchTxBuffer(void);
  uint8_t bufferPointer=0;
  ATdataBuffer_t *TXoutput = NULL;
  ATdataBuffer_t *TXinput = NULL;
  ATdataBuffer_t TXbuf1;
  ATdataBuffer_t TXbuf2;
  
  ATdataBuffer_t RXmsg;
  File *logFile = NULL;
  File *rxDataFile = NULL;
  
  uint16_t charcount = 0;
  uint16_t charcount2 = 0;
  void printStringAsHEXBytes(String data);
  int resetPin;
  void restart(void);
  void clearTimeout(void);
  
  enum StateMachine{
	  LOOKING_FOR_START,
	  PREFIX,
	  LINEFEED1,
	  LINEFEED2,
	  LINEFEED3,
	  RESPONSE1,
	  RESPONSE2,
	  SEND_RESPONSE,
	  END
  };
  
  
  enum GPRSLinkState{
	  RESET=1,
	  SETUP_MUX,
	  IP_INITIAL,
	  IP_START,
	  IP_CONFIG,
	  IP_GPRSACT,
	  IP_STATUS,
	  IP_PROCESSING,
	  SET_ATE,
	  WAIT_TO_SEND,
	  START_SEND,
	  SENDING,
	  PDP_DEACT	,
	  CONNECTION_STATUS,
	  GET_RSSI
  };

  enum ATReadSIM800L{
	  LOOKING_FOR_CR,
	  READ_PREFIX,
	  READ_RESPONSE1,
	  HANDLE_RESPONSE1,
	  READ_RESPONSE2,
	  HANDLE_RESPONSE2,
	  WAIT_FOR_0x20,
	  WAIT_FOR_0x3E,
	  READ_INCOMMING_DATA,
	  READ_ACK
  };

  SIM800LStatus_t status;
  ATReply_t msgreply;
  
  StateMachine state = LOOKING_FOR_START; // ENUM used by state maschine in update();
  ATReadSIM800L rxState = LOOKING_FOR_CR;
  GPRSLinkState linkState = RESET;

  HardwareSerial  *SIM800Serial = NULL;       // Pointer to the serial port used
  
  void resetRXdata(void);
  void serviceGPRSLink(void);
  void serviceRXdata(void);
  String outputData;
  void debugPrint(String);
  void debugPrintTimeStamp(String);
  
  uint16_t CRC = 0;			// Variable for calculated CRC.
  uint16_t CRC_RESULT = 0;	// Variable for calculated CRC result.
  uint16_t index=0;
  uint8_t readByte=0;
  
  // for UDP GPRS
  String ip;
  String port;
  String apn;
  
  //Send error count:
  uint32_t sendErrorCount =0;
  
  //timers
  uint32_t nextRSSIrequest = 0;
  uint32_t nextLoopTimeout = 0;
  uint32_t restartCounter = 0;
  
  // Connection status;
  String conStatus = "";
  uint8_t rssi = 0;
  uint8_t ber = 0;
  
  //debugPrint
  uint32_t timetoprint=0;
};

	
#endif
