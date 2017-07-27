#include "../main.h"
#include "../game/util.h"

#include "../buildinfo.h"

extern CGame		 *pGame;
extern CChatWindow   *pChatWindow;
extern CCmdWindow	 *pCmdWindow;

INCAR_SYNC_DATA DebugSync;
BOOL bDebugUpdate=FALSE;

#define NETGAME_VERSION 8866 // 0.2.5
//#define NETGAME_VERSION 7766 // 0.2.2

int iExceptMessageDisplayed=0;

int iVehiclesBench=0;
int iPlayersBench=0;
int iPicksupsBench=0;
int iMenuBench=0;
int iObjectBench=0;
int iTextDrawBench=0;

int iVehiclePoolProcessFlag=0;
int iPickupPoolProcessFlag=0;
extern int iSyncedRandomNumber;



//----------------------------------------------------

BYTE GetPacketID(Packet *p)
{
	if (p==0) return 255;

	if (p->data[0] == ID_TIMESTAMP) {
		assert(p->length > sizeof(unsigned char) + sizeof(unsigned long));
		return (unsigned char) p->data[sizeof(unsigned char) + sizeof(unsigned long)];
	}
	else {
		return (unsigned char) p->data[0];
	}
}

//----------------------------------------------------

CNetGame::CNetGame(PCHAR szHostOrIp, int iPort, 
				   PCHAR szPlayerName, PCHAR szPass)
{
	strcpy(m_szHostName, "San Andreas Multiplayer");
	strncpy(m_szHostOrIp, szHostOrIp, sizeof(m_szHostOrIp));
	m_iPort = iPort;

	// Setup player pool
	m_pPlayerPool = new CPlayerPool();
	m_pPlayerPool->SetLocalPlayerName(szPlayerName);

	m_pVehiclePool = new CVehiclePool();
	m_pPickupPool  = new CPickupPool();
	m_pObjectPool	= new CObjectPool();
	m_pMenuPool = new CMenuPool();
	m_pTextDrawPool = new CTextDrawPool();
	m_pGangZonePool = new CGangZonePool();
	m_pActorPool = new CActorPool();
	m_pLabelPool = new CLabelPool();

	m_pRakClient = RakNetworkFactory::GetRakClientInterface();
//	m_pRakClient->InitializeSecurity(0, 0);
//	m_pRakClient->SetTrackFrequencyTable(true);
//	m_pRakClient->SetTimeoutTime(30000);

	// Use this to send all packets to the chat window
	//pPacketLogger = new PacketLogger();
	//m_pRakClient->AttachPlugin(pPacketLogger);

	RegisterRPCs(m_pRakClient);
	RegisterScriptRPCs(m_pRakClient);	// Register server-side scripting RPCs.

	//m_pRakClient->SetMTUSize(1492);

	m_pRakClient->SetPassword(szPass);

	m_dwLastConnectAttempt = GetTickCount();
	m_iGameState = GAMESTATE_WAIT_CONNECT;
	
	m_iSpawnsAvailable = 0;
	m_byteWorldTime = 12;
	m_byteWorldMinute = 0;
	m_byteWeather	= 10;
	m_fGravity = (float)0.008000000;
	m_iDeathDropMoney = 0;
	m_bLanMode = FALSE;
	m_bNameTagLOS = true;
	m_byteHoldTime = 1;
	m_bUseCJWalk = FALSE;
	m_bDisableEnterExits = false;
	m_fNameTagDrawDistance = 70.0f;

	int i;
	for (i = 0; i < 32; i++) m_dwMapIcon[i] = NULL;

	m_byteFriendlyFire = 1;
	pGame->EnableClock(0); // Hide the clock by default
	pGame->EnableZoneNames(0);
	m_bZoneNames = FALSE;
	m_bInstagib = FALSE;
	m_iCheckLoadedStuff = 0;

	if(pChatWindow) pChatWindow->AddDebugMessage("SA:MP " SAMP_VERSION " Initialized");
}

//----------------------------------------------------

CNetGame::~CNetGame()
{
	m_pRakClient->Disconnect(0);
	UnRegisterRPCs(m_pRakClient);
	UnRegisterScriptRPCs(m_pRakClient);	// Unregister server-side scripting RPCs.
	RakNetworkFactory::DestroyRakClientInterface(m_pRakClient);
	SAFE_DELETE(m_pPlayerPool);
	SAFE_DELETE(m_pVehiclePool);
	SAFE_DELETE(m_pPickupPool);
	SAFE_DELETE(m_pObjectPool);
	SAFE_DELETE(m_pMenuPool);
	SAFE_DELETE(m_pTextDrawPool);
	SAFE_DELETE(m_pGangZonePool);
}

