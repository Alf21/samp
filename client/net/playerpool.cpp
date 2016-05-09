#include "../main.h"

extern CGame		 *pGame;
extern CChatWindow   *pChatWindow;
extern CCmdWindow	 *pCmdWindow;

char szQuitReasons[][32] = {
"Timeout",
"Leaving",
"Kicked"
};

int iExceptPlayerMessageDisplayed=0;

//----------------------------------------------------

CPlayerPool::CPlayerPool()
{
	m_pLocalPlayer = new CLocalPlayer();

	m_iLocalPlayerScore = 0;
	m_dwLocalPlayerPing = 0;

	// loop through and initialize all net players to null and slot states to false
	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++) {
		m_bPlayerSlotState[playerId] = FALSE;
		m_pPlayers[playerId] = NULL;
		m_iPlayerScores[playerId] = 0;
		m_dwPlayerPings[playerId] = 0;
	}
}

//----------------------------------------------------

CPlayerPool::~CPlayerPool()
{
	delete m_pLocalPlayer;

	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++) {
		Delete(playerId,0);
	}
}

//----------------------------------------------------

BOOL CPlayerPool::New(PLAYERID playerId, PCHAR szPlayerName)
{
	m_pPlayers[playerId] = new CRemotePlayer();

	if(m_pPlayers[playerId])
	{
		strcpy(m_szPlayerNames[playerId],szPlayerName);
		m_pPlayers[playerId]->SetID(playerId);
		m_bPlayerSlotState[playerId] = TRUE;
		//if(pChatWindow) pChatWindow->AddInfoMessage("*** %s joined the server.",szPlayerName);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//----------------------------------------------------

BOOL CPlayerPool::Delete(PLAYERID playerId, BYTE byteReason)
{
	if(!GetSlotState(playerId) || !m_pPlayers[playerId]) {
		return FALSE; // Player already deleted or not used.
	}

	if (GetLocalPlayer()->IsSpectating() && GetLocalPlayer()->m_SpectateID == playerId) {
		GetLocalPlayer()->ToggleSpectating(FALSE);
	}

	m_bPlayerSlotState[playerId] = FALSE;
	delete m_pPlayers[playerId];
	m_pPlayers[playerId] = NULL;

	//if(pChatWindow) {
		//pChatWindow->AddInfoMessage("*** %s left the server. (%s)",
		//m_szPlayerNames[playerId],szQuitReasons[byteReason]);
	//}

	return TRUE;
}

//----------------------------------------------------

BOOL CPlayerPool::Process()
{
	// Process all CRemotePlayers
	
	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++) {
		if(TRUE == m_bPlayerSlotState[playerId]) {
			
			try {
				m_pPlayers[playerId]->Process();
			} catch(...) {
				if(!iExceptPlayerMessageDisplayed) {
					pChatWindow->AddDebugMessage("Warning: Error Processing Player(%u)",playerId);
					//Delete(playerId,0);
					iExceptPlayerMessageDisplayed++;
				}
			}
		}
	}

	// Process the LocalPlayer
	try {
		m_pLocalPlayer->Process();
	} catch(...) {
		if(!iExceptPlayerMessageDisplayed) {
			pChatWindow->AddDebugMessage("Warning: Error Processing Player");
			iExceptPlayerMessageDisplayed++;
		}
	}
	
	return TRUE;
}

//----------------------------------------------------

PLAYERID CPlayerPool::FindRemotePlayerIDFromGtaPtr(PED_TYPE * pActor)
{
	CPlayerPed *pPlayerPed;

	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
	{
		if(TRUE == m_bPlayerSlotState[playerId])
		{
			pPlayerPed = m_pPlayers[playerId]->GetPlayerPed();

			if(pPlayerPed) {
				PED_TYPE *pTestActor = pPlayerPed->GetGtaActor();
				if((pTestActor != NULL) && (pActor == pTestActor)) // found it
					return m_pPlayers[playerId]->GetID();
			}
		}
	}

	return INVALID_PLAYER_ID;	
}

//----------------------------------------------------

int CPlayerPool::GetCount()
{
	int Count=0;
	for(int playerId = 0; playerId < MAX_PLAYERS; playerId++) {
		if(TRUE == m_bPlayerSlotState[playerId]) {
			Count++;
		}
	}
	return Count;
}

//----------------------------------------------------

void CPlayerPool::DeactivateAll()
{
	m_pLocalPlayer->m_bIsActive = FALSE;
	m_pLocalPlayer->m_iSelectedClass = 0;

	for(PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++) {
		if(TRUE == m_bPlayerSlotState[playerId]) {
			m_pPlayers[playerId]->Deactivate();
		}
	}
}

//----------------------------------------------------
// EOF