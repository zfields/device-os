#include "application.h"
#include "string.h"
#include "spark_utilities.h"
#include "handshake.h"

extern long sparkSocket;

static int Do_Spark_Handshake(long socket);


int runitup = 0;

/*
int toggle = 0;
int UserLedToggle(char *ledPin);

double testReal = 99.99;
*/

//unsigned char ciphertext[256];
//int err, encrypt = 1;

//unsigned char nonce[40] ={
//  1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1 };
unsigned char id[12] = {
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1 };

//unsigned char id[12];
//unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];

void setup()
{
	// Serial Test
	Serial.begin(9600);
	Serial.println("Hello!");

	// runs once
	//memset(ciphertext, 0, 256);
	memcpy(id, (void *)0x01fff7e8, 12);
	//FLASH_Read_ServerPublicKey(pubkey);
 
 	//pinMode(D7, OUTPUT);
  //digitalWrite(D7, LOW);
  //delay(500);
  //digitalWrite(D7, HIGH);
  //delay(500);
  //digitalWrite(D7, LOW);






/*
	//Register UserLedToggle() function
	Spark.function("UserLed", UserLedToggle);

	//Register testReal variable
	Spark.variable("testReal", &testReal, DOUBLE);
*/
}

void loop()
{
	// runs repeatedly

/*
	// Serial loopback test: what is typed on serial console
	// using Hyperterminal/Putty should echo back on the console
	if(Serial.available())
	{
		Serial.write(Serial.read());
	}

	// Serial print test
	Serial.print("Hello ");
	Serial.println("Spark");
	Serial.print("err: ");
	Serial.println(0 == err ? "zero" : "non-zero");
	Serial.print("ciphertext: ");
	Serial.println((char *)ciphertext);
	delay(2000);
*/
	if (runitup == 0) {
		runitup = 1;
		Do_Spark_Handshake(sparkSocket);
	}


	//if(encrypt)
	//{
	//	err = ciphertext_from_nonce_and_id(nonce, id, pubkey, ciphertext);
	//	if(err == 0)
	//	{
	//		//Success
	//		digitalWrite(D7, HIGH);
	//	}
	//	encrypt = 0;
	//}

/*
	// Call this in the process_command() to schedule the "UserLedToggle" function to execute
	userFuncSchedule("UserLed", 0xc3, "D7");

	// Call this in the process_command() to schedule the return of "testReal" value
	userVarSchedule("testReal", 0xa1);

	delay(1000);
*/
}

/*
int UserLedToggle(char *ledPin)
{
	if(0 == strncmp("D7", ledPin, strlen(ledPin)))
	{
		toggle ^= 1;
		digitalWrite(D7, toggle);
		return 1;
	}
	return 0;
}
*/


static int Do_Spark_Handshake(long socket) {
	
		Serial.println("Spark 1");
	
		//read nonce (exactly 40 bytes)
		unsigned char nonce[40] = {
			  1, 1, 1, 1, 1, 1, 1, 1,
			  1, 1, 1, 1, 1, 1, 1, 1,
			  1, 1, 1, 1, 1, 1, 1, 1,
			  1, 1, 1, 1, 1, 1, 1, 1,
			  1, 1, 1, 1, 1, 1, 1, 1 };

		//READNEXT40

		//-------------------------------
		//send coreid
	
		unsigned char id[12];
		unsigned char ciphertext[256];		
		unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];

		Serial.println("Spark 2");

		//read in the core id, from... place?
		memcpy(id, (void *)0x01fff7e8, 12);

		Serial.println("Spark 3");

		//read in the server public key...
		FLASH_Read_ServerPublicKey(pubkey);

		Serial.println("Spark 4");

		//clear our ciphertext buffer
		memset(ciphertext, 0, 256);

		Serial.println("Spark 5");

		//create the public-key encrypted chunk to send serverside.
		int err = ciphertext_from_nonce_and_id(nonce, id, pubkey, ciphertext);
		if (err != 0) {
			Serial.println("Spark 6");
			return -1;
		}

		Serial.println("Spark 7");

		Serial.println("Spark 8");
		Serial.println("Spark 9");
		Serial.println("Spark 10");

		//-------------------------------
		// read session key
		// READNEXT256
		//...

		




		//read sessionkey
		//send hello
		//get hello

		return 0;
}