//----------------------------------------------------

void CNetGame::ShutdownForGameModeRestart()
{
	m_byteWorldTime = 12;
	m_byteWorldMinute = 0;
	m_byteWeather	= 10;
	m_byteHoldTime = 1;
	m_bNameTagLOS = true;
	m_bUseCJWalk = FALSE;
	m_fGravity = (float)0.008000000;
	m_iDeathDropMoney = 0;
	pGame->ToggleEnterExits(true);
	pGame->SetGravity(m_fGravity);
	pGame->SetWantedLevel(0);
	pGame->EnableClock(0);
	m_bDisableEnterExits = false;
	m_fNameTagDrawDistance = 70.0f;

	for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++) {
		CRemotePlayer* pPlayer = m_pPlayerPool->GetAt(playerId);
		if (pPlayer) {
			pPlayer->SetTeam(NO_TEAM);
			pPlayer->ResetAllSyncAttributes();
		}
	}

	CLocalPlayer *pLocalPlayer = m_pPlayerPool->GetLocalPlayer();
	if (pLocalPlayer) {
		pLocalPlayer->ResetAllSyncAttributes();
		pLocalPlayer->ToggleSpectating(FALSE);
	}

	m_iGameState = GAMESTATE_RESTARTING;

	pChatWindow->AddInfoMessage("Game mode restarting..");

	// Disable the ingame players and reset the vehicle pool.
	m_pPlayerPool->DeactivateAll();
	
	// Process the pool one last time
	m_pPlayerPool->Process();

	ResetVehiclePool();
	ResetPickupPool();
	ResetObjectPool();
	ResetMenuPool();
	ResetTextDrawPool();
	ResetGangZonePool();
	ResetActorPool();
	ResetMapIcons();

	CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
	if (pPlayerPed) {
		pPlayerPed->SetInterior(0);
		pPlayerPed->SetDead();
		pPlayerPed->SetArmour(0.0f);
	}

	pGame->ToggleCheckpoints(FALSE);
	pGame->ToggleRaceCheckpoints(FALSE);
	pGame->ResetLocalMoney();
	pGame->EnableZoneNames(0);
	m_bZoneNames = FALSE;
	
	GameResetRadarColors();
}

//----------------------------------------------------

void CNetGame::InitGameLogic()
{
	GameResetRadarColors();

	m_WorldBounds[0] = 20000.0f;
	m_WorldBounds[1] = -20000.0f;
	m_WorldBounds[2] = 20000.0f;
	m_WorldBounds[3] = -20000.0f;
}

//----------------------------------------------------

