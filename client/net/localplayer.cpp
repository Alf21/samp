#include "../main.h"
#include "../game/util.h"
#include "../game/keystuff.h"

extern CGame		 *pGame;
extern CChatWindow   *pChatWindow;
extern CCmdWindow	 *pCmdWindow;
extern CSpawnScreen	 *pSpawnScreen;
extern CNetGame		 *pNetGame;

using namespace RakNet;


#define IS_TARGETING(x) (x & 128)
#define IS_FIRING(x) (x & 4)

BOOL bFirstSpawn = TRUE;
DWORD dwEnterVehTimeElasped = -1;

extern int iTimesDataModified;

int iNetModeIdleOnfootSendRate		= NETMODE_IDLE_ONFOOT_SENDRATE;
int iNetModeNormalOnfootSendRate	= NETMODE_NORMAL_ONFOOT_SENDRATE;
int iNetModeIdleIncarSendRate		= NETMODE_IDLE_INCAR_SENDRATE;
int iNetModeNormalIncarSendRate		= NETMODE_NORMAL_INCAR_SENDRATE;
int iNetModeFiringSendRate			= NETMODE_FIRING_SENDRATE;
int iNetModeSendMultiplier			= NETMODE_SEND_MULTIPLIER;

//----------------------------------------------------------

CLocalPlayer::CLocalPlayer()
{
	m_bHasSpawnInfo = FALSE;
	m_pPlayerPed = pGame->FindPlayerPed();
	m_bIsActive = FALSE;
	m_bIsWasted = FALSE;
	m_ulThisSyncFrame = 0;
	m_ulLastSyncFrame = 0;
	m_wLastKeys = 0;
	m_iDisplayZoneTick = 0;
	m_dwLastSendTick = GetTickCount();
	m_dwLastSendSpecTick = GetTickCount();
	m_dwLastAimSendTick = m_dwLastSendTick;
	m_dwLastStatsUpdateTick = m_dwLastSendTick;
	m_bWantsAnotherClass = FALSE;
	m_bWaitingForSpawnRequestReply = FALSE;
	m_iSelectedClass = 0;
	m_dwLastSpawnSelectionTick = GetTickCount();
	m_dwLastHeadUpdate = GetTickCount();
	m_bInRCMode = FALSE;
	m_bSurfingMode = FALSE;

	m_bIsSpectating = FALSE;
	m_byteSpectateType = SPECTATE_TYPE_NONE;
	m_SpectateID = 0xFFFFFFFF;
	ResetAllSyncAttributes();
		
	BYTE i;
	for (i = 0; i < 13; i++)
	{
		m_byteLastWeapon[i] = 0;
		m_dwLastAmmo[i] = 0;
	}
	m_byteTeam = NO_TEAM;
}

//----------------------------------------------------------

BOOL CLocalPlayer::DestroyPlayer()
{
	return TRUE;
}

//----------------------------------------------------------

void CLocalPlayer::ResetAllSyncAttributes()
{
	m_byteCurInterior = 0;
	m_LastVehicle = INVALID_VEHICLE_ID;
	m_bInRCMode = FALSE;
	m_DamageVehicleUpdating = INVALID_VEHICLE_ID;
}

//----------------------------------------------------------

