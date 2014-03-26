/**
 ******************************************************************************
 * @file    application.cpp
 * @authors  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    05-November-2013
 * @brief   Tinker application
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#define DFU_BUILD_ENABLE 1

/* Includes ------------------------------------------------------------------*/  
#include "application.h"


extern uint8_t WLAN_MANUAL_CONNECT;
extern volatile uint8_t WLAN_DHCP;

uint8_t serialAvailable();
int32_t serialRead();
void serialPrintln(const char * str);
void serialPrint(const char * str);
void checkWifiSerial(char c);
void tokenizeCommand(char *cmd, char** parts);
void tester_connect(char *ssid, char *pass);

class StartupEarly {
    public:
        StartupEarly();
};
StartupEarly::StartupEarly() {

    SPARK_CLOUD_CONNECT = 0;
    WLAN_SMART_CONFIG_START = 0;
    WLAN_MANUAL_CONNECT = 1;
}

StartupEarly foo;



uint8_t notifiedAboutDHCP = 0;
int state = 0;
int cmd_index = 0, cmd_length = 256;
char command[256];
const char cmd_CONNECT[] = "CONNECT:";


void setup()
{
    RGB.control(true);
    RGB.color(64, 0, 0);

    pinMode(D2, OUTPUT);
    digitalWrite(D2, LOW);

    Serial.begin(9600);
    Serial1.begin(9600);



    //TODO: startup without wifi
    //TODO: run setup/loop without wifi
    //TODO: try to connect to manual wifi asap
    //TODO: set the pin / report via serial
}


void loop()
{
    if (WLAN_DHCP) {
		Serial.println("DHCP DHCP DHCP !");
		Serial1.println("DHCP DHCP DHCP !");
		RGB.color(255, 0, 255);
		digitalWrite(D2, HIGH);
		delay(1000);
	}
	else {
	    state = !state;
	    RGB.color(64, (state) ? 255 : 0, 64);
	    //delay(100);

//        Serial.println("waiting...");
//        delay(500);
	}


	if (serialAvailable()) {
		int c = serialRead();

		checkWifiSerial((char)c);

    }
//
//		if ((c >= pins_start) && (c <= pins_end)) {
//		    //if ((c >= 0) && (c < numPins)) {
//			//if we should receive a byte value in the range of '0'-'9'
//			//lets assume they would like us to turn off all the pins, and turn on just that pin.
//
//			handlePinMessage(c - pins_start);
//			serialPrintln("OK PIN \n");
//		}
//		else if ((c >= rgb_start) && (c <= rgb_end)) {
//			handleRGBMessage(c - rgb_start);
//			serialPrintln("OK LED \n");
//		}
//		else if (c == 'T') {
//			char buffer[32];
//			unsigned char size = itoa(millis(), buffer);
//			buffer[size] = '\0';
//
//			serialPrint("The time is:");
//			serialPrint(buffer);
//			serialPrintln(":");
//		}
//		else if (c == 'X') {
//			sendAlives = 1;
//		}
//		else if (c == 'Z') {
//			sendAlives = 0;
//		}
//		else if (c == 'V') {
//			serialPrintln("Serial+Pin+Wifi+RTC+RGB Tester!");
//		}
//		else if (c == 'M') {
//
//			char buffer[25];
//			unsigned char coreid[12];
//			memcpy(coreid, (void *)ID1, 12);
//			coreIdToHex(coreid, 12, buffer);
//
//			serialPrint("ID:");
//			serialPrint(buffer);
//			serialPrintln(":END");
//		}
//		else if (c == ' ') {
//			;
//		}
//		else if  (c == 'j') {
//			unsigned char pubkey[294];
//			char buffer[588];
//			FLASH_Read_ServerPublicKey(pubkey);
//			coreIdToHex(pubkey, 294, buffer);
//
//			serialPrintln("public key is");
//			serialPrintln(buffer);
//		}
//		else {
//			//other commands...?
//
//			unsigned char in[1];
//			in[0] = c;
//
//			char buffer[3];
//			coreIdToHex(in, 1, buffer);
//
//			serialPrint("HUH: ");
//			serialPrintln(buffer);
//		}
//	}
}


