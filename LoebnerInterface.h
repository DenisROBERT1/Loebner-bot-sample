#ifndef LoebnerInterface_h
#define LoebnerInterface_h

//------------------------------------------------------------

// Message socket
#define UM_MESSAGE   (WM_USER + 10)

typedef void (CALLBACK *LI_ONNEWROUND)(void);
typedef void (CALLBACK *LI_ONSTARTROUND)(void);
typedef void (CALLBACK *LI_ONENDROUND)(void);
typedef void (CALLBACK *LI_ONMESSAGE)(char *);
typedef void (CALLBACK *LI_ONDISCONNECT)(char *);

#define MAX_JSON_VALUES 12   // Maximum number of values in a JSON string
#define MAX_LEN_NAME 50   // Maximum length of names (bots, judges and confederates)

enum JSON_TYPE {
	json_list,
	json_map
} ;

//-----------------------FUNCTIONS PROTOTYPES--------------------------

bool Loebner_Initialisation(HWND hWnd,
														LI_ONNEWROUND OnNewRound,
														LI_ONSTARTROUND OnStartRound,
														LI_ONENDROUND OnEndRound,
														LI_ONMESSAGE OnMessage,
														LI_ONDISCONNECT OnDisconnect);
bool Loebner_End();
bool Loebner_Start(const char *szName, const char *szSecret, const char *szAddress, int Port);
bool Loebner_Stop();

void Loebner_SendMessage(const char *szLine);

#endif