BOOL CLocalPlayer::Process()
{
	DWORD dwThisTick;

	if(m_bIsActive && (NULL != m_pPlayerPed))
	{
		// ACTIVE LOCAL PLAYER

		// Clear any spawn text if any
		pSpawnScreen->SetSpawnText(NULL);
		
		// HANDLE I'M A DEAD LOCAL PLAYER PED
		if (!m_bIsWasted && m_pPlayerPed->GetActionTrigger() == ACTION_DEATH || m_pPlayerPed->IsDead()) {
			// DEAD
			ToggleSpectating(FALSE); // Player shouldn't die while spectating, but scripts may mess with that
			
			if(m_pPlayerPed->IsDancing()) {
				m_pPlayerPed->StopDancing(); // there's no need to dance when you're dead
			}

			if(m_pPlayerPed->IsCellphoneEnabled()) {
				m_pPlayerPed->ToggleCellphone(0);
			}

			// A hack for reseting the animations/tasks
			m_pPlayerPed->TogglePlayerControllable(1);

			if(m_bInRCMode) {
				m_bInRCMode = FALSE;
				m_pPlayerPed->Add();
			}

			if (m_pPlayerPed->IsInVehicle() && !m_pPlayerPed->IsAPassenger())
			{
				SendInCarFullSyncData(); // One last time - for explosions
				m_LastVehicle = pNetGame->GetVehiclePool()->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());
			}			

			m_pPlayerPed->ExtinguishFire();

			SendWastedNotification();

			m_bIsActive = FALSE;
			m_bIsWasted = TRUE;

			// Disable zone names till they respawn (looks silly in request spawn)
			pGame->EnableZoneNames(0);

			return TRUE;
		}
	
		// HANDLE DANCING LOCAL PED
		if(m_pPlayerPed->IsDancing()) {
			pGame->DisplayHud(FALSE);
			m_pPlayerPed->ProcessDancing();
			if(GameGetInternalKeys()->wKeys1[17]) m_pPlayerPed->StopDancing();
		}

		// Handle the camera mode for goggles
		if(m_pPlayerPed->HasGoggles() && m_pPlayerPed->m_pPed->dwActiveVision != 0) {
			if (m_pPlayerPed->m_pPed->dwActiveVision == 0xC402B9 ||
				m_pPlayerPed->m_pPed->dwActiveVision == 0xC402B8) {
				*(BYTE*)(m_pPlayerPed->m_pPed->dwActiveVision) = 1;
			}
		} 

		// If the player doesn't have goggles, we need to make
		// sure that the camera modes will be resetted as well.
		else if(*(short*)0xC402B8 != 0)
			*(short*)0xC402B8 = 0;

		// HANDLE I GOT MY HANDS UP BUT DON'T WANT TO ANYMORE
		if(m_pPlayerPed->HasHandsUp() && GameGetInternalKeys()->wKeys1[17]) {
			m_pPlayerPed->TogglePlayerControllable(1);
		}

		if(m_pPlayerPed->IsInVehicle() && m_pPlayerPed->IsDancing()) 
			m_pPlayerPed->StopDancing(); // can't dance in vehicle

		dwThisTick = GetTickCount();

		if ( dwEnterVehTimeElasped != -1 && 
			(dwThisTick - dwEnterVehTimeElasped) > 5000 &&
			!m_pPlayerPed->IsInVehicle() )
		{
			pGame->GetCamera()->SetBehindPlayer();
			dwEnterVehTimeElasped = -1;
		}
		
		// Enable zone names if required on spawn so they're not there in class selection
		// 1 second delay so it doesn't display your selection area when you spawn
		if ((int)dwThisTick >= m_iDisplayZoneTick) {
			pGame->EnableZoneNames(pNetGame->m_bZoneNames);
		}

		// SERVER CHECKPOINTS UPDATE
		pGame->UpdateCheckpoints();

		// STATS UPDATES
		if((dwThisTick - m_dwLastStatsUpdateTick) > STATS_UPDATE_TICKS) {
			SendStatsUpdate();
			m_dwLastStatsUpdateTick = dwThisTick;
		}
		
		CheckWeapons();
		
		// Handle interior updates to the server
		BYTE byteInterior = pGame->GetActiveInterior();
		if (byteInterior != m_byteCurInterior) {
			UpdateRemoteInterior(byteInterior);
		}
		
		// Disabled weapons
		if ((byteInterior != 0) &&
			(!pNetGame->m_bAllowWeapons)) {
			m_pPlayerPed->SetArmedWeapon(0);
		}
				
		// The regime for adjusting sendrates is based on the number
		// of players that will be effected by this update. The more players
		// there are within a small radius, the more we must scale back
		// the number of sends.
		int iNumberOfPlayersInLocalRange = DetermineNumberOfPlayersInLocalRange();
		if(!iNumberOfPlayersInLocalRange) iNumberOfPlayersInLocalRange = 20;

		// PLAYER DATA UPDATES
		if (m_bIsSpectating) {
			ProcessSpectating();
			m_bPassengerDriveByMode = FALSE;
		}
		// DRIVER CONDITIONS
		else if(m_pPlayerPed->IsInVehicle() && !m_pPlayerPed->IsAPassenger())
		{
            ProcessInCarWorldBounds();

			CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
			CVehicle *pVehicle;
			if (pVehiclePool)
				m_CurrentVehicle = pVehiclePool->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());
			pVehicle = pVehiclePool->GetAt(m_CurrentVehicle);

			ProcessVehicleDamageUpdates(m_CurrentVehicle);

			// HANDLE DRIVING AN RC VEHICLE
			if(pVehicle && !m_bInRCMode && pVehicle->IsRCVehicle()) {
				m_pPlayerPed->Remove();
				m_bInRCMode = TRUE;
			}

			if(m_bInRCMode && !pVehicle) {
				m_pPlayerPed->SetHealth(0.0f);
				m_pPlayerPed->SetDead();
			}

			if(m_bInRCMode && pVehicle && pVehicle->GetHealth() == 0.0f) {
				m_pPlayerPed->SetHealth(0.0f);
				m_pPlayerPed->SetDead();
			}

			if((dwThisTick - m_dwLastSendTick) > (UINT)GetOptimumInCarSendRate(iNumberOfPlayersInLocalRange)) {
				m_dwLastSendTick = GetTickCount();
				SendInCarFullSyncData(); // INCAR - DRIVER				
			}
			m_bPassengerDriveByMode = FALSE;
		}
		// ONFOOT CONDITIONS
		else if(m_pPlayerPed->GetActionTrigger() == ACTION_NORMAL || m_pPlayerPed->GetActionTrigger() == ACTION_SCOPE) // Scoped - THIS IS A QUICK HACK CHANGEME
		{
			UpdateSurfing();

			// MAKE MY HEAD MOVE WITH THE CAMERA
			if((dwThisTick - m_dwLastHeadUpdate) > 200) {
                VECTOR LookAt;
				CAMERA_AIM *Aim = GameGetInternalAim();
				LookAt.X = Aim->pos1x + (Aim->f1x * 20.0f);
				LookAt.Y = Aim->pos1y + (Aim->f1y * 20.0f);
				LookAt.Z = Aim->pos1z + (Aim->f1z * 20.0f);
				pGame->FindPlayerPed()->ApplyCommandTask("FollowPedSA",0,2000,-1,&LookAt,0,0.1f,500,3,0);
				m_dwLastHeadUpdate = dwThisTick;
			}

			if(m_bInRCMode) {
				m_bInRCMode = FALSE;
				m_pPlayerPed->Add();
			}

			HandlePassengerEntry();
			ProcessOnFootWorldBounds();

			// Invalidate the last driven vehicle
			if (m_CurrentVehicle != INVALID_VEHICLE_ID) {
				m_LastVehicle = m_CurrentVehicle;
				m_CurrentVehicle = INVALID_VEHICLE_ID;
			}

			// TIMING FOR ONFOOT SEND RATES
			if((dwThisTick - m_dwLastSendTick) > (UINT)GetOptimumOnFootSendRate(iNumberOfPlayersInLocalRange)) {
				m_dwLastSendTick = GetTickCount();
				SendOnFootFullSyncData(); // ONFOOT
			}

			// TIMING FOR ONFOOT AIM SENDS
			WORD lrAnalog,udAnalog;
			WORD wKeys = m_pPlayerPed->GetKeys(&lrAnalog,&udAnalog);
			
			// Not targeting or firing. We need a very slow rate to sync the head.
			if(!IS_TARGETING(wKeys) && !IS_FIRING(wKeys)) {
				if((dwThisTick - m_dwLastAimSendTick) > (UINT)NETMODE_HEADSYNC_SENDRATE){
					m_dwLastAimSendTick = dwThisTick;
					SendAimSyncData();
				}
			}
			// Targeting only. Just synced for show really, so use a slower rate
			else if(IS_TARGETING(wKeys) && !IS_FIRING(wKeys)) {
				if((dwThisTick - m_dwLastAimSendTick) > (UINT)NETMODE_AIM_SENDRATE+(iNumberOfPlayersInLocalRange*iNetModeSendMultiplier)){
					m_dwLastAimSendTick = dwThisTick;
					SendAimSyncData();
				}
			}
			// Targeting and Firing. Needs a very accurate send rate.
			else if(IS_TARGETING(wKeys) && IS_FIRING(wKeys)) {
				if((dwThisTick - m_dwLastAimSendTick) > (UINT)iNetModeFiringSendRate+(iNumberOfPlayersInLocalRange*iNetModeSendMultiplier)) {
					m_dwLastAimSendTick = dwThisTick;
					SendAimSyncData();
				}
			}
			// Firing without targeting. Needs a normal onfoot sendrate.
			else if(!IS_TARGETING(wKeys) && IS_FIRING(wKeys)) {
				if((dwThisTick - m_dwLastAimSendTick) > (UINT)GetOptimumOnFootSendRate(iNumberOfPlayersInLocalRange)) {
					m_dwLastAimSendTick = dwThisTick;
					SendAimSyncData();
				}
			}

			m_bPassengerDriveByMode = FALSE;
		}
		// PASSENGER CONDITIONS
		else if(m_pPlayerPed->IsInVehicle() && m_pPlayerPed->IsAPassenger())
		{
			if(m_bInRCMode) {
				m_bInRCMode = FALSE;
				m_pPlayerPed->Add();
			}

			GTA_CONTROLSET * Controls = GameGetInternalKeys();
			int iWeapon = m_pPlayerPed->GetCurrentWeapon();
		
			// FOR ENTERING PASSENGER DRIVEBY MODE
			if(!m_bPassengerDriveByMode && Controls->wKeys1[18]) {
				// NOT IN DRIVEBY MODE AND HORN HELD 
				if(iWeapon == WEAPON_UZI || iWeapon == WEAPON_MP5 || iWeapon == WEAPON_TEC9) {
					if(m_pPlayerPed->StartPassengerDriveByMode()) {
						m_bPassengerDriveByMode = TRUE;
					}	
				}
			}
			if((dwThisTick - m_dwLastSendTick) > (UINT)GetOptimumInCarSendRate(iNumberOfPlayersInLocalRange)) {
				m_dwLastSendTick = GetTickCount();
				SendPassengerFullSyncData(); // INCAR - PASSENGER
			}
		}
			
		m_ulThisSyncFrame++;
	}

	// HANDLE !IsActive spectating
	if(m_bIsSpectating && !m_bIsActive) {
		ProcessSpectating();
		return TRUE;
	}

	// HANDLE THE 'WANTS ANOTHER CLASS BUTTON'
	if(!m_bWantsAnotherClass && GetAsyncKeyState(VK_F4))
	{
		m_bWantsAnotherClass = TRUE;
		pChatWindow->AddInfoMessage("Returning to class selection after next death");
	}

	// HANDLE NEEDS TO RESPAWN
	if(m_bIsWasted && (m_pPlayerPed->GetActionTrigger() != ACTION_WASTED) &&
		(m_pPlayerPed->GetActionTrigger() != ACTION_DEATH) )
	{
		if( IsClearedToSpawn() && 
			!m_bWantsAnotherClass &&
			pNetGame->GetGameState() == GAMESTATE_CONNECTED ) {

			//pGame->ToggleKeyInputsDisabled(TRUE);
			
			if (m_pPlayerPed->GetHealth() > 0.0f)
			{
				Spawn();
			}

		} else {
			m_bIsWasted = FALSE;
			HandleClassSelection();
			m_bWantsAnotherClass = FALSE;
		}

		return TRUE;
	}

	// HAND CONTROL OVER TO THE GAMELOGIC
	if((m_pPlayerPed->GetActionTrigger() != ACTION_WASTED) &&
		(m_pPlayerPed->GetActionTrigger() != ACTION_DEATH) &&
		pNetGame->GetGameState() == GAMESTATE_CONNECTED &&
		!m_bIsActive && !m_bIsSpectating) {
	
		ProcessClassSelection();
	}

	return TRUE;
}