void CNetGame::Process()
{	
	UpdateNetwork();

	if (m_byteHoldTime)	{
		pGame->SetWorldTime(m_byteWorldTime, m_byteWorldMinute);
	}

	// Keep the weather fixed at m_byteWeather so it doesnt desync
	pGame->SetWorldWeather(m_byteWeather);

	// KEEP THE FOLLOWING ANIMS LOADED DURING THE NETGAME
	if(!pGame->IsAnimationLoaded("PARACHUTE")) pGame->RequestAnimation("PARACHUTE");
	//if(!pGame->IsAnimationLoaded("DANCING")) pGame->RequestAnimation("DANCING");
	//if(!pGame->IsAnimationLoaded("GFUNK")) pGame->RequestAnimation("GFUNK");
	//if(!pGame->IsAnimationLoaded("RUNNINGMAN"))	pGame->RequestAnimation("RUNNINGMAN");
    //if(!pGame->IsAnimationLoaded("WOP")) pGame->RequestAnimation("WOP");
	//if(!pGame->IsAnimationLoaded("STRIP")) pGame->RequestAnimation("STRIP");
				
	if(!pGame->IsModelLoaded(OBJECT_PARACHUTE)) {
		pGame->RequestModel(OBJECT_PARACHUTE);
	}

	// keep the throwable weapon models loaded
	if (!pGame->IsModelLoaded(WEAPON_MODEL_TEARGAS))
		pGame->RequestModel(WEAPON_MODEL_TEARGAS);
	if (!pGame->IsModelLoaded(WEAPON_MODEL_GRENADE))
		pGame->RequestModel(WEAPON_MODEL_GRENADE);
	if (!pGame->IsModelLoaded(WEAPON_MODEL_MOLTOV))
		pGame->RequestModel(WEAPON_MODEL_MOLTOV);

	// cellphone
	if (!pGame->IsModelLoaded(330)) pGame->RequestModel(330);

	if(GetGameState() == GAMESTATE_CONNECTED) {

		DWORD dwStartTick = GetTickCount();

		if(m_pPlayerPool) m_pPlayerPool->Process();
		if(m_pActorPool) m_pActorPool->Process();
		
		iPlayersBench += GetTickCount() - dwStartTick;

		if(m_pVehiclePool && iVehiclePoolProcessFlag > 5) {
			dwStartTick = GetTickCount();

			try { m_pVehiclePool->Process(); }
			catch(...) { 
				if(!iExceptMessageDisplayed) {				
					pChatWindow->AddDebugMessage("Warning: Error processing vehicle pool"); 
					iExceptMessageDisplayed++;
				}
			}
			iVehiclesBench += GetTickCount() - dwStartTick;
			iVehiclePoolProcessFlag = 0;
		} else {
			iVehiclePoolProcessFlag++;
		}
			
		if(m_pPickupPool && iPickupPoolProcessFlag > 10) {

			dwStartTick = GetTickCount();

			try { m_pPickupPool->Process(); }
			catch(...) {
				if(!iExceptMessageDisplayed) {				
					pChatWindow->AddDebugMessage("Warning: Error processing pickup pool"); 
					iExceptMessageDisplayed++;
				}
			}
			iPicksupsBench += GetTickCount() - dwStartTick;
			iPickupPoolProcessFlag = 0;
		}
		else
		{
			iPickupPoolProcessFlag++;
		}

		if(m_pObjectPool) {
			dwStartTick = GetTickCount();
			try { m_pObjectPool->Process(); }
			catch(...) { 
				if(!iExceptMessageDisplayed) {				
					pChatWindow->AddDebugMessage("Warning: Error processing object pool"); 
					iExceptMessageDisplayed++;
				}
			}
			iObjectBench += GetTickCount() - dwStartTick;
		}

		if(m_pMenuPool) {
			dwStartTick = GetTickCount();
			try { m_pMenuPool->Process(); }
			catch(...) { 
				if(!iExceptMessageDisplayed) {				
					pChatWindow->AddDebugMessage("Warning: Error processing menu pool"); 
					iExceptMessageDisplayed++;
				}
			}
			iMenuBench += GetTickCount() - dwStartTick;
		}
	}
	else {
		CPlayerPed *pPlayer = pGame->FindPlayerPed(); 
		CCamera *pCamera = pGame->GetCamera();

		if (pPlayer && pCamera) {
			if(pPlayer->IsInVehicle()) {
				pPlayer->RemoveFromVehicleAndPutAt(1500.0f,-887.0979f,32.56055f);
			} else {
				pPlayer->TeleportTo(1500.0f,-887.0979f,32.56055f);
			}
			pCamera->SetPosition(1497.803f,-887.0979f,62.56055f,0.0f,0.0f,0.0f);
			pCamera->LookAtPoint(1406.65f,-795.7716f,82.2771f,2);
			pGame->DisplayHud(FALSE);
		}
	}

	if( GetGameState() == GAMESTATE_WAIT_CONNECT && 
		(GetTickCount() - m_dwLastConnectAttempt) > 3000) 
	{
		if(pChatWindow) pChatWindow->AddDebugMessage("Connecting to %s:%d...",m_szHostOrIp,m_iPort);
		m_pRakClient->Connect(m_szHostOrIp,m_iPort,0,0,5);
		m_dwLastConnectAttempt = GetTickCount();
		SetGameState(GAMESTATE_CONNECTING);
	}
}

//----------------------------------------------------
// UPDATE NETWORK
//----------------------------------------------------

void CNetGame::UpdateNetwork()
{
	Packet* pkt = NULL;
	unsigned char packetIdentifier;
	
	while (pkt = m_pRakClient->Receive())
	{
		packetIdentifier = GetPacketID(pkt);

//		pChatWindow->AddDebugMessage("Packet Recieved: %i", packetIdentifier);
		switch (packetIdentifier)
		{
		case ID_RSA_PUBLIC_KEY_MISMATCH:
			Packet_RSAPublicKeyMismatch(pkt);
			break;
		case ID_CONNECTION_BANNED:
			Packet_ConnectionBanned(pkt);
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			Packet_NoFreeIncomingConnections(pkt);
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			Packet_DisconnectionNotification(pkt);
			break;
		case ID_CONNECTION_LOST:
			Packet_ConnectionLost(pkt);
			break;
		case ID_INVALID_PASSWORD:
			Packet_InvalidPassword(pkt);
			break;
		case ID_MODIFIED_PACKET:
			Packet_ModifiedPacket(pkt);
			break;
		case ID_CONNECTION_ATTEMPT_FAILED: 
			Packet_ConnectAttemptFailed(pkt); 
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			Packet_ConnectionSucceeded(pkt);
			break;
		case ID_PLAYER_SYNC:
			Packet_PlayerSync(pkt);
			break;
		case ID_VEHICLE_SYNC:
			Packet_VehicleSync(pkt);
			break;
		case ID_PASSENGER_SYNC:
			Packet_PassengerSync(pkt);
			break;
		case ID_AIM_SYNC:
			Packet_AimSync(pkt);
			break;
		case ID_TRAILER_SYNC:
			Packet_TrailerSync(pkt);
			break;
		case ID_UNOCCUPIED_SYNC:
			Packet_UnoccupiedSync(pkt);
			break;
		/*case ID_RANDOM_NUMBER:
			RakNet::BitStream bsRandomNum((PCHAR)pkt->data, pkt->length, false);
			BYTE bytePacketID;
			bsRandomNum.Read(bytePacketID);
			bsRandomNum.Read(iSyncedRandomNumber);
			//pChatWindow->AddDebugMessage("New rand=%d\n",iSyncedRandomNumber);
			break;*/
		}

		m_pRakClient->DeallocatePacket(pkt);		
	}

}

