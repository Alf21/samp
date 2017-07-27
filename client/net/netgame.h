#pragma once

#define GAMESTATE_NONE			0 // used for debugging modes and such
#define GAMESTATE_CONNECTING	1
#define GAMESTATE_CONNECTED		2
#define GAMESTATE_AWAIT_JOIN	3
#define GAMESTATE_DISCONNECTED  4
#define GAMESTATE_RESTARTING	5
#define GAMESTATE_WAIT_CONNECT	6

#define LOCAL_CLIENT_PORT		8150

#define PACK_VEHICLE_HEALTH(f)		(BYTE)(f / 4)
#define UNPACK_VEHICLE_HEALTH(b)	(float)b * 4

// SEND RATE TICKS
#define NETMODE_IDLE_ONFOOT_SENDRATE	80
#define NETMODE_NORMAL_ONFOOT_SENDRATE	30
#define NETMODE_IDLE_INCAR_SENDRATE		80
#define NETMODE_NORMAL_INCAR_SENDRATE	30
#define NETMODE_HEADSYNC_SENDRATE		1000
#define NETMODE_AIM_SENDRATE			100
#define NETMODE_FIRING_SENDRATE			30
#define LANMODE_IDLE_ONFOOT_SENDRATE	20
#define LANMODE_NORMAL_ONFOOT_SENDRATE	15
#define LANMODE_IDLE_INCAR_SENDRATE		30
#define LANMODE_NORMAL_INCAR_SENDRATE	15
#define NETMODE_SEND_MULTIPLIER			2
#define STATS_UPDATE_TICKS 1000 // 1 second

//----------------------------------------------------


class CNetGame
{
private:
	// Should always be the first one in class (for the sake of ACGhost)
	RakClientInterface	*m_pRakClient;

	CPlayerPool			*m_pPlayerPool;
	CPickupPool			*m_pPickupPool;
	CVehiclePool		*m_pVehiclePool;
	CActorPool			*m_pActorPool;
	CObjectPool			*m_pObjectPool;
	CMenuPool			*m_pMenuPool;
	CTextDrawPool		*m_pTextDrawPool;
	CGangZonePool		*m_pGangZonePool;
	CLabelPool			*m_pLabelPool;

	int					m_iGameState;
	BOOL				m_bLanMode;
	DWORD				m_dwLastConnectAttempt;
	int					m_iCheckLoadedStuff;
	
	void UpdateNetwork();

	// Packet handlers
	void Packet_AimSync(Packet *p);
	void Packet_PlayerSync(Packet *p);
	void Packet_VehicleSync(Packet *p);
	void Packet_PassengerSync(Packet *p);
	void Packet_ConnectionSucceeded(Packet *p);
	void Packet_UnoccupiedSync(Packet *p);
	void Packet_RSAPublicKeyMismatch(Packet* packet);
	void Packet_ConnectionBanned(Packet* packet);
	void Packet_ConnectionRequestAccepted(Packet* packet);
	void Packet_NoFreeIncomingConnections(Packet* packet);
	void Packet_DisconnectionNotification(Packet* packet);
	void Packet_ConnectionLost(Packet* packet);
	void Packet_InvalidPassword(Packet* packet);
	void Packet_ModifiedPacket(Packet* packet);
	void Packet_ConnectAttemptFailed(Packet* packet);
	void Packet_TrailerSync(Packet *p);

public:

	int			m_iSpawnsAvailable;
	bool		m_bShowPlayerMarkers;
	bool		m_bShowPlayerTags;
	bool		m_bTirePopping;
	BYTE		m_byteWorldTime;
	BYTE		m_byteWorldMinute;
	BYTE		m_byteWeather;
	float		m_WorldBounds[4];
	BYTE		m_byteFriendlyFire;
	bool		m_bAllowWeapons;
	bool		m_bNameTagLOS; // Check for nametag-LOS?
	float		m_fGravity;
	int			m_iDeathDropMoney;
	BYTE		m_byteHoldTime;
	bool		m_bInstagib;
	bool		m_bZoneNames;
	bool		m_bLimitGlobalChatRadius;
	bool		m_bUseCJWalk;
	float		m_fGlobalChatRadius;
	float		m_fNameTagDrawDistance; // The distance which players will start rendering nametags
	bool		m_bDisableEnterExits; // Interior enter/exits disabled?
	DWORD		m_dwMapIcon[32];
	
	char m_szHostName[255];
	char m_szHostOrIp[128];
	int m_iPort;

	CNetGame(PCHAR szHostOrIp,int iPort,PCHAR szPlayerName,PCHAR szPass);
	~CNetGame();

	int GetGameState() { return m_iGameState; };
	void SetGameState(int iGameState) { m_iGameState = iGameState; };
	BOOL IsLanMode() { return m_bLanMode; };
	BOOL GetWalkStyle() { return m_bUseCJWalk; };
	void SetLanMode(BOOL bMode) { m_bLanMode = bMode; };

	CPlayerPool * GetPlayerPool() { return m_pPlayerPool; };
	CVehiclePool * GetVehiclePool() { return m_pVehiclePool; };
	CActorPool * GetActorPool() { return m_pActorPool; };
	CPickupPool * GetPickupPool() { return m_pPickupPool; };
	CObjectPool	* GetObjectPool() { return m_pObjectPool; };
	CMenuPool * GetMenuPool() { return m_pMenuPool; };
	CTextDrawPool * GetTextDrawPool() { return m_pTextDrawPool; };
	CGangZonePool * GetGangZonePool() { return m_pGangZonePool; };
	CLabelPool * GetLabelPool() { return m_pLabelPool; };
	RakClientInterface * GetRakClient() { return m_pRakClient; };
	void InitGameLogic();
	void Process();

	void UpdatePlayerScoresAndPings();
	void ResetVehiclePool();
	void ResetPlayerPool();
	void ResetPickupPool();
	void ResetObjectPool();
	void ResetMenuPool();
	void ResetTextDrawPool();
	void ResetGangZonePool();
	void ResetActorPool();
	void ShutdownForGameModeRestart();

	void  ResetMapIcons();
	void  SetMapIcon(BYTE byteIndex, float fX, float fY, float fZ, BYTE byteIcon, DWORD dwColor);
	void  DisableMapIcon(BYTE byteIndex);

};

//----------------------------------------------------
