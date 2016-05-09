#include "../main.h"

extern CGame		 *pGame;
extern CChatWindow   *pChatWindow;
extern CCmdWindow	 *pCmdWindow;

using namespace RakNet;
extern CNetGame* pNetGame;



#define REJECT_REASON_BAD_VERSION   1
#define REJECT_REASON_BAD_NICKNAME  2
#define REJECT_REASON_BAD_MOD		3
#define REJECT_REASON_BAD_PLAYERID	4

void ProcessIncommingEvent(PLAYERID playerId, int iEventType, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);

//----------------------------------------------------
// Sent when a client joins the server we're
// currently connected to.

void ServerJoin(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	CHAR szPlayerName[MAX_PLAYER_NAME+1];
	PLAYERID playerId;
	BYTE byteNameLen=0;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	
	bsData.Read(playerId);
	bsData.Read(byteNameLen);
	bsData.Read(szPlayerName,byteNameLen);
	szPlayerName[byteNameLen] = '\0';

	// Add this client to the player pool.
	pPlayerPool->New(playerId, szPlayerName);

	/*
	switch (bytePlayerState) {
		case PLAYER_STATE_SPECTATING:
			pPlayerPool->GetAt(playerId)->SetState(PLAYER_STATE_SPECTATING);
	}*/
}

//----------------------------------------------------
// Sent when a client joins the server we're
// currently connected to.

void ServerQuit(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	PLAYERID playerId;
	BYTE byteReason;

	bsData.Read(playerId);
	bsData.Read(byteReason);

	// Delete this client from the player pool.
	pPlayerPool->Delete(playerId,byteReason);
}

//----------------------------------------------------
// Server is giving us basic init information.

extern int iNetModeIdleOnfootSendRate;
extern int iNetModeNormalOnfootSendRate;
extern int iNetModeIdleIncarSendRate;
extern int iNetModeNormalIncarSendRate;
extern int iNetModeFiringSendRate;
extern int iNetModeSendMultiplier;

void InitGame(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsInitGame(rpcParams->input,(iBitLength/8)+1,false);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *pLocalPlayer = NULL; 

	if (pPlayerPool) pLocalPlayer = pPlayerPool->GetLocalPlayer();

	PLAYERID MyPlayerID;
	bool bLanMode, bStuntBonus;
	BYTE byteVehicleModels[212];

	bsInitGame.Read(pNetGame->m_iSpawnsAvailable);
	bsInitGame.Read(MyPlayerID);
	bsInitGame.Read(pNetGame->m_bShowPlayerTags);
	bsInitGame.Read(pNetGame->m_bShowPlayerMarkers);
	bsInitGame.Read(pNetGame->m_bTirePopping);
	bsInitGame.Read(pNetGame->m_byteWorldTime);
	bsInitGame.Read(pNetGame->m_byteWeather);
	bsInitGame.Read(pNetGame->m_fGravity);
	bsInitGame.Read(bLanMode);
	bsInitGame.Read((int)pNetGame->m_iDeathDropMoney);
	bsInitGame.Read(pNetGame->m_bInstagib);
	bsInitGame.Read(pNetGame->m_bZoneNames);
	bsInitGame.Read(pNetGame->m_bUseCJWalk);	
	bsInitGame.Read(pNetGame->m_bAllowWeapons);
	bsInitGame.Read(pNetGame->m_bLimitGlobalChatRadius);
	bsInitGame.Read(pNetGame->m_fGlobalChatRadius);
	bsInitGame.Read(bStuntBonus);
	bsInitGame.Read(pNetGame->m_fNameTagDrawDistance);
	bsInitGame.Read(pNetGame->m_bDisableEnterExits);
	bsInitGame.Read(pNetGame->m_bNameTagLOS);
	pNetGame->m_bNameTagLOS = true;

	// Server's send rate restrictions
	bsInitGame.Read(iNetModeIdleOnfootSendRate);
	bsInitGame.Read(iNetModeNormalOnfootSendRate);
	bsInitGame.Read(iNetModeIdleIncarSendRate);
	bsInitGame.Read(iNetModeNormalIncarSendRate);
	bsInitGame.Read(iNetModeFiringSendRate);
	bsInitGame.Read(iNetModeSendMultiplier);

	BYTE byteStrLen;
	bsInitGame.Read(byteStrLen);
	if(byteStrLen) {
		memset(pNetGame->m_szHostName,0,sizeof(pNetGame->m_szHostName));
		bsInitGame.Read(pNetGame->m_szHostName, byteStrLen);
	}
	pNetGame->m_szHostName[byteStrLen] = '\0';

	bsInitGame.Read((char *)&byteVehicleModels[0],212);
	pGame->SetRequiredVehicleModels(byteVehicleModels);

	if (pPlayerPool) pPlayerPool->SetLocalPlayerID(MyPlayerID);
	pGame->EnableStuntBonus(bStuntBonus);
	if (bLanMode) pNetGame->SetLanMode(TRUE);

	pNetGame->InitGameLogic();

	// Set the gravity now
	pGame->SetGravity(pNetGame->m_fGravity);

	// Disable the enter/exits if needed.
	if(pNetGame->m_bDisableEnterExits) {
		pGame->ToggleEnterExits( true );
	}
	
	pNetGame->SetGameState(GAMESTATE_CONNECTED);
	if (pLocalPlayer) pLocalPlayer->HandleClassSelection();

	pChatWindow->AddDebugMessage("Connected to %.64s",pNetGame->m_szHostName);

}