//----------------------------------------------------
// PACKET HANDLERS INTERNAL
//----------------------------------------------------

void CNetGame::Packet_PlayerSync(Packet *p)
{
	CRemotePlayer * pPlayer;
	RakNet::BitStream bsPlayerSync((unsigned char *)p->data, p->length, false);
	ONFOOT_SYNC_DATA ofSync;
	BYTE bytePacketID=0;
	PLAYERID playerId;
	
	bool bHasLR,bHasUD;
	bool bMoveSpeedX,bMoveSpeedY,bMoveSpeedZ;
	bool bHasVehicleSurfingInfo;

	if(GetGameState() != GAMESTATE_CONNECTED) return;

	memset(&ofSync,0,sizeof(ONFOOT_SYNC_DATA));

	bsPlayerSync.Read(bytePacketID);
	bsPlayerSync.Read(playerId);

	//bsPlayerSync.Read((PCHAR)&ofSync,sizeof(ONFOOT_SYNC_DATA));

	// LEFT/RIGHT KEYS
	bsPlayerSync.Read(bHasLR);
	if(bHasLR) bsPlayerSync.Read(ofSync.lrAnalog);
	
	// UP/DOWN KEYS
	bsPlayerSync.Read(bHasUD);
	if(bHasUD) bsPlayerSync.Read(ofSync.udAnalog);

	// GENERAL KEYS
	bsPlayerSync.Read(ofSync.wKeys);

	// VECTOR POS
	bsPlayerSync.Read((char*)&ofSync.vecPos,sizeof(VECTOR));

	// ROTATION
	bsPlayerSync.Read(ofSync.fRotation);
	
	// HEALTH/ARMOUR (COMPRESSED INTO 1 BYTE)
	BYTE byteHealthArmour;
	BYTE byteArmTemp=0,byteHlTemp=0;

	bsPlayerSync.Read(byteHealthArmour);
	byteArmTemp = (byteHealthArmour & 0x0F);
	byteHlTemp = (byteHealthArmour >> 4);

	if(byteArmTemp == 0xF) ofSync.byteArmour = 100;
	else if(byteArmTemp == 0) ofSync.byteArmour = 0;
	else ofSync.byteArmour = byteArmTemp * 7;

	if(byteHlTemp == 0xF) ofSync.byteHealth = 100;
	else if(byteHlTemp == 0) ofSync.byteHealth = 0;
	else ofSync.byteHealth = byteHlTemp * 7;

	// CURRENT WEAPON
	bsPlayerSync.Read(ofSync.byteCurrentWeapon);

	// Special Action
	bsPlayerSync.Read(ofSync.byteSpecialAction);
	
	// READ MOVESPEED VECTORS

	bsPlayerSync.Read(bMoveSpeedX);
	if(bMoveSpeedX)	bsPlayerSync.Read(ofSync.vecMoveSpeed.X);
	else ofSync.vecMoveSpeed.X = 0.0f;

	bsPlayerSync.Read(bMoveSpeedY);
	if(bMoveSpeedY)	bsPlayerSync.Read(ofSync.vecMoveSpeed.Y);
	else ofSync.vecMoveSpeed.Y = 0.0f;

	bsPlayerSync.Read(bMoveSpeedZ);
	if(bMoveSpeedZ)	bsPlayerSync.Read(ofSync.vecMoveSpeed.Z);
	else ofSync.vecMoveSpeed.Z = 0.0f;

	bsPlayerSync.Read(bHasVehicleSurfingInfo);
	if(bHasVehicleSurfingInfo) {
		bsPlayerSync.Read(ofSync.wSurfInfo);
		bsPlayerSync.Read(ofSync.vecSurfOffsets.X);
		bsPlayerSync.Read(ofSync.vecSurfOffsets.Y);
		bsPlayerSync.Read(ofSync.vecSurfOffsets.Z);
	} else {
		ofSync.wSurfInfo = INVALID_VEHICLE_ID;
	}

	if (m_pPlayerPool) {
		pPlayer = m_pPlayerPool->GetAt(playerId);
		if(pPlayer) {
			pPlayer->StoreOnFootFullSyncData(&ofSync);
		}
	}
}