void CLocalPlayer::ProcessUndrivenSync(VEHICLEID VehicleID, CVehicle *pVehicle, int iNumberOfPlayersInLocalRange)
{
	DWORD dwThisTick = GetTickCount();

	int iSendRate = GetOptimumUndrivenSendRate(iNumberOfPlayersInLocalRange);

	if((dwThisTick - m_dwLastUndrivenUpdate) > (UINT)iSendRate) {

		CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
		VEHICLE_TYPE *pVehicleType = pVehicle->m_pVehicle;

		if (!pVehicleType || !m_pPlayerPed) return;
		if (pVehicle->IsATrainPart()) return;

		// Don't sync if there's a PLAYER driver, or its sitting still.
		if ( (pVehicleType->pDriver && IN_VEHICLE(pVehicleType->pDriver)
			 && (pVehicleType->pDriver->dwPedType == 0))  || 
				pVehicle->IsStationary() ) { 
			pVehicle->m_bUnoccupiedSync = false;
			pVehicle->m_bRemoteUnocSync = false;
			return;
		}

		// Check if we are the first passenger of this vehicle.
		for (int p = 0; p <= 7; p++) {									// climb occupied passenger slots
			PED_TYPE *pPassenger = pVehicleType->pPassengers[p];
			if (pPassenger && pPassenger->dwPedType == 0) {
				if (pPassenger == m_pPlayerPed->m_pPed) goto sync;		// its us! we sync
				return;													// leave the syncing upto someone else
			}
		}

		// At this point we must determine the real closest player ID.
		float fDistance = 0.0f, fSmallest = 1000000.0f;
		int iClosestPlayerID=0;

		for (int i=0; i<MAX_PLAYERS; i++) {
			CPlayerPed* pPlayer = NULL;

			if (i == pPlayerPool->GetLocalPlayerID()) {
				pPlayer = m_pPlayerPed;
			} else {
				CRemotePlayer* pTmpPlayer = pPlayerPool->GetAt(i);
				if (pTmpPlayer) pPlayer = pTmpPlayer->GetPlayerPed();
			}

			if (pPlayer && pPlayer->IsAdded()) {	
				fDistance = pPlayer->GetDistanceFromVehicle(pVehicle);
				if (i) {							
					if (fDistance < fSmallest) {
						fSmallest = fDistance;
						iClosestPlayerID=i;
					}
				} else {
					fSmallest = fDistance;
				}
			}
		}

		// Are we the closest player, and within 70 units of a moving nondriven vehicle?
		if	( (iClosestPlayerID == pPlayerPool->GetLocalPlayerID()) &&
			( fSmallest <= UNOCCUPIED_SYNC_RADIUS ) ) {

	sync:
			//pChatWindow->AddDebugMessage("UNDRIVEN [ID: %i sendrate: %i ms]", VehicleID, iSendRate);
			RakNet::BitStream bsUnoccupiedSync;

			VECTOR vecMoveSpeed, vecTurnSpeed;
			MATRIX4X4 matVehicle;
			UNOCCUPIED_SYNC_DATA UnSync;

			pVehicle->GetMatrix(&matVehicle);
			pVehicle->GetMoveSpeedVector(&vecMoveSpeed);
			pVehicle->GetTurnSpeedVector(&vecTurnSpeed);

			CompressNormalVector(&matVehicle.right,&UnSync.cvecRoll);
			CompressNormalVector(&matVehicle.up,&UnSync.cvecDirection);

			UnSync.VehicleID = VehicleID;
			UnSync.vecMoveSpeed.X = vecMoveSpeed.X;
			UnSync.vecMoveSpeed.Y = vecMoveSpeed.Y;
			UnSync.vecMoveSpeed.Z = vecMoveSpeed.Z;
			UnSync.vecTurnSpeed.X = vecTurnSpeed.X;
			UnSync.vecTurnSpeed.Y = vecTurnSpeed.Y;
			UnSync.vecTurnSpeed.Z = vecTurnSpeed.Z;
			UnSync.vecPos.X = matVehicle.pos.X;
			UnSync.vecPos.Y = matVehicle.pos.Y;
			UnSync.vecPos.Z = matVehicle.pos.Z;
			UnSync.fHealth = pVehicle->GetHealth();
			UnSync.dwDoorDamage = pVehicle->GetDoorDamageStatus();
			UnSync.dwPanelDamage = pVehicle->GetPanelDamageStatus();
			UnSync.byteLightDamage = pVehicle->GetLightDamageStatus();

			bsUnoccupiedSync.Write((BYTE)ID_UNOCCUPIED_SYNC);
			bsUnoccupiedSync.Write((PCHAR)&UnSync,sizeof(UNOCCUPIED_SYNC_DATA));
			pNetGame->GetRakClient()->Send(&bsUnoccupiedSync,HIGH_PRIORITY,UNRELIABLE_SEQUENCED,0);

			CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
			if (pVehiclePool) {
				pVehiclePool->SetLastUndrivenID(VehicleID, pPlayerPool->GetLocalPlayerID()); // us
			}
				
			pVehicle->m_bUnoccupiedSync = true;
			m_dwLastUndrivenUpdate = dwThisTick;
		}
	}

}

//----------------------------------------------------------

void CLocalPlayer::HandlePassengerEntry()
{
	GTA_CONTROLSET *pControls = GameGetInternalKeys();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(pControls->wKeys1[8] && !pControls->wKeys2[8]) { // RECRUIT KEY JUST DOWN.

		VEHICLEID ClosestVehicleID = pVehiclePool->FindNearestToLocalPlayerPed();

		if(ClosestVehicleID < MAX_VEHICLES && pVehiclePool->GetSlotState(ClosestVehicleID)) {
			CVehicle *pVehicle = pVehiclePool->GetAt(ClosestVehicleID);
			if(pVehicle->GetDistanceFromLocalPlayerPed() < 4.0f) {
				// If armed with the para, set to fists
				if(m_pPlayerPed->GetCurrentWeapon() == WEAPON_PARACHUTE) {
					m_pPlayerPed->SetArmedWeapon(0);
				}
				// Enter locally
				m_pPlayerPed->EnterVehicle(pVehicle->m_dwGTAId,TRUE);
				// Send Net Notification
				SendEnterVehicleNotification(ClosestVehicleID,TRUE);
			}
		}
	}
}

//----------------------------------------------------------

void CLocalPlayer::ApplySpecialAction(BYTE byteSpecialAction)
{
	switch(byteSpecialAction) {

		case SPECIAL_ACTION_USEJETPACK:
			if(!m_pPlayerPed->IsInJetpackMode()) m_pPlayerPed->StartJetpack();
			break;

		case SPECIAL_ACTION_DANCE1:
			m_pPlayerPed->StartDancing(0);
			break;

		case SPECIAL_ACTION_DANCE2:
			m_pPlayerPed->StartDancing(1);
			break;

		case SPECIAL_ACTION_DANCE3:
			m_pPlayerPed->StartDancing(2);
			break;

		case SPECIAL_ACTION_DANCE4:
			m_pPlayerPed->StartDancing(3);
			break;

		case SPECIAL_ACTION_HANDSUP:
			m_pPlayerPed->HandsUp();
			break;

		case SPECIAL_ACTION_USECELLPHONE:
			if(!m_pPlayerPed->IsInVehicle()) {
				m_pPlayerPed->ToggleCellphone(1);
			}
			break;

		case SPECIAL_ACTION_STOPUSECELLPHONE:
			if(m_pPlayerPed->IsCellphoneEnabled()) {
				m_pPlayerPed->ToggleCellphone(0);
			}
			break;

		case SPECIAL_ACTION_THERMALVISION:
		case SPECIAL_ACTION_NIGHTVISION:
			m_pPlayerPed->StartGoggles();
			break;

		case SPECIAL_ACTION_NONE:
			if (m_pPlayerPed->HasGoggles())
				m_pPlayerPed->StopGoggles();
			break;

	}
}

//----------------------------------------------------------

BYTE CLocalPlayer::GetSpecialAction()
{
	if(m_pPlayerPed->IsInJetpackMode()) {
		return SPECIAL_ACTION_USEJETPACK;
	}

	if(m_pPlayerPed->HasGoggles()) {
		return m_pPlayerPed->GetGoggleType();
	}

	if(m_pPlayerPed->IsDancing()) {
		switch(m_pPlayerPed->m_iDanceStyle) {
			case 0:
				return SPECIAL_ACTION_DANCE1;
			case 1:
				return SPECIAL_ACTION_DANCE2;
			case 2:
				return SPECIAL_ACTION_DANCE3;
			case 3:
				return SPECIAL_ACTION_DANCE4;
		}
	}

	if(m_pPlayerPed->HasHandsUp()) {
		//pChatWindow->AddDebugMessage("SPECIAL_ACTION_HANDSUP");
		return SPECIAL_ACTION_HANDSUP;
	}

	if(m_pPlayerPed->IsCellphoneEnabled()) {
		return SPECIAL_ACTION_USECELLPHONE;
	}

	return SPECIAL_ACTION_NONE;
}

//----------------------------------------------------------

void CLocalPlayer::UpdateSurfing()
{	
	VEHICLE_TYPE *Contact = m_pPlayerPed->GetGtaContactVehicle();

	if(!Contact) {
		m_bSurfingMode = FALSE;
		m_vecLockedSurfingOffsets.X = 0.0f;
		m_vecLockedSurfingOffsets.Y = 0.0f;
		m_vecLockedSurfingOffsets.Z = 0.0f;
		m_SurfingID = INVALID_VEHICLE_ID;
		return;
	}

	VEHICLEID vehID = pNetGame->GetVehiclePool()->FindIDFromGtaPtr(Contact);

	if(vehID && vehID != INVALID_VEHICLE_ID) {
		CVehicle *pVehicle = pNetGame->GetVehiclePool()->GetAt(vehID);

		if( pVehicle && pVehicle->IsOccupied() && 
			pVehicle->GetDistanceFromLocalPlayerPed() < 5.0f ) {

			VECTOR vecSpeed;
			VECTOR vecTurn;
			MATRIX4X4 matPlayer;
			MATRIX4X4 matVehicle;
			VECTOR vecVehiclePlane = {0.0f,0.0f,0.0f};

			pVehicle->GetMatrix(&matVehicle);
			m_pPlayerPed->GetMatrix(&matPlayer);

			//_Multiply3x3(&vecVehiclePlane,Contact->entity.mat,&vecVehiclePlane);
			/* if(m_bSurfingMode == TRUE && (lr || ud)) {
				// allow them to update their surfing offsets while
				// analog keys/stick is non-0
				m_vecLockedSurfingOffsets.X = matPlayer.pos.X - (matVehicle.pos.X + vecVehiclePlane.X);
				m_vecLockedSurfingOffsets.Y = matPlayer.pos.Y - (matVehicle.pos.Y + vecVehiclePlane.Y);
				m_vecLockedSurfingOffsets.Z = matPlayer.pos.Z - (matVehicle.pos.Z + vecVehiclePlane.Z);
				return;
			}*/

			m_bSurfingMode = TRUE;
			m_SurfingID = vehID;

			m_vecLockedSurfingOffsets.X = matPlayer.pos.X - matVehicle.pos.X;
			m_vecLockedSurfingOffsets.Y = matPlayer.pos.Y - matVehicle.pos.Y;
			m_vecLockedSurfingOffsets.Z = matPlayer.pos.Z - matVehicle.pos.Z;

			vecSpeed.X = Contact->entity.vecMoveSpeed.X;
			vecSpeed.Y = Contact->entity.vecMoveSpeed.Y;
			vecSpeed.Z = Contact->entity.vecMoveSpeed.Z;
			vecTurn.X = Contact->entity.vecTurnSpeed.X;
			vecTurn.Y = Contact->entity.vecTurnSpeed.Y;
			vecTurn.Z = Contact->entity.vecTurnSpeed.Z;

			//m_pPlayerPed->SetMoveSpeedVector(vecSpeed);
			//m_pPlayerPed->SetTurnSpeedVector(vecTurn);
			return;
		}
	}
	m_bSurfingMode = FALSE;
	m_vecLockedSurfingOffsets.X = 0.0f;
	m_vecLockedSurfingOffsets.Y = 0.0f;
	m_vecLockedSurfingOffsets.Z = 0.0f;
	m_SurfingID = INVALID_VEHICLE_ID;
}