//----------------------------------------------------
// Remote player has sent a chat message.

void Chat(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	PLAYERID playerId;
	BYTE byteTextLen;

	if(pNetGame->GetGameState() != GAMESTATE_CONNECTED)	return;

	unsigned char szText[256];
	memset(szText,0,256);

	bsData.Read(playerId);
	bsData.Read(byteTextLen);
	bsData.Read((char*)szText,byteTextLen);

	szText[byteTextLen] = '\0';

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	if (playerId == pPlayerPool->GetLocalPlayerID())
	{
		CLocalPlayer *pLocalPlayer = pPlayerPool->GetLocalPlayer();
		if (pLocalPlayer) {
			pChatWindow->AddChatMessage(pPlayerPool->GetLocalPlayerName(),
			pLocalPlayer->GetPlayerColorAsARGB(), (char*)szText);
		}
	} else {
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) {
			pRemotePlayer->Say(szText);	
		}
	}
}

//----------------------------------------------------
// Remote player has sent a private chat message.

void Privmsg(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	if(pNetGame->GetGameState() != GAMESTATE_CONNECTED)	return;

	PLAYERID playerId;
	PLAYERID toPlayerId;
	BYTE byteTextLen;
	CHAR szText[256];
	CHAR szStr[256];

	bsData.Read(playerId);
	bsData.Read(toPlayerId);
	bsData.Read(byteTextLen);
	bsData.Read(szText,byteTextLen);

	szText[byteTextLen] = '\0';

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	if (playerId == pPlayerPool->GetLocalPlayerID())
	{
		sprintf(szStr, "PM sent to %s: %s", pPlayerPool->GetPlayerName(toPlayerId), szText);
		pChatWindow->AddClientMessage(D3DCOLOR_ARGB(255,220,24,26), szStr);
	} else {
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) {
			pRemotePlayer->Privmsg(szText);	
		}
	}
}

//----------------------------------------------------
// Remote player has sent a team chat message.

void TeamPrivmsg(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	if(pNetGame->GetGameState() != GAMESTATE_CONNECTED)	return;

    PLAYERID playerId;
	BYTE byteTextLen;
	CHAR szText[256];

	bsData.Read(playerId);
	bsData.Read(byteTextLen);
	bsData.Read(szText,byteTextLen);

	szText[byteTextLen] = '\0';

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	if (playerId == pPlayerPool->GetLocalPlayerID())
	{
		char szTempBuffer[256];
		sprintf(szTempBuffer, "Team PM sent: %s", szText);
		pChatWindow->AddClientMessage(D3DCOLOR_ARGB(255,220,24,26), szTempBuffer);
	} else {
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) {
			pRemotePlayer->TeamPrivmsg(szText);
		}
	}
}

