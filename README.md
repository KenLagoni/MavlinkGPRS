# Mavlink Ardupilot Telemetry via GPRS (2G) with multiple client support:
![alt text](http://lagoni.org/Github/MavlinkGPRS/blokdiagram-mavlinkGPRS.png)

# Introduction
As an alternative to having a high power long range telemetry link, I wanted to try and relay data via a simple GPRS link using a simple cheap SIM800L module. Since these opprates at ~850MHz with a power ouput up to 2W, it should have plenty of range and bandwith if you fly where there is Telecom coverage.

# Features (Drone side):
The Arduino MKRZero has the task of recieving Mavlink packages from the Ardupilot and relay them to the Raspberry Pi via GPRS. It also implemnts a package filter on the mavlink data from the Ardupilot to ensure only relevant packages are relayed in order to limit the bandwith usages or handle them ie. RSSI injection.  
- Packages with RSSI from Ardupilot is intercepted by the Arduino and replace the RSSI value with the 2G GPRS RSSI before it is relayed to the server. This will make the RSSI inducator on the groundstation indicate the GPRS link RSSI and not the RC link RSSI.


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





# Features (Ground side):
On the ground everything is handled by the raspberry pi which is connected to the internet via my home router. The Pi has two servers running:
1. GPRS server for UDP data to and from the Drone.
This will listen for UDP data sent from the Arduino on the drone. It will also download and save all drone parameters and mission waypoints if selected. This will only be done once, and thus clients (Qgroundcontrol/mission planner) connecting to the server will get data the cashed data on the server, ensuring not to load the drone.
All data recieved from the drone is relayed to all clients connected.

2. Client server for handling multiple clients (Qgroundcontrol / Mission Planner).
The Client server listens for Qgroundcontrol / Mission PLanner TCP connection on port 5760 (and when connection will negotiate another port for data). I found UDP packages from my router to a client on 4G is not working and thus only TCP should be used.
When clients connect, they think they are connected to a drone (Ardupilot) and thus will ask for parameters and mission waypoints which the server will provide given the GPRS server has made them available. 

Now the program will allways ask drone for parameters/mission when it detects a reboot. The newly downloaded data will be saved in [DATE_TIME]-Parameters.txt and [DATE_TIME]-Waypoints.txt - The client must then do a reconnect to get the updated values (disconnect/connect).


# How to Build

## Arduino MKRZero
Download the Arduino files and put the Libary folder in the Arduino Libary folder. Open the Arduino Sketch and change the SIM800L line to match your SIM APN and the IP to your fixed IP for the home router
`SIM800L ModemGPRS(&Serial3, 115200, PIN_SIM800L_RESET, "", "[SIM APN]", "[IP FOR HOME ROUTER]", 14450, "UDP", 100, 2000);`
This could be:
`SIM800L ModemGPRS(&Serial3, 115200, PIN_SIM800L_RESET, "", "internet", "10.11.12.13", 14450, "UDP", 100, 2000);`
Ensure the SAMD liberies are installed and the sketch sould now build and upload to the ArduinoMKRZero.

## Raspberry Pi server
Download the "MavlinkGPRSServer" to the Raspberry Pi with a standard Buster image installed. Start the server by running the command:
`./MavlinkGPRSServer [GPRS INPUT UDP PORT] [TCP CLIENTHS PORT] [UDP CLIENT PORT]`

Old command which saved or loaded waypoints/parameters, this is now outdated:
~~`./mavlinkGPRSServer [GPRS INPUT UDP PORT] [TCP LISTEN PORT] [UDP CLIENT PORT] [WAYPOINT FILE] -[R/W] [PARAMETER FILE] -[R/W]`~~

~~Start the server and download waypoints to waypoint.txt and Ardupilot parameters to parameters.txt:~~
~~`./mavlinkGPRSServer 14450 5760 6000 waypoint.txt -W parameters.txt -W`~~

~~Start the server but don't download waypoints or parameters, but instead use the waypoint.txt and parameters.txt files:~~
~~`./mavlinkGPRSServer 14450 5760 6000 waypoint.txt -R parameters.txt -R`~~

~~Start the server but only download waypoints:~~
~~`./mavlinkGPRSServer 14450 5760 6000 waypoint.txt -W parameters.txt -R`~~