//----------------------------------------------------------

void CLocalPlayer::SendOnFootFullSyncData()
{
	RakNet::BitStream bsPlayerSync;
	MATRIX4X4 matPlayer;
	VECTOR vecMoveSpeed;
	WORD lrAnalog,udAnalog;
	WORD wKeys = m_pPlayerPed->GetKeys(&lrAnalog,&udAnalog);
	
	ONFOOT_SYNC_DATA ofSync;

	m_pPlayerPed->GetMatrix(&matPlayer);
	m_pPlayerPed->GetMoveSpeedVector(&vecMoveSpeed);

	// GENERAL PLAYER SYNC DATA
	ofSync.lrAnalog = lrAnalog;
	ofSync.udAnalog = udAnalog;
	ofSync.wKeys = wKeys;
	ofSync.vecPos.X = matPlayer.pos.X;
	ofSync.vecPos.Y = matPlayer.pos.Y;
	ofSync.vecPos.Z = matPlayer.pos.Z;

	// Rotation stuff
	ofSync.fRotation = m_pPlayerPed->GetTargetRotation();
	ofSync.byteHealth = (BYTE)m_pPlayerPed->GetHealth();
	ofSync.byteArmour = (BYTE)m_pPlayerPed->GetArmour();
	
	ofSync.byteCurrentWeapon = m_pPlayerPed->GetCurrentWeapon();
	ofSync.byteSpecialAction = GetSpecialAction();
		
	ofSync.vecMoveSpeed.X = vecMoveSpeed.X;
	ofSync.vecMoveSpeed.Y = vecMoveSpeed.Y;
	ofSync.vecMoveSpeed.Z = vecMoveSpeed.Z;

	if ( m_bSurfingMode ) {
		ofSync.vecSurfOffsets.X = m_vecLockedSurfingOffsets.X;
		ofSync.vecSurfOffsets.Y = m_vecLockedSurfingOffsets.Y;
		ofSync.vecSurfOffsets.Z = m_vecLockedSurfingOffsets.Z;
		ofSync.wSurfInfo = m_SurfingID;
	} else {
		ofSync.vecSurfOffsets.X = 0.0f;
		ofSync.vecSurfOffsets.Y = 0.0f;
		ofSync.vecSurfOffsets.Z = 0.0f;
		ofSync.wSurfInfo = 0;
	}

	bsPlayerSync.Write((BYTE)ID_PLAYER_SYNC);
	bsPlayerSync.Write((PCHAR)&ofSync,sizeof(ONFOOT_SYNC_DATA));
	pNetGame->GetRakClient()->Send(&bsPlayerSync,HIGH_PRIORITY,UNRELIABLE_SEQUENCED,0);
}

//----------------------------------------------------------

void CLocalPlayer::SendAimSyncData()
{
	RakNet::BitStream bsAimSync;
	AIM_SYNC_DATA aimSync;
	CAMERA_AIM * caAim = m_pPlayerPed->GetCurrentAim();

	aimSync.byteCamMode = m_pPlayerPed->GetCameraMode();

	aimSync.vecAimf1.X = caAim->f1x;
	aimSync.vecAimf1.Y = caAim->f1y;
	aimSync.vecAimf1.Z = caAim->f1z;
	aimSync.vecAimf2.X = caAim->f2x;
	aimSync.vecAimf2.Y = caAim->f2y;
	aimSync.vecAimf2.Z = caAim->f2z;

	aimSync.vecAimPos.X = caAim->pos1x;
	aimSync.vecAimPos.Y = caAim->pos1y;
	aimSync.vecAimPos.Z = caAim->pos1z;

	aimSync.fAimZ = m_pPlayerPed->GetAimZ();
	
	aimSync.byteCamExtZoom = (BYTE)(m_pPlayerPed->GetCameraExtendedZoom() * 63.0f);
	
	WEAPON_SLOT_TYPE* pwstWeapon = m_pPlayerPed->GetCurrentWeaponSlot();
	if (pwstWeapon->dwState == 2)
		aimSync.byteWeaponState = WS_RELOADING;
	else
		aimSync.byteWeaponState = (pwstWeapon->dwAmmoInClip > 1) ? WS_MORE_BULLETS : pwstWeapon->dwAmmoInClip;

	bsAimSync.Write((BYTE)ID_AIM_SYNC);
	bsAimSync.Write((PCHAR)&aimSync,sizeof(AIM_SYNC_DATA));
	pNetGame->GetRakClient()->Send(&bsAimSync,HIGH_PRIORITY,UNRELIABLE_SEQUENCED,0);
}

//----------------------------------------------------------

void CLocalPlayer::SendInCarFullSyncData()
{
	RakNet::BitStream bsVehicleSync;
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	MATRIX4X4 matPlayer;
	VECTOR vecMoveSpeed;

	WORD lrAnalog,udAnalog;
	WORD wKeys = m_pPlayerPed->GetKeys(&lrAnalog,&udAnalog);
	CVehicle *pGameVehicle=NULL;
	
	INCAR_SYNC_DATA icSync;

	if(m_pPlayerPed)
	{
		icSync.VehicleID = pVehiclePool->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());
		
		if(icSync.VehicleID == INVALID_VEHICLE_ID) return; // not valid

		icSync.lrAnalog = lrAnalog;
		icSync.udAnalog = udAnalog;
		icSync.wKeys = wKeys;

		// get the vehicle matrix
		pGameVehicle = pVehiclePool->GetAt(icSync.VehicleID);
		if(!pGameVehicle) return;

		pGameVehicle->GetMatrix(&matPlayer);

		CompressNormalVector(&matPlayer.right,&icSync.cvecRoll);
		CompressNormalVector(&matPlayer.up,&icSync.cvecDirection);

		icSync.vecPos.X = matPlayer.pos.X;
		icSync.vecPos.Y = matPlayer.pos.Y;
		icSync.vecPos.Z = matPlayer.pos.Z;
			
		pGameVehicle->GetMoveSpeedVector(&vecMoveSpeed);

		icSync.vecMoveSpeed.X = vecMoveSpeed.X;
		icSync.vecMoveSpeed.Y = vecMoveSpeed.Y;
		icSync.vecMoveSpeed.Z = vecMoveSpeed.Z;

		icSync.fCarHealth = pGameVehicle->GetHealth();
		icSync.bytePlayerHealth = (BYTE)m_pPlayerPed->GetHealth();
		icSync.bytePlayerArmour = (BYTE)m_pPlayerPed->GetArmour();

		// Note: Train Speed and Tire Popping values are mutually exclusive, which means
		//       if one is set, the other one will be affected.

		if( pGameVehicle->GetModelIndex() == TRAIN_PASSENGER_LOCO ||
			pGameVehicle->GetModelIndex() == TRAIN_FREIGHT_LOCO ||
			pGameVehicle->GetModelIndex() == TRAIN_TRAM) {
				icSync.fTrainSpeed = pGameVehicle->GetTrainSpeed();
		} else {
			icSync.fTrainSpeed = 0.0f;
			if (pNetGame->m_bTirePopping) {
				if (pGameVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_BIKE) {
					icSync.byteTires[0] = pGameVehicle->GetWheelPopped(0);
					icSync.byteTires[1] = pGameVehicle->GetWheelPopped(1);
					icSync.byteTires[2] = 0;
					icSync.byteTires[3] = 0;
				} else if ( pGameVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) {
					icSync.byteTires[0] = pGameVehicle->GetWheelPopped(0);
					icSync.byteTires[1] = pGameVehicle->GetWheelPopped(1);
					icSync.byteTires[2] = pGameVehicle->GetWheelPopped(2);
					icSync.byteTires[3] = pGameVehicle->GetWheelPopped(3);
				}
			}
		}
	
		icSync.dwPanelDamage = pGameVehicle->GetPanelDamageStatus();
		icSync.dwDoorDamage = pGameVehicle->GetDoorDamageStatus();
		icSync.byteLightDamage = pGameVehicle->GetLightDamageStatus();

		icSync.TrailerID = 0;
		VEHICLE_TYPE* vehTrailer = (VEHICLE_TYPE*)pGameVehicle->m_pVehicle->dwTrailer;
		if (vehTrailer != NULL)	{
			if ( ScriptCommand(&is_trailer_on_cab, 
				 pVehiclePool->FindGtaIDFromGtaPtr(vehTrailer), 
				 pGameVehicle->m_dwGTAId) )
			{
				icSync.TrailerID = pVehiclePool->FindIDFromGtaPtr(vehTrailer);
			} else {
				icSync.TrailerID = 0;
			}
		}

		// SPECIAL STUFF
		if(pGameVehicle->GetModelIndex() == HYDRA)
			icSync.dwHydraThrustAngle = pGameVehicle->GetHydraThrusters();
		else icSync.dwHydraThrustAngle = 0;
		
		// Some other SPECIAL sync stuff (these can be optimized for specific vehicles, someday!)
		//icSync.byteFlags = 0;
		//if (pGameVehicle->IsSirenOn()) icSync.bitSirenState = 1;
		//if (pGameVehicle->AreLightsEnabled()) icSync.bitLightState = 1;

		if(pGameVehicle->IsSirenOn()) icSync.byteSirenOn = 1;
		else icSync.byteSirenOn = 0;

		if(pGameVehicle->GetLandingGearState() == LGS_UP) 
			icSync.byteLandingGearState = 0;
		else icSync.byteLandingGearState = 1;

		if (wKeys & 4) { // firing
			BYTE byteCurrentWeapon = m_pPlayerPed->GetCurrentWeapon();
			if (byteCurrentWeapon == WEAPON_UZI || 
				byteCurrentWeapon == WEAPON_MP5 || 
				byteCurrentWeapon == WEAPON_TEC9) 
				icSync.byteCurrentWeapon = byteCurrentWeapon;
		}

		// send
		bsVehicleSync.Write((BYTE)ID_VEHICLE_SYNC);
		bsVehicleSync.Write((PCHAR)&icSync,sizeof(INCAR_SYNC_DATA));
		pNetGame->GetRakClient()->Send(&bsVehicleSync,HIGH_PRIORITY,UNRELIABLE_SEQUENCED,0);

		// For the tank/firetruck, we need some info on aiming
		if (pGameVehicle->HasTurret()) SendAimSyncData();		
			
		if (icSync.TrailerID && icSync.TrailerID < MAX_VEHICLES)
		{
			MATRIX4X4 matTrailer;
			TRAILER_SYNC_DATA trSync;
			CVehicle* pTrailer = pVehiclePool->GetAt(icSync.TrailerID);
			if (pTrailer)
			{
				pTrailer->GetMatrix(&matTrailer);
				
				CompressNormalVector(&matTrailer.right,&trSync.cvecRoll);
				CompressNormalVector(&matTrailer.up,&trSync.cvecDirection);
				
				trSync.vecPos.X = matTrailer.pos.X;
				trSync.vecPos.Y = matTrailer.pos.Y;
				trSync.vecPos.Z = matTrailer.pos.Z;
				
				pTrailer->GetMoveSpeedVector(&trSync.vecMoveSpeed);
				
				RakNet::BitStream bsTrailerSync;
				bsTrailerSync.Write((BYTE)ID_TRAILER_SYNC);
				bsTrailerSync.Write((PCHAR)&trSync, sizeof (TRAILER_SYNC_DATA));
				pNetGame->GetRakClient()->Send(&bsTrailerSync,HIGH_PRIORITY,UNRELIABLE_SEQUENCED,0);
			}
		}
	}
}