//----------------------------------------------------
// Reply to our class request from the server.

void RequestClass(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteRequestOutcome=0;
	PLAYER_SPAWN_INFO SpawnInfo;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *pPlayer = NULL;

	if (pPlayerPool) pPlayer = pPlayerPool->GetLocalPlayer();

	bsData.Read(byteRequestOutcome);
	bsData.Read((PCHAR)&SpawnInfo,sizeof(PLAYER_SPAWN_INFO));

	if (pPlayer) {
		if(byteRequestOutcome) {
			pPlayer->SetSpawnInfo(&SpawnInfo);
			pPlayer->HandleClassSelectionOutcome(TRUE);
		}
		else {
			pPlayer->HandleClassSelectionOutcome(FALSE);
		}
	}
}

//----------------------------------------------------
// The server has allowed us to spawn!

void RequestSpawn(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	BYTE byteRequestOutcome=0;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(byteRequestOutcome);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *pPlayer = NULL;
	if (pPlayerPool) pPlayer = pPlayerPool->GetLocalPlayer();

	if (pPlayer) { 
		if (byteRequestOutcome == 2 || (byteRequestOutcome && pPlayer->m_bWaitingForSpawnRequestReply)) {
			pPlayer->Spawn();
		}
		else {
			pPlayer->m_bWaitingForSpawnRequestReply = false;
		}
	}
}

//----------------------------------------------------
// Add a physical ingame player for this remote player.

void WorldPlayerAdd(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CRemotePlayer *pRemotePlayer;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	PLAYERID playerId;
	BYTE byteFightingStyle=4;
	BYTE byteTeam=0;
	int iSkin=0;
	VECTOR vecPos;
	float fRotation=0;
	DWORD dwColor=0;
	BOOL bVisible;

	bsData.Read(playerId);
	bsData.Read(byteTeam);
	bsData.Read(iSkin);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fRotation);
	bsData.Read(dwColor);
	bsData.Read(byteFightingStyle);
	bsData.Read(bVisible);

	//pChatWindow->AddDebugMessage("WorldPlayerAdd(%u)",playerId);

	if(pPlayerPool) {
		pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) pRemotePlayer->Spawn(byteTeam,iSkin,&vecPos,fRotation,dwColor,byteFightingStyle,bVisible);
	}
}

//----------------------------------------------------
// Physical player is dead

void WorldPlayerDeath(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	PLAYERID playerId;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	bsData.Read(playerId);

	//pChatWindow->AddDebugMessage("WorldPlayerDeath(%u)",playerId);

	if(pPlayerPool) {
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) pRemotePlayer->HandleDeath();
	}
}

//----------------------------------------------------
// Physical player should be removed

void WorldPlayerRemove(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	PLAYERID playerId=0;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	bsData.Read(playerId);

	//pChatWindow->AddDebugMessage("WorldPlayerRemove(%u)",playerId);

	if(pPlayerPool) {
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) pRemotePlayer->Remove();
	}
}

//----------------------------------------------------
// crashors. but much smoother model loading.

void LoadRequestedModelsThread(PVOID n)
{
    pGame->LoadRequestedModels();
	_endthread();
}

//----------------------------------------------------

void WorldVehicleAdd(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(!pVehiclePool) return;

	NEW_VEHICLE NewVehicle;

	bsData.Read((char *)&NewVehicle,sizeof(NEW_VEHICLE));

	//pChatWindow->AddDebugMessage("WorldVehicleAdd(%u)",NewVehicle.VehicleId);

	if(NewVehicle.iVehicleType < 400 || NewVehicle.iVehicleType > 611) return; 
	
	//pGame->RequestModel(NewVehicle.iVehicleType);
	//_beginthread(LoadRequestedModelsThread,0,NULL); // <- leet crash CRenderer:ConstructRenderList
	//pGame->LoadRequestedModels();

    pVehiclePool->New(&NewVehicle);    
}

