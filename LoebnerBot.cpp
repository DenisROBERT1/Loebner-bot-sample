// LoebnerBot.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include "LoebnerInterface.h"
#include "LoebnerBot.h"

//--------------------------GLOBAL VARIABLES---------------------------------

HINSTANCE HInstance;								// current instance
HWND hWndTop;

//-----------------------FUNCTIONS PROTOTYPES--------------------------

INT_PTR CALLBACK	MainForm(HWND, UINT, WPARAM, LPARAM);


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
		return (INT_PTR) TRUE;

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
		Loebner_Stop();
		Loebner_End();
		return (INT_PTR) TRUE;

	}
	return (INT_PTR) FALSE;
}

//---------------------------------------------------------------------------
void CALLBACK OnNewRound(void) {
	// TODO 3 : Start automatically your bot
	MessageBox(hWndTop, "New round about to begin\nPlease start the bot.", "New round", MB_OK | MB_ICONINFORMATION);
}

//---------------------------------------------------------------------------
void CALLBACK OnStartRound(void) {
}

//---------------------------------------------------------------------------
void CALLBACK OnEndRound(void) {
	// TODO 4 : Stop automatically your bot
	MessageBox(hWndTop, "Round has ended\nPlease stop the bot.", "End round", MB_OK | MB_ICONINFORMATION);
}

//---------------------------------------------------------------------------
void CALLBACK OnMessage(const char *szMessage) {
	// TODO 5 : Implement a strong AGI and insert it here
	char *szResponse;

	szResponse = new char[strlen(szMessage) + 13];
	wsprintf(szResponse, "You say : \"%s\"", szMessage);
	Loebner_SendMessage(szResponse);
	delete szResponse;
}

//---------------------------------------------------------------------------
void CALLBACK OnDisconnect(const char *szReason) {
	EnableWindow(GetDlgItem(hWndTop, IDC_NAME), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_SECRET), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_IP), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_PORT), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_START), TRUE);
	EnableWindow(GetDlgItem(hWndTop, IDC_STOP), FALSE);
}

//---------------------------------------------------------------------------