//----------------------------------------------------------

void CLocalPlayer::SendPassengerFullSyncData()
{
	RakNet::BitStream bsPassengerSync;
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	WORD lrAnalog,udAnalog;
	WORD wKeys = m_pPlayerPed->GetKeys(&lrAnalog,&udAnalog);
	PASSENGER_SYNC_DATA psSync;
	MATRIX4X4 mat;

	psSync.VehicleID = pVehiclePool->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());
	
	if(psSync.VehicleID == INVALID_VEHICLE_ID) return;  // not valid

	psSync.lrAnalog = lrAnalog;
	psSync.udAnalog = udAnalog;
	psSync.wKeys = wKeys;
	psSync.bytePlayerHealth = (BYTE)m_pPlayerPed->GetHealth();
	psSync.bytePlayerArmour = (BYTE)m_pPlayerPed->GetArmour();

	psSync.byteSeatFlags = m_pPlayerPed->GetVehicleSeatID();
	psSync.byteDriveBy = m_bPassengerDriveByMode;

	psSync.byteCurrentWeapon = m_pPlayerPed->GetCurrentWeapon();

	m_pPlayerPed->GetMatrix(&mat);
	psSync.vecPos.X = mat.pos.X;
	psSync.vecPos.Y = mat.pos.Y;
	psSync.vecPos.Z = mat.pos.Z;

	bsPassengerSync.Write((BYTE)ID_PASSENGER_SYNC);
	bsPassengerSync.Write((PCHAR)&psSync,sizeof(PASSENGER_SYNC_DATA));
	pNetGame->GetRakClient()->Send(&bsPassengerSync,HIGH_PRIORITY,UNRELIABLE_SEQUENCED,0);

	if(m_bPassengerDriveByMode)	SendAimSyncData();
}

//----------------------------------------------------------

int CLocalPlayer::GetOptimumInCarSendRate(int iPlayersEffected)
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	CVehicle	 *pGameVehicle=NULL;
	VECTOR		 vecMoveSpeed;
	VEHICLEID	 VehicleID=0;

	if(m_pPlayerPed) {

		VehicleID = pVehiclePool->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());
		pGameVehicle = pVehiclePool->GetAt(VehicleID);

		if(pGameVehicle) {

			pGameVehicle->GetMoveSpeedVector(&vecMoveSpeed);

			if( (vecMoveSpeed.X == 0.0f) &&
				(vecMoveSpeed.Y == 0.0f) &&
				(vecMoveSpeed.Z == 0.0f) ) {

				if(pNetGame->IsLanMode()) return LANMODE_IDLE_INCAR_SENDRATE;
				else return (iNetModeIdleIncarSendRate + (int)iPlayersEffected*iNetModeSendMultiplier);
			}
			else {
				if(pNetGame->IsLanMode()) return LANMODE_NORMAL_INCAR_SENDRATE;
				else return (iNetModeNormalIncarSendRate + (int)iPlayersEffected*iNetModeSendMultiplier);
			}
		}
	}
	return 1000;
}

//----------------------------------------------------------

int CLocalPlayer::GetOptimumUndrivenSendRate(int iPlayersEffected)
{
	if(m_pPlayerPed) {

		if (m_pPlayerPed->GetGtaVehicle()) // ramming an undriven vehicle
		{
			return GetOptimumInCarSendRate(iPlayersEffected);
	
		} else { // pushing an undriven vehicle

			return GetOptimumOnFootSendRate(iPlayersEffected);
		}

	}
	return 1000;
}

//----------------------------------------------------------

int CLocalPlayer::GetOptimumOnFootSendRate(int iPlayersEffected)
{	
	VECTOR	 vecMoveSpeed;

	WORD lrAnalog,udAnalog;
	WORD wKeys = m_pPlayerPed->GetKeys(&lrAnalog,&udAnalog);

	if(m_pPlayerPed) {

		m_pPlayerPed->GetMoveSpeedVector(&vecMoveSpeed);

		if( (vecMoveSpeed.X == 0.0f) &&
			(vecMoveSpeed.Y == 0.0f) &&
			(vecMoveSpeed.Z == 0.0f) &&
			!IS_TARGETING(wKeys) ) {

			if(pNetGame->IsLanMode()) return LANMODE_IDLE_ONFOOT_SENDRATE;
			else return (iNetModeIdleOnfootSendRate + (int)iPlayersEffected*iNetModeSendMultiplier); // scale to number of players.
		}
		else {
			if(pNetGame->IsLanMode()) return LANMODE_NORMAL_ONFOOT_SENDRATE;
			else return (iNetModeNormalOnfootSendRate + (int)iPlayersEffected*iNetModeSendMultiplier); // scale to number of players.
		}
	}

	return 1000;
}

//----------------------------------------------------------