//----------------------------------------------------

void WorldVehicleRemove(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	if(!pVehiclePool) return;

	VEHICLEID VehicleID;
	VEHICLEID MyVehicleID;

	bsData.Read(VehicleID);
	if (pPlayerPed) {

		MyVehicleID = pVehiclePool->FindIDFromGtaPtr(pPlayerPed->GetGtaVehicle());

		//pChatWindow->AddDebugMessage("WorldVehicleRemove(%u)",VehicleID);

		if(MyVehicleID == VehicleID) {
			MATRIX4X4 mat;
			pPlayerPed->GetMatrix(&mat);
			pPlayerPed->RemoveFromVehicleAndPutAt(mat.pos.X,mat.pos.Y,mat.pos.Z);
		}
		pVehiclePool->Delete(VehicleID);
	}
}

//----------------------------------------------------

void DamageVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
   
	VEHICLEID VehicleID;
	DWORD	dwPanels;
	DWORD	dwDoors;
	BYTE	byteLights;

	bsData.Read(VehicleID);
	bsData.Read(dwPanels);
	bsData.Read(dwDoors);
	bsData.Read(byteLights);

	CVehicle *pVehicle = pNetGame->GetVehiclePool()->GetAt(VehicleID);
	if(pVehicle) {
		//pChatWindow->AddDebugMessage("UpdateDamageStatus(%u)",VehicleID);
		pVehicle->UpdateDamageStatus(dwPanels,dwDoors,byteLights);
	}
}

//----------------------------------------------------
// Remote client is trying to enter vehicle gracefully.

void EnterVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	PLAYERID playerId;
	VEHICLEID VehicleID=0;
	BYTE bytePassenger=0;
	BOOL bPassenger=FALSE;

	bsData.Read(playerId);
	bsData.Read(VehicleID);
	bsData.Read(bytePassenger);

	if(bytePassenger) bPassenger = TRUE;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	if (pPlayerPool) {
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) {
			pRemotePlayer->EnterVehicle(VehicleID,bPassenger);
		}
	}

	//pChatWindow->AddDebugMessage("Player(%u)::EnterVehicle(%u)",playerId,byteVehicleID);
}

//----------------------------------------------------
// Remote client is trying to enter vehicle gracefully.

void ExitVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	PLAYERID playerId;
	VEHICLEID VehicleID=0;

	bsData.Read(playerId);
	bsData.Read(VehicleID);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	if (pPlayerPool) {
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(playerId);
		if(pRemotePlayer) {
			pRemotePlayer->ExitVehicle();
		}
	}
		//pChatWindow->AddDebugMessage("Player(%u)::ExitVehicle(%u)",playerId,byteVehicleID);
}

//----------------------------------------------------

void SetCheckpoint(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	float fX, fY, fZ, fSize;

	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	bsData.Read(fSize);

	VECTOR Pos,Extent;

	Pos.X = fX;
	Pos.Y = fY;
	Pos.Z = fZ;
	Extent.X = fSize;
	Extent.Y = fSize;
	Extent.Z = fSize;

	pGame->SetCheckpointInformation(&Pos, &Extent);
	pGame->ToggleCheckpoints(TRUE);
}

//----------------------------------------------------

void DisableCheckpoint(RPCParameters *rpcParams)
{
	//
	//int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	pGame->ToggleCheckpoints(FALSE);
}

//----------------------------------------------------

void SetRaceCheckpoint(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	float fX, fY, fZ;
	BYTE byteType; //, byteSize;
	VECTOR Pos,Next;

	bsData.Read(byteType);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	Pos.X = fX;
	Pos.Y = fY;
	Pos.Z = fZ;

	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	Next.X = fX;
	Next.Y = fY;
	Next.Z = fZ;

	bsData.Read(fX);

	pGame->SetRaceCheckpointInformation(byteType, &Pos, &Next, fX);
	pGame->ToggleRaceCheckpoints(TRUE);
}

