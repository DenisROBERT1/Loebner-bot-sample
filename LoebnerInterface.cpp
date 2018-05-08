
#include <windows.h>
#include <String>
#include <Vector>
#include <assert.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <shlobj.h>
#include <Shlwapi.h>

#include "LoebnerInterface.h"
#include "LoebnerBot.h"
#include "WebSocket.h"


//--------------------------GLOBAL VARIABLES---------------------------------
extern HWND hWndTop;

LI_ONNEWROUND Li_OnNewRound = NULL;
LI_ONSTARTROUND Li_OnStartRound = NULL;
LI_ONENDROUND Li_OnEndRound = NULL;
LI_ONMESSAGE Li_OnMessage = NULL;
LI_ONDISCONNECT Li_OnDisconnect = NULL;

char szBotId[MAX_LEN_NAME];
char szBotSecret[MAX_LEN_NAME];
char szPartner[MAX_LEN_NAME];

bool NewRound;
bool RoundStarted;

typedef struct {
	int NbValues;
	char *szKeys[MAX_JSON_VALUES];
	char *szValues[MAX_JSON_VALUES];
} JSON_VALUE;

//-----------------------FUNCTIONS PROTOTYPES--------------------------

bool Loebner_Register();
bool Loebner_Status();
void Loebner_Emit(const char *szJSON);
char *JSON_stringify(JSON_TYPE Json_Type, const char *szArg1, ...);
JSON_VALUE JSON_parse(const char *szJSON);
char *JSON_getvalue(JSON_VALUE &JSON_Value, const char *szKey);
char *JSON_getvalue(JSON_VALUE &JSON_Value, int Index);
void JSON_release(JSON_VALUE &JSON_Value);
int EscapeString(const char *szIn, char *szOut, int SizeOut);
int UnescapeString(const char *szIn, char *szOut, int SizeOut);

void CALLBACK Loebner_OnError(const char *szErrorMsg);
void CALLBACK Loebner_OnDisconnect(const char *szReason);
void CALLBACK Loebner_OnMessage(const char *szMessage);


// ----------------------------------------------------------------------------
bool Loebner_Initialisation(HWND hWnd,
														LI_ONNEWROUND OnNewRound,
														LI_ONSTARTROUND OnStartRound,
														LI_ONENDROUND OnEndRound,
														LI_ONMESSAGE OnMessage,
														LI_ONDISCONNECT OnDisconnect) {

	hWndTop = hWnd;

	Li_OnNewRound = OnNewRound;
	Li_OnStartRound = OnStartRound;
	Li_OnEndRound = OnEndRound;
	Li_OnMessage = OnMessage;
	Li_OnDisconnect = OnDisconnect;

	NewRound = false;
	RoundStarted = false;

	return WebSocket_Initialisation(Loebner_OnError, Loebner_OnDisconnect, Loebner_OnMessage);
}

// ----------------------------------------------------------------------------
bool Loebner_End() {

	return WebSocket_End();
}

// ----------------------------------------------------------------------------
bool Loebner_Start(const char *szName, const char *szSecret, const char *szAddress, int Port) {

	lstrcpyn(szBotId, szName, MAX_LEN_NAME);
	lstrcpyn(szBotSecret, szSecret, MAX_LEN_NAME);

	if (!WebSocket_Connect(szAddress, Port)) return false;

	ProcessMessages();

	if (!Loebner_Register()) return false;

	ProcessMessages();

	if (!Loebner_Status()) return false;

	return true;
}

// ----------------------------------------------------------------------------
bool Loebner_Stop() {

	if (!WebSocket_Disconnect()) return false;

	return true;
}

// ----------------------------------------------------------------------------
bool Loebner_Register() {

	char *szJSONMessage;
	char *szJSONFrame;

	szJSONMessage = JSON_stringify(json_map, "status", "register", "id", szBotId, "secret", szBotSecret, NULL);
	szJSONFrame = JSON_stringify(json_list, "control", szJSONMessage, NULL);

	Loebner_Emit(szJSONFrame);

	delete[] szJSONFrame;
	delete[] szJSONMessage;

	return true;
}

