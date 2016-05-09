#include <windows.h>

void GameDebugEntity(DWORD dwEnt1, DWORD dwEnt2, int type);
BOOL IsGameDebugScreenOn();
void GameDebugScreensOff();
void GameDebugSetSyncActors(DWORD dwID1, DWORD dwID2);
void GameDebugDrawDebugScreens();
void GameDebugDrawSpawnInfo();
void GameDebugGetEntityType( BYTE bType, char * cType );
void GameDebugInitTextScreen();
void GameDebugAddMessage(char *format, ...);
void GameDrawDebugTextInfo();

//-----------------------------------------------------------