void checkWifiSerial(char c) {
	if (cmd_index < cmd_length) {
		command[cmd_index] = c;
		cmd_index++;
	}
	else {
		cmd_index = 0;
	}

	if (c == ' ') {
		//reset the command index.
		cmd_index = 0;
	}
	else if (c == ';') {
		serialPrintln("got semicolon.");
		serialPrint("checking command: ");
		serialPrintln(command);

		char *parts[5];
		char *start;

//		if (start = strstr(command, cmd_ENABLE)) {
//			serialPrintln("enable wifi...");
//			//initializeWifi();
//			wifiEnable();
//			serialPrintln("DONE DONE DONE DONE DONE DONE DONE DONE DONE DONE");
//		}
//		else if (start = strstr(command, cmd_DISABLE)) {
//			serialPrintln("disable wifi...");
//			//initializeWifi();
//			wifiDisable();
//			serialPrintln("DONE DONE DONE DONE DONE DONE DONE DONE DONE DONE");
//		}
	 if (start = strstr(command, cmd_CONNECT)) {
			cmd_index = 0;

			serialPrintln("tokenizing...");

			//expecting CONNECT:SSID:PASS;
			tokenizeCommand(start, parts);
			serialPrintln("parts...");
			serialPrintln(parts[0]);
			serialPrintln(parts[1]);
			serialPrintln(parts[2]);

			serialPrintln("connecting...");
			tester_connect(parts[1], parts[2]);
		}
//		else if (start = strstr(command, cmd_OPEN)) {
//			cmd_index = 0;
//
//			//expecting OPEN:IP:PORT:MSG;
//			serialPrintln("tokenizing...");
//			tokenizeCommand(start, parts);
//
//			serialPrintln(parts[0]);
//			serialPrintln(parts[1]);
//			serialPrintln(parts[2]);
//			serialPrintln(parts[3]);
//
//			serialPrintln("sending...");
//			//char *ip, char *port, char *msg
//			tester_ping(parts[1], parts[2], parts[3]);
//
//		}
//		else if (start = strstr(command, cmd_PARSE)) {
//			cmd_index = 0;
//
//			serialPrintln("tokenizing...");
//			tokenizeCommand(start, parts);
//
//			serialPrintln(parts[0]);
//			serialPrintln(parts[1]);
//			serialPrintln(parts[2]);
//			serialPrintln(parts[3]);
//
//			int ip[4];
//			parseIP(parts[1], ip);
//			printIP(ip);
//
//			int test[4];
//			test[0] = 11;
//			test[1] = 22;
//			test[2] = 33;
//			test[3] = 44;
//			printIP(test);
//		}
	}
}


void serialPrintln(const char * str) {
	Serial.println(str);
	Serial1.println(str);
}
void serialPrint(const char * str) {
	Serial.print(str);
	Serial1.print(str);
}

uint8_t serialAvailable() {
	return Serial.available() | Serial1.available();
}

int32_t serialRead() {
	if (Serial.available()) {
		return Serial.read();
	}
	else if (Serial1.available()) {
		return Serial1.read();
	}
	return 0;
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




void tester_connect(char *ssid, char *pass) {

     RGB.color(64, 64, 0);

    SPARK_MANUAL_CREDS(ssid, pass);

	RGB.color(0, 0, 64);
//
//	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
//	wlan_connect(WLAN_SEC_WPA2, ssid, strlen(ssid), NULL, pass, strlen(pass));
//	WLAN_MANUAL_CONNECT = 0;
//
//	RGBColor = 0xFF00FF;		//purple
	//USERLED_SetRGBColor(0xFF00FF);		//purple
	//USERLED_On(LED_RGB);
	serialPrintln("  WIFI Connected?    ");
}