// ----------------------------------------------------------------------------
bool Loebner_Status() {

	char *szJSONMessage;
	char *szJSONFrame;

	szJSONMessage = JSON_stringify(json_map, "status", "roundInformation", "id", szBotId, "secret", szBotSecret, NULL);
	szJSONFrame = JSON_stringify(json_list, "control", szJSONMessage, NULL);

	Loebner_Emit(szJSONFrame);

	delete[] szJSONFrame;
	delete[] szJSONMessage;

	return true;
}

//---------------------------------------------------------------------------
void Loebner_SendMessage(const char *szLine) {

	if (RoundStarted) {
		char *szJSONMessage;
		char *szJSONFrame;

		szJSONMessage = JSON_stringify(json_map, "content", szLine, "to", szPartner, "id", szBotId, "secret", szBotSecret, NULL);
		szJSONFrame = JSON_stringify(json_list, "message", szJSONMessage, NULL);

		Loebner_Emit(szJSONFrame);

		delete[] szJSONFrame;
		delete[] szJSONMessage;
	}

}

// ----------------------------------------------------------------------------
void CALLBACK Loebner_OnError(const char *szErrorMsg) {
	MessageBox(hWndTop, szErrorMsg, "Web Socket Error", MB_OK | MB_ICONSTOP);
}

// ----------------------------------------------------------------------------
void CALLBACK Loebner_OnDisconnect(const char *szReason) {
	MessageBox(hWndTop, szReason, "Web Socket deconnection", MB_OK | MB_ICONSTOP);
	if (Li_OnDisconnect != NULL) Li_OnDisconnect(szReason);
}

// ----------------------------------------------------------------------------
void CALLBACK Loebner_OnMessage(const char *szMessage) {
	JSON_VALUE Data;
	JSON_VALUE Msg;
	JSON_VALUE Partners;
	JSON_VALUE Partner;
	char *szValue1;
	char *szValue2;
	char *szStatus;
	char *szPartners;
	char *szBotPartner;
	char *szContent;


	if (strlen(szMessage) < 4) return;
	if (memcmp(szMessage, "42", 2)) return;

	Data = JSON_parse(&szMessage[2]);
	szValue1 = JSON_getvalue(Data, 0);

	if (szValue1 != NULL) {
		if (!strcmp(szValue1,  "control")) {
			szValue2 = JSON_getvalue(Data, 1);
			if (szValue2 != NULL) {
				Msg = JSON_parse(szValue2);
				szStatus = JSON_getvalue(Msg, "status");
				if (szStatus != NULL) {
					if (!strcmp(szStatus, "newRound")) {
						szPartners = JSON_getvalue(Msg, "partners");
						if (szPartners != NULL) {
							Partners = JSON_parse(szPartners);
							szBotPartner = JSON_getvalue(Partners, szBotId);
							if (szBotPartner != NULL) {
								Partner = JSON_parse(szBotPartner);
								lstrcpyn(szPartner, JSON_getvalue(Partner, 0), MAX_LEN_NAME);
							}
							JSON_release(Partners);
						}
						if (Li_OnNewRound != NULL) Li_OnNewRound();
						NewRound = true;
					}
					else if (!strcmp(szStatus, "startRound")) {
						if (!NewRound) {
							if (Li_OnNewRound != NULL) Li_OnNewRound();
							NewRound = true;
						}
						if (!RoundStarted) {
							if (Li_OnStartRound != NULL) Li_OnStartRound();
							RoundStarted = true;
						}
					}
					else if (!strcmp(szStatus, "endRound")) {
						NewRound = false;
						RoundStarted = false;
						if (Li_OnEndRound != NULL) Li_OnEndRound();
					}
				}
				JSON_release(Msg);
			}
		}

		else if (!strcmp(szValue1, "message")) {
			if (RoundStarted) {
				szValue2 = JSON_getvalue(Data, 1);
				Msg = JSON_parse(szValue2);
				szContent = JSON_getvalue(Msg, "content");
				if (szContent != NULL) {
					if (Li_OnMessage != NULL) Li_OnMessage(szContent);
				}
				JSON_release(Msg);
			}
		}

		else if (!strcmp(szValue1, "roundInformation")) {
			//handle round information - if the round is running already then set flag RoundStarted to true.
			szValue2 = JSON_getvalue(Data, 1);
			if (szValue2 != NULL) {
				Msg = JSON_parse(szValue2);

				// Partner
				szPartners = JSON_getvalue(Msg, "partners");
				if (szPartners != NULL) {
					Partners = JSON_parse(szPartners);
					szBotPartner = JSON_getvalue(Partners, szBotId);
					if (szBotPartner != NULL) {
						Partner = JSON_parse(szBotPartner);
						lstrcpyn(szPartner, JSON_getvalue(Partner, 0), MAX_LEN_NAME);
					}
					JSON_release(Partners);
				}

				// Status
				szStatus = JSON_getvalue(Msg, "status");
				if (szStatus != NULL) {
					if (!strcmp(szStatus, "Not Started")) {
						if (Li_OnNewRound != NULL) Li_OnNewRound();
						NewRound = true;
					}
					else if (!strcmp(szStatus, "Running")) {
						if (!NewRound) {
							if (Li_OnNewRound != NULL) Li_OnNewRound();
							NewRound = true;
						}
						if (!RoundStarted) {
							if (Li_OnStartRound != NULL) Li_OnStartRound();
							RoundStarted = true;
						}
					}
					else if (!strcmp(szStatus, "Finished")) {
						if (RoundStarted) {
							NewRound = false;
							RoundStarted = false;
							if (Li_OnEndRound != NULL) Li_OnEndRound();
						}
					}
				}

				JSON_release(Msg);
			}
		}

		else if (!strcmp(szValue1, "AuthError")) {
			MessageBox(hWndTop, "Authentication error (name or secret incorrect)", "Web Socket Error", MB_OK | MB_ICONSTOP);
			WebSocket_Disconnect();
		}

	}

	JSON_release(Data);
}

