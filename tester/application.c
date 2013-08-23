#include "hw_config.h"
#include "spark_utilities.h"
#include "application.h"
#include "spark_wiring.h"
#include "socket.h"
#include "netapp.h"
#include <string.h>
#include <stdlib.h>

void setup();
void loop();
void checkSerial();
void tester_connect(char *ssid, char *pass);
void tester_ping(char *ip, char *port, char *msg);
void tokenizeCommand(char *cmd, char** parts);
int* parseIP(char *ip);

int Connect_IP(char *ip, int port, char *msg);
int Disconnect_IP(void);
long testSocket = 0;


int cmd_index = 0, cmd_length = 256;
char command[256];
const char cmd_CONNECT[] = "CONNECT:";
const char cmd_OPEN[] = "OPEN:";



void setup()
{   
	//take over the light.
	Set_RGBUserMode(1);
	USERLED_SetRGBColor(0x0000FF);		//blue
	Serial.begin(9600);	
}



void loop()
{
	checkSerial();	
}


void checkSerial() {

	if (Serial.available()) {
		char c = (char)Serial.read();
		
		if (cmd_index < cmd_length) {
			command[cmd_index] = c;
		}
		
		if (c == ';') {
			char* parts[5];			
			
			if (0 == (strncmp(command, cmd_CONNECT, strlen(cmd_CONNECT)))) {
				//expecting CONNECT:SSID:PASS;
				tokenizeCommand(command, parts);
				
				
				tester_connect(parts[1], parts[2]);
			
			}
			else if (0 == (strncmp(command, cmd_OPEN, strlen(cmd_OPEN)))) {
				//expecting OPEN:IP:PORT:MSG;				
				tokenizeCommand(command, parts);
				
				tester_ping(parts[1], parts[2], parts[3]);
			}
		}
	}
}

void tester_connect(char *ssid, char *pass) {

	wlan_connect(WLAN_SEC_WPA2, ssid, strlen(ssid), NULL, *pass, strlen(*pass));
	USERLED_SetRGBColor(0xFF00FF);		//purple
}

void tester_ping(char *ip, char *port, char *msg) {

	Connect_IP(ip, 8989, msg);
	USERLED_SetRGBColor(0x00FFFF);		//purple
}


void tokenizeCommand(char *cmd, char* parts[]) {
	char * pch;
	int idx = 0;
	
	//printf ("Splitting string \"%s\" into tokens:\n", cmd);
	pch = strtok (cmd,":");
	while (pch != NULL)
	{
		if (idx < 5) {
			parts[idx++] = pch;
		}
		pch = strtok (NULL, ":");
	}
}


int* parseIP(char *ip) {
	int parts[4];
	
	int idx = 0;
	char pch = strtok (ip,".");
	while (pch != NULL)
	{
		if (idx < 4) {
			parts[idx++] = atoi(pch);
		}
		pch = strtok (NULL, ".");
	}
	return parts;
}

int Connect_IP(char *ip, int port, char *msg)
{
	int* parts = parseIP(ip);
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
		return -1;
	}
	else
	{
		retVal = send(socket, msg, strlen(msg), 0);
		//retVal = Spark_Send_Device_Message(testSocket, (char *)Device_Secret, NULL, NULL);
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
