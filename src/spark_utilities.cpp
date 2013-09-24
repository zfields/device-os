#include "spark_utilities.h"
//#include "socket.h"
//#include "netapp.h"
#include "string.h"
#include <stdarg.h>
#include "handshake.h"
#include "spark_protocol.h"

long sparkSocket;
sockaddr tSocketAddr;

timeval timeout;
_types_fd_set_cc3000 readSet;

const char Device_Name[] = "sparkdemodevice";

char digits[] = "0123456789";
char recvBuff[SPARK_BUF_LEN];

int spark_expected_message_length = 0;

uint32_t chunkIndex;

void (*pHandleMessage)(void);
char msgBuff[SPARK_BUF_LEN];

int User_Var_Count;
int User_Func_Count;

SparkProtocol spark_protocol;


struct User_Var_Lookup_Table_t
{
	void *userVar;
	char userVarKey[USER_VAR_KEY_LENGTH];
	Spark_Data_TypeDef userVarType;
	bool userVarSchedule;
	unsigned char token; //not sure we require this here
} User_Var_Lookup_Table[USER_VAR_MAX_COUNT];

struct User_Func_Lookup_Table_t
{
	int (*pUserFunc)(char *userArg);
	char userFuncKey[USER_FUNC_KEY_LENGTH];
	char userFuncArg[USER_FUNC_ARG_LENGTH];
	int userFuncRet;
	bool userFuncSchedule;
	unsigned char token; //not sure we require this here
} User_Func_Lookup_Table[USER_FUNC_MAX_COUNT];

static unsigned char uitoa(unsigned int cNum, char *cString);
static unsigned int atoui(char *cString);

Spark_Namespace Spark =
{
	Spark_Variable,
	Spark_Function,
	Spark_Event,
	Spark_Sleep,
	Spark_Connected,
	Spark_Connect,
	Spark_Disconnect
};

void Spark_Variable(const char *varKey, void *userVar, Spark_Data_TypeDef userVarType)
{
	int i = 0;
	if(NULL != userVar && NULL != varKey)
	{
		if(User_Var_Count == USER_VAR_MAX_COUNT)
			return;

		for(i = 0; i < User_Var_Count; i++)
		{
			if(User_Var_Lookup_Table[i].userVar == userVar && (0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH)))
			{
				return;
			}
		}

		User_Var_Lookup_Table[User_Var_Count].userVar = userVar;
		User_Var_Lookup_Table[User_Var_Count].userVarType = userVarType;
		memset(User_Var_Lookup_Table[User_Var_Count].userVarKey, 0, USER_VAR_KEY_LENGTH);
		memcpy(User_Var_Lookup_Table[User_Var_Count].userVarKey, varKey, USER_VAR_KEY_LENGTH);
		User_Var_Lookup_Table[User_Var_Count].userVarSchedule = false;
		User_Var_Lookup_Table[User_Var_Count].token = 0;
		User_Var_Count++;
	}
}

void Spark_Function(const char *funcKey, int (*pFunc)(char *paramString))
{
	int i = 0;
	if(NULL != pFunc && NULL != funcKey)
	{
		if(User_Func_Count == USER_FUNC_MAX_COUNT)
			return;

		for(i = 0; i < User_Func_Count; i++)
		{
			if(User_Func_Lookup_Table[i].pUserFunc == pFunc && (0 == strncmp(User_Func_Lookup_Table[i].userFuncKey, funcKey, USER_FUNC_KEY_LENGTH)))
			{
				return;
			}
		}

		User_Func_Lookup_Table[User_Func_Count].pUserFunc = pFunc;
		memset(User_Func_Lookup_Table[User_Func_Count].userFuncArg, 0, USER_FUNC_ARG_LENGTH);
		memset(User_Func_Lookup_Table[User_Func_Count].userFuncKey, 0, USER_FUNC_KEY_LENGTH);
		memcpy(User_Func_Lookup_Table[User_Func_Count].userFuncKey, funcKey, USER_FUNC_KEY_LENGTH);
		User_Func_Lookup_Table[User_Func_Count].userFuncSchedule = false;
		User_Func_Lookup_Table[User_Func_Count].token = 0;
		User_Func_Count++;
	}
}

void Spark_Event(char *eventName, char *eventResult)
{

}

void Spark_Sleep(Spark_Sleep_TypeDef sleepMode, long seconds)
{
#if defined (SPARK_RTC_ENABLE)
	/* Set the RTC Alarm */
	RTC_SetAlarm(RTC_GetCounter() + (uint32_t)seconds);

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	switch(sleepMode)
	{
	case SLEEP_MODE_WLAN:
		SPARK_WLAN_SLEEP = 1;
		break;

	case SLEEP_MODE_PEPH:
		//To Do
		break;

	case SLEEP_MODE_DEEP:
		Enter_STANDBY_Mode();
		break;
	}
#endif
}