//---------------------------------------------------------------------------
void Loebner_Emit(const char *szJSON) {
	char *szBufEmit;


	szBufEmit = new char[strlen(szJSON) + 2 + 1];  // size of (42) + ending sero
	wsprintf(szBufEmit, "42%s", szJSON);
	WebSocket_Send(oc_text, szBufEmit);
	delete[] szBufEmit;

}

//---------------------------------------------------------------------------
char *JSON_stringify(JSON_TYPE Json_Type, const char *szArg1, ...) {
	const char ** szArg;
	int LenNeeded;
	char *szBufJSON;
	int i;


	// Calcul of length
	LenNeeded = 3; // length of SepBeg + SepEnd + ending zero
	szArg = &szArg1;
	while (*szArg) {
		LenNeeded += strlen(*szArg) * 2 + 3;  // * 2 because of escape, 3 = Quotes and separator
		szArg += 1;
	}


	szBufJSON = new char[LenNeeded];
	i = 0;

	if (Json_Type == json_list) szBufJSON[i++] = '[';
	else if (Json_Type == json_map) szBufJSON[i++] = '{';

	szArg = &szArg1;
	while (*szArg) {

		if (i > 1) szBufJSON[i++] = ',';
		szBufJSON[i++] = '"';
		i += EscapeString(*szArg, &szBufJSON[i], LenNeeded - i - 3);
		szBufJSON[i++] = '"';
		szArg += 1;

		if (Json_Type == json_map && *szArg) {
			szBufJSON[i++] = ':';
			szBufJSON[i++] = '"';
			i += EscapeString(*szArg, &szBufJSON[i], LenNeeded - i - 3);
			szBufJSON[i++] = '"';
			szArg += 1;
		}

	}

	if (Json_Type == json_list) szBufJSON[i++] = ']';
	else if (Json_Type == json_map) szBufJSON[i++] = '}';
	szBufJSON[i++] = '\0';

	return szBufJSON;
}

