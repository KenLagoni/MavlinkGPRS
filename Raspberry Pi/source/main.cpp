// Server program 
#include <arpa/inet.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 

#include <sys/time.h>

#include "parameters.h"
#include "deviceServer.h"
#include "deviceClient.h"
#include "connection.h"

#include <list> // for clients "array"

#include <iostream>  
#include <string> 

#define PORT 6000 

#define MAX_CLIENTS 2 

int max(int x, int y) 
{ 
    if (x > y) 
        return x; 
    else
        return y; 
} 

int main(int argc, char** argv) 

	// Input arguments ./main [GPRS INPUT UDP PORT] [TCP LISTEN PORT] [UDP CLIENT PORT] [WAYPOINT FILE] [R/W] [PARAMETER FILE] [R/W]
	// ./main 14450 5760 6500 waypoints.txt -R parameters.txt -R
	// argv[0] 	./main - not used
	// argv[1] 	[GPRS INPUT UDP PORT] must be same as SIM800L transmitting port (14450)
	// argv[2] 	[TCP LISTEN PORT] default MP and QGC (5760)
	// argv[3] 	[UDP CLIENT PORT] pick a port (6500)
	// argv[4] 	[WAYPOINT FILE] "waypoint.txt" load or save waypoint here.
	// argv[5] 	-R means load waypoint from file and don't ask drone. -W means get waypoints from drone and write to file.
	// argv[6] 	[PARAMETER FILE] "parameter.txt" load or save parameters here.
	// argv[7] 	-R means load parameters from file and don't ask drone. -W means get parameters from drone and write to file.
{
    int nready, maxfdp1; 
    char buffer[MAXLINE]; 
    fd_set rset; 
    ssize_t n; 

	char *p;
	int gprsPort = strtol(argv[1], &p, 10);
	int tcpPort = strtol(argv[2], &p, 10);
	int udpClientPort= strtol(argv[3], &p, 10);	
//	std::string waypointFile(argv[4]);
	std::string waypointFileMode(argv[5]);
//	std::string parameterFile = argv[6];
	std::string parameterFileMode(argv[7]);

	printf("Starting Lagoni's Drone via. GPRS server\n");

	int i;
    for(i=0;i<argc-1;i++)
    {
        printf("Input Argument %d:%s:\n",argc,argv[i]);
    }
/*	
	struct timeval  timeout;
	timeout.tv_sec = 5; 
    timeout.tv_usec = 0; 
*/	
	// classe for connection to TCP listen socket:
	Connection tcpListen(tcpPort, SOCK_STREAM);		
	
	// Initilize Paraemter list
	DataMode_t parameterMode = READ_ONLY;
	if((parameterFileMode.compare("-W") == 0) || (parameterFileMode.compare("-w") == 0)){
		printf("Parameter file will be download from drone and saved to %s\n", argv[6]);	
		parameterMode = UPDATE;
	}else if((parameterFileMode.compare("-R") == 0) || (parameterFileMode.compare("-r") == 0)){
		printf("Parameter file be read from file %s and not download.\n", argv[6]);	
		parameterMode = READ_ONLY;
	}else{
		printf("Parameter file mode is not correct, use -R or -W\n");
		return 0;	
	}
	Parameters parametersList("", argv[6], parameterMode);	

	// Initilize waypoint list
	DataMode_t waypointMode = READ_ONLY;
	if((waypointFileMode.compare("-W") == 0) || (waypointFileMode.compare("-w") == 0)){
		printf("Waypoint file will be download from drone and saved to %s\n", argv[4]);	
		waypointMode = UPDATE;
	}else if((waypointFileMode.compare("-R") == 0) || (waypointFileMode.compare("-r") == 0)){
		printf("Waypoint file be read from file %s and not download.\n", argv[4]);	
		waypointMode = READ_ONLY;
	}else{
		printf("Waypoint file mode is not correct, use -R or -W\n");
		return 0;	
	}
	Waypoints waypointList("", argv[4], waypointMode);	
	
	
	DeviceServer gprsLink(gprsPort, SOCK_DGRAM, "GPRSLink", &parametersList, &waypointList);
	
	std::list<DeviceClient> listofClients = {
		DeviceClient(udpClientPort, SOCK_DGRAM, "UDPClients", &parametersList, &waypointList)
	};
	
	
	printf("The following Clients are on the list:\n");	
	for(std::list<DeviceClient>::iterator it = listofClients.begin(); it != listofClients.end(); ++it){
		std::cout<< (*it).getName() << "\n";
	}
	
//	DeviceClient udpClients(6500, SOCK_DGRAM, "UDPClients", &parametersList);
//	DeviceClient clients[MAX_CLIENTS]; // should be changed to DeviceClients
	//clients[0]=udpClients;
		
    for (;;) { 
 	  
		// clear the descriptor set 
		FD_ZERO(&rset); 
		
		maxfdp1 = max(gprsLink.getFD(), tcpListen.getFD()); 
	
		printf("service client list\n");
		/*
		for(std::list<DeviceClient>::iterator it = listofClients.begin(); it != listofClients.end(); ++it){
			if((*it).getFD() != 0){
				std::cout<< "Client: " << (*it).getName() << " - Added to new list!\n";
				listofClients.erase(it);	
			}
		}
		printf("Done service client list\n");
		
		printf("Loop clients file descriptors\n");
		for(std::list<DeviceClient>::iterator it = listofClients.begin(); it != listofClients.end(); ++it){
			std::cout<< "Client: " << (*it).getName() << " with FD=" << (*it).getFD() << " added to next loop!\n";		
			maxfdp1 = max((*it).getFD(), maxfdp1);
		}

		printf("Max file descriptor + 1 found to be=%d\n",maxfdp1);
		*/
		
		for(std::list<DeviceClient>::iterator it = listofClients.begin(); it != listofClients.end(); ++it){
			if((*it).getFD() == 0){
				std::cout<< "Client: " << (*it).getName() << " - Removed!\n";
				it = listofClients.erase(it);	
			}else{
				std::cout<< "Client: " << (*it).getName() << " with FD=" << (*it).getFD() << " added to next loop!\n";			
				maxfdp1 = max((*it).getFD(), maxfdp1);
			}
		}
		printf("Done service client list!\n");
		
		
		maxfdp1++;			
		tcpListen.setFD_SET(&rset);
		gprsLink.setFD_SET(&rset);		
		for(std::list<DeviceClient>::iterator it = listofClients.begin(); it != listofClients.end(); ++it){
		//	std::cout<< (*it).getName() << "\n";		
			(*it).setFD_SET(&rset);
		}
		printf("**********************");
	
       // select the ready descriptor 
       // nready = select(maxfdp1, &rset, NULL, NULL, &timeout); 
	    nready = select(maxfdp1, &rset, NULL, NULL, NULL); 
	
        // if tcp socket is readable then handle 
        // it by accepting the connection 
        if (FD_ISSET(tcpListen.getFD(), &rset)) {
			DeviceClient client(tcpListen.getFD(), "Client TCP", &parametersList, &waypointList);
			listofClients.push_back(client);
        }
		else if (FD_ISSET(gprsLink.getFD(), &rset)) { // if udp socket is readable receive the message. 
			uint16_t nMsg=gprsLink.service();
	//		printf("MAIN: Time to service GPRSLink, rxFIFO is=%d\n",nMsg); 			
			if(nMsg>0){
				do{
//					printf("MAIN: nMSG=%d\n",nMsg); 
				mavlink_message_t msg = gprsLink.getMSG();
					for(std::list<DeviceClient>::iterator it = listofClients.begin(); it != listofClients.end(); ++it){
						//std::cout<< (*it).getName() << "\n";		
						(*it).sendMSG(msg);
					}

					if(nMsg>0){
						nMsg--;	
					}		
				}while(nMsg>0);
			}
        }else{ // check if data from any clients:
			for(std::list<DeviceClient>::iterator it = listofClients.begin(); it != listofClients.end(); ++it){
				if(FD_ISSET((*it).getFD(), &rset)){ // check to see which client has data
					uint16_t nMsg=(*it).service();
					if(nMsg>0){
						do{
//							printf("MAIN: nMSG=%d\n",nMsg); 
							mavlink_message_t msg = (*it).getMSG();
//							gprsLink.sendMSG(msg); // do we want to do this?
							if(nMsg>0){
								nMsg--;	
							}		
						}while(nMsg>0);
					}						
				}
			}		
		} 
		/// TIMEOUT, ask for parameters
		//askForParameters
		/*
		if(nready == 0){
			printf("Socket Timeout\n");
		}
		if(gprsLink.parametersReceived == false){ // lets send #22 to drone
			printf("Requisting Parameters from drone!\n");
			mavlink_message_t outgoing;
			mavlink_param_request_list_t dummy;
			dummy.target_system = 1;
			dummy.target_component = 1;			
			mavlink_msg_param_request_list_encode(0xFF, 0xBE, &outgoing, &dummy);
			gprsLink.sendMSG(outgoing);
		}*/
		
    } 
} 