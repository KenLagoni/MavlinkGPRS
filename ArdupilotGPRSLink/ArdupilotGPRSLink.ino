#include "SIM800L.h"
#include "wiring_private.h" // pinPeripheral() function
#include "mavlinkArduino.h"
#include "mavPackageFilter.h" // for mavFilter
#include "SDlogger.h" // SDLogger class to SD card logging.

// SD Card:
#define PIN_SD_CS 28

// Serial3 pin and pad definitions (in Arduino files Variant.h & Variant.cpp)
#define PIN_SERIAL3_RX       (1ul)               // Pin description number for PIO_SERCOM on D1
#define PIN_SERIAL3_TX       (0ul)               // Pin description number for PIO_SERCOM on D0
#define PAD_SERIAL3_TX       (UART_TX_PAD_0)      // SERCOM pad 0 (PA22 or Arduino pin 0 on MKRzero)
#define PAD_SERIAL3_RX       (SERCOM_RX_PAD_1)    // SERCOM pad 1 (PA23 or Arduino pin 1 on MKRzero)
#define PIN_SIM800L_RESET (3ul)
#define MAXLINE 1024 

// Instantiate the Serial3 class
Uart Serial3(&sercom3, PIN_SERIAL3_RX, PIN_SERIAL3_TX, PAD_SERIAL3_RX, PAD_SERIAL3_TX); // For SIM800L

//Serial1 for Ardupilot.
//Serial3 for SIM800l

// Filter settings:
void setMavFilterFromPixHawkRules(MavPackageFilter& filter);
void setMavFilterFromSIM800LRules(MavPackageFilter& filter);

uint8_t buf[MAXLINE]; 
void sendHeartBeatToPixHawk(void);
void requestDataStream(void);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // debug log output
  Serial3.begin(115200);
  pinPeripheral(PIN_SERIAL3_RX, PIO_SERCOM); 
  pinPeripheral(PIN_SERIAL3_TX, PIO_SERCOM);
  Serial1.begin(57600); // for Pixhawk.  
}

