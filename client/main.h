#pragma once


#include <stdio.h>
#include <windows.h>

#include <process.h>

#pragma warning(disable:4005)
#pragma warning(disable:4996)

#define NETCODE_CONNCOOKIELULZ 0x6969
#define NETGAME_VERSION 4057
#define RAKSAMP_CLIENT

#define ARRAY_SIZE(a)	( sizeof((a)) / sizeof(*(a)) )
#define SAFE_DELETE(p)	{ if (p) { delete (p); (p) = NULL; } }
#define SAFE_RELEASE(p)	{ if (p) { (p)->Release(); (p) = NULL; } }

#define IDC_CMDEDIT		1
#define IDC_CHATBACK	2
#define IDC_CHATSCROLL  3

#define MAX_PLAYER_NAME			24
#define MAX_ACTOR_NAME			24
#define MAX_SETTINGS_STRING		256

#define GTASA_VERSION_UNKNOWN	0
#define GTASA_VERSION_USA10		1
#define GTASA_VERSION_EU10		2
#define LOCKING_DISTANCE		200.0f
#define CSCANNER_DISTANCE		200.0f
#define PSCANNER_DISTANCE		600.0f

typedef struct _GAME_SETTINGS {
	BOOL bDebug;
	BOOL bPlayOnline;
	BOOL bWindowedMode;
	CHAR szConnectPass[MAX_SETTINGS_STRING+1];
	CHAR szConnectHost[MAX_SETTINGS_STRING+1];
	CHAR szConnectPort[MAX_SETTINGS_STRING+1];
	CHAR szNickName[MAX_SETTINGS_STRING+1];
} GAME_SETTINGS;

#include "d3d9/include/d3d9.h"
#include "d3d9/include/d3dx9core.h"
#include "d3d9/common/dxstdafx.h"
#include "game/game.h"


#include "raknet/PacketEnumerations.h"
#include "raknet/RakNetworkFactory.h"
#include "raknet/RakClientInterface.h"
#include "raknet/NetworkTypes.h"
#include "raknet/BitStream.h"
#include "raknet/StringCompressor.h"

#include "samp_netencr.h"
#include "samp_auth.h"
#include "SAMPRPC.h"
#include "SAMP_VER.h"


#include "net/localplayer.h"
#include "net/remoteplayer.h"
#include "net/remoteactor.h"
#include "net/netrpc.h"
#include "net/actorpool.h"
#include "net/playerpool.h"
#include "net/vehiclepool.h"
#include "net/actorpool.h"
#include "net/pickuppool.h"
#include "net/objectpool.h"
#include "net/gangzonepool.h"
#include "net/menupool.h"
#include "net/textdrawpool.h"
#include "net/netgame.h"
#include "net/scriptrpc.h"

#include "fontrender.h"

#include "chatwindow.h"
#include "cmdwindow.h"
#include "deathwindow.h"
#include "spawnscreen.h"
#include "playertags.h"
#include "newplayertags.h"
#include "scoreboard.h"
#include "label.h"
#include "netstats.h"
#include "svrnetstats.h"
#include "helpdialog.h"
#include "archive/ArchiveFS.h"
#include "game/scripting.h"
#include "d3dhook/IDirect3DDevice9Hook.h"

void SetStringFromCommandLine(char *szCmdLine, char *szString);
void InitSettings();
void QuitGame();

void UnFuck(DWORD addr, int size);

#include "outputdebugstring.h"

//----------------------------------------------------
// EOF