bool Spark_Connected(void)
{
	if(SPARK_DEVICE_ACKED)
		return true;
	else
		return false;
}

int Spark_Connect(void)
{
	int retVal = 0;

    sparkSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sparkSocket < 0)
    {
        //wlan_stop();
        return -1;
    }

	// the family is always AF_INET
    tSocketAddr.sa_family = AF_INET;

	// the destination port
    tSocketAddr.sa_data[0] = (SPARK_SERVER_PORT & 0xFF00) >> 8;
    tSocketAddr.sa_data[1] = (SPARK_SERVER_PORT & 0x00FF);

	// the destination IP address
	tSocketAddr.sa_data[2] = 10;	// First Octet of destination IP
	tSocketAddr.sa_data[3] = 105;	// Second Octet of destination IP
	tSocketAddr.sa_data[4] = 5; 	// Third Octet of destination IP
	tSocketAddr.sa_data[5] = 244;	// Fourth Octet of destination IP

	retVal = connect(sparkSocket, &tSocketAddr, sizeof(tSocketAddr));

	if (retVal < 0)
	{
		// Unable to connect
		return -1;
	}
	else
	{
		//Do_Spark_Handshake(sparkSocket);
		SPARK_DEVICE_ACKED = 1;
		SPARK_DEVICE_HANDSHAKING = 1;
		retVal = 1;
	}

    return retVal;
}

int Spark_Disconnect(void)
{
    int retVal = 0;

    retVal = closesocket(sparkSocket);

    if(retVal == 0)
    	sparkSocket = 0xFFFFFFFF;

    return retVal;
}

// called repeatedly from an interrupt handler, so DO NOT BLOCK
// returns: number of bytes received
//          -1 on error, signifying socket disconnected
int receive()
{
  // reset the fd_set structure
  FD_ZERO(&readSet);
  FD_SET(sparkSocket, &readSet);

  // tell select to timeout after 500 microseconds
  timeout.tv_sec = 0;
  timeout.tv_usec = 500;

  int bytes_received = 0;
  int num_fds_ready = select(sparkSocket + 1, &readSet, NULL, NULL, &timeout);

  if (0 < num_fds_ready)
  {
    if (FD_ISSET(sparkSocket, &readSet))
    {
      bytes_received = recv(sparkSocket, recvBuff, SPARK_BUF_LEN, 0);
      if (0 > bytes_received)
        return bytes_received;

      int bytes_pushed = spark_protocol.queue_push(recvBuff, bytes_received);
      if (bytes_pushed != bytes_received)
        return -2; // TODO queue not big enough or not being popped fast enough
    }
  }

  return bytes_received;
}

// Tell receive_line to stop when we get a certain # of bytes.
void receive_chunk(int size) {
	spark_expected_message_length = size;
}

void process_chunk(void)
{
	uint32_t chunkBytesAvailable = 0;
	uint32_t chunkCRCValue = 0;
	uint32_t computedCRCValue = 0;
	char *chunkToken;

	chunkToken = strtok(&recvBuff[chunkIndex], "\n");
	if(chunkToken != NULL)
	{
		chunkCRCValue = atoui(chunkToken);
		chunkIndex = chunkIndex + strlen(chunkToken) + 1;//+1 for '\n'
	}

	chunkToken = strtok(NULL, "\n");
	if(chunkToken != NULL)
	{
		chunkBytesAvailable = atoui(chunkToken);
		chunkIndex = chunkIndex + strlen(chunkToken) + 1;//+1 for '\n'
	}

	if(chunkBytesAvailable)
	{
		char CRCStr[11];
		memset(CRCStr, 0, 11);
		computedCRCValue = Compute_CRC32((uint8_t *)&recvBuff[chunkIndex], chunkBytesAvailable);
		if(chunkCRCValue == computedCRCValue)
		{
			FLASH_Update((uint8_t *)&recvBuff[chunkIndex], chunkBytesAvailable);
		}
		uitoa(computedCRCValue, CRCStr);
    // TODO Send CoAP Message
	}
}

bool userVarSchedule(const char *varKey, unsigned char token)
{
	int i = 0;
	for(i = 0; i < User_Var_Count; i++)
	{
		if(0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH))
		{
			User_Var_Lookup_Table[i].userVarSchedule = true;
			User_Var_Lookup_Table[i].token = token;
			return true;
		}
	}
	return false;
}

