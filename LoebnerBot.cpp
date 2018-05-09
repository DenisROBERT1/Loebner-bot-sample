// LoebnerBot.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <string>
#include "LoebnerInterface.h"
#include "LoebnerBot.h"

//--------------------------GLOBAL VARIABLES---------------------------------

HINSTANCE HInstance;								// current instance
HWND hWndTop;

bool bBusy;
int Delay, Decoup;
std::string sResponse;

// TODO 6 : Change these parameter (MIN_SPLIT_XXXX and BOT_SPEED) to have a "human like" behaviour

// If MIN_SPLIT_XXXX is différent of zero : long sentences will be splitted in several parts after a point or a comma :
// For example "Bacchus has drowned more men, than Neptune." will be sent in two parts : 
// "Bacchus has drowned more men,"
// "than Neptune."
#define MIN_SPLIT_COMMA 15  // Min length in characters of a string splitted by a comma
#define MIN_SPLIT_POINT 25  // Min length in characters of a string splitted by a point

// BOT_SPEED : if different of zero : defines a delay before sending a message in 1/100 sec per character
// 15 = experimented writer
// 50 = two fingers typing writer
#define BOT_SPEED 15   // Speed of typing


//-----------------------FUNCTIONS PROTOTYPES--------------------------

INT_PTR CALLBACK	MainForm(HWND, UINT, WPARAM, LPARAM);

int Split(std::string sString);
void OnTimer();

//-----------------------PROGRAM ENTRY--------------------------

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HInstance = hInstance;

	DialogBox(HInstance, MAKEINTRESOURCE(IDD_MAINFORMBOX), GetDesktopWindow(), MainForm);

	return 0;
}


//---------------------------------------------------------------------------
// Message handler for main form box.
//---------------------------------------------------------------------------

INT_PTR CALLBACK MainForm(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	char szName[50];
	char szSecret[50];
	char szIp[50];
	int Port;
	static UINT IdTimer;


	switch (message) {
	case WM_INITDIALOG:
	  hWndTop = hDlg;
		Loebner_Initialisation(hDlg, OnNewRound, OnStartRound, OnEndRound, OnMessage, OnDisconnect);
		SetClassLong(hDlg, GCL_HICON, (LONG) LoadIcon(HInstance, MAKEINTRESOURCE(IDI_MAINICON)));
		SetWindowText(hDlg, "Loebner interface for bot");
		SetDlgItemText(hDlg, IDC_NAME, "ai0");
		SetDlgItemText(hDlg, IDC_SECRET, "abc123");
		SetDlgItemText(hDlg, IDC_IP, "127.0.0.1");
		SetDlgItemInt(hDlg, IDC_PORT, 8080, FALSE);
	  IdTimer = SetTimer(hDlg, 1, 10, NULL);
		return (INT_PTR) TRUE;

  case WM_TIMER:
		OnTimer();
    return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDC_START:
				SetDlgItemText(hDlg, IDC_STATUS, "Connecting...");
				ProcessMessages();
				GetDlgItemText(hDlg, IDC_NAME, szName, 50);
				GetDlgItemText(hDlg, IDC_SECRET, szSecret, 50);
				GetDlgItemText(hDlg, IDC_IP, szIp, 50);
				Port = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
				if (Loebner_Start(szName, szSecret, szIp, Port)) {
					SetDlgItemText(hDlg, IDC_STATUS, "Connected");
					EnableWindow(GetDlgItem(hDlg, IDC_NAME), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_SECRET), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_IP), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_PORT), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_START), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_STOP), TRUE);
				}
				else {
					SetDlgItemText(hDlg, IDC_STATUS, "Not connected");
				}
				break;
			case IDC_STOP:
				SetDlgItemText(hDlg, IDC_STATUS, "Enter parameters and click start");
				if (Loebner_Stop()) {
					EnableWindow(GetDlgItem(hDlg, IDC_NAME), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_SECRET), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_IP), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_PORT), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_START), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_STOP), FALSE);
				}
				break;
			case IDCANCEL:
				EndDialog(hDlg, LOWORD(wParam));
				break;
		}
		break;

	case WM_DESTROY:
	  KillTimer(NULL, IdTimer);
		Loebner_Stop();
		Loebner_End();
		return (INT_PTR) TRUE;

	}
	return (INT_PTR) FALSE;
}