//----------------------------------------------------

void DisableRaceCheckpoint(RPCParameters *rpcParams)
{
	//
	//int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	pGame->ToggleRaceCheckpoints(FALSE);
}

//----------------------------------------------------

void UpdateScoresPingsIPs(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	PLAYERID playerId;
	int  iPlayerScore;
	DWORD dwPlayerPing;

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();

	for (PLAYERID i=0; i<(iBitLength/8)/9; i++)
	{
		bsData.Read(playerId);
		bsData.Read(iPlayerScore);
		bsData.Read(dwPlayerPing);

		pPlayerPool->UpdateScore(playerId, iPlayerScore);
		pPlayerPool->UpdatePing(playerId, dwPlayerPing);
	}
}

//----------------------------------------------------
extern RakNetStatisticsStruct RakServerStats;

void SvrStats(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read((char *)&RakServerStats,sizeof(RakNetStatisticsStruct));
}

//----------------------------------------------------

void GameModeRestart(RPCParameters *rpcParams)
{
	pNetGame->ShutdownForGameModeRestart();
}

//----------------------------------------------------

void ConnectionRejected(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteRejectReason;

	bsData.Read(byteRejectReason);

	if(byteRejectReason==REJECT_REASON_BAD_VERSION) {
		pChatWindow->AddInfoMessage("CONNECTION REJECTED. INCORRECT SA-MP VERSION!");
	}
	else if(byteRejectReason==REJECT_REASON_BAD_NICKNAME)
	{
		pChatWindow->AddInfoMessage("CONNECTION REJECTED. BAD NICKNAME!");
		pChatWindow->AddInfoMessage("Please choose another nick between 3-16 characters");
		pChatWindow->AddInfoMessage("containing only A-Z a-z 0-9 [ ] or _");
		pChatWindow->AddInfoMessage("Use /quit to exit or press ESC and select Quit Game");
	}
	else if(byteRejectReason==REJECT_REASON_BAD_MOD)
	{
		pChatWindow->AddInfoMessage("CONNECTION REJECTED");
		pChatWindow->AddInfoMessage("YOUR'RE USING AN INCORRECT MOD!");
	}
	else if(byteRejectReason==REJECT_REASON_BAD_PLAYERID)
	{
		pChatWindow->AddInfoMessage("Connection was closed by the server.");
		pChatWindow->AddInfoMessage("Unable to allocate a player slot. Try again.");
	}

	pNetGame->GetRakClient()->Disconnect(500);
}

//----------------------------------------------------

void ClientMessage(RPCParameters *rpcParams)
{
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	DWORD dwStrLen;
	DWORD dwColor;

	bsData.Read(dwColor);
	bsData.Read(dwStrLen);
	char* szMsg = (char*)malloc(dwStrLen+1);
	bsData.Read(szMsg, dwStrLen);
	szMsg[dwStrLen] = 0;

	pChatWindow->AddClientMessage(dwColor,szMsg);

	free(szMsg);
}

//----------------------------------------------------

void WorldTime(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteWorldTime;
	bsData.Read(byteWorldTime);
	pNetGame->m_byteWorldTime = byteWorldTime;	
}

//----------------------------------------------------

void Pickup(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	PICKUP Pickup;
	int iIndex;

	bsData.Read(iIndex);
	bsData.Read((PCHAR)&Pickup, sizeof (PICKUP));

	//pChatWindow->AddDebugMessage("Pickup: %d %d %f %f %f",iModel,iType,x,y,z);

	CPickupPool *pPickupPool = pNetGame->GetPickupPool();
	if (pPickupPool) pPickupPool->New(&Pickup, iIndex);
}

//----------------------------------------------------

void DestroyPickup(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	int iIndex;

	bsData.Read(iIndex);

	CPickupPool *pPickupPool = pNetGame->GetPickupPool();
	if (pPickupPool) pPickupPool->Destroy(iIndex);
}

