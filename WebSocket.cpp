// Minimum version of windows = VISTA (for IP v6 addresses) but it's works however on XP sp3
#if(_WIN32_WINNT < 0x0600)
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif // _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif // _WIN32_WINNT < 0x0600

#include <winsock2.h>

#include <windows.h>
#include <String>
#include <Vector>
#include <assert.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include <wincrypt.h>

#include "WebSocket.h"


//---------------------------------------------------------------------------
// GLOBAL VARIABLES
//---------------------------------------------------------------------------

#define SIZE_RECVBUF 4096
#define SIZE_SOCKETID 50

#define PROTOCOL_NULL		0
#define PROTOCOL_HTTP		1
#define PROTOCOL_SOCKIO	2

// Traces
char szLogFile[MAX_PATH];

// Cryptographics datas
HCRYPTPROV hCryptProv = NULL;
HCRYPTHASH hCryptHash = NULL;
HCRYPTKEY hCryptKey = NULL;

// Socket
WSADATA WSAData;
SOCKET Socket;
SOCKET SocketDistant;
HWND hWndSocket;
int Port;
char RecvBuf[SIZE_RECVBUF];
char SocketID[SIZE_SOCKETID];
bool bEventRead;
bool bEventConnect;
bool bConnected;

char *RequestGET = NULL;
char *RequestAccept = NULL;
char *RequestAccept_Language = NULL;
char *RequestAccept_Encoding = NULL;
char *RequestOrigin = NULL;
char *RequestSec_WebSocket_Key = NULL;
char *RequestConnection = NULL;
char *RequestUpgrade = NULL;
char *RequestSec_WebSocket_Version = NULL;
char *RequestUser_Agent = NULL;
char *RequestHost = NULL;
char *RequestDNT = NULL;
char *RequestCache_Control = NULL;
char *RequestCookie = NULL;

char *ReturnContent_Type = NULL;
char *ReturnContent_Length = NULL;
char *ReturnAccess_Control_Allow_Origin = NULL;
char *ReturnX_XSS_Protection = NULL;
char *ReturnUpgrade = NULL;
char *ReturnSec_WebSocket_Accept = NULL;
char *ReturnSet_Cookie = NULL;
char *ReturnDate = NULL;
char *ReturnConnection = NULL;

char ApplicationData[SIZE_RECVBUF];

int ReturnCode;
int Reason;

int Protocol = PROTOCOL_NULL;

char *Cookies = NULL;

WS_ONERROR WS_OnError;
WS_ONDISCONNECT WS_OnDisconnect;
WS_ONMESSAGE WS_OnMessage;

#define _T(s) s


//-----------------------FUNCTIONS PROTOTYPES--------------------------

bool HTTPRequest();
bool HTTPResponse(char *szFrame);
bool SockIOResponse(char *szFrame);
bool SockIOEncode(const char *szAppData, OPCODE OpCode, bool Fin, bool RSV1, bool RSV2, bool RSV3, bool Masked, int *Len, char *szFrame);
bool SockIODecode(const char *szFrame, OPCODE *OpCode, bool *Fin, bool *RSV1, bool *RSV2, bool *RSV3, bool *Masked, int *Len, char *szAppData);
void DisplayError(const char *szFunction, int LastError);
void DisplayError(const char *szFunction, char *szError);
LRESULT FAR PASCAL WndSocketProc(HWND Handle, UINT Message, WPARAM wParam, LPARAM lParam);
int SendBuf(const void *Buf, int Count);
int ReceiveBuf(void *Buf, int Count);
bool Trace(const void *Buf, int Count, bool bClient);
bool Base64Encode(const BYTE *BufIn, DWORD SizeIn, char *BufOut, DWORD *dwOut);
bool Base64Decode(const char *BufIn, DWORD SizeIn, BYTE *BufOut, DWORD *dwOut);
char *CloseReason(int Reason);
LPSTR lstrtok(LPSTR FAR * lpNext, char delim);
char *TimeStamp();

bool OnConnect();
bool OnDisconnect();
bool OnRead();


