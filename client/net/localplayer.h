#pragma once

#define SPECTATE_TYPE_NONE				0
#define SPECTATE_TYPE_PLAYER			1
#define SPECTATE_TYPE_VEHICLE			2

#define SPECIAL_ACTION_NONE				0
#define SPECIAL_ACTION_USEJETPACK		2
#define SPECIAL_ACTION_DANCE1			5
#define SPECIAL_ACTION_DANCE2			6
#define SPECIAL_ACTION_DANCE3			7
#define SPECIAL_ACTION_DANCE4			8
#define SPECIAL_ACTION_HANDSUP			10
#define SPECIAL_ACTION_USECELLPHONE		11
#define SPECIAL_ACTION_SITTING			12
#define SPECIAL_ACTION_STOPUSECELLPHONE 13
#define SPECIAL_ACTION_NIGHTVISION		14
#define SPECIAL_ACTION_THERMALVISION	15


typedef struct _PLAYER_SPAWN_INFO
{
	BYTE byteTeam;
	int iSkin;
	VECTOR vecPos;
	float fRotation;
	int iSpawnWeapons[3];
	int iSpawnWeaponsAmmo[3];
} PLAYER_SPAWN_INFO;


typedef struct _ONFOOT_SYNC_DATA
{
	WORD lrAnalog;
	WORD udAnalog;
	WORD wKeys;
	VECTOR vecPos;
	float fRotation;
	BYTE byteHealth;
	BYTE byteArmour;
	BYTE byteCurrentWeapon;
	BYTE byteSpecialAction;	
	VECTOR vecMoveSpeed;
	VECTOR vecSurfOffsets;
	WORD wSurfInfo;
} ONFOOT_SYNC_DATA;

enum eWeaponState
{
	WS_NO_BULLETS = 0,
	WS_LAST_BULLET = 1,
	WS_MORE_BULLETS = 2,
	WS_RELOADING = 3,
};


typedef struct _AIM_SYNC_DATA
{
	BYTE byteCamMode;
	BYTE byteCamExtZoom : 6;	// 0-63 normalized
	BYTE byteWeaponState : 2;	// see eWeaponState
	VECTOR vecAimf1;
	VECTOR vecAimf2;
	VECTOR vecAimPos;
	float fAimZ;
} AIM_SYNC_DATA;


typedef struct _UNOCCUPIED_SYNC_DATA
{
	VEHICLEID VehicleID;
	C_VECTOR1 cvecRoll;
	C_VECTOR1 cvecDirection;
	VECTOR vecPos;
	VECTOR vecMoveSpeed;
	VECTOR vecTurnSpeed;
	float fHealth;
} UNOCCUPIED_SYNC_DATA;


typedef struct _TAGINFO
{
	char szTag[MAX_PLAYER_NAME+1];
	int iTagLen;
	DWORD dwColor;
	float fZOffset;
	BOOL bActive;
} TAG_INFO;


typedef struct _INCAR_SYNC_DATA
{
	VEHICLEID VehicleID;
	WORD lrAnalog;
	WORD udAnalog;
	WORD wKeys;
	C_VECTOR1 cvecRoll;
	C_VECTOR1 cvecDirection;
	VECTOR vecPos;
	VECTOR vecMoveSpeed;
	float fCarHealth;
	BYTE bytePlayerHealth;
	BYTE bytePlayerArmour;
	BYTE byteCurrentWeapon;
    BYTE byteSirenOn;
	BYTE byteLandingGearState;
	BYTE byteTires[4];
	DWORD dwPanelDamage;
	DWORD dwDoorDamage;
	BYTE byteLightDamage;
	VEHICLEID TrailerID;
	DWORD dwHydraThrustAngle;
	FLOAT fTrainSpeed;
} INCAR_SYNC_DATA;


typedef struct _PASSENGER_SYNC_DATA
{
	VEHICLEID VehicleID;
	BYTE byteSeatFlags : 7;
	BYTE byteDriveBy : 1;
	BYTE byteCurrentWeapon;
	BYTE bytePlayerHealth;
	BYTE bytePlayerArmour;
	WORD lrAnalog;
	WORD udAnalog;
	WORD wKeys;
	VECTOR vecPos;
} PASSENGER_SYNC_DATA;


typedef struct _SPECTATOR_SYNC_DATA
{
	WORD lrAnalog;
	WORD udAnalog;
	WORD wKeys;
	VECTOR vecPos;
} SPECTATOR_SYNC_DATA;


typedef struct _TRAILER_SYNC_DATA
{
	C_VECTOR1 cvecRoll;
	C_VECTOR1 cvecDirection;
	VECTOR vecPos;
	VECTOR vecMoveSpeed;
} TRAILER_SYNC_DATA;


typedef struct _CAR_MOD_INFO
{
	BYTE byteCarMod[14];
	BYTE bytePaintJob;
	int iColor0;
	int iColor1;
} CAR_MOD_INFO;

//----------------------------------------------------------


class CLocalPlayer
{
private:

	PLAYER_SPAWN_INFO		m_SpawnInfo;
	BOOL					m_bHasSpawnInfo;
	ULONG					m_ulThisSyncFrame;
	ULONG					m_ulLastSyncFrame;
	CHAR					m_szPlayerName[256];
	WORD					m_wLastKeys;
	DWORD					m_dwLastSendTick;
	DWORD					m_dwLastSendSpecTick;
	DWORD					m_dwLastAimSendTick;
	DWORD					m_dwLastStatsUpdateTick;
	DWORD					m_dwLastHeadUpdate;
	DWORD					m_dwLastUndrivenUpdate;

	BOOL					m_bPassengerDriveByMode;
	BYTE					m_byteCurInterior;
	BOOL					m_bInRCMode;

	BOOL					m_bSurfingMode;
	VECTOR					m_vecLockedSurfingOffsets;
	VEHICLEID				m_SurfingID;
	
	// SPAWNING STUFF
	BOOL					m_bClearedToSpawn;
	DWORD					m_dwLastSpawnSelectionTick;// delays left and right selection
	DWORD					m_dwInitialSelectionTick;// delays initial selection

	BOOL					m_bIsSpectating;

	BYTE					m_byteTeam;

public:
	BYTE					m_byteSpectateMode;
	BYTE					m_byteSpectateType;
	DWORD					m_SpectateID; // Vehicle or player id
	BOOL					m_bSpectateProcessed;

	CPlayerPed				*m_pPlayerPed;
	BOOL					m_bIsActive;
	BOOL					m_bIsWasted;
	BOOL					m_bWantsAnotherClass;
	int						m_iSelectedClass;
	BOOL					m_bWaitingForSpawnRequestReply;
	BYTE					m_byteVirtualWorld;

	VEHICLEID				m_CurrentVehicle;
	VEHICLEID				m_LastVehicle;
	int						m_iDisplayZoneTick;
	
	BYTE					m_byteLastWeapon[13];
	DWORD					m_dwLastAmmo[13];

	VEHICLEID				m_DamageVehicleUpdating;
	DWORD					m_dwLastPanelDamageStatus;
	DWORD					m_dwLastDoorDamageStatus;
	BYTE					m_byteLastLightsDamageStatus;

	CLocalPlayer();
	~CLocalPlayer(){};

	BOOL IsActive() { return m_bIsActive; };
	BOOL IsWasted() { return m_bIsWasted; };

	void HandlePassengerEntry();
	BOOL Process();
	BOOL DestroyPlayer();
	
	BYTE GetSpecialAction();
	void ApplySpecialAction(BYTE byteSpecialAction);
	void UpdateSurfing();

	void SendOnFootFullSyncData();
	void SendInCarFullSyncData();
	void SendPassengerFullSyncData();
	void SendAimSyncData();
	void ResetAllSyncAttributes();

	int  GetOptimumInCarSendRate(int iPlayersEffected);
	int  GetOptimumOnFootSendRate(int iPlayersEffected);
	int  GetOptimumUndrivenSendRate(int iPlayersEffected);

	void ProcessUndrivenSync(VEHICLEID VehicleID, CVehicle *pVehicle, int iNumberOfPlayersInLocalRange);
	
	void SendWastedNotification();
	
	void RequestClass(int iClass);
	void RequestSpawn();

	void SetSpawnInfo(PLAYER_SPAWN_INFO *pSpawn);

	BOOL Spawn();

	CPlayerPed * GetPlayerPed() { return m_pPlayerPed; };

	void Say(PCHAR szText);
	void Msg(PLAYERID ToPlayer, PCHAR szText);
	void TeamMsg(PCHAR szText);

	void SendExitVehicleNotification(VEHICLEID VehicleID);
	void SendEnterVehicleNotification(VEHICLEID VehicleID,BOOL bPassenger);
	
	void SetPlayerColor(DWORD dwColor);
	DWORD GetPlayerColorAsRGBA();
	DWORD GetPlayerColorAsARGB();
	void ProcessOnFootWorldBounds();
	void ProcessInCarWorldBounds();

	void SendStatsUpdate();
	void UpdateRemoteInterior(BYTE byteInterior);

	void HandleClassSelectionOutcome(BOOL bOutcome);
	void HandleClassSelection();
	void ProcessClassSelection();
	BOOL IsClearedToSpawn() { return m_bClearedToSpawn; };
	
	void CheckWeapons();
	void SetVirtualWorld(BYTE byteWorld) { m_byteVirtualWorld = byteWorld; };
	BYTE GetVirtualWorld() { return m_byteVirtualWorld; };

	void ToggleSpectating(BOOL bToggle);
	void SpectateVehicle(VEHICLEID VehicleID);
	void SpectatePlayer(PLAYERID playerId);
	void ProcessSpectating();
	BOOL IsSpectating() { return m_bIsSpectating; };
	void ReturnToClassSelection() { m_bWantsAnotherClass = TRUE; };

	BYTE GetTeam() { return m_byteTeam; };
	void SetTeam(BYTE byteTeam) { m_byteTeam = byteTeam; };

	BOOL IsInRCMode() { return m_bInRCMode; };

	int DetermineNumberOfPlayersInLocalRange();

	void ProcessVehicleDamageUpdates(VEHICLEID CurrentVehicle);
};

//----------------------------------------------------------