void CLocalPlayer::SendWastedNotification()
{
	RakNet::BitStream bsPlayerDeath;
	BYTE byteDeathReason;
	PLAYERID WhoWasResponsible;

	byteDeathReason = m_pPlayerPed->FindDeathReasonAndResponsiblePlayer(&WhoWasResponsible);
	
	bsPlayerDeath.Write(byteDeathReason);
	bsPlayerDeath.Write(WhoWasResponsible);
	pNetGame->GetRakClient()->RPC(&RPC_Death, &bsPlayerDeath, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

//----------------------------------------------------------

void CLocalPlayer::RequestClass(int iClass)
{
	RakNet::BitStream bsSpawnRequest;
	bsSpawnRequest.Write(iClass);
	pNetGame->GetRakClient()->RPC(&RPC_RequestClass,&bsSpawnRequest,HIGH_PRIORITY,RELIABLE,0,false, UNASSIGNED_NETWORK_ID, NULL);
}

//----------------------------------------------------------

void CLocalPlayer::RequestSpawn()
{
	RakNet::BitStream bsSpawnRequest;
	pNetGame->GetRakClient()->RPC(&RPC_RequestSpawn,&bsSpawnRequest,HIGH_PRIORITY,RELIABLE,0,false, UNASSIGNED_NETWORK_ID, NULL);
}

//----------------------------------------------------------

void CLocalPlayer::SetSpawnInfo(PLAYER_SPAWN_INFO *pSpawn)
{
	memcpy(&m_SpawnInfo,pSpawn,sizeof(PLAYER_SPAWN_INFO));
	m_bHasSpawnInfo = TRUE;
}

//----------------------------------------------------------

void CLocalPlayer::UpdateRemoteInterior(BYTE byteInterior)
{
	m_byteCurInterior = byteInterior;
	RakNet::BitStream bsUpdateInterior;
	bsUpdateInterior.Write(byteInterior);
	pNetGame->GetRakClient()->RPC(&RPC_SetInteriorId,&bsUpdateInterior,HIGH_PRIORITY,RELIABLE,0,false, UNASSIGNED_NETWORK_ID, NULL);
}

//----------------------------------------------------------

BOOL CLocalPlayer::Spawn()
{
	if(!m_bHasSpawnInfo) return FALSE;

	CCamera			*pGameCamera;
	pGameCamera = pGame->GetCamera();
	pGameCamera->Restore();
	pGameCamera->SetBehindPlayer();
	pGame->DisplayHud(TRUE);
	m_pPlayerPed->TogglePlayerControllable(1);
	m_pPlayerPed->SetCollisionChecking(1);
	m_pPlayerPed->SetGravityProcessing(1);

	iTimesDataModified = 0;

	if(!bFirstSpawn) {
		m_pPlayerPed->SetInitialState();
	} else {
		bFirstSpawn = FALSE;
	}

	pGame->RefreshStreamingAt(m_SpawnInfo.vecPos.X,m_SpawnInfo.vecPos.Y);

	m_pPlayerPed->RestartIfWastedAt(&m_SpawnInfo.vecPos, m_SpawnInfo.fRotation);
	m_pPlayerPed->SetModelIndex(m_SpawnInfo.iSkin);
	m_pPlayerPed->ClearAllWeapons();
	m_pPlayerPed->ResetDamageEntity();

	if(m_SpawnInfo.iSpawnWeapons[2] != (-1))
		m_pPlayerPed->GiveWeapon(m_SpawnInfo.iSpawnWeapons[2],
		m_SpawnInfo.iSpawnWeaponsAmmo[2]);

	if(m_SpawnInfo.iSpawnWeapons[1] != (-1))
		m_pPlayerPed->GiveWeapon(m_SpawnInfo.iSpawnWeapons[1],
		m_SpawnInfo.iSpawnWeaponsAmmo[1]);

	if(m_SpawnInfo.iSpawnWeapons[0] != (-1))
		m_pPlayerPed->GiveWeapon(m_SpawnInfo.iSpawnWeapons[0],
		m_SpawnInfo.iSpawnWeaponsAmmo[0]);

	pGame->SetMaxStats();
	pGame->DisableTrainTraffic();
	
	// No fading CCamera_Fade. (applied here because otherwise there's audio volume issues)
	UnFuck(0x50AC20,3);
	*(PBYTE)0x50AC20 = 0xC2;
	*(PBYTE)0x50AC21 = 0x08;
	*(PBYTE)0x50AC22 = 0x00;
		
	m_pPlayerPed->TeleportTo(m_SpawnInfo.vecPos.X,
		m_SpawnInfo.vecPos.Y,(m_SpawnInfo.vecPos.Z + 1.0f));
	
	m_pPlayerPed->SetTargetRotation(m_SpawnInfo.fRotation);
		
	m_bIsWasted = FALSE;
	m_bIsActive = TRUE;
	m_bWaitingForSpawnRequestReply = FALSE;

	// Let the rest of the network know we're spawning.
	RakNet::BitStream bsSendSpawn;
	pNetGame->GetRakClient()->RPC(&RPC_Spawn,&bsSendSpawn,HIGH_PRIORITY,
		RELIABLE_SEQUENCED,0,false, UNASSIGNED_NETWORK_ID, NULL);

	m_iDisplayZoneTick = GetTickCount() + 1000;
	
	return TRUE;
}

//----------------------------------------------------------

void CLocalPlayer::Say(PCHAR szText)
{
	if (!pNetGame) return;
	BYTE byteTextLen = strlen(szText);

	RakNet::BitStream bsSend;
	bsSend.Write(byteTextLen);
	bsSend.Write(szText,byteTextLen);

	pNetGame->GetRakClient()->RPC(&RPC_Chat,&bsSend,HIGH_PRIORITY,RELIABLE,0,false, UNASSIGNED_NETWORK_ID, NULL);
	
	// Comment by spookie:
	//   Local player chat is now sent to the server so it can be filtered by the
	//   scripts before it is displayed in the chat window.
}

//----------------------------------------------------------

void CLocalPlayer::Msg(PLAYERID ToPlayer, PCHAR szText)
{
	if (ToPlayer == pNetGame->GetPlayerPool()->GetLocalPlayerID()) {
		pChatWindow->AddInfoMessage("You cannot PM yourself!");
		return;
	}
	if (ToPlayer > MAX_PLAYERS || !pNetGame->GetPlayerPool()->GetSlotState(ToPlayer)) {
		pChatWindow->AddInfoMessage("Enter a valid player ID!");
		return;
	}

	BYTE byteTextLen = strlen(szText);

	RakNet::BitStream bsSend;
	bsSend.Write(ToPlayer);
	bsSend.Write(byteTextLen);
	bsSend.Write(szText,byteTextLen);

	//pNetGame->GetRakClient()->RPC(&RPC_Privmsg,&bsSend,HIGH_PRIORITY,RELIABLE,0,false, UNASSIGNED_NETWORK_ID, NULL);
}

//----------------------------------------------------------

void CLocalPlayer::TeamMsg(PCHAR szText)
{
	BYTE byteTextLen = strlen(szText);

	RakNet::BitStream bsSend;
	bsSend.Write(byteTextLen);
	bsSend.Write(szText,byteTextLen);

//	pNetGame->GetRakClient()->RPC(&RPC_TeamPrivmsg,&bsSend,HIGH_PRIORITY,RELIABLE,0,false, UNASSIGNED_NETWORK_ID, NULL);
}

//----------------------------------------------------------

void CLocalPlayer::SendEnterVehicleNotification(VEHICLEID VehicleID, BOOL bPassenger)
{
	RakNet::BitStream bsSend;
	BYTE bytePassenger=0;

	//pChatWindow->AddDebugMessage("Enter Vehicle: %u %d",VehicleID,bPassenger);

	if(bPassenger) {
		bytePassenger = 1;
	}
	bsSend.Write(VehicleID);
	bsSend.Write(bytePassenger);

	pNetGame->GetRakClient()->RPC(&RPC_EnterVehicle,&bsSend,HIGH_PRIORITY,RELIABLE_SEQUENCED,0,false, UNASSIGNED_NETWORK_ID, NULL);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	CVehicle* pVehicle = pVehiclePool->GetAt(VehicleID);

	if (pVehicle && pVehicle->IsATrainPart()) {
		DWORD dwVehicle = pVehicle->m_dwGTAId;
		ScriptCommand(&camera_on_vehicle, dwVehicle, 3, 2);
		dwEnterVehTimeElasped = GetTickCount();
	}
}

//----------------------------------------------------------

void CLocalPlayer::SendExitVehicleNotification(VEHICLEID VehicleID)
{
	RakNet::BitStream bsSend;
	//pChatWindow->AddDebugMessage("Sent Exit: %u\n",byteVehicleID);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	CVehicle* pVehicle = pVehiclePool->GetAt(VehicleID);
	
	if(pVehicle)
	{ 
		if (!m_pPlayerPed->IsAPassenger()) {
			// This allows the code to sync vehicles blowing up without them being occupied
			m_LastVehicle = VehicleID;
		}

		if ( pVehicle->IsATrainPart() )	{
			pGame->GetCamera()->SetBehindPlayer();
		}

		if(!pVehicle->IsRCVehicle()) {
			bsSend.Write(VehicleID);
			pNetGame->GetRakClient()->RPC(&RPC_ExitVehicle,&bsSend,HIGH_PRIORITY,RELIABLE_SEQUENCED,0,false, UNASSIGNED_NETWORK_ID, NULL);
		}
	}
}

//----------------------------------------------------

void CLocalPlayer::SetPlayerColor(DWORD dwColor)
{
	SetRadarColor(pNetGame->GetPlayerPool()->GetLocalPlayerID(),dwColor);	
}

//----------------------------------------------------

DWORD CLocalPlayer::GetPlayerColorAsRGBA()
{
	return TranslateColorCodeToRGBA(pNetGame->GetPlayerPool()->GetLocalPlayerID());
}

//----------------------------------------------------

DWORD CLocalPlayer::GetPlayerColorAsARGB()
{
	return (TranslateColorCodeToRGBA(pNetGame->GetPlayerPool()->GetLocalPlayerID()) >> 8) | 0xFF000000;	
}

//----------------------------------------------------

void CLocalPlayer::ProcessOnFootWorldBounds()
{
	if(pGame->GetActiveInterior() != 0) return; // can't enforce inside interior

	if(m_pPlayerPed->EnforceWorldBoundries(pNetGame->m_WorldBounds[0],pNetGame->m_WorldBounds[1],
		pNetGame->m_WorldBounds[2],pNetGame->m_WorldBounds[3]))
	{
		m_pPlayerPed->SetArmedWeapon(0);
		pGame->DisplayGameText("Stay within the ~r~world boundries",1000,5);
	}
}

//----------------------------------------------------

void CLocalPlayer::ProcessInCarWorldBounds()
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	VEHICLEID VehicleID = (VEHICLEID)pVehiclePool->FindIDFromGtaPtr(m_pPlayerPed->GetGtaVehicle());
	CVehicle *pGameVehicle;

	if(pGame->GetActiveInterior() != 0) return; // can't enforce inside interior

	if(VehicleID != INVALID_VEHICLE_ID) {
		pGameVehicle = pVehiclePool->GetAt(VehicleID);
		if(!pGameVehicle) return;

		if( pGameVehicle->EnforceWorldBoundries(
			pNetGame->m_WorldBounds[0],pNetGame->m_WorldBounds[1],
			pNetGame->m_WorldBounds[2],pNetGame->m_WorldBounds[3]) )
		{
			pGame->DisplayGameText("Stay within the ~r~world boundries",1000,5);
		}
	}
}

//-----------------------------------------------------

void CLocalPlayer::SendStatsUpdate()
{
	RakNet::BitStream bsStats;
	int iMoney = pGame->GetLocalMoney();
	WORD wAmmo = m_pPlayerPed->GetAmmo();
	//ScriptCommand(&get_player_weapon_ammo, GetCurrentWeapon()

	bsStats.Write((BYTE)ID_STATS_UPDATE);
	bsStats.Write(iMoney);
	bsStats.Write(wAmmo);
	pNetGame->GetRakClient()->Send(&bsStats,HIGH_PRIORITY,UNRELIABLE,0);
}

//----------------------------------------------------------

void CLocalPlayer::ProcessClassSelection()
{
	DWORD			dwTicksSinceLastSelection;
	MATRIX4X4		matPlayer;
	char			szMsg[1024];
	char			szClassInfo[256];

	pGame->DisplayHud(FALSE);

	// DONT ALLOW ANY ACTIONS IF WE'RE STILL FADING OR WAITING.
	if((GetTickCount() - m_dwInitialSelectionTick) < 2000) return;

	//ApplySpawnAnim(m_iCurSpawnAnimIndex);

	if( !m_bWaitingForSpawnRequestReply &&
		m_bClearedToSpawn && 
		(GetAsyncKeyState(VK_SHIFT)&0x8000) &&
		!pCmdWindow->isEnabled() )
	{
		pSpawnScreen->SetSpawnText(NULL);
		RequestSpawn();
		m_bWaitingForSpawnRequestReply = TRUE;
		return;
	}
	else if(m_bClearedToSpawn) // WE ARE CLEARED TO SPAWN OR SELECT ANOTHER CLASS
	{
		// SHOW INFO ABOUT THE SELECTED CLASS..
		if(pChatWindow) {
			szMsg[0] = '\0';
			strcat(szMsg,"Use left and right arrow keys to select class.\n");
			strcat(szMsg,"Press SHIFT when ready to spawn.\n\n");
			
			sprintf(szClassInfo,"Class %u Weapons:\n- %s\n- %s\n- %s",m_iSelectedClass,
				pGame->GetWeaponName(m_SpawnInfo.iSpawnWeapons[0]),
				pGame->GetWeaponName(m_SpawnInfo.iSpawnWeapons[1]),
				pGame->GetWeaponName(m_SpawnInfo.iSpawnWeapons[2]));

			strcat(szMsg, szClassInfo);
		
			pSpawnScreen->SetSpawnText(szMsg);
		}

		// GRAB PLAYER MATRIX FOR SOUND POSITION
		m_pPlayerPed->GetMatrix(&matPlayer);
		dwTicksSinceLastSelection = GetTickCount() - m_dwLastSpawnSelectionTick; // used to delay reselection.

		// ALLOW ANOTHER SELECTION WITH THE LEFT KEY
		if((GetAsyncKeyState(VK_LEFT)&0x8000) && (dwTicksSinceLastSelection > 250)) { // LEFT ARROW
				
			m_bClearedToSpawn = FALSE;
			m_dwLastSpawnSelectionTick = GetTickCount();
				
			if(m_iSelectedClass == 0) m_iSelectedClass = (pNetGame->m_iSpawnsAvailable - 1);
			else m_iSelectedClass--;		
			
			pGame->PlaySound(1053,matPlayer.pos.X,matPlayer.pos.Y,matPlayer.pos.Z);
			RequestClass(m_iSelectedClass);
		}
		// ALLOW ANOTHER SELECTION WITH THE RIGHT KEY
		else if((GetAsyncKeyState(VK_RIGHT)&0x8000) && (dwTicksSinceLastSelection > 250)) { // RIGHT ARROW
			
			m_bClearedToSpawn = FALSE;
			m_dwLastSpawnSelectionTick = GetTickCount();
				
			if(m_iSelectedClass == (pNetGame->m_iSpawnsAvailable - 1)) m_iSelectedClass = 0;
			else m_iSelectedClass++;

			pGame->PlaySound(1052,matPlayer.pos.X,matPlayer.pos.Y,matPlayer.pos.Z);
			RequestClass(m_iSelectedClass);
		}
	}
}

//----------------------------------------------------------

void CLocalPlayer::HandleClassSelection()
{
	m_bClearedToSpawn = FALSE;
	if(m_pPlayerPed) {
		m_pPlayerPed->SetInitialState();
		m_pPlayerPed->SetHealth(100.0f);
		m_pPlayerPed->TogglePlayerControllable(0);
	}
	RequestClass(m_iSelectedClass);
	m_dwInitialSelectionTick = GetTickCount();
	m_dwLastSpawnSelectionTick = GetTickCount();
}

//----------------------------------------------------------

void CLocalPlayer::HandleClassSelectionOutcome(BOOL bOutcome)
{
	if(bOutcome) {
		if(m_pPlayerPed) {
			m_pPlayerPed->ClearAllWeapons();
			m_pPlayerPed->SetModelIndex(m_SpawnInfo.iSkin);
		}
		m_bClearedToSpawn = TRUE;
	}
}

//-----------------------------------------------------------
// Check all the players unarmed weapons to see if they've been changed and inform the server if they have

void CLocalPlayer::CheckWeapons()
{
	if (m_pPlayerPed->IsInVehicle()) return;
	BYTE i;
	BYTE byteCurWep = m_pPlayerPed->GetCurrentWeapon();
	BOOL bMSend = false;

	RakNet::BitStream bsWeapons;
	bsWeapons.Write((BYTE)ID_WEAPONS_UPDATE);

	for (i = 0; i < 13; i++)
	{
		if (m_byteLastWeapon[i] != byteCurWep)
		{
			//bsWeapons.Write(i);
			BOOL bSend = false;
			if (m_byteLastWeapon[i] != m_pPlayerPed->m_pPed->WeaponSlots[i].dwType)
			{
				// non-current weapon has changed
				m_byteLastWeapon[i] = (BYTE)m_pPlayerPed->m_pPed->WeaponSlots[i].dwType;
				bSend = true;
			}
			//bsWeapons.Write(m_byteLastWeapon[i]);
			if (m_dwLastAmmo[i] != m_pPlayerPed->m_pPed->WeaponSlots[i].dwAmmo)
			{
				// non-current ammo has changed
				m_dwLastAmmo[i] = m_pPlayerPed->m_pPed->WeaponSlots[i].dwAmmo;
				bSend = true;
			}
			//bsWeapons.Write(m_dwLastAmmo[i]);
			if (bSend)
			{
				//pChatWindow->AddDebugMessage("Id: %u, Weapon: %u, Ammo: %d\n", i, m_byteLastWeapon[i], m_dwLastAmmo[i]);
				bsWeapons.Write((BYTE)i);
				bsWeapons.Write((BYTE)m_byteLastWeapon[i]);
				bsWeapons.Write((WORD)m_dwLastAmmo[i]);
				bMSend = true;
			}
		}
	}
	if (bMSend)
	{
		pNetGame->GetRakClient()->Send(&bsWeapons,HIGH_PRIORITY,UNRELIABLE,0);
	}
}

//-----------------------------------------------------------

int iNumPlayersInRange=0;
int iCyclesUntilNextCount=0;

int CLocalPlayer::DetermineNumberOfPlayersInLocalRange()
{
	int iRet=0;
	PLAYERID x=0;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	// We only want to perform this operation
	// once every few cycles. Doing it every frame
	// would be a little bit too CPU intensive.

	if(iCyclesUntilNextCount) {
		iCyclesUntilNextCount--;
		return iNumPlayersInRange;
	}

	// This part is only processed when iCyclesUntilNextCount is 0
	iCyclesUntilNextCount = 30;
	iNumPlayersInRange = 0;

	if(pPlayerPool) {		
		while(x!=MAX_PLAYERS) {
			if(pPlayerPool->GetSlotState(x)) {
				if(pPlayerPool->GetAt(x)->IsActive()) {
					iNumPlayersInRange++;
				}
			}
			x++;
		}
	}

	return iNumPlayersInRange;
}

//-----------------------------------------------------------

void CLocalPlayer::ProcessSpectating()
{
	RakNet::BitStream bsSpectatorSync;
	SPECTATOR_SYNC_DATA spSync;
	MATRIX4X4 matPos;

	WORD lrAnalog,udAnalog;
	WORD wKeys = m_pPlayerPed->GetKeys(&lrAnalog,&udAnalog);
	pGame->GetCamera()->GetMatrix(&matPos);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(!pPlayerPool || !pVehiclePool) return;
	
	spSync.vecPos.X = matPos.pos.X;
	spSync.vecPos.Y = matPos.pos.Y;
	spSync.vecPos.Z = matPos.pos.Z;
	spSync.lrAnalog = lrAnalog;
	spSync.udAnalog = udAnalog;
	spSync.wKeys = wKeys;

	if((GetTickCount() - m_dwLastSendSpecTick) > 200) {
		m_dwLastSendSpecTick = GetTickCount();
		bsSpectatorSync.Write((BYTE)ID_SPECTATOR_SYNC);
		bsSpectatorSync.Write((PCHAR)&spSync,sizeof(SPECTATOR_SYNC_DATA));
		pNetGame->GetRakClient()->Send(&bsSpectatorSync,HIGH_PRIORITY,UNRELIABLE,0);
	}
	
	pGame->DisplayHud(FALSE);

	m_pPlayerPed->SetHealth(100.0f);
	GetPlayerPed()->TeleportTo(spSync.vecPos.X, spSync.vecPos.Y, spSync.vecPos.Z + 20.0f);
	
	// HANDLE SPECTATE PLAYER LEFT THE SERVER
	if (m_byteSpectateType == SPECTATE_TYPE_PLAYER &&
		!pPlayerPool->GetSlotState((BYTE)m_SpectateID)) {
			m_byteSpectateType = SPECTATE_TYPE_NONE;
			m_bSpectateProcessed = false;
	}

	// HANDLE SPECTATE PLAYER IS NO LONGER ACTIVE (ie Died)
	if (m_byteSpectateType == SPECTATE_TYPE_PLAYER &&
		pPlayerPool->GetSlotState((BYTE)m_SpectateID) &&
		(!pPlayerPool->GetAt((BYTE)m_SpectateID)->IsActive() ||
		pPlayerPool->GetAt((BYTE)m_SpectateID)->GetState() == PLAYER_STATE_WASTED)) {
			m_byteSpectateType = SPECTATE_TYPE_NONE;
			m_bSpectateProcessed = false;
	}

	if (m_bSpectateProcessed) return;

	if (m_byteSpectateType == SPECTATE_TYPE_NONE)
	{
		GetPlayerPed()->RemoveFromVehicleAndPutAt(0.0f, 0.0f, 10.0f);
		pGame->GetCamera()->SetPosition(50.0f, 50.0f, 50.0f, 0.0f, 0.0f, 0.0f);
		pGame->GetCamera()->LookAtPoint(60.0f, 60.0f, 50.0f, 2);
		m_bSpectateProcessed = TRUE;
	}
	else if (m_byteSpectateType == SPECTATE_TYPE_PLAYER)
	{
		DWORD dwGTAId = 0;
		CPlayerPed *pPlayerPed = NULL;

		if (pPlayerPool->GetSlotState((BYTE)m_SpectateID)) {
			pPlayerPed = pPlayerPool->GetAt((BYTE)m_SpectateID)->GetPlayerPed();
			if(pPlayerPed) {
				dwGTAId = pPlayerPed->m_dwGTAId;
				//pChatWindow->AddDebugMessage("Spectating Player: 0x%X", dwGTAId);
				ScriptCommand(&camera_on_actor, dwGTAId, m_byteSpectateMode, 2);
				m_bSpectateProcessed = TRUE;
			}
		}
	}
	else if (m_byteSpectateType == SPECTATE_TYPE_VEHICLE)
	{
		CVehicle *pVehicle = NULL;
		DWORD dwGTAId = 0;

		if (pVehiclePool->GetSlotState((VEHICLEID)m_SpectateID)) {
			pVehicle = pVehiclePool->GetAt((VEHICLEID)m_SpectateID);
			if(pVehicle) {
				dwGTAId = pVehicle->m_dwGTAId;
				//pChatWindow->AddDebugMessage("Spectating Vehicle: 0x%X", dwGTAId);
				ScriptCommand(&camera_on_vehicle, dwGTAId, m_byteSpectateMode, 2);
				m_bSpectateProcessed = TRUE;
			}
		}
	}
}

//-----------------------------------------------------------

void CLocalPlayer::ToggleSpectating(BOOL bToggle)
{
	if (m_bIsSpectating && !bToggle) {
		Spawn();
	}
	m_bIsSpectating = bToggle;
	m_byteSpectateType = SPECTATE_TYPE_NONE;
	m_SpectateID = 0xFFFFFFFF;
	m_bSpectateProcessed = FALSE;
}

//-----------------------------------------------------------

void CLocalPlayer::SpectateVehicle(VEHICLEID VehicleID)
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	//pChatWindow->AddDebugMessage("SpectateVehicle(%u)",VehicleID);

	if (pVehiclePool && pVehiclePool->GetSlotState(VehicleID)) {
		m_byteSpectateType = SPECTATE_TYPE_VEHICLE;
		m_SpectateID = VehicleID;
		m_bSpectateProcessed = FALSE;
	}
}