// ----------------------------------------------------------------------------
bool WebSocket_Initialisation(WS_ONERROR OnError, WS_ONDISCONNECT OnDisconnect, WS_ONMESSAGE OnMessage) {
	char szTempPath[MAX_PATH];


  Port = 0;
  Socket = INVALID_SOCKET;
	SocketDistant = INVALID_SOCKET;
	bConnected = false;
	HINSTANCE HInstance = NULL;
	WS_OnError = OnError;
	WS_OnDisconnect = OnDisconnect;
	WS_OnMessage = OnMessage;

	// Log file
	GetTempPath(MAX_PATH, szTempPath);
	GetTempFileName(szTempPath, _T("LOG"), 0, szLogFile);

	if (WSAStartup(MAKEWORD(1, 1), &WSAData)) {
		WS_OnError("Cannot initialize WSAStartup");
    return false;
  }

  if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
    // Error message init cryptographics functions
    int LastError = GetLastError();
		DisplayError(_T("Cannot initialize crypto API"), LastError);
    return false;
  }

	WNDCLASS WC;
	if (!GetClassInfo(HInstance, _T("CLASSWEBSOCKET"), &WC)) {
		WC.style = 0;
		WC.lpfnWndProc = WndSocketProc;
		WC.cbClsExtra = 0;
		WC.cbWndExtra = 0;
		WC.hInstance = HInstance;
		WC.hIcon = NULL;
		WC.hCursor = NULL;
		WC.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		WC.lpszMenuName = 0;
		WC.lpszClassName = _T("CLASSWEBSOCKET");
		if (!RegisterClass(&WC)) {
			int LastError = GetLastError();
			DisplayError(_T("RegisterClass CLASSWEBSOCKET"), LastError);
			return false;
		}
	}

  hWndSocket = CreateWindow(_T("CLASSWEBSOCKET"),
                            _T("WebSocket"),
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            NULL,
                            NULL, HInstance, NULL);
  if (!hWndSocket) {
    int LastError = GetLastError();
    DisplayError(_T("CreateWindow CLASSWEBSOCKET"), LastError);
    return false;
  }

	return true;
}

// ----------------------------------------------------------------------------
bool WebSocket_End() {

	// Destroy crypto key.
  if (hCryptKey) CryptDestroyKey(hCryptKey);

  // Destroy object hCryptHash.
  if (hCryptHash) CryptDestroyHash(hCryptHash);

  // Free handle of CSP.
  if (hCryptProv) CryptReleaseContext(hCryptProv, 0);

  WSACleanup();

	delete[] Cookies;

  return true;
}

// ----------------------------------------------------------------------------
bool WebSocket_Connect(const char *szAddress, int Port) {
  struct sockaddr_in SockAddr4;
  unsigned int r;
  int LastError;


  Socket = INVALID_SOCKET;
	SocketDistant = INVALID_SOCKET;

  SockAddr4.sin_addr.S_un.S_addr = inet_addr(szAddress);

  Socket = socket(AF_INET, SOCK_STREAM, 0);
  if (Socket == INVALID_SOCKET) {
    LastError = WSAGetLastError();
    if (LastError) {
      DisplayError("socket", LastError);
    }
    return false;
  }

  WSAAsyncSelect(Socket, hWndSocket, UM_MESSAGE, FD_CONNECT | FD_CLOSE | FD_READ);

	SockAddr4.sin_family = AF_INET;
	SockAddr4.sin_port = htons((u_short) Port);
	r = connect(Socket, (LPSOCKADDR) &SockAddr4, sizeof(SockAddr4));

	if (r == INVALID_SOCKET) {
    LastError = WSAGetLastError();
    if (LastError != WSAEWOULDBLOCK) {
      DisplayError("connect", LastError);
			closesocket(Socket);
			Socket = INVALID_SOCKET;
	    return false;
    }
  }
	
	// Waiting for message UM_MESSAGE FD_CONNECT (else SocketDistant is not valued)
	bEventConnect = false;
	while (!bEventConnect) {
		ProcessMessages();
		Sleep(20);
	}

	if (!bConnected || SocketDistant == INVALID_SOCKET) return false;

	return true;
}

// ----------------------------------------------------------------------------
bool WebSocket_Disconnect() {

	if (Socket != INVALID_SOCKET) {
    closesocket(Socket);
    Socket = INVALID_SOCKET;
  }
	bConnected = false;

	return true;
}

//---------------------------------------------------------------------------
bool WebSocket_Receive() {
	int i;

	bEventRead = false;

	for (i = 0; i < 1000; i += 20) {

		// Messages loop:
		ProcessMessages();

		if (bEventRead) break;
		Sleep(20);
	}

	return true;
}

// ----------------------------------------------------------------------------
bool WebSocket_Send(OPCODE OpCode, const char *szMessage) {
	char *szFrame;
	int Len, MaxLen;
	bool Ok;


	Len = strlen(szMessage);
	MaxLen = Len + 14; // FIN + RSV + OpCode = 1, Len <= 9, MaskingKey = 4
	szFrame = new char[MaxLen];

	Ok = SockIOEncode(szMessage, OpCode, true, false, false, false, true, &Len, szFrame);

	if (Ok) {
		SendBuf(szFrame, Len);
		Trace(szFrame, Len, true);
	}

	delete[] szFrame;

	return Ok;
}

