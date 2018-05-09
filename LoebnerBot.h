#pragma once

#include "resource.h"

// Process Windows messages
void ProcessMessages(void);

// Sends a message to judge
void SendMessage(const char *szMessage);

// Callbacks on events
void CALLBACK OnNewRound(void);
void CALLBACK OnStartRound(void);
void CALLBACK OnEndRound(void);
void CALLBACK OnMessage(const char *szMessage);
void CALLBACK OnDisconnect(const char *szReason);
