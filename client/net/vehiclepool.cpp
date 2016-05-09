#include "../main.h"
#include "../game/util.h"

extern CNetGame *pNetGame;
extern CGame *pGame;
extern CChatWindow *pChatWindow;
//----------------------------------------------------

CVehiclePool::CVehiclePool()
{
	// loop through and initialize all vehicle properties to 0
	for(VEHICLEID VehicleID = 0; VehicleID < MAX_VEHICLES; VehicleID++) {
		m_bVehicleSlotState[VehicleID] = FALSE;
		m_pVehicles[VehicleID] = NULL;
		m_pGTAVehicles[VehicleID] = NULL;
	}
	memset(&m_bWaitingSlotState[0],0,MAX_VEHICLE_WAITING_SLOTS * sizeof(BOOL));
}

//----------------------------------------------------

CVehiclePool::~CVehiclePool()
{
	for(VEHICLEID VehicleID = 0; VehicleID < MAX_VEHICLES; VehicleID++) {
		Delete(VehicleID);
	}
}

//----------------------------------------------------

BOOL CVehiclePool::New(NEW_VEHICLE *pNewVehicle)
{
	if(m_pVehicles[pNewVehicle->VehicleId] != NULL) {
		pChatWindow->AddDebugMessage("Warning: vehicle %u was not deleted",pNewVehicle->VehicleId);
		Delete(pNewVehicle->VehicleId);
	}

	m_pVehicles[pNewVehicle->VehicleId] = pGame->NewVehicle(pNewVehicle->iVehicleType,
		pNewVehicle->vecPos.X,pNewVehicle->vecPos.Y,pNewVehicle->vecPos.Z,
		pNewVehicle->fRotation, NULL);

	if(m_pVehicles[pNewVehicle->VehicleId])
	{	
		if(pNewVehicle->aColor1 != -1 || pNewVehicle->aColor2 != -1) {
			m_pVehicles[pNewVehicle->VehicleId]->SetColor( 
					pNewVehicle->aColor1,pNewVehicle->aColor2 );
		}

		m_pVehicles[pNewVehicle->VehicleId]->SetHealth(pNewVehicle->fHealth);
        
		m_pGTAVehicles[pNewVehicle->VehicleId] = m_pVehicles[pNewVehicle->VehicleId]->m_pVehicle;
		m_bVehicleSlotState[pNewVehicle->VehicleId] = TRUE;

		m_vecSpawnPos[pNewVehicle->VehicleId].X = pNewVehicle->vecPos.X;
		m_vecSpawnPos[pNewVehicle->VehicleId].Y = pNewVehicle->vecPos.Y;
		m_vecSpawnPos[pNewVehicle->VehicleId].Z = pNewVehicle->vecPos.Z;

		if(pNewVehicle->byteDoorsLocked) 
			m_pVehicles[pNewVehicle->VehicleId]->SetDoorState(1);

		if (pNewVehicle->byteInterior > 0)	{
			LinkToInterior(pNewVehicle->VehicleId, pNewVehicle->byteInterior);
		}

		m_pVehicles[pNewVehicle->VehicleId]->UpdateDamageStatus(
			pNewVehicle->dwPanelDamageStatus,
			pNewVehicle->dwDoorDamageStatus,
			pNewVehicle->byteLightDamageStatus);

		m_bIsActive[pNewVehicle->VehicleId] = TRUE;
		m_bIsWasted[pNewVehicle->VehicleId] = FALSE;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//----------------------------------------------------

BOOL CVehiclePool::Delete(VEHICLEID VehicleID)
{
	if(!GetSlotState(VehicleID) || !m_pVehicles[VehicleID])
	{
		return FALSE; // Vehicle already deleted or not used.
	}

	m_bVehicleSlotState[VehicleID] = FALSE;
	delete m_pVehicles[VehicleID];
	m_pVehicles[VehicleID] = NULL;
	m_pGTAVehicles[VehicleID] = 0;

	return TRUE;
}

//----------------------------------------------------

void CVehiclePool::LinkToInterior(VEHICLEID VehicleID, int iInterior)
{
	if(m_bVehicleSlotState[VehicleID]) {
		m_pVehicles[VehicleID]->LinkToInterior(iInterior);
	}
}

//----------------------------------------------------

void CVehiclePool::AssignSpecialParamsToVehicle(VEHICLEID VehicleID, BYTE byteObjective, BYTE byteDoorsLocked)
{
	if(!GetSlotState(VehicleID)) return;
	CVehicle *pVehicle = m_pVehicles[VehicleID];

	if(pVehicle && m_bIsActive[VehicleID]) {
		if (byteObjective)
		{
			pVehicle->m_byteObjectiveVehicle = 1;
			pVehicle->m_bSpecialMarkerEnabled = false;
		}
		pVehicle->SetDoorState(byteDoorsLocked);
	}
}

//----------------------------------------------------

VEHICLEID CVehiclePool::FindIDFromGtaPtr(VEHICLE_TYPE * pGtaVehicle)
{
	int x=1;
	
	while(x!=MAX_VEHICLES) {
		if(pGtaVehicle == m_pGTAVehicles[x]) return x;
		x++;
	}

	return INVALID_VEHICLE_ID;
}

//----------------------------------------------------

int CVehiclePool::FindGtaIDFromID(int iID)
{
	if(m_pGTAVehicles[iID]) {
		return GamePool_Vehicle_GetIndex(m_pGTAVehicles[iID]);
	} else {
		return INVALID_VEHICLE_ID;
	}
}

//----------------------------------------------------

int CVehiclePool::FindGtaIDFromGtaPtr(VEHICLE_TYPE * pGtaVehicle)
{
	if(pGtaVehicle) {
		return GamePool_Vehicle_GetIndex(pGtaVehicle);
	} else {
		return INVALID_VEHICLE_ID;
	}
}

//-----------------------------------------------------------

void CVehiclePool::Process()
{
	// Process all vehicles in the vehicle pool.
	CVehicle *pVehicle;
	DWORD dwThisTime = GetTickCount();
	
	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();

	int iLocalPlayersInRange = 0;
	if (pLocalPlayer)
		iLocalPlayersInRange = pLocalPlayer->DetermineNumberOfPlayersInLocalRange();

	ProcessWaitingList();

	for(VEHICLEID x = 0; x != MAX_VEHICLES; x++)
	{
		if(GetSlotState(x) == TRUE)
		{
			// It's in use.
			pVehicle = m_pVehicles[x];

			if(m_bIsActive[x])
			{
				if(pVehicle->IsDriverLocalPlayer()) {
					pVehicle->SetInvulnerable(FALSE);
				} else {
					pVehicle->SetInvulnerable(TRUE);
				}

				if (pVehicle->GetHealth() == 0.0f) // || pVehicle->IsWrecked()) // It's dead
				{
					if (pLocalPlayer->m_LastVehicle == x) // Notify server of death
					{
						NotifyVehicleDeath(x);
					}
					continue;
				}
	
				if( pVehicle->GetVehicleSubtype() != VEHICLE_SUBTYPE_BOAT &&
					pVehicle->HasSunk() ) // Not boat and has sunk.
				{
					if (pLocalPlayer->m_LastVehicle == x) {
						NotifyVehicleDeath(x);
					}
					continue;
				}
									
				// Active and in world.
				pLocalPlayer->ProcessUndrivenSync(x, pVehicle, iLocalPlayersInRange); // sync unoccupied vehicle if needed

				/* This has an impact on undriven passenger sync. check for driver instead 
				// Kye: This the source of the audio bug?
				if(!pVehicle->IsOccupied()) {
					pVehicle->SetEngineState(FALSE);
				} else {
					pVehicle->SetEngineState(TRUE);
				}*/

				// Update the actual ingame pointer if it's not the same as the one we have listed.
				if(pVehicle->m_pVehicle != m_pGTAVehicles[x]) {
					m_pGTAVehicles[x] = pVehicle->m_pVehicle;
				}

				pVehicle->ProcessMarkers();
			}
		}
	}
}


//----------------------------------------------------

void CVehiclePool::ProcessUnoccupiedSync(VEHICLEID VehicleID, CVehicle *pVehicle)
{

}

//----------------------------------------------------

void CVehiclePool::NotifyVehicleDeath(VEHICLEID VehicleID)
{
	RakNet::BitStream bsDeath;
	bsDeath.Write(VehicleID);
	pNetGame->GetRakClient()->RPC(&RPC_VehicleDestroyed, &bsDeath, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
	pNetGame->GetPlayerPool()->GetLocalPlayer()->m_LastVehicle = 0xFFFF; // Mark as notification sent
}

//----------------------------------------------------

int CVehiclePool::FindNearestToLocalPlayerPed()
{
	float fLeastDistance=10000.0f;
	float fThisDistance;
	VEHICLEID ClosestSoFar=INVALID_VEHICLE_ID;

	VEHICLEID x=0;
	while(x < MAX_VEHICLES) {
		if(GetSlotState(x) && m_bIsActive[x]) {
			fThisDistance = m_pVehicles[x]->GetDistanceFromLocalPlayerPed();
			if(fThisDistance < fLeastDistance) {
				fLeastDistance = fThisDistance;
				ClosestSoFar = x;
			}
		}
		x++;
	}

	return ClosestSoFar;
}

//----------------------------------------------------

void CVehiclePool::ProcessWaitingList()
{
	int x=0;
	while(x!=MAX_VEHICLE_WAITING_SLOTS) {
		if(m_bWaitingSlotState[x] && pGame->IsModelLoaded(m_NewVehicleWaiting[x].iVehicleType))
		{		
			New(&m_NewVehicleWaiting[x]);
    
			// TRAIN STUFF
			if(m_NewVehicleWaiting[x].iVehicleType == TRAIN_FREIGHT_LOCO) {
				m_NewVehicleWaiting[x].iVehicleType = TRAIN_FREIGHT;

				m_NewVehicleWaiting[x].VehicleId++;
				New(&m_NewVehicleWaiting[x]);

				m_NewVehicleWaiting[x].VehicleId++;
				New(&m_NewVehicleWaiting[x]);

				m_NewVehicleWaiting[x].VehicleId++;
				New(&m_NewVehicleWaiting[x]);	
			}

			if(m_NewVehicleWaiting[x].iVehicleType == TRAIN_PASSENGER_LOCO) {
				m_NewVehicleWaiting[x].iVehicleType = TRAIN_PASSENGER;

				m_NewVehicleWaiting[x].VehicleId++;
				New(&m_NewVehicleWaiting[x]);

				m_NewVehicleWaiting[x].VehicleId++;
				New(&m_NewVehicleWaiting[x]);

				m_NewVehicleWaiting[x].VehicleId++;
				New(&m_NewVehicleWaiting[x]);	
			}
			m_bWaitingSlotState[x] = FALSE;
		}
		x++;
	}
}

//----------------------------------------------------

void CVehiclePool::NewWhenModelLoaded(NEW_VEHICLE *pNewVehicle)
{
	// find a free slot
	int x=0;
	while(x!=MAX_VEHICLE_WAITING_SLOTS) {
		if(m_bWaitingSlotState[x] == FALSE) break;
		x++;
	}

	if(x==MAX_VEHICLE_WAITING_SLOTS) {
		pChatWindow->AddDebugMessage("All vehicle waiting slots are consumed!");
		return;
	}

    m_bWaitingSlotState[x] = TRUE;
	memcpy(&m_NewVehicleWaiting[x],pNewVehicle,sizeof(NEW_VEHICLE));
}

//----------------------------------------------------