void loop() { 
  delay(2000);
  Serial.print("\nInitializing SD card...");  
  
  SDLogger SDlog; 
  if(SDlog.begin(PIN_SD_CS, "log.txt", LOG_APPEND)){
    Serial.print("Card failed, not present or mode not supported!\n");
    do{}while(1); 
  }
  Serial.print("Initialized.\n"); 
  Serial.print("Starting new log at:" +  SDlog.getFileName() + ":\n");

  SIM800L ModemGPRS(&Serial3, 115200, PIN_SIM800L_RESET, "", "[SIM APN]", "[IP FOR HOME ROUTER]", 14450, "UDP", 100, 2000);
 
  // how to filter packages.
  MavPackageFilter dataFromPixhawkFilter;
  MavPackageFilter dataFromSIM800LFilter;
  
  setMavFilterFromPixHawkRules(dataFromPixhawkFilter);
  setMavFilterFromSIM800LRules(dataFromSIM800LFilter); 
  
  SDlog.print("Starting Mavlink to GPRS program\n");
  
  //Mavlink Parser for Pixhawk
  mavlink_status_t statusPix;
  mavlink_message_t msgPix;
  
  //Mavlink Parser SIM800L
  mavlink_status_t statusSim;
  mavlink_message_t msgSim;
  //  mavlink parser buffer and time service counter.
  uint32_t nextTime=millis()+5000;
  uint16_t totalLength=0;
  uint32_t msgCount[256];
  for(int a=0;a<256;a++)
    msgCount[a]=0;
    
  do{
    if(Serial1.available()){ // service serial data from pixhawk.
      uint8_t rbyte= Serial1.read();
      
      if (mavlink_parse_char(MAVLINK_COMM_0, rbyte, &msgPix, &statusPix))
      {
        msgCount[msgPix.msgid]++;
        switch(dataFromPixhawkFilter.getPackageFilter(msgPix.msgid))
        {
          case FORWARD:
          {
  //          Serial.print("\n" + String(millis())+":MAIN:Forwarding  MSG from Pixhawk ID" + String(msgPix.msgid));
            totalLength = mavlink_msg_to_send_buffer(buf, &msgPix);   
            for(int a=0;a<totalLength;a++)
              ModemGPRS.write(buf[a]); // send the raw package to SIM800L.
          }
          break;
          
          case HANDLE:  
          {            
            if(msgPix.msgid != 35) 
              Serial.print("\n" + String(millis())+":MAIN:Hanlding    MSG from Pixhawk ID" + String(msgPix.msgid));

            if(MAVLINK_MSG_ID_RC_CHANNELS_RAW == msgPix.msgid){ // inject RSSI to MSG35
              mavlink_rc_channels_raw_t newmsg;
              mavlink_msg_rc_channels_raw_decode(&msgPix, &newmsg);
              newmsg.rssi = ModemGPRS.getRSSI();
              mavlink_msg_rc_channels_raw_encode(msgPix.sysid, msgPix.compid, &msgPix, &newmsg);
              totalLength = mavlink_msg_to_send_buffer(buf, &msgPix); 
              for(int a=0;a<totalLength;a++){
                 ModemGPRS.write(buf[a]); // send the package to SIM800L after RSSI has been replaced.
              }     
            }
            if(MAVLINK_MSG_ID_STATUSTEXT == msgPix.msgid){ // #253 long text (at startup lets just send it.
              totalLength = mavlink_msg_to_send_buffer(buf, &msgPix); 
              for(int a=0;a<totalLength;a++){
                 ModemGPRS.write(buf[a]); // send the package to SIM800L after RSSI has been replaced.
              }   

            }
 /*           
            if(MAVLINK_MSG_ID_COMMAND_ACK == msgPix.msgid){
              mavlink_command_ack_t cmdAck;
              mavlink_msg_command_ack_decode(&msgPix, &cmdAck);
              Serial.print("\n" + String(millis()) + ":MAIN:Command Ack Comamnd=" + String(cmdAck.command) + " Result=" + String(cmdAck.result));
             
              totalLength = mavlink_msg_to_send_buffer(buf, &msgSim); 
              for(int a=0;a<totalLength;a++)
                 ModemGPRS.write(buf[a]); // send the raw package to SIM800L.
            }            
            */
          }
          break;
  
          case DISCARD:
          {   
          //  Serial.print("\n" + String(millis())+":MAIN:Discharding MSG from Pixhawk ID" + String(msgPix.msgid));
          }
          break;
  
          case UNKONWN_PACKAGE:   
          {
            Serial.print("\n" + String(millis())+":MAIN:Unkown      MSG from Pixhawk ID" + String(msgPix.msgid));
          }
          default:  
          
          break;
        }             
      } 
    } // service serial data from pixhawk.
    
  
    if(ModemGPRS.service()){ // service SIM800L module.
      uint8_t rbyte = ModemGPRS.read();
      if (mavlink_parse_char(MAVLINK_COMM_0, rbyte, &msgSim, &statusSim))
      {
        switch(dataFromSIM800LFilter.getPackageFilter(msgSim.msgid))
        {
          case FORWARD:
          {
 //           Serial.print("\n" + String(millis())+":MAIN:ForwardD    MSG from SIM800L ID" + String(msgSim.msgid));
            totalLength = mavlink_msg_to_send_buffer(buf, &msgSim);   
            for(int a=0;a<totalLength;a++)
              Serial1.write(buf[a]);
          }
          break;
          
          case HANDLE:   
            Serial.print("\n" + String(millis())+":MAIN:Handling    MSG from SIM800L ID" + String(msgSim.msgid));
            if(MAVLINK_MSG_ID_COMMAND_LONG == msgSim.msgid){
              mavlink_command_long_t cmdLong;
              mavlink_msg_command_long_decode(&msgSim, &cmdLong);
              Serial.print("\n" + String(millis())+":MAIN:Command Long Param1=" + String(cmdLong.param1) + " Param2="+ String(cmdLong.param2) + " Param3="+
                                  String(cmdLong.param3) +" Param4="+ String(cmdLong.param4) +" Param5="+ String(cmdLong.param5) + " Param6="+ String(cmdLong.param6) +" Param7="+ String(cmdLong.param7) +
                                  " Command=" + String(cmdLong.command) + " target_system=" +  String(cmdLong.target_system) + " target_component="+  String(cmdLong.target_component) + " confirmation=" + String(cmdLong.confirmation));
              if(cmdLong.command = 519){ // MAV_CMD_REQUEST_PROTOCOL_VERSION
                totalLength = mavlink_msg_to_send_buffer(buf, &msgSim); 
                for(int a=0;a<totalLength;a++)
                 Serial1.write(buf[a]);               
              }
              
              if(cmdLong.command = 520){ // MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES (MSG #148)
                totalLength = mavlink_msg_to_send_buffer(buf, &msgSim); 
                for(int a=0;a<totalLength;a++)
                 Serial1.write(buf[a]);               
              }
            }                   

          break;
    
          case DISCARD:   
 //           Serial.print("\n" + String(millis())+":MAIN:Discharding MSG from SIM800L ID" + String(msgSim.msgid));
          break;
    
          
          case UNKONWN_PACKAGE:   
          default: 
            Serial.print("\n" + String(millis())+":MAIN:Unkown      MSG from SIM800L ID" + String(msgSim.msgid));
          break;
        }          
      } 
    }// service SIM800L module.
  
 
    //Every 3 second houskeeping.
    if(millis() >= nextTime){     
      nextTime=millis() + 3000;
      //String logmsg = "\n" + String(millis())+":SIM800L:" + ModemGPRS.getStatusMSG();
      String logmsg = String(millis())+":SIM800L:" + ModemGPRS.getConnectionStatus();
/*
      // print out message status:
      Serial.print("\n" + String(millis())+":MAIN:Message Statistics:");
      uint8_t spaces =0;
      for(int a = 0;a<256;a++){
        if(msgCount[a] > 0){
          if(a>=100){          
            Serial.print("\nMSG with ID=" + String(a)+ " has been sent times=" +String(msgCount[a]));
          }else if(a>=10){          
            Serial.print("\nMSG with ID=" + String(a)+ "  has been sent times=" +String(msgCount[a]));
          }else{
            Serial.print("\nMSG with ID=" + String(a)+ "   has been sent times=" +String(msgCount[a]));
          }

          if(msgCount[a] >= 100){
            spaces=5;
          }else if(msgCount[a] >=10){
            spaces=6;
          }else{
            spaces=7;
          }

          for(int b=0;b<spaces;b++)
            Serial.print(" ");
          
          if(dataFromPixhawkFilter.getPackageFilter(a) == DISCARD){
            Serial.print("<DISCARD>");
          }else if(dataFromPixhawkFilter.getPackageFilter(a) == UNKONWN_PACKAGE){
            Serial.print("<UNKONWN_PACKAGED>");   
          }else if(dataFromPixhawkFilter.getPackageFilter(a) == FORWARD){
            Serial.print("<FORWARD>"); 
          }else if(dataFromPixhawkFilter.getPackageFilter(a) == HANDLE){
            Serial.print("<HANDLE>"); 
          }
        }
      }
      Serial.print("\n");
  */    
      Serial.print(logmsg);
    
      // print status message to log
      SDlog.print(logmsg);
 //     SDlog.flush();
    } 
  }while(1);
}


