#include "../main.h"
#include "../game/util.h"

extern CGame		 *pGame;
extern CChatWindow   *pChatWindow;
extern CCmdWindow	 *pCmdWindow;

using namespace RakNet;
extern CNetGame* pNetGame;

//-----------------------------------------------------------

CRemoteActor::CRemoteActor()
{
	m_pActorPed = NULL;
	m_pCurrentVehicle = NULL;
	m_byteUpdateFromNetwork = UPDATE_TYPE_NONE;
	m_byteState = PLAYER_STATE_NONE;
	m_VehicleID = INVALID_VEHICLE_ID;
	m_dwWaitForEntryExitAnims = GetTickCount();
	m_byteSeatID = 0;
}

//-----------------------------------------------------------

CRemoteActor::~CRemoteActor()
{
	if(m_pActorPed) {
		m_pActorPed->Destroy();
		delete m_pActorPed;
		m_pActorPed = NULL;
	}
}

//-----------------------------------------------------------

BOOL CRemoteActor::Spawn(int ActorID, int iSkin, VECTOR * vecPos, float fRotation)
{
	if(!pGame->IsGameLoaded()) return FALSE;

	if(m_pActorPed != NULL) {
		m_pActorPed->Destroy();
		delete m_pActorPed;
	}

	CActorPed *pActor = pGame->NewActor(iSkin, vecPos->X, vecPos->Y, vecPos->Z, fRotation);

	CActorPool *pActorPool = pNetGame->GetActorPool();

	if(pActorPool && pActor) 
	{
		pActorPool->m_pGTAPed[ActorID] = pActor->m_pPed;
		m_pActorPed = pActor;
		m_fReportedHealth = 100.0;
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------

void CRemoteActor::Process()
{
	if(m_pActorPed)
	{
		HandleActorPedStreaming();
			
		if(GetTickCount() > m_dwWaitForEntryExitAnims) {
			HandleVehicleEntryExit();
		}
	}
}

//-----------------------------------------------------------

void CRemoteActor::EnterVehicle(VEHICLEID VehicleID, int iSeat)
{
	CVehicle *pVehicle = pNetGame->GetVehiclePool()->GetAt(VehicleID);

	if (pVehicle && m_pActorPed) {
		m_pActorPed->EnterVehicle(pVehicle->m_dwGTAId,iSeat);
		m_VehicleID = VehicleID;
		m_byteSeatID = iSeat;
		SetState(PLAYER_STATE_DRIVER);
		m_dwWaitForEntryExitAnims = GetTickCount() + 3000;
		m_pCurrentVehicle = pVehicle;
	}
}

//-----------------------------------------------------------

void CRemoteActor::ExitVehicle()
{
	if (m_pCurrentVehicle && m_pActorPed) {
		m_pActorPed->ExitCurrentVehicle();
		m_VehicleID = INVALID_VEHICLE_ID;
		SetState(PLAYER_STATE_ONFOOT);
		m_dwWaitForEntryExitAnims = GetTickCount() + 2000;
	}
}

//-----------------------------------------------------------

void CRemoteActor::MoveTo(VECTOR vecPos, int iMoveType)
{


}

//-----------------------------------------------------------

void CRemoteActor::HandleVehicleEntryExit()
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	MATRIX4X4 mat;

	if(!m_pActorPed) return;

	if(GetState() == PLAYER_STATE_ONFOOT) {
		if(m_pActorPed->IsInVehicle()) {
			m_pActorPed->GetMatrix(&mat);
			m_pActorPed->RemoveFromVehicleAndPutAt(mat.pos.X,mat.pos.Y,mat.pos.Z);
		}
		return;
	}

	if( GetState() == PLAYER_STATE_DRIVER || GetState() == PLAYER_STATE_PASSENGER )
	{
		if(!m_pActorPed->IsInVehicle()) {
			CVehicle *pVehicle = pVehiclePool->GetAt(m_VehicleID);
			if(pVehicle) {
				int iCarID = pVehiclePool->FindGtaIDFromID(m_VehicleID);
				m_pActorPed->PutDirectlyInVehicle(iCarID,m_byteSeatID);
			}
		}
	}
}

//-----------------------------------------------------------

void CRemoteActor::HandleActorPedStreaming()
{
	if (m_pActorPed)
	{
		if(m_pActorPed->GetDistanceFromLocalPlayerPed() < LOCKING_DISTANCE) {
			m_pActorPed->Add();

		} else {
			m_pActorPed->Remove();
		}
	}
}

//-----------------------------------------------------------