//----------------------------------------------------

void DestroyWeaponPickup(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	BYTE byteIndex;

	bsData.Read(byteIndex);

	CPickupPool *pPickupPool = pNetGame->GetPickupPool();
	if (pPickupPool) pPickupPool->DestroyDropped(byteIndex);
}

//----------------------------------------------------

void ScmEvent(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	PLAYERID playerId;
	int iEvent;
	DWORD dwParam1,dwParam2,dwParam3;

	bsData.Read(playerId);
	bsData.Read(iEvent);
	bsData.Read(dwParam1);
	bsData.Read(dwParam2);
	bsData.Read(dwParam3);

	ProcessIncommingEvent(playerId,iEvent,dwParam1,dwParam2,dwParam3);
}

//----------------------------------------------------

void Weather(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteWeather;
	bsData.Read(byteWeather);
	pNetGame->m_byteWeather = byteWeather;	
}

//----------------------------------------------------

void SetTimeEx(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteHour;
	BYTE byteMinute;
	bsData.Read(byteHour);
	bsData.Read(byteMinute);
	//pNetGame->m_byteHoldTime = 0;
	pGame->SetWorldTime(byteHour, byteMinute);
	pNetGame->m_byteWorldTime = byteHour;
	pNetGame->m_byteWorldMinute = byteMinute;
}

//----------------------------------------------------

void ToggleClock(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	//PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteClock;
	bsData.Read(byteClock);
	pGame->EnableClock(byteClock);	
	if (byteClock)
	{
		pNetGame->m_byteHoldTime = 0;
	}
	else
	{
		pNetGame->m_byteHoldTime = 1;
		pGame->GetWorldTime((int*)&pNetGame->m_byteWorldTime, (int*)&pNetGame->m_byteWorldMinute);
	}
}

//----------------------------------------------------

void Instagib(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(pNetGame->m_bInstagib);
}

//----------------------------------------------------

// AntiCheat
void ACServerProtected(RPCParameters *rpcParams)
{

}

//----------------------------------------------------

void RegisterRPCs(RakClientInterface * pRakClient )
{
	/*REGISTER_STATIC_RPC(pRakClient,ServerJoin);
	REGISTER_STATIC_RPC(pRakClient,ServerQuit);	
	REGISTER_STATIC_RPC(pRakClient,InitGame);
	REGISTER_STATIC_RPC(pRakClient,Chat);
	REGISTER_STATIC_RPC(pRakClient,Privmsg);
	REGISTER_STATIC_RPC(pRakClient,TeamPrivmsg);
	REGISTER_STATIC_RPC(pRakClient,RequestClass);
	REGISTER_STATIC_RPC(pRakClient,RequestSpawn);
	
	REGISTER_STATIC_RPC(pRakClient,WorldPlayerAdd);
	REGISTER_STATIC_RPC(pRakClient,WorldPlayerDeath);
	REGISTER_STATIC_RPC(pRakClient,WorldPlayerRemove);
	REGISTER_STATIC_RPC(pRakClient,WorldVehicleAdd);
	REGISTER_STATIC_RPC(pRakClient,WorldVehicleRemove);

	REGISTER_STATIC_RPC(pRakClient,DamageVehicle);
	REGISTER_STATIC_RPC(pRakClient,EnterVehicle);
	REGISTER_STATIC_RPC(pRakClient,ExitVehicle);

	REGISTER_STATIC_RPC(pRakClient,SetCheckpoint); //14
	REGISTER_STATIC_RPC(pRakClient,DisableCheckpoint);
	REGISTER_STATIC_RPC(pRakClient,SetRaceCheckpoint);
	REGISTER_STATIC_RPC(pRakClient,DisableRaceCheckpoint);
	REGISTER_STATIC_RPC(pRakClient,UpdateScoresPingsIPs);
	REGISTER_STATIC_RPC(pRakClient,SvrStats);
	REGISTER_STATIC_RPC(pRakClient,GameModeRestart);
	REGISTER_STATIC_RPC(pRakClient,ConnectionRejected);
	REGISTER_STATIC_RPC(pRakClient,ClientMessage);
	REGISTER_STATIC_RPC(pRakClient,WorldTime);
	REGISTER_STATIC_RPC(pRakClient,Pickup);
	REGISTER_STATIC_RPC(pRakClient,DestroyPickup);
	REGISTER_STATIC_RPC(pRakClient,DestroyWeaponPickup);
	REGISTER_STATIC_RPC(pRakClient,ScmEvent);
	REGISTER_STATIC_RPC(pRakClient,Weather);
	REGISTER_STATIC_RPC(pRakClient,Instagib);
	REGISTER_STATIC_RPC(pRakClient,SetTimeEx);
	REGISTER_STATIC_RPC(pRakClient,ToggleClock); // 31
	REGISTER_STATIC_RPC(pRakClient,ACServerProtected);*/
}

