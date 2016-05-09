#pragma once

#define INVALID_PLAYER_ID 0xFFFF
#define NO_TEAM 255

//----------------------------------------------------
#pragma pack(1)
class CPlayerPool
{
private:
	
	BOOL			m_bPlayerSlotState[MAX_PLAYERS];
	CLocalPlayer	*m_pLocalPlayer;
	PLAYERID		m_LocalPlayerID;
	CRemotePlayer	*m_pPlayers[MAX_PLAYERS];
	CHAR			m_szLocalPlayerName[MAX_PLAYER_NAME+1];
	CHAR			m_szPlayerNames[MAX_PLAYERS][MAX_PLAYER_NAME+1];

	int	  m_iLocalPlayerScore;
	int	  m_iPlayerScores[MAX_PLAYERS];
	DWORD m_dwLocalPlayerPing;
	DWORD m_dwPlayerPings[MAX_PLAYERS];
	ULONG m_ulIPAddresses[MAX_PLAYERS];
	
public:
	// Process All CPlayers
	BOOL Process();

	void SetLocalPlayerName(PCHAR szName) { strcpy(m_szLocalPlayerName,szName); };
	PCHAR GetLocalPlayerName() { return m_szLocalPlayerName; };
	PCHAR GetPlayerName(PLAYERID playerId) { return m_szPlayerNames[playerId]; };
	void SetPlayerName(PLAYERID playerId, PCHAR szName) {
		strcpy(m_szPlayerNames[playerId], szName);
	}

	CLocalPlayer * GetLocalPlayer() { return m_pLocalPlayer; };
	PLAYERID FindRemotePlayerIDFromGtaPtr(PED_TYPE * pActor);

	BOOL New(PLAYERID playerId, PCHAR szPlayerName);
	BOOL Delete(PLAYERID playerId, BYTE byteReason);

	CRemotePlayer* GetAt(PLAYERID playerId) {
		if(playerId > MAX_PLAYERS) { return NULL; }
		return m_pPlayers[playerId];
	};

	// Find out if the slot is inuse.
	BOOL GetSlotState(PLAYERID playerId) {
		if(playerId > MAX_PLAYERS) { return FALSE; }
		return m_bPlayerSlotState[playerId];
	};
	
	void SetLocalPlayerID(PLAYERID MyPlayerID) {
		strcpy(m_szPlayerNames[MyPlayerID],m_szLocalPlayerName);
		m_LocalPlayerID = MyPlayerID;
	};

	PLAYERID GetLocalPlayerID() { return m_LocalPlayerID; };

	int GetCount();

	void UpdateScore(PLAYERID playerId, int iScore)
	{ 
		if (playerId == m_LocalPlayerID)
		{
			m_iLocalPlayerScore = iScore;
		} else {
			if (playerId >= MAX_PLAYERS) { return; }
			m_iPlayerScores[playerId] = iScore;
		}
	};

	void UpdatePing(PLAYERID playerId, DWORD dwPing) { 
		if (playerId == m_LocalPlayerID)
		{
			m_dwLocalPlayerPing = dwPing;
		} else {
			if (playerId >= MAX_PLAYERS) { return; }
			m_dwPlayerPings[playerId] = dwPing;
		}
	};

	void UpdateIPAddress(PLAYERID playerId, ULONG ulIPAddress) {
		if (playerId >= MAX_PLAYERS) { return; }
		m_ulIPAddresses[playerId] = ulIPAddress;
	}

	int GetLocalPlayerScore() {
		return m_iLocalPlayerScore;
	};

	DWORD GetLocalPlayerPing() {
		return m_dwLocalPlayerPing;
	};

	int GetPlayerScore(PLAYERID playerId) {
		if (playerId >= MAX_PLAYERS) { return 0; }
		return m_iPlayerScores[playerId];
	};

	DWORD GetPlayerPing(PLAYERID playerId)
	{
		if (playerId >= MAX_PLAYERS) { return 0; }
		return m_dwPlayerPings[playerId];
	};

	ULONG GetPlayerIP(PLAYERID playerId) {
		if (playerId >= MAX_PLAYERS) { return 0; }
		return m_ulIPAddresses[playerId];
	};

	void DeactivateAll();

	CPlayerPool();
	~CPlayerPool();
};

//----------------------------------------------------