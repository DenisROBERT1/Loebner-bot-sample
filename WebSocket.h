#ifndef WebSocket_h
#define WebSocket_h

// Message socket
#define UM_MESSAGE   (WM_USER + 10)
#define DELAY_PING   10000

enum OPCODE {
	oc_continue = 0x00,
	oc_text = 0x01,
	oc_binary = 0x02,
	oc_close = 0x08,
	oc_ping = 0x09,
	oc_pong = 0x0A,
	oc_control = 0x0B
};

typedef void (CALLBACK *WS_ONERROR)(const char *);
typedef void (CALLBACK *WS_ONDISCONNECT)(const char *);
typedef void (CALLBACK *WS_ONMESSAGE)(const char *);


#ifdef _MSC_VER
#define sprintf_s_2 sprintf_s
#else
#define strcpy_s(strDest, size, strSrc) strcpy(strDest, strSrc)
#define strcat_s(strDest, size, strSrc) strcat(strDest, strSrc)
#define strncpy_s(strDest, size, strSrc, len) strncpy(strDest, strSrc, len)
#define sprintf_s(strDest, size, strFormat, arg) sprintf(strDest, strFormat, arg)
#define sprintf_s_2(strDest, size, strFormat, arg1, arg2) sprintf(strDest, strFormat, arg1, arg2)
#endif // _MSC_VER

//-----------------------FUNCTIONS PROTOTYPES--------------------------
bool WebSocket_Initialisation(WS_ONERROR OnError, WS_ONDISCONNECT OnDisconnect, WS_ONMESSAGE OnMessage);
bool WebSocket_End();
bool WebSocket_Connect(const char *szAddress, int Port);
bool WebSocket_Disconnect();
bool WebSocket_Send(OPCODE OpCode, const char *szMessage);
bool WebSocket_Receive();
bool WebSocket_Ping();

void ProcessMessages();

#endif