//----------------------------------------------------

void CNetGame::Packet_UnoccupiedSync(Packet *p)
{
	RakNet::BitStream bsUnocSync((unsigned char *)p->data, p->length, false);
	UNOCCUPIED_SYNC_DATA unocSync;
	BYTE bytePacketID=0;
	PLAYERID playerId;

	if(GetGameState() != GAMESTATE_CONNECTED) return;

	memset(&unocSync,0,sizeof(UNOCCUPIED_SYNC_DATA));

	bsUnocSync.Read(bytePacketID);
	bsUnocSync.Read(playerId);

	bsUnocSync.Read((char *)&unocSync,sizeof(UNOCCUPIED_SYNC_DATA));

	if (m_pPlayerPool) {
		CRemotePlayer *pPlayer = m_pPlayerPool->GetAt(playerId);
		if(pPlayer) {
			pPlayer->StoreUnoccupiedSyncData(&unocSync);
		}
	}
}

//----------------------------------------------------


void CNetGame::Packet_AimSync(Packet *p)
{  
	CRemotePlayer * pPlayer;
	RakNet::BitStream bsAimSync((unsigned char *)p->data, p->length, false);
	AIM_SYNC_DATA aimSync;
	BYTE bytePacketID=0;
	PLAYERID playerId;

	if(GetGameState() != GAMESTATE_CONNECTED) return;

	bsAimSync.Read(bytePacketID);
	bsAimSync.Read(playerId);
	bsAimSync.Read((PCHAR)&aimSync,sizeof(AIM_SYNC_DATA));

	if (m_pPlayerPool) {
		pPlayer = m_pPlayerPool->GetAt(playerId);
		if(pPlayer) {
			pPlayer->UpdateAimFromSyncData(&aimSync);
		}
	}
}

//----------------------------------------------------


void CNetGame::Packet_VehicleSync(Packet *p)
{
	CRemotePlayer * pPlayer;
	RakNet::BitStream bsSync((unsigned char *)p->data, p->length, false);
	BYTE		bytePacketID=0;
	PLAYERID playerId;
	INCAR_SYNC_DATA icSync;

	bool bTire,bLandingGear;
	bool bHydra,bTrain,bTrailer;
	bool bSiren;

	if(GetGameState() != GAMESTATE_CONNECTED) return;

	memset(&icSync,0,sizeof(INCAR_SYNC_DATA));

	bsSync.Read(bytePacketID);
	bsSync.Read(playerId);
	bsSync.Read(icSync.VehicleID);

	//bsSync.Read((PCHAR)&icSync,sizeof(INCAR_SYNC_DATA));

	// KEYS
	bsSync.Read(icSync.lrAnalog);
	bsSync.Read(icSync.udAnalog);
	bsSync.Read(icSync.wKeys);

	// ROLL / DIRECTION / POSITION / MOVE SPEED
	bsSync.Read((char*)&icSync.cvecRoll,sizeof(C_VECTOR1));
	bsSync.Read((char*)&icSync.cvecDirection,sizeof(C_VECTOR1));
	bsSync.Read((char*)&icSync.vecPos,sizeof(VECTOR));
	bsSync.Read((char*)&icSync.vecMoveSpeed,sizeof(VECTOR));

	// VEHICLE HEALTH
	WORD wTempVehicleHealth;
	bsSync.Read(wTempVehicleHealth);
	icSync.fCarHealth = (float)wTempVehicleHealth;

	// HEALTH/ARMOUR (COMPRESSED INTO 1 BYTE)
	BYTE byteHealthArmour;
	BYTE byteArmTemp=0,byteHlTemp=0;

	bsSync.Read(byteHealthArmour);
	byteArmTemp = (byteHealthArmour & 0x0F);
	byteHlTemp = (byteHealthArmour >> 4);

	if(byteArmTemp == 0xF) icSync.bytePlayerArmour = 100;
	else if(byteArmTemp == 0) icSync.bytePlayerArmour = 0;
	else icSync.bytePlayerArmour = byteArmTemp * 7;

	if(byteHlTemp == 0xF) icSync.bytePlayerHealth = 100;
	else if(byteHlTemp == 0) icSync.bytePlayerHealth = 0;
	else icSync.bytePlayerHealth = byteHlTemp * 7;

	// CURRENT WEAPON
	bsSync.Read(icSync.byteCurrentWeapon);
	
	// SIREN
	bsSync.Read(bSiren);
	if(bSiren) icSync.byteSirenOn = 1;

	// LANDING GEAR
	bsSync.Read(bLandingGear);
	if(bLandingGear) icSync.byteLandingGearState = 1;

	if (m_bTirePopping) {
		bsSync.Read(bTire);
		if (bTire) icSync.byteTires[0] = 1;
		bsSync.Read(bTire);
		if (bTire) icSync.byteTires[1] = 1;
		bsSync.Read(bTire);
		if (bTire) icSync.byteTires[2] = 1;
		bsSync.Read(bTire);
		if (bTire) icSync.byteTires[3] = 1;
	}

	bsSync.Read(icSync.dwPanelDamage);
	bsSync.Read(icSync.dwDoorDamage);
	bsSync.Read(icSync.byteLightDamage);
		
	// HYDRA SPECIAL
	bsSync.Read(bHydra);
	if(bHydra) bsSync.Read(icSync.dwHydraThrustAngle);

	// TRAIN SPECIAL
	bsSync.Read(bTrain);
	if(bTrain) bsSync.Read(icSync.fTrainSpeed);

	// TRAILER ID
	bsSync.Read(bTrailer);
	if(bTrailer) bsSync.Read(icSync.TrailerID);
	
	if (m_pPlayerPool) {
		pPlayer = m_pPlayerPool->GetAt(playerId);
		if(pPlayer)	{
			pPlayer->StoreInCarFullSyncData(&icSync);
		}
	}
}

