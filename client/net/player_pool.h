#pragma once

#define INVALID_PLAYER_ID 255

//----------------------------------------------------

class CPlayerPool
{
private:
	
	BOOL m_bPlayerSlotState[MAX_PLAYERS];
	CRemotePlayer *m_pPlayers[MAX_PLAYERS];
	CHAR	m_szPlayerName[MAX_PLAYERS][MAX_PLAYER_NAME+1];

public:
	
	CPlayerPool();
	~CPlayerPool();

	// Process All CNetPlayers
	BOOL Process();

	// Add a new CNetPlayerRemote.
	BOOL AddPlayer(PLAYERID player, PCHAR szPlayerName);

	// Delete a CNetPlayerRemote.
	BOOL DeletePlayer(PLAYERID player);	
		
	// Retrieve a player
	CPlayer* GetPlayer(PLAYERID player) {
		if(player > MAX_PLAYERS) { return NULL; }
		return m_pPlayers[player];
	};

	// Find out if the slot is inuse.
	BOOL GetPlayerSlotState(PLAYERID player) {
		if(player > MAX_PLAYERS) { return FALSE; }
		return m_bPlayerSlotState[player];
	};
};

//----------------------------------------------------