void setMavFilterFromPixHawkRules(MavPackageFilter& filter){

  //  Packages that we should just forward:
  filter.setPackageFilter(MAVLINK_MSG_ID_HEARTBEAT, FORWARD);   //#0
  filter.setPackageFilter(MAVLINK_MSG_ID_SYS_STATUS, FORWARD);  //#1
  filter.setPackageFilter(MAVLINK_MSG_ID_PARAM_VALUE, FORWARD); //#20
  filter.setPackageFilter(MAVLINK_MSG_ID_GPS_RAW_INT, FORWARD); //#24  GPS RAW INIT. Fro SAT count and HDOP
  filter.setPackageFilter(MAVLINK_MSG_ID_ATTITUDE, FORWARD);            //#30 
  filter.setPackageFilter(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, FORWARD); //#33  

  // mission stuff:
  filter.setPackageFilter(MAVLINK_MSG_ID_MISSION_ITEM, FORWARD); //#33  
  filter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST, FORWARD);        //#42 
  filter.setPackageFilter(MAVLINK_MSG_ID_MISSION_CURRENT, FORWARD);        //#42 
  filter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST_LIST, FORWARD);   //#43
  filter.setPackageFilter(MAVLINK_MSG_ID_MISSION_COUNT, FORWARD);          //#44 
  filter.setPackageFilter(MAVLINK_MSG_ID_MISSION_ITEM_REACHED, FORWARD);   //#44  
  filter.setPackageFilter(MAVLINK_MSG_ID_MISSION_ACK, FORWARD);            //#47
    
  filter.setPackageFilter(MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN, FORWARD);      //#49  Publishes the GPS co-ordinates of the vehicle local origin (0,0,0) position. Emitted whenever a new GPS-Local position mapping is requested or set - e.g. following SET_GPS_GLOBAL_ORIGIN message.
  filter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST_INT, FORWARD);    //#51  
  filter.setPackageFilter(MAVLINK_MSG_ID_VFR_HUD, FORWARD);                //#74  VFR_HUD
  filter.setPackageFilter(MAVLINK_MSG_ID_TERRAIN_REPORT, FORWARD);         //#136 Response from a TERRAIN_CHECK request  
  filter.setPackageFilter(MAVLINK_MSG_ID_BATTERY_STATUS, FORWARD);         //#147 Battery status.
  filter.setPackageFilter(MAVLINK_MSG_ID_AUTOPILOT_VERSION, FORWARD);      //#148 Autopilot version, this is requested by CMD#76 command=520.
  filter.setPackageFilter(MAVLINK_MSG_ID_HOME_POSITION, FORWARD);          //#242 This message can be requested by sending the MAV_CMD_GET_HOME_POSITION command.  The position the system will return to and land on. 
  filter.setPackageFilter(MAVLINK_MSG_ID_STATUSTEXT, FORWARD);             // #253 Lets see if this one comes :-)
  
  
    
  //  Packages that we should inspect:
  filter.setPackageFilter(MAVLINK_MSG_ID_RC_CHANNELS_RAW, HANDLE); // #35 Inject SIM800 RSSI to this package.
  filter.setPackageFilter(MAVLINK_MSG_ID_RADIO_STATUS, HANDLE);    // #109 RADIO STATUS, not seen from pixhawk. Perhaps only relevant if it is the radio RSSI (not the RC RSSI).
  filter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_LONG, HANDLE);    // #76 CMD Long. // perhabs a command to me? :-)
  filter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_ACK, HANDLE);     // #77 CMD ACK. 
 // filter.setPackageFilter(MAVLINK_MSG_ID_STATUSTEXT, HANDLE);      // #253 Lets see i this one comes :-)

     
  //  Packages that we should dishard:
  filter.setPackageFilter(MAVLINK_MSG_ID_VIBRATION, DISCARD);                  // Vibration, ignor this.
  filter.setPackageFilter(MAVLINK_MSG_ID_SYSTEM_TIME, DISCARD);                //#2