//----------------------------------------------------

void CNetGame::Packet_PassengerSync(Packet *p)
{
	CRemotePlayer * pPlayer;
	RakNet::BitStream bsPassengerSync((unsigned char *)p->data, p->length, false);
	BYTE		bytePacketID=0;
	PLAYERID	playerId;
	PASSENGER_SYNC_DATA psSync;

	if(GetGameState() != GAMESTATE_CONNECTED) return;

	bsPassengerSync.Read(bytePacketID);
	bsPassengerSync.Read(playerId);
	bsPassengerSync.Read((PCHAR)&psSync,sizeof(PASSENGER_SYNC_DATA));

	if (m_pPlayerPool) {
		pPlayer = m_pPlayerPool->GetAt(playerId);
		//OutputDebugString("Getting Passenger Packets");
		if(pPlayer)	{
			pPlayer->StorePassengerFullSyncData(&psSync);
		}
	}
}

//----------------------------------------------------

void CNetGame::Packet_TrailerSync(Packet *p)
{
	CRemotePlayer * pPlayer;
	RakNet::BitStream bsSpectatorSync((unsigned char *)p->data, p->length, false);

	if(GetGameState() != GAMESTATE_CONNECTED) return;

	BYTE bytePacketID=0;
	PLAYERID playerId;
	TRAILER_SYNC_DATA trSync;
	
	bsSpectatorSync.Read(bytePacketID);
	bsSpectatorSync.Read(playerId);
	bsSpectatorSync.Read((PCHAR)&trSync, sizeof(TRAILER_SYNC_DATA));

	if (m_pPlayerPool) {
		pPlayer = m_pPlayerPool->GetAt(playerId);
		if(pPlayer)	{
			pPlayer->StoreTrailerFullSyncData(&trSync);
		}
	}
}

//----------------------------------------------------

void CNetGame::Packet_RSAPublicKeyMismatch(Packet* packet)
{
	pChatWindow->AddDebugMessage("Failed to initialize encryption.");
}

//----------------------------------------------------

void CNetGame::Packet_ConnectionBanned(Packet* packet)
{
	pChatWindow->AddDebugMessage("You're banned from this server.");
}

//----------------------------------------------------

void CNetGame::Packet_ConnectionRequestAccepted(Packet* packet)
{
	pChatWindow->AddDebugMessage("Server has accepted the connection.");
}

//----------------------------------------------------

void CNetGame::Packet_NoFreeIncomingConnections(Packet* packet)
{
	pChatWindow->AddDebugMessage("The server is full. Retrying...");
	SetGameState(GAMESTATE_WAIT_CONNECT);	
}

//----------------------------------------------------

void CNetGame::Packet_DisconnectionNotification(Packet* packet)
{
	pChatWindow->AddDebugMessage("Server closed the connection.");
	m_pRakClient->Disconnect(0);
}

