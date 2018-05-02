#ifndef WebSocket_h
#define WebSocket_h

// Message socket
#define UM_MESSAGE   (WM_USER + 10)
#define DELAY_PING   30000

enum OPCODE {
	oc_continue = 0x00,
	oc_text = 0x01,
	oc_binary = 0x02,
	oc_close = 0x08,
	oc_ping = 0x09,
	oc_pong = 0x0A,
	oc_control = 0x0B
};

typedef void (CALLBACK *WS_ONERROR)(char *);
typedef void (CALLBACK *WS_ONDISCONNECT)(char *);
typedef void (CALLBACK *WS_ONMESSAGE)(char *);


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