//---------------------------------------------------------------------------
JSON_VALUE JSON_parse(const char *szJSON) {
	int i;
	int c;
	char *szStringEscaped, *szString;
	int SizeStringEscaped, SizeString;
	int Start = 0, End = 0;
	char szQuotes[128];
	int LevelQuotes;
	JSON_VALUE JSON_Value;


	SizeStringEscaped = strlen(szJSON) + 1;
	szStringEscaped = new char[SizeStringEscaped];

	for (i = 0; i < MAX_JSON_VALUES; i++) {
		JSON_Value.szKeys[i] = NULL;
		JSON_Value.szValues[i] = NULL;
	}

	i = 0;
	LevelQuotes = false;
	JSON_Value.NbValues = 0;
	while ((c = szJSON[i++]) != '\0') {

		if (LevelQuotes == 1 && (c == ',' || c == ':' || c == szQuotes[0])) {
			strncpy_s(szStringEscaped, SizeStringEscaped, &szJSON[Start], End - Start);
			szStringEscaped[End - Start] = '\0';
			SizeString = End - Start + 1;
			szString = new char[SizeString];
			UnescapeString(szStringEscaped, szString, SizeString);
			if (c == ':') {
				JSON_Value.szKeys[JSON_Value.NbValues] = szString;
			}
			else {
				JSON_Value.szValues[JSON_Value.NbValues++] = szString;
			}
		}

		if (c == '\\') {
			// Any escaped character
			i++;
		}
		else if (c == '"') {
			// Quote
			if (LevelQuotes == 0 || szQuotes[LevelQuotes - 1] != '"') {
				if (LevelQuotes == 1) Start = i;
				szQuotes[LevelQuotes++] = '"';
			}
			else {
				LevelQuotes--;
				if (LevelQuotes == 1) End = i - 1;
			}
		}
		else if (c == '{') {
			// Braces 1 {}
			if (LevelQuotes == 1) Start = i - 1;
			szQuotes[LevelQuotes++] = '}';
		}
		else if (c == '}') {
			// Braces 1 {}
			if (LevelQuotes > 0 && szQuotes[LevelQuotes - 1] == '}') {
				LevelQuotes--;
				if (LevelQuotes == 1) End = i;
			}
		}
		else if (c == '[') {
			// Braces 2 []
			if (LevelQuotes == 1) Start = i - 1;
			szQuotes[LevelQuotes++] = ']';
		}
		else if (c == ']') {
			// Braces 2 []
			if (LevelQuotes > 0 && szQuotes[LevelQuotes - 1] == ']') {
				LevelQuotes--;
				if (LevelQuotes == 1) End = i;
			}
		}
	}

	delete[] szStringEscaped;

	return JSON_Value;
}

//---------------------------------------------------------------------------
char *JSON_getvalue(JSON_VALUE &JSON_Value, const char *szKey) {
	int i;


	for (i = 0; i < JSON_Value.NbValues; i++) {
		if (JSON_Value.szKeys[i] != NULL && !strcmp(JSON_Value.szKeys[i], szKey)) {
			return JSON_Value.szValues[i];
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
char *JSON_getvalue(JSON_VALUE &JSON_Value, int Index) {

	if (Index < JSON_Value.NbValues) {
		return JSON_Value.szValues[Index];
	}

	return NULL;
}

//---------------------------------------------------------------------------
void JSON_release(JSON_VALUE &JSON_Value) {
	int i;


	for (i = 0; i < JSON_Value.NbValues; i++) {
		delete[] JSON_Value.szKeys[i];
		delete[] JSON_Value.szValues[i];
	}

}

// ----------------------------------------------------------------------------
int EscapeString(const char *szIn, char *szOut, int SizeOut) {
	int i, j;
	char c;


	i = 0;
	j = 0;
	while ((c = szIn[i++]) != '\0') {
		if (c == '\\' || c == '"') {
			if (j >= SizeOut - 2) break;
			szOut[j++] = '\\';
			szOut[j++] = c;
		}
		else {
			if (j >= SizeOut - 1) break;
			szOut[j++] = c;
		}
	}
	szOut[j] = '\0';

	return j;
}

// ----------------------------------------------------------------------------
int UnescapeString(const char *szIn, char *szOut, int SizeOut) {
	int i, j;
	char c;


	i = 0;
	j = 0;
	while ((c = szIn[i++]) != '\0') {
		if (j >= SizeOut - 1) break;
		if (c == '\\') {
			c = szIn[i++];
			if (c == '\0') break;
		}
		szOut[j++] = c;
	}
	szOut[j] = '\0';

	return j;
}


// ----------------------------------------------------------------------------
