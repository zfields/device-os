#include "hw_config.h"
#include "spark_utilities.h"
#include "application.h"
#include "spark_wiring.h"
#include "socket.h"
#include "netapp.h"
#include <string.h>
#include <stdlib.h>


extern uint8_t WLAN_MANUAL_CONNECT;
extern uint8_t WLAN_DHCP;
extern uint8_t SPARK_SOCKET_CONNECTED;
extern uint8_t SPARK_DEVICE_ACKED;

void setup();
void loop();
void checkSerial();
void tester_connect(char *ssid, char *pass);
void tester_ping(char *ip, char *port, char *msg);
void tokenizeCommand(char *cmd, char** parts);
int* parseIP(char *ip, int *parts);
void printIP(int* parts);

int Connect_IP(char *ip, int port, char *msg);
int Disconnect_IP(void);
long testSocket = 0;


int cmd_index = 0, cmd_length = 256;
char command[256];
const char cmd_CONNECT[] = "CONNECT:";
const char cmd_OPEN[] = "OPEN:";
const char cmd_PARSE[] = "PARSE:";



void setup()
{   
	//take over the light.
	Set_RGBUserMode(1);
	USERLED_SetRGBColor(RGB_COLOR_BLUE);		//blue
	USERLED_On(LED_RGB);;
	Serial.begin(9600);	
}

int notYetConnected = 1;
int notifiedAboutDHCP = 0;



void loop()
{
	checkSerial();
	
	/*
	//DEBUG//
	if (notYetConnected) {
		notYetConnected = 0;
		Serial.println("connecting...");
		tester_connect("CoCo", "________");	
	}
	//DEBUG//
	*/
	
	
	if (!WLAN_MANUAL_CONNECT && WLAN_DHCP && !notifiedAboutDHCP) {
		notifiedAboutDHCP = 1;
		Serial.println("We have DHCP!");
		SPARK_SOCKET_CONNECTED = 1;
		SPARK_DEVICE_ACKED = 1;
	}
}


void checkSerial() {

	if (Serial.available()) {
		char c = (char)Serial.read();
		
		if (cmd_index < cmd_length) {
			command[cmd_index] = c;
			cmd_index++;
		}
		
		if (c == ';') {
			Serial.println("got semicolon.");
			Serial.print("checking command: ");
			Serial.println(command);
			
			char* parts[5];			
			char *start;
			
			if (start = strstr(command, cmd_CONNECT)) {
				cmd_index = 0;
				
				Serial.println("tokenizing...");
				
				//expecting CONNECT:SSID:PASS;
				tokenizeCommand(start, parts);
				Serial.println("parts...");
				Serial.println(parts[0]);
				Serial.println(parts[1]);
				Serial.println(parts[2]);
				
				Serial.println("connecting...");
				tester_connect(parts[1], parts[2]);	
			}
			else if (start = strstr(command, cmd_OPEN)) { 
				cmd_index = 0;
			
				//expecting OPEN:IP:PORT:MSG;
				Serial.println("tokenizing...");
				tokenizeCommand(start, parts);
				
				Serial.println(parts[0]);
				Serial.println(parts[1]);
				Serial.println(parts[2]);
				Serial.println(parts[3]);
				
				Serial.println("sending...");
				tester_ping(parts[1], parts[2], parts[3]);
				
			}
			else if (start = strstr(command, cmd_PARSE)) {
				cmd_index = 0;
				
				Serial.println("tokenizing...");
				tokenizeCommand(start, parts);
				
				Serial.println(parts[0]);
				Serial.println(parts[1]);
				Serial.println(parts[2]);
				Serial.println(parts[3]);
				
				int ip[4];
				parseIP(parts[1], ip);
				printIP(ip);
				
				int test[4];
				test[0] = 11;
				test[1] = 22;
				test[2] = 33;
				test[3] = 44;
				printIP(test);
			}
		}
	}
}

void tester_connect(char *ssid, char *pass) {

	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
	wlan_connect(WLAN_SEC_WPA2, ssid, strlen(ssid), NULL, pass, strlen(pass));
	WLAN_MANUAL_CONNECT = 0;
	
	USERLED_SetRGBColor(0xFF00FF);		//purple
	USERLED_On(LED_RGB);
	Serial.println("WIFI Connected?");
}

void tester_ping(char *ip, char *port, char *msg) {

	Connect_IP(ip, 8989, msg);
	Serial.println("Msg sent?");
	USERLED_SetRGBColor(0x00FFFF);		//cyan?
	USERLED_On(LED_RGB);
}


void tokenizeCommand(char *cmd, char* parts[]) {
	char * pch;
	int idx = 0;
	
	//printf ("Splitting string \"%s\" into tokens:\n", cmd);
	pch = strtok (cmd,":;");
	while (pch != NULL)
	{
		if (idx < 5) {
			parts[idx++] = pch;
		}
		pch = strtok (NULL, ":;");
	}
}


int* parseIP(char *ip, int* parts) {
	int idx = 0;
	char *pch = strtok (ip,".");
	while (pch != NULL)
	{
		if (idx < 4) {
			parts[idx++] = (char)atoi(pch);
		}
		pch = strtok (NULL, ".");
	}
	return parts;
}

void printIP(int* parts) {
	char ip[6];
	int i=0;
	for(i=0;i<4;i++) {
		itoa(parts[i], ip);
		Serial.print(ip);
		Serial.print(".");
	}
}

int Connect_IP(char *ip, int port, char *msg)
{
	int parts[4];
	parseIP(ip, parts);
	int retVal = 0;

    long testSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (testSocket < 0)
    {
        //wlan_stop();
        return -1;
    }

	// the family is always AF_INET
	sockaddr tSocketAddr;
    tSocketAddr.sa_family = AF_INET;

	// the destination port
    tSocketAddr.sa_data[0] = (SPARK_SERVER_PORT & 0xFF00) >> 8;
    tSocketAddr.sa_data[1] = (SPARK_SERVER_PORT & 0x00FF);
	
	// the destination IP address
	tSocketAddr.sa_data[2] = parts[0];	// First Octet of destination IP
	tSocketAddr.sa_data[3] = parts[1];	// Second Octet of destination IP
	tSocketAddr.sa_data[4] = parts[2]; 	// Third Octet of destination IP
	tSocketAddr.sa_data[5] = parts[3];	// Fourth Octet of destination IP

	retVal = connect(testSocket, &tSocketAddr, sizeof(tSocketAddr));

	if (retVal < 0)
	{
		// Unable to connect
		Serial.println("Not connected?");
		return -1;
	}
	else
	{
		Serial.println("Connected?! sending msg...");
		retVal = send(testSocket, msg, strlen(msg), 0);
	}

    return retVal;
}

int Disconnect_IP(void)
{
    int retVal = 0;

    retVal = closesocket(testSocket);

    if(retVal == 0)
    	testSocket = 0xFFFFFFFF;

    return retVal;
}