// ----------------------------------------------------------------------------
bool WebSocket_Ping() {
	char szPing[30];
	
	strcpy_s(szPing, 20, TimeStamp());
	WebSocket_Send(oc_text, "2");

	if (!WebSocket_Receive()) {
	  DisplayError("HTTPRequest", "Ping timeout");
		return false;
	}

	if (strcmp(ApplicationData, "3")) {
		WebSocket_Disconnect();
		DisplayError(_T("OnConnect"), _T("Bad response to ping"));
		return false;
	}
	return true;
}

// ----------------------------------------------------------------------------
bool OnConnect() {
	DWORD pcbIn, pcbOut;
	char szArgsGET[50];
	char szGet[256];
	BYTE szWebSocketKey[16];
	char szWebSocketKeyBase64[61]; // 24 (Base64) + 36 (GUID) + 1
	char szAcceptKey[80];
	BYTE szHashSha1[256];
	int i;
	BOOL Ok;


	Protocol = PROTOCOL_HTTP;
	wsprintf(szArgsGET, "&transport=polling&t=%s", TimeStamp());
	RequestGET = szArgsGET;
	RequestAccept = "*/*";
	RequestAccept_Language = "en-EN";
	RequestAccept_Encoding = "gzip, deflate";
	RequestUser_Agent = "Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko";
	RequestHost = "127.0.0.1:8080";
	RequestDNT = "1";
	RequestConnection = "Keep-Alive";
	RequestCookie = "io=pTzblLqqYY3Vn6w5AAAm";
	if (!HTTPRequest()) return false;

	if (!memcmp(Cookies, "io=", 3)) {
		lstrcpyn(SocketID, &Cookies[3], SIZE_SOCKETID);
	}

	for (i = 0; i < 16; i++) {
		szWebSocketKey[i] = rand() * 256 / RAND_MAX;
	}
	pcbOut = 61;
  Ok = Base64Encode(szWebSocketKey, 16, szWebSocketKeyBase64, &pcbOut);
	if (!Ok) {
		DisplayError(_T("Base64Encode"), _T("Buffer too small"));
		return false;
	}

	sprintf_s(szGet, 256, "&transport=websocket&sid=%s", SocketID);
	RequestGET = szGet;
	RequestOrigin = "null";
	RequestSec_WebSocket_Key = szWebSocketKeyBase64;
	RequestConnection = "Upgrade";
	RequestUpgrade = "Websocket";
	RequestSec_WebSocket_Version = "13";
	RequestCache_Control = "no-cache";
	RequestCookie = Cookies;
	if (!HTTPRequest()) return false;
	if (ReturnSec_WebSocket_Accept == NULL) return false; 

	strcat_s(szWebSocketKeyBase64, 61, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

	Ok = CryptCreateHash(hCryptProv, CALG_SHA1, 0, 0, &hCryptHash);
	if (!Ok) {
		int LastError = GetLastError();
		DisplayError(_T("CryptCreateHash"), LastError);
		return false;
	}

  Ok = CryptHashData(hCryptHash, (BYTE *) szWebSocketKeyBase64, strlen(szWebSocketKeyBase64), 0);
	if (!Ok) {
		int LastError = GetLastError();
		DisplayError(_T("CryptHashData"), LastError);
		return false;
	}

  pcbOut = 256;
  Ok = CryptGetHashParam(hCryptHash, HP_HASHVAL, szHashSha1, &pcbOut, 0);
	if (!Ok) {
		int LastError = GetLastError();
		DisplayError(_T("CryptGetHashParam"), LastError);
		return false;
	}

  pcbIn = pcbOut;
	pcbOut = 80;
  Ok = Base64Encode(szHashSha1, pcbIn, szAcceptKey, &pcbOut);
	if (!Ok) {
		DisplayError(_T("Base64Encode"), _T("Buffer too small"));
		return false;
	}

	Ok = !strcmp(szAcceptKey, ReturnSec_WebSocket_Accept);
	if (!Ok) {
		DisplayError(_T("OnConnect"), _T("Bad accept key"));
		return false;
	}

	Protocol = PROTOCOL_SOCKIO;

	WebSocket_Send(oc_text, "2probe");

	if (!WebSocket_Receive()) {
		DisplayError(_T("OnConnect"), _T("No response to ping"));
		return false;
	}
	if (strcmp(ApplicationData, "3probe")) {
		DisplayError(_T("OnConnect"), _T("Bad response to ping"));
		return false;
	}

	WebSocket_Send(oc_text, "5");

	bConnected = true;

	return true;
}

// ----------------------------------------------------------------------------
bool OnDisconnect() {
	WS_OnDisconnect("Socket deconnected");

	return true;
}

// ----------------------------------------------------------------------------
bool OnRead() {
	int NbRead;


	NbRead = ReceiveBuf(RecvBuf, SIZE_RECVBUF);
	Trace(RecvBuf, NbRead, false);
	if (Protocol == PROTOCOL_HTTP) {
		HTTPResponse(RecvBuf);
	}
	else if (Protocol == PROTOCOL_SOCKIO) {
		SockIOResponse(RecvBuf);
	}
	bEventRead = true;

	return true;
}

//---------------------------------------------------------------------------
bool HTTPRequest() {
	char *szFrame;
	int i, MaxLen;


	MaxLen = 1024;
	szFrame = new char[MaxLen];
	i = 0;
	if (RequestGET) {
		sprintf_s(&szFrame[i], MaxLen - i, "GET /socket.io/?EIO=3%s HTTP/1.1\n", RequestGET);
		i = strlen(szFrame);
	}
	if (RequestAccept) {
		sprintf_s(&szFrame[i], MaxLen - i, "Accept: %s\n", RequestAccept);
		i = strlen(szFrame);
	}
	if (RequestAccept_Language) {
		sprintf_s(&szFrame[i], MaxLen - i, "Accept-Language: %s\n", RequestAccept_Language);
		i = strlen(szFrame);
	}
	if (RequestAccept_Encoding) {
		sprintf_s(&szFrame[i], MaxLen - i, "Accept-Encoding: %s\n", RequestAccept_Encoding);
		i = strlen(szFrame);
	}
	if (RequestOrigin) {
		sprintf_s(&szFrame[i], MaxLen - i, "Origin: %s\n", RequestOrigin);
		i = strlen(szFrame);
	}
	if (RequestSec_WebSocket_Key) {
		sprintf_s(&szFrame[i], MaxLen - i, "Sec-WebSocket-Key: %s\n", RequestSec_WebSocket_Key);
		i = strlen(szFrame);
	}
	if (RequestSec_WebSocket_Version) {
		sprintf_s(&szFrame[i], MaxLen - i, "Sec-WebSocket-Version: %s\n", RequestSec_WebSocket_Version);
		i = strlen(szFrame);
	}
	if (RequestUser_Agent) {
		sprintf_s(&szFrame[i], MaxLen - i, "User-Agent: %s\n", RequestUser_Agent);
		i = strlen(szFrame);
	}
	if (RequestHost) {
		sprintf_s(&szFrame[i], MaxLen - i, "Host: %s\n", RequestHost);
		i = strlen(szFrame);
	}
	if (RequestDNT) {
		sprintf_s(&szFrame[i], MaxLen - i, "DNT: %s\n", RequestDNT);
		i = strlen(szFrame);
	}
	if (RequestConnection) {
		sprintf_s(&szFrame[i], MaxLen - i, "Connection: %s\n", RequestConnection);
		i = strlen(szFrame);
	}
	if (RequestUpgrade) {
		sprintf_s(&szFrame[i], MaxLen - i, "Upgrade: %s\n", RequestUpgrade);
		i = strlen(szFrame);
	}
	if (RequestCache_Control) {
		sprintf_s(&szFrame[i], MaxLen - i, "Cache-Control: %s\n", RequestCache_Control);
		i = strlen(szFrame);
	}
	if (RequestCookie) {
		sprintf_s(&szFrame[i], MaxLen - i, "Cookie: %s\n", RequestCookie);
		i = strlen(szFrame);
	}
	sprintf_s(&szFrame[i], MaxLen - i, "\n");

	SendBuf(szFrame, strlen(szFrame));
	Trace(szFrame, strlen(szFrame), true);
	delete[] szFrame;

	// Reset all fields except RequestUser_Agent, RequestHost, RequestDNT and RequestCookie
	// The others are in principle not repeated
	RequestGET = NULL;
	RequestAccept = NULL;
	RequestAccept_Language = NULL;
	RequestAccept_Encoding = NULL;
	RequestOrigin = NULL;
	RequestSec_WebSocket_Key = NULL;
	RequestConnection = NULL;
	RequestUpgrade = NULL;
	RequestSec_WebSocket_Version = NULL;
	RequestCache_Control = NULL;

	if (!WebSocket_Receive()) {
	  DisplayError("HTTPRequest", "Timeout request");
		return false;
	}

	if (ReturnCode != 200 && ReturnCode != 101) {
		char szErrMsg[30];
		sprintf_s(szErrMsg, 30, "Error HTTP %i", ReturnCode);
	  DisplayError("HTTPRequest", szErrMsg);
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------
bool HTTPResponse(char *szFrame) {
	char *lp, *lp2;
	char szArg1[64];
	char szArg2[128];
	int i;

	/*
	HTTP/1.1 200 OK
	Content-Type: application/octet-stream
	Content-Length: 101
	Access-Control-Allow-Origin: *
	X-XSS-Protection: 0
	Set-Cookie: io=-V6RthdA_184EZ0GAAAn; Path=/; HttpOnly
	Date: Mon, 23 Jan 2017 20:57:58 GMT
	Connection: keep-alive
	*/

	// First line : HTTP
	lp = szFrame;
	lp2 = lstrtok(&lp, '\n');
	lstrcpyn(szArg1, lstrtok(&lp2, ' '), 64);
	if (strcmp(szArg1, "HTTP/1.1")) {
		ReturnCode = 400;
		return false;
	}

	lstrcpyn(szArg2, lstrtok(&lp2, ' '), 128);
	ReturnCode = atoi(szArg2);
	if (ReturnCode != 200 && ReturnCode != 101) return false;

	// Next lines
	do {

		lstrcpyn(szArg1, lstrtok(&lp2, ' '), 64);
		if (!strcmp(szArg1, "Content-Type:")) {
			ReturnContent_Type = lstrtok(&lp2, ' ');
		}
		else if (!strcmp(szArg1, "Content-Length:")) {
			ReturnContent_Length = lstrtok(&lp2, ' ');
		}
		else if (!strcmp(szArg1, "Access-Control-Allow-Origin:")) {
			ReturnAccess_Control_Allow_Origin = lstrtok(&lp2, ' ');
		}
		else if (!strcmp(szArg1, "X-XSS-Protection:")) {
			ReturnX_XSS_Protection = lstrtok(&lp2, ' ');
		}
		else if (!strcmp(szArg1, "Upgrade:")) {
			ReturnUpgrade = lstrtok(&lp2, ' ');
		}
		else if (!strcmp(szArg1, "Sec-WebSocket-Accept:")) {
			ReturnSec_WebSocket_Accept = lstrtok(&lp2, ' ');
		}
		else if (!strcmp(szArg1, "Set-Cookie:")) {
			ReturnSet_Cookie = lstrtok(&lp2, ';');
			int Len = strlen(ReturnSet_Cookie) + 1;
			delete[] Cookies;
			Cookies = new char[Len];
			lstrcpyn(Cookies, ReturnSet_Cookie, Len);
		}
		else if (!strcmp(szArg1, "Date:")) {
			ReturnDate = lp2;
		}
		else if (!strcmp(szArg1, "Connection:")) {
			ReturnConnection = lstrtok(&lp2, ' ');
		}

		lp2 = lstrtok(&lp, '\n');
		
		// Deleting end carriage returns
		i = strlen(lp2) - 1;
		while (i >= 0 && lp2[i] == '\r') lp2[i--] = '\0';

	} while (*lp2);

	return true;
}

//---------------------------------------------------------------------------
bool SockIOResponse(char *szFrame) {
	OPCODE OpCode;
	bool Fin, RSV1, RSV2, RSV3, Masked;
	int Len;
	int i;


	if (!SockIODecode(szFrame, &OpCode, &Fin, &RSV1, &RSV2, &RSV3, &Masked, &Len, ApplicationData)) return false;

	if (OpCode == oc_ping) {
		WebSocket_Send(oc_pong, ApplicationData);
	}
	else if (OpCode == oc_close) {
		Reason = 0;
		for (i = 0; i < Len; i++) {
			Reason *= 256;
			Reason += (BYTE) ApplicationData[i];
		}
		WS_OnDisconnect(CloseReason(Reason));
		WebSocket_Disconnect();
	}
	else if (OpCode == oc_text) {
		if (bConnected) {
			WS_OnMessage(ApplicationData);
		}
	}

	return true;
}

//---------------------------------------------------------------------------
bool SockIOEncode(const char *szAppData, OPCODE OpCode, bool Fin, bool RSV1, bool RSV2, bool RSV3, bool Masked, int *Len, char *szFrame) {
	int i, j, LenAppData;
	BYTE b;
	BYTE MaskingKey[4];


	LenAppData = strlen(szAppData);
	memset(&MaskingKey, '\0', sizeof(MaskingKey));
	i = 0;

	b = 0x80 | OpCode;
	szFrame[i++] = b;

	if (Masked) b = 0x80;
	else b = 0x00;

	if (LenAppData < 126) {
		b |= (BYTE) LenAppData;
		szFrame[i++] = b;
	}
	else if (LenAppData < 65536) {
		b |= 126;
		szFrame[i++] = b;
		for (j = 0; j < 2; j++) {
			b = (BYTE) (LenAppData >> (8 * (1 - j))) & 0xFF;
			szFrame[i++] = b;
		}
	}
	else {
		b |= 127;
		szFrame[i++] = b;
		for (j = 0; j < 8; j++) {
			b = (BYTE) (LenAppData >> (8 * (1 - j))) & 0xFF;
			szFrame[i++] = b;
		}
	}

	if (Masked) {
		for (j = 0; j < 4; j++) {
			MaskingKey[j] = rand() * 256 / RAND_MAX;
			szFrame[i++] = MaskingKey[j];
		}
	}

	for (j = 0; j < LenAppData; j++) {
		szFrame[i++] = szAppData[j] ^ MaskingKey[j % 4];
	}
	szFrame[i] = '\0';

	*Len = i;

	return true;
}

//---------------------------------------------------------------------------
bool SockIODecode(const char *szFrame, OPCODE *OpCode, bool *Fin, bool *RSV1, bool *RSV2, bool *RSV3, bool *Masked, int *Len, char *szAppData) {
	int i = 0;
	int j;
	BYTE MaskingKey[4];
	BYTE b;


	b = szFrame[i++];
	*Fin = ((b & 0x80) != 0);
	*RSV1 = ((b & 0x40) != 0);
	*RSV2 = ((b & 0x20) != 0);
	*RSV3 = ((b & 0x10) != 0);
	*OpCode = (OPCODE) (b & 0x0F);
	if (*RSV1 || *RSV2 || *RSV3) return false;

	b = szFrame[i++];
	*Masked = ((b & 0x80) != 0);
	*Len = b & 0x7F;

	if (*Len == 126) {
		*Len = 0;
		for (j = 0; j < 2; j++) {
			(*Len) *= 256;
			(*Len) += szFrame[i++];
		}
	}
	else if (*Len == 127)  {
		for (j = 0; j < 8; j++) {
			(*Len) *= 256;
			(*Len) += szFrame[i++];
		}
	}

	if (*Masked) {
		for (j = 0; j < 4; j++) {
			MaskingKey[j] = szFrame[i++];
		}
	}
	else {
		memset(&MaskingKey, '\0', sizeof(MaskingKey));
	}

	for (j = 0; j < *Len; j++) {
		szAppData[j] = szFrame[i++] ^ MaskingKey[j % 4];
	}
	szAppData[j] = '\0';

	return true;
}

//---------------------------------------------------------------------------
int SendBuf(const void *Buf, int Count) {
	if (SocketDistant == INVALID_SOCKET) {
    int LastError = WSAENOTCONN;
    DisplayError(_T("SendBuf"), LastError);
		return 0;
	}
  return send(SocketDistant, (const char *) Buf, Count, 0);
}

//---------------------------------------------------------------------------
int ReceiveBuf(void *Buf, int Count) {
	if (SocketDistant == INVALID_SOCKET) {
    int LastError = WSAENOTCONN;
    DisplayError(_T("ReceiveBuf"), LastError);
		return 0;
	}
  return recv(SocketDistant, (char *) Buf, Count, 0);
}

//---------------------------------------------------------------------------
bool Trace(const void *Buf, int Count, bool bClient) {
	HANDLE hFile;
  DWORD NbOctets;
	char szHexa[3];
	int i;


  hFile = CreateFile(szLogFile,
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                     NULL);
	SetFilePointer(hFile, 0, NULL, FILE_END);
	if (bClient) WriteFile(hFile, "\n---------- CLIENT ----------\n", 30, &NbOctets, NULL);
	else WriteFile(hFile, "\n---------- SERVER ----------\n", 30, &NbOctets, NULL);
	if (Protocol == PROTOCOL_HTTP) {
		WriteFile(hFile, Buf, Count, &NbOctets, NULL);
	}
	else if (Protocol == PROTOCOL_SOCKIO) {
		OPCODE OpCode;
		bool Fin, RSV1, RSV2, RSV3, Masked;
		int Len;
		char *szFrame;
		char szOpCode[20];


		for (i = 0; i < Count; i++) {
			sprintf_s(szHexa, 3, "%02X", ((BYTE *) Buf)[i]);
			WriteFile(hFile, szHexa, 2, &NbOctets, NULL);
		}
		WriteFile(hFile, "\n", 1, &NbOctets, NULL);
		szFrame = new char[SIZE_RECVBUF];
		if (SockIODecode((char *) Buf, &OpCode, &Fin, &RSV1, &RSV2, &RSV3, &Masked, &Len, szFrame)) {
			sprintf_s(szOpCode, 20, "OpCode = %i ", OpCode);
			WriteFile(hFile, szOpCode, strlen(szOpCode), &NbOctets, NULL);
			if (Masked) WriteFile(hFile, "masked ", 7, &NbOctets, NULL);
			else WriteFile(hFile, "not masked ", 11, &NbOctets, NULL);
			WriteFile(hFile, szFrame, Len, &NbOctets, NULL);
			WriteFile(hFile, "\n", 1, &NbOctets, NULL);
		}
		delete[] szFrame;
	}
  CloseHandle(hFile);

	return true;
}

//---------------------------------------------------------------------------
bool Base64Encode(const BYTE *BufIn, DWORD SizeIn, char *BufOut, DWORD *dwOut) {
  DWORD i;
	int j;
  char c;
  char c6bits;
  DWORD dwIn;
  int cbIn;
	int NbBits;
	DWORD SizeOut;


  dwIn = 0;
  cbIn = 0;
	NbBits = 16;
	SizeOut = ((SizeIn + 2) / 3) * 4 + 1;  // First multiple of 4 >= SizeIn * 4/3 + ending character
	if (*dwOut < SizeOut) {
		*dwOut = SizeOut;
		return false;
	}
	*dwOut = 0;
  for (i = 0; i < SizeIn; i++) {
    dwIn |= (BufIn[i] << NbBits);
		NbBits -= 8;
    if (++cbIn == 3 || i == SizeIn - 1) {
      for (j = 0; j < 4; j++) {
				c = '=';
				if (j < cbIn + 1) {  // The real calcul would be (cbIn * 8 + 5) / 6 but while 0 < cbIn <= 3 it is the same thing
					c6bits = (char) ((dwIn & 0x00FC0000) >> 18);
					if (c6bits < 26) c = 'A' + c6bits;
					else if (c6bits < 52) c = 'a' + c6bits - 26;
					else if (c6bits < 62) c = '0' + c6bits - 52;
					else if (c6bits == 62) c = '+';
					else if (c6bits == 63) c = '/';
					dwIn <<= 6;
				}
				BufOut[(*dwOut)++] = c;
      }
      dwIn = 0;
      cbIn = 0;
			NbBits = 16;
    }
  }
  BufOut[(*dwOut)++] = '\0';
	assert(*dwOut <= SizeOut);

  return true;
}

//---------------------------------------------------------------------------
bool Base64Decode(const char *BufIn, DWORD SizeIn, BYTE *BufOut, DWORD *dwOut) {
  DWORD i;
	int j;
  char c;
  char c6bits;
  DWORD dwIn;
	int NbBits;
	DWORD SizeOut;
  int cbIn;


  dwIn = 0;
  cbIn = 0;
	NbBits = 18;
	SizeOut = *dwOut;
	*dwOut = 0;
	for (i = 0; i < SizeIn; i++) {
    c = BufIn[i];
    if ((BYTE) c >= ' ') {
      if ('A' <= c && c <= 'Z') c6bits = (DWORD) (BYTE) c - 'A';
      else if ('a' <= c && c <= 'z') c6bits = (DWORD) (BYTE) c - 'a' + 26;
      else if ('0' <= c && c <= '9') c6bits = (DWORD) (BYTE) c - '0' + 52;
      else if (c == '+') c6bits = 62;
      else if (c == '/') c6bits = 63;
      dwIn |= (c6bits << NbBits);
			NbBits -= 6;
      if (c == '=' || ++cbIn == 4 || i == SizeIn - 1) {
				for (j = 0; j < cbIn - 1; j++) {  // The real calcul would be (cbIn * 6) / 8 but while 1 < cbIn <= 4 it is the same thing
					if ((*dwOut) + 1 >= SizeOut) return false;
					BufOut[(*dwOut)++] = (char) ((dwIn & 0x00FF0000) >> 16);
					dwIn <<= 8;
				}
        dwIn = 0;
        cbIn = 0;
				NbBits = 18;
      }
			if (c == '=') break;
    }
  }

  return true;
}

//---------------------------------------------------------------------------
char *CloseReason(int Reason) {
	switch (Reason) {
	case 1000:
		return "1000 Normal closure";
	case 1001:
		return "1001 Endpoint is going away";
	case 1002:
		return "1002 Protocol error";
	case 1003:
		return "Incorrect type of data";
	case 1004:
		return "1004 Reserved reason";
	case 1005:
		return "1005 No status code is actually present";
	case 1006:
		return "1006 Connection was closed abnormally";
	case 1007:
		return "1007 Data within a message that was not consistent";
	case 1008:
		return "1008 Message violates the policy";
	case 1009:
		return "1009 Message too big";
	case 1010:
		return "1010 Extension expected";
	case 1011:
		return "1011 Unexpected condition";
	case 1015:
		return "1015 Failure to perform a TLS handshake";
	default:
		return "Unknown reason";
	}

}

//****************************************************************************
// String split
//****************************************************************************

LPSTR lstrtok(LPSTR FAR * lpNext, char delim) {
  LPSTR lpBuf;

  lpBuf = (*lpNext);

  while ( ** lpNext) {
    if ( ** lpNext == delim) {
       ** lpNext = 0;
      (*lpNext)++;
      break;
    }

    (*lpNext)++;
  }

  return lpBuf;
}

//---------------------------------------------------------------------------
char *TimeStamp() {
	time_t Now;
	static char szTimeStramp[30];
	const char Code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+";
	int i;
	char c;
	int Len;

	
	time(&Now);
	i = 0;
	while (Now) {
		szTimeStramp[i++] = Code[Now % sizeof(Code)];
		Now /= sizeof(Code);
	}
	szTimeStramp[i++] = '\0';

	Len = strlen(szTimeStramp);
	for (i = 0; i < Len / 2; i++) {
		c = szTimeStramp[i];
		szTimeStramp[i] = szTimeStramp[Len - i - 1];
		szTimeStramp[Len - i - 1] = c;
	}

	return szTimeStramp;
}

//---------------------------------------------------------------------------
void ProcessMessages(void) {
	MSG msg;


	// Messages loop:
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

}

//---------------------------------------------------------------------------
void DisplayError(const char *szFunction, int LastError) {
  char szMessage[256];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, LastError, 0, szMessage, 256, NULL);
	DisplayError(szFunction, szMessage);

}

//---------------------------------------------------------------------------
void DisplayError(const char *szFunction, char *szError) {
  char szMessage[256];

  sprintf_s(szMessage, 256, "Error in function %s : %s",
											(LPCSTR) szFunction, szError);
	if (WS_OnError) {
		WS_OnError(szMessage);
	}
	else {
		MessageBox(NULL, szMessage, _T("WebSocket"), MB_OK | MB_ICONSTOP);
	}
}



//---------------------------------------------------------------------------
// FUNCTION: WndSocketProc (HWND, UINT, WPARAM, LPARAM)
//   Window for socket processing
//---------------------------------------------------------------------------

LRESULT FAR PASCAL WndSocketProc(HWND Handle,
    UINT Message, WPARAM wParam, LPARAM lParam) {
  int LastError;
	static UINT IdTimer;


  switch (Message) {
  case WM_CREATE:
	  IdTimer = SetTimer(Handle, 1, DELAY_PING, NULL);
    return TRUE;

  case WM_TIMER:
		if (bConnected)	WebSocket_Ping();
    return TRUE;

  case UM_MESSAGE:
    switch (LOWORD(lParam)) {
    case FD_CONNECT:
      LastError = WSAGETSELECTERROR(lParam);
      if (!LastError) {
        SocketDistant = (SOCKET) wParam;
				if (!OnConnect()) {
					WebSocket_Disconnect();
				}
      }
      else {
        DisplayError(_T("connect"), LastError);
      }
			bEventConnect = true;
      break;
    case FD_CLOSE:
      SocketDistant = (SOCKET) wParam;
			closesocket(SocketDistant);
			SocketDistant = INVALID_SOCKET;
			OnDisconnect();
      break;
    case FD_READ:
      LastError = WSAGETSELECTERROR(lParam);
      if (!LastError) {
        SocketDistant = (SOCKET) wParam;
				OnRead();
      }
      else {
        DisplayError(_T("read"), LastError);
      }
      break;
    }
    return TRUE;


  case WM_DESTROY:
	  KillTimer(NULL, IdTimer);
    return 0;

  }

  return DefWindowProc(Handle, Message, wParam, lParam);
}

// ----------------------------------------------------------------------------