//----------------------------------------------------

void UnRegisterRPCs(RakClientInterface * pRakClient)
{
	/*UNREGISTER_STATIC_RPC(pRakClient,ServerJoin);
	UNREGISTER_STATIC_RPC(pRakClient,ServerQuit);
	UNREGISTER_STATIC_RPC(pRakClient,InitGame);
	UNREGISTER_STATIC_RPC(pRakClient,Chat);
	UNREGISTER_STATIC_RPC(pRakClient,Privmsg);
	UNREGISTER_STATIC_RPC(pRakClient,TeamPrivmsg);
	UNREGISTER_STATIC_RPC(pRakClient,RequestClass);
	UNREGISTER_STATIC_RPC(pRakClient,RequestSpawn);
		
	UNREGISTER_STATIC_RPC(pRakClient,WorldPlayerAdd);
	UNREGISTER_STATIC_RPC(pRakClient,WorldPlayerDeath);
	UNREGISTER_STATIC_RPC(pRakClient,WorldPlayerRemove);
	UNREGISTER_STATIC_RPC(pRakClient,WorldVehicleAdd);
	UNREGISTER_STATIC_RPC(pRakClient,WorldVehicleRemove);

	UNREGISTER_STATIC_RPC(pRakClient,DamageVehicle);
	UNREGISTER_STATIC_RPC(pRakClient,EnterVehicle);
	UNREGISTER_STATIC_RPC(pRakClient,ExitVehicle);
	
	UNREGISTER_STATIC_RPC(pRakClient,SetCheckpoint);
	UNREGISTER_STATIC_RPC(pRakClient,DisableCheckpoint);
	UNREGISTER_STATIC_RPC(pRakClient,SetRaceCheckpoint);
	UNREGISTER_STATIC_RPC(pRakClient,DisableRaceCheckpoint);
	UNREGISTER_STATIC_RPC(pRakClient,UpdateScoresPingsIPs);
	UNREGISTER_STATIC_RPC(pRakClient,SvrStats);
	UNREGISTER_STATIC_RPC(pRakClient,GameModeRestart);
	UNREGISTER_STATIC_RPC(pRakClient,ConnectionRejected);
	UNREGISTER_STATIC_RPC(pRakClient,ClientMessage);
	UNREGISTER_STATIC_RPC(pRakClient,WorldTime);
	UNREGISTER_STATIC_RPC(pRakClient,Pickup);
	UNREGISTER_STATIC_RPC(pRakClient,DestroyPickup);
	UNREGISTER_STATIC_RPC(pRakClient,DestroyWeaponPickup);
	UNREGISTER_STATIC_RPC(pRakClient,ScmEvent);
	UNREGISTER_STATIC_RPC(pRakClient,Weather);
	UNREGISTER_STATIC_RPC(pRakClient,Instagib);
	UNREGISTER_STATIC_RPC(pRakClient,SetTimeEx);
	UNREGISTER_STATIC_RPC(pRakClient,ToggleClock);
	UNREGISTER_STATIC_RPC(pRakClient,ACServerProtected);*/
}

//----------------------------------------------------