//----------------------------------------------------

void CNetGame::Packet_ConnectionLost(Packet* packet)
{
	pChatWindow->AddDebugMessage("Lost connection to the server. Reconnecting..");
	ShutdownForGameModeRestart();
    SetGameState(GAMESTATE_WAIT_CONNECT);	
}

//----------------------------------------------------

void CNetGame::Packet_InvalidPassword(Packet* packet)
{
	pChatWindow->AddDebugMessage("Wrong server password.");
	m_pRakClient->Disconnect(0);
}

//----------------------------------------------------

void CNetGame::Packet_ModifiedPacket(Packet* packet)
{
#ifdef _DEBUG
	char szBuffer[256];
	sprintf(szBuffer, "Packet was modified, sent by id: %d, ip: %s", 
					(unsigned int)packet->playerIndex, packet->playerId.ToString());
	pChatWindow->AddDebugMessage(szBuffer);
	//m_pRakClient->Disconnect(0);
#endif
}

//----------------------------------------------------
// RST

void CNetGame::Packet_ConnectAttemptFailed(Packet* packet)
{
	pChatWindow->AddDebugMessage("The server didn't respond. Retrying..");
	SetGameState(GAMESTATE_WAIT_CONNECT);
}

//----------------------------------------------------
// Connection Success

void BIG_NUM_MUL(unsigned long in[5], unsigned long out[6], unsigned long factor)
{
	/*
	Based on TTMath library by Tomasz Sowa.
	*/

	unsigned long src[5] = { 0 };
	for (int i = 0; i < 5; i++)
		src[i] = ((in[4 - i] >> 24) | ((in[4 - i] << 8) & 0x00FF0000) | ((in[4 - i] >> 8) & 0x0000FF00) | (in[4 - i] << 24));

	unsigned long long tmp = 0;

	tmp = unsigned long long(src[0])*unsigned long long(factor);
	out[0] = tmp & 0xFFFFFFFF;
	out[1] = tmp >> 32;
	tmp = unsigned long long(src[1])*unsigned long long(factor) + unsigned long long(out[1]);
	out[1] = tmp & 0xFFFFFFFF;
	out[2] = tmp >> 32;
	tmp = unsigned long long(src[2])*unsigned long long(factor) + unsigned long long(out[2]);
	out[2] = tmp & 0xFFFFFFFF;
	out[3] = tmp >> 32;
	tmp = unsigned long long(src[3])*unsigned long long(factor) + unsigned long long(out[3]);
	out[3] = tmp & 0xFFFFFFFF;
	out[4] = tmp >> 32;
	tmp = unsigned long long(src[4])*unsigned long long(factor) + unsigned long long(out[4]);
	out[4] = tmp & 0xFFFFFFFF;
	out[5] = tmp >> 32;

	for (int i = 0; i < 12; i++)
	{
		unsigned char temp = ((unsigned char*)out)[i];
		((unsigned char*)out)[i] = ((unsigned char*)out)[23 - i];
		((unsigned char*)out)[23 - i] = temp;
	}
}

