#pragma once

#include "resource.h"

void ProcessMessages(void);

void CALLBACK OnNewRound(void);
void CALLBACK OnStartRound(void);
void CALLBACK OnEndRound(void);
void CALLBACK OnMessage(char *szMessage);
void CALLBACK OnDisconnect(char *szReason);