//-----------------------------------------------------------

void CLocalPlayer::SpectatePlayer(PLAYERID playerId)
{
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	//pChatWindow->AddDebugMessage("SpectatePlayer(%u)",playerId);

	if (pPlayerPool && pPlayerPool->GetSlotState(playerId)) {
		if (pPlayerPool->GetAt(playerId)->GetState() != PLAYER_STATE_NONE
			&& pPlayerPool->GetAt(playerId)->GetState() != PLAYER_STATE_WASTED) {
			m_byteSpectateType = SPECTATE_TYPE_PLAYER;
			m_SpectateID = playerId;
			m_bSpectateProcessed = FALSE;
		}
	}
}

//-----------------------------------------------------------

void CLocalPlayer::ProcessVehicleDamageUpdates(VEHICLEID CurrentVehicle)
{
	CVehicle *pVehicle = pNetGame->GetVehiclePool()->GetAt(CurrentVehicle);
	if(!pVehicle) return;

	// If this isn't the vehicle we were last monitoring for damage changes
	// update our stored data and return.
	if(CurrentVehicle != m_DamageVehicleUpdating) {
        m_dwLastPanelDamageStatus = pVehicle->GetPanelDamageStatus();
		m_dwLastDoorDamageStatus = pVehicle->GetDoorDamageStatus();
		m_byteLastLightsDamageStatus = pVehicle->GetLightDamageStatus();
		m_DamageVehicleUpdating = CurrentVehicle;
		return;
	}

	if( m_dwLastPanelDamageStatus != pVehicle->GetPanelDamageStatus() ||
		m_dwLastDoorDamageStatus != pVehicle->GetDoorDamageStatus() ||
		m_byteLastLightsDamageStatus != pVehicle->GetLightDamageStatus() ) 
	{			
			m_dwLastPanelDamageStatus = pVehicle->GetPanelDamageStatus();
			m_dwLastDoorDamageStatus = pVehicle->GetDoorDamageStatus();
			m_byteLastLightsDamageStatus = pVehicle->GetLightDamageStatus();

			// We need to update the server that the vehicle we're driving
			// has had its damage model modified.
			//pChatWindow->AddDebugMessage("Local::DamageModelChanged");
			RakNet::BitStream bsVehicleDamageUpdate;
			bsVehicleDamageUpdate.Write(m_DamageVehicleUpdating);
			bsVehicleDamageUpdate.Write(m_dwLastPanelDamageStatus);
			bsVehicleDamageUpdate.Write(m_dwLastDoorDamageStatus);
            bsVehicleDamageUpdate.Write(m_byteLastLightsDamageStatus);
			pNetGame->GetRakClient()->RPC(&RPC_DamageVehicle,&bsVehicleDamageUpdate,HIGH_PRIORITY,RELIABLE,0,false, UNASSIGNED_NETWORK_ID, NULL);
	}    
}

//-----------------------------------------------------------

void CLocalPlayer::SetWaypoint(float x, float y) {
	for (int i = 0; i<(0xAF * 0x28); i += 0x28) {
		if (*(short*)(0xBA873D + i) == 4611) {
			float* pos = (float*)(0xBA86F8 + 0x28 + i);
			*(pos + 1) = y;
			*pos = x;

			pChatWindow->AddDebugMessage("CLocalPlayer::SetWaypoint(playerid, %f, %f) | pos = 0x%X", *pos, *(pos + 1), pos);
		}
	}
}

//-----------------------------------------------------------