//  filter.setPackageFilter(MAVLINK_MSG_ID_GPS_RAW_INT, DISCARD);                //#24  GPS RAW INIT.
  filter.setPackageFilter(MAVLINK_MSG_ID_RAW_IMU, DISCARD);                    //#27  RAW IMU.
  filter.setPackageFilter(MAVLINK_MSG_ID_SCALED_PRESSURE, DISCARD);            //#29  The pressure readings for the typical setup of one absolute and differential pressure sensor.
  filter.setPackageFilter(MAVLINK_MSG_ID_SERVO_OUTPUT_RAW, DISCARD);           //#36  The RAW values of the servo outputs 
  filter.setPackageFilter(MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT, DISCARD);      //#62  The state of the fixed wing navigation and position controller.
  filter.setPackageFilter(MAVLINK_MSG_ID_RC_CHANNELS, DISCARD);                //#65  The PPM values of the RC channels received.
  filter.setPackageFilter(MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT, DISCARD); //#87  Reports the current commanded vehicle position, velocity, and acceleration as specified by the autopilot. 
  filter.setPackageFilter(MAVLINK_MSG_ID_TIMESYNC, DISCARD);                   //#111 Time synchronization message.
  filter.setPackageFilter(MAVLINK_MSG_ID_SCALED_IMU2, DISCARD);                //#116 The RAW IMU readings for secondary 9DOF sensor setup.
  filter.setPackageFilter(MAVLINK_MSG_ID_POWER_STATUS, DISCARD);               //#125 Power Status. (5V and Servo rail) - not relevant.
}