void userVarReturn(void)
{
	int i = 0;
	for(i = 0; i < User_Var_Count; i++)
	{
		if(true == User_Var_Lookup_Table[i].userVarSchedule)
		{
			User_Var_Lookup_Table[i].userVarSchedule = false;

			//Send the "Variable value" back to the server here OR in a separate thread
			if(User_Var_Lookup_Table[i].token)
			{
/*
				bool boolVal;
				int intVal;
				char *stringVal;
				double doubleVal;
*/

				unsigned char buf[16];
				memset(buf, 0, 16);

				switch(User_Var_Lookup_Table[i].userVarType)
				{
				case BOOLEAN:
/*
					boolVal = *((bool*)User_Var_Lookup_Table[i].userVar);
					//spark_protocol.variable_value(buf, User_Func_Lookup_Table[i].token, boolVal);
*/
					break;

				case INT:
/*
					intVal = *((int*)User_Var_Lookup_Table[i].userVar);
					//spark_protocol.variable_value(buf, User_Func_Lookup_Table[i].token, intVal);
*/
					break;

				case STRING:
/*
					stringVal = ((char*)User_Var_Lookup_Table[i].userVar);
					//spark_protocol.variable_value(buf, User_Func_Lookup_Table[i].token, stringVal, strlen(stringVal));
*/
					break;

				case DOUBLE:
/*
					doubleVal = *((double*)User_Var_Lookup_Table[i].userVar);
					//spark_protocol.variable_value(buf, User_Func_Lookup_Table[i].token, doubleVal);
*/
					break;
				}

				User_Var_Lookup_Table[i].token = 0;
			}
		}
	}
}

bool userFuncSchedule(const char *funcKey, unsigned char token, const char *paramString)
{
	int i = 0;
	for(i = 0; i < User_Func_Count; i++)
	{
		if(NULL != paramString && (0 == strncmp(User_Func_Lookup_Table[i].userFuncKey, funcKey, USER_FUNC_KEY_LENGTH)))
		{
			size_t paramLength = strlen(paramString);
			if(paramLength > USER_FUNC_ARG_LENGTH)
				paramLength = USER_FUNC_ARG_LENGTH;
			memcpy(User_Func_Lookup_Table[i].userFuncArg, paramString, paramLength);
			User_Func_Lookup_Table[i].userFuncSchedule = true;
			User_Func_Lookup_Table[i].token = token;
			return true;
		}
	}
	return false;
}

void userFuncExecute(void)
{
	int i = 0;
	for(i = 0; i < User_Func_Count; i++)
	{
		if(true == User_Func_Lookup_Table[i].userFuncSchedule)
		{
			User_Func_Lookup_Table[i].userFuncSchedule = false;
			User_Func_Lookup_Table[i].userFuncRet = User_Func_Lookup_Table[i].pUserFunc(User_Func_Lookup_Table[i].userFuncArg);
/*
			//Send the "Function Return" back to the server here OR in a separate thread
			if(User_Func_Lookup_Table[i].token)
			{
				unsigned char buf[16];
				memset(buf, 0, 16);
				spark_protocol.function_return(buf, User_Func_Lookup_Table[i].token, User_Func_Lookup_Table[i].userFuncRet);
				User_Func_Lookup_Table[i].token = 0;
			}
*/
		}
	}
}

// Convert unsigned integer to ASCII in decimal base
static unsigned char uitoa(unsigned int cNum, char *cString)
{
    char* ptr;
    unsigned int uTemp = cNum;
    unsigned char length;

    // value 0 is a special case
    if (cNum == 0)
    {
        length = 1;
        *cString = '0';

        return length;
    }

    // Find out the length of the number, in decimal base
    length = 0;
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    // Do the actual formatting, right to left
    uTemp = cNum;
    ptr = cString + length;
    while (uTemp > 0)
    {
        --ptr;
        *ptr = digits[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}

// Convert ASCII to unsigned integer
static unsigned int atoui(char *cString)
{
	unsigned int cNum = 0;
	if (cString)
	{
		while (*cString && *cString <= '9' && *cString >= '0')
		{
			cNum = (cNum * 10) + (*cString - '0');
			cString++;
		}
	}
	return cNum;
}

// returns number of bytes transmitted or -1 on error
int Spark_Send_Message(long socket, const void *buffer, int bufferLength)
{
	unsigned char lenBuf[2];
	lenBuf[0] = bufferLength >> 8;
	lenBuf[1] = bufferLength & 255;
	send(socket, lenBuf, 2, 0);

	return send(socket, buffer, bufferLength, 0);
}