//---------------------------------------------------------------------------
void SendMessage(const char *szMessage) {

	SetDlgItemText(hWndTop, IDC_MEMOBOT, szMessage);
	if (BOT_SPEED > 0) {
		sResponse += std::string(szMessage);
		if (Decoup == 0) {
			Decoup = Split(szMessage);
			Delay = Decoup * BOT_SPEED;
		}
	}
	else {
		Loebner_SendMessage(szMessage);
	}
}

//---------------------------------------------------------------------------
void CALLBACK OnNewRound(void) {
	SetDlgItemText(hWndTop, IDC_STATUS, "New round about to begin");
	SetDlgItemText(hWndTop, IDC_MEMOBOT, "");
	SetDlgItemText(hWndTop, IDC_MEMOJUDGE, "");
	bBusy = false;
	Decoup = 0;
	Delay = -1;
	// TODO 3 : Start automatically your bot
	MessageBox(hWndTop, "New round about to begin\nPlease start the bot.", "New round", MB_OK | MB_ICONINFORMATION);
}

//---------------------------------------------------------------------------
void CALLBACK OnStartRound(void) {
	SetDlgItemText(hWndTop, IDC_STATUS, "Round has started");
}

//---------------------------------------------------------------------------
void CALLBACK OnEndRound(void) {
	SetDlgItemText(hWndTop, IDC_STATUS, "Round has ended");
	// TODO 4 : Stop automatically your bot
	MessageBox(hWndTop, "Round has ended\nPlease stop the bot.", "End round", MB_OK | MB_ICONINFORMATION);
}

//---------------------------------------------------------------------------
void CALLBACK OnMessage(const char *szMessage) {
	SetDlgItemText(hWndTop, IDC_MEMOJUDGE, szMessage);
	// TODO 5 : Implement a strong AGI and insert it here
	char *szResponse;

	szResponse = new char[strlen(szMessage) + 13];
	wsprintf(szResponse, "You say : \"%s\"", szMessage);
	SendMessage(szResponse);
	delete szResponse;
}

//---------------------------------------------------------------------------
void CALLBACK OnDisconnect(const char *szReason) {
	SetDlgItemText(hWndTop, IDC_STATUS, "Not connected");
	EnableWindow(GetDlgItem(hWndTop, IDC_NAME), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_SECRET), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_IP), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_PORT), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_START), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_STOP), FALSE);
}

//---------------------------------------------------------------------------
int Split(std::string sString) {
	int Len;
	int i;
	char c, pc;


	Len = sResponse.length();
	for (i = 0; i < Len; i++) {
		c = sString[i];
		if (i + 2 <= Len) pc = sString[i + 1];
		else pc = '\0';
		
		// New line or line feed : always a split
		if (c == '\r' || c == '\n') return i;
		
		// Point : test that the next character is not a point (for "!!!", "...", etc)
		if (pc != '.' && pc != '!' && pc != '?') {
			// Test to avoid to have a remainding of 2 or 3 characters
			if (i < Len - 3) {
				if ((c == '.' || c == '!' || c == '?') && MIN_SPLIT_POINT != 0 && i > MIN_SPLIT_POINT) return i + 1;
				if ((c == ',' || c == ';' || c == ':') && MIN_SPLIT_COMMA != 0 && i > MIN_SPLIT_COMMA) return i + 1;
			}
		}
	}

	return i;
}

//---------------------------------------------------------------------------
void OnTimer() {

	if (bBusy) return;
	bBusy = true;

	if (Delay != -1) {
		if (Delay-- == 0) {
			std::string csLigne;
			char c;
			int Len;


			Len = sResponse.length();
			csLigne = sResponse.substr(0, Decoup);
			Loebner_SendMessage(csLigne.c_str());

			if (Decoup < Len) {
				c = sResponse[Decoup];
				while (c == '\r' || c == '\n' || c == ' ') {
					if (++Decoup >= Len) break;
					c = sResponse[Decoup];
				}
			}
			if (Decoup < Len) {
				sResponse = sResponse.substr(Decoup, Len - Decoup);
				Decoup = Split(sResponse);
				Delay = Decoup * BOT_SPEED;
			}
			else {
				sResponse = "";
				Decoup = 0;
				Delay = -1;
			}

		}
	}

	bBusy = false;

}

//---------------------------------------------------------------------------