int gen_gpci(char buf[64], unsigned long factor) /* by bartekdvd */
{
	unsigned char out[6 * 4] = { 0 };

	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	for (int i = 0; i < 6 * 4; ++i)
		out[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

	out[6 * 4] = 0;

	BIG_NUM_MUL((unsigned long*)out, (unsigned long*)out, factor);

	unsigned int notzero = 0;
	buf[0] = '0'; buf[1] = '\0';

	if (factor == 0) return 1;

	int pos = 0;
	for (int i = 0; i < 24; i++)
	{
		unsigned char tmp = out[i] >> 4;
		unsigned char tmp2 = out[i] & 0x0F;

		if (notzero || tmp)
		{
			buf[pos++] = (char)((tmp > 9) ? (tmp + 55) : (tmp + 48));
			if (!notzero) notzero = 1;
		}

		if (notzero || tmp2)
		{
			buf[pos++] = (char)((tmp2 > 9) ? (tmp2 + 55) : (tmp2 + 48));
			if (!notzero) notzero = 1;
		}
	}
	buf[pos] = 0;

	return pos;
}


void CNetGame::Packet_ConnectionSucceeded(Packet *p)
{
	if(pChatWindow) {
		pChatWindow->AddDebugMessage("Connection success. Loading network game...");
	}

	m_iGameState = GAMESTATE_AWAIT_JOIN;

	RakNet::BitStream bsSuccAuth((unsigned char *)p->data, p->length, false);
	PLAYERID myPlayerID;
	unsigned int uiChallenge;

	bsSuccAuth.IgnoreBits(8); // ID_CONNECTION_REQUEST_ACCEPTED
	bsSuccAuth.IgnoreBits(32); // binaryAddress
	bsSuccAuth.IgnoreBits(16); // port

	bsSuccAuth.Read(myPlayerID);
	bsSuccAuth.Read(uiChallenge);

	



	int iVersion = NETGAME_VERSION;
	unsigned int uiClientChallengeResponse = uiChallenge ^ iVersion;
	BYTE byteMod = 0x01;

	//char auth_bs[4 * 16] = { 0 };
	//gen_gpci(auth_bs, 0x3e9);

	//BYTE byteAuthBSLen;
	//byteAuthBSLen = (BYTE)strlen(auth_bs);
	BYTE byteNameLen = (BYTE)strlen(m_pPlayerPool->GetLocalPlayerName());
	BYTE iClientVerLen = (BYTE)strlen(SAMP_VERSION);

	RakNet::BitStream bsSend;

	bsSend.Write(iVersion);
	bsSend.Write(byteMod);
	bsSend.Write(byteNameLen);
	bsSend.Write(m_pPlayerPool->GetLocalPlayerName(), byteNameLen);
	bsSend.Write(uiClientChallengeResponse);

	m_pRakClient->RPC(&RPC_ClientJoin, &bsSend, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

//----------------------------------------------------

void CNetGame::UpdatePlayerScoresAndPings()
{
	static DWORD dwLastUpdateTick = 0;

	if ((GetTickCount() - dwLastUpdateTick) > 3000) {
		dwLastUpdateTick = GetTickCount();
		RakNet::BitStream bsParams;
		m_pRakClient->RPC(&RPC_UpdateScoresPingsIPs, &bsParams, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
	}
}

//----------------------------------------------------

void CNetGame::ResetVehiclePool()
{
	if(m_pVehiclePool) {
		delete m_pVehiclePool;
	}
	m_pVehiclePool = new CVehiclePool();
}

//----------------------------------------------------

void CNetGame::ResetPlayerPool()
{
	if(m_pPlayerPool) {
		delete m_pPlayerPool;
	}
	m_pPlayerPool = new CPlayerPool();
}

//----------------------------------------------------

void CNetGame::ResetPickupPool()
{
	if(m_pPickupPool) {
		delete m_pPickupPool;
	}
	m_pPickupPool = new CPickupPool();
}

//----------------------------------------------------

void CNetGame::ResetMenuPool()
{
	if(m_pMenuPool) {
		delete m_pMenuPool;
	}
	m_pMenuPool = new CMenuPool();
}

//----------------------------------------------------

void CNetGame::ResetTextDrawPool()
{
	if(m_pTextDrawPool) {
		delete m_pTextDrawPool;
	}
	m_pTextDrawPool = new CTextDrawPool();
}

//----------------------------------------------------

void CNetGame::ResetObjectPool()
{
	if(m_pObjectPool) {
		delete m_pObjectPool;
	}
	m_pObjectPool = new CObjectPool();
}

//----------------------------------------------------

void CNetGame::ResetGangZonePool()
{
	if(m_pGangZonePool) {
		delete m_pGangZonePool;
	}
	m_pGangZonePool = new CGangZonePool();
}

//-----------------------------------------------------------

void CNetGame::ResetActorPool()
{
	if (m_pActorPool) {
		delete m_pActorPool;
	}
	m_pActorPool = new CActorPool();
}

//-----------------------------------------------------------
// Puts a personal marker using any of the radar icons on the map

void CNetGame::SetMapIcon(BYTE byteIndex, float fX, float fY, float fZ, BYTE byteIcon, DWORD dwColor)
{
	if (byteIndex >= 32) return;
	if (m_dwMapIcon[byteIndex] != NULL) DisableMapIcon(byteIndex);
	//ScriptCommand(&create_radar_marker_without_sphere, fX, fY, fZ, byteIcon, &m_dwMapIcon);
	m_dwMapIcon[byteIndex] = pGame->CreateRadarMarkerIcon(byteIcon, fX, fY, fZ, dwColor);
}

//-----------------------------------------------------------
// Removes the Map Icon

void CNetGame::DisableMapIcon(BYTE byteIndex)
{
	if (byteIndex >= 32) return;
	ScriptCommand(&disable_marker, m_dwMapIcon[byteIndex]);
	m_dwMapIcon[byteIndex] = NULL;
}

//----------------------------------------------------

void CNetGame::ResetMapIcons()
{
	BYTE i;
	for (i = 0; i < 32; i++)
	{
		if (m_dwMapIcon[i] != NULL) DisableMapIcon(i);
	}
}

//----------------------------------------------------
// EOF