void setMavFilterFromSIM800LRules(MavPackageFilter& filter){
  filter.setAllPackageFilter(FORWARD);
//  filter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_INT, HANDLE); // #75 Command Int.   
  filter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_LONG, HANDLE); // #76
}

void requestDataStream(void){
  mavlink_message_t msg;
  mavlink_request_data_stream_t streammsg;
  streammsg.req_message_rate = 10;
  streammsg.target_system = 1;
  streammsg.target_component = 1;
  streammsg.req_stream_id = 30;
  streammsg.start_stop = 1;
  mavlink_msg_request_data_stream_encode(0xFF, 0xBE, &msg, &streammsg);
  uint16_t totalLength = mavlink_msg_to_send_buffer(buf, &msg); 
  for(int a=0;a<totalLength;a++)
    Serial1.write(buf[a]);    
}

void sendHeartBeatToPixHawk(void){
  mavlink_message_t msg;
  mavlink_heartbeat_t heartbeat;
  heartbeat.custom_mode=0;
  heartbeat.type = MAV_TYPE_GCS; //
  heartbeat.autopilot = MAV_AUTOPILOT_INVALID;
  heartbeat.base_mode = MAV_MODE_FLAG_SAFETY_ARMED + MAV_MODE_FLAG_MANUAL_INPUT_ENABLED;
  heartbeat.system_status = MAV_STATE_ACTIVE;
  mavlink_msg_heartbeat_encode(0xFF, 0xBE, &msg, &heartbeat);
  uint16_t totalLength = mavlink_msg_to_send_buffer(buf, &msg); 
  for(int a=0;a<totalLength;a++)
    Serial1.write(buf[a]);    
}

/*
 * 
 * 
  //// test
  for(;;){
    if(Serial1.available()){ // service serial data from pixhawk.
        uint8_t rbyte= Serial1.read();
        
        if (mavlink_parse_char(MAVLINK_COMM_0, rbyte, &msgPix, &statusPix)){
          msgCount[msgPix.msgid]++;
        }
    }
    //Every 5 second houskeeping.
    if(millis() >= nextTime){     
      nextTime=millis() + 1000;
    
      // print out message status:
      Serial.print("\n" + String(millis())+":MAIN:Message Statistics:");
      uint8_t spaces =0;
      for(int a = 0;a<256;a++){
        if(msgCount[a] > 0){
          if(a>=100){          
            Serial.print("\nMSG with ID=" + String(a)+ " has been sent times=" +String(msgCount[a]));
          }else if(a>=10){          
            Serial.print("\nMSG with ID=" + String(a)+ "  has been sent times=" +String(msgCount[a]));
          }else{
            Serial.print("\nMSG with ID=" + String(a)+ "   has been sent times=" +String(msgCount[a]));
          }
    
          if(msgCount[a] >= 100){
            spaces=5;
          }else if(msgCount[a] >=10){
            spaces=6;
          }else{
            spaces=7;
          }
    
          for(int b=0;b<spaces;b++)
            Serial.print(" ");
          
          if(dataFromPixhawkFilter.getPackageFilter(a) == DISCARD){
            Serial.print("<DISCARD>");
          }else if(dataFromPixhawkFilter.getPackageFilter(a) == UNKONWN_PACKAGE){
            Serial.print("<UNKONWN_PACKAGED>");   
          }else if(dataFromPixhawkFilter.getPackageFilter(a) == FORWARD){
            Serial.print("<FORWARD>"); 
          }else if(dataFromPixhawkFilter.getPackageFilter(a) == HANDLE){
            Serial.print("<HANDLE>"); 
          }
        }
      }
      Serial.print("\n");
      sendHeartBeatToPixHawk();
    }
  }
 */

void SERCOM3_Handler()    // Interrupt handler for SERCOM3
{
  Serial3.IrqHandler();
}
