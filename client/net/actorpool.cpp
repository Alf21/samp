#include "../main.h"
#include "../game/util.h"

extern CNetGame *pNetGame;
extern CGame *pGame;
extern CChatWindow *pChatWindow;
//----------------------------------------------------

CActorPool::CActorPool()
{
	for(ACTORID ActorID = 0; ActorID < MAX_ACTORS; ActorID++) {
		m_bActorSlotState[ActorID] = FALSE;
		m_pActors[ActorID] = NULL;
		m_pGTAPed[ActorID] = NULL;
	}
}

//----------------------------------------------------

CActorPool::~CActorPool()
{
	for(ACTORID ActorID = 0; ActorID < MAX_ACTORS; ActorID++) {
		Delete(ActorID);
	}
}

//----------------------------------------------------

BOOL CActorPool::New( ACTORID ActorID, int iSkin, VECTOR * vecPos, float fRotation, PCHAR szName )
{
	memset(&m_SpawnInfo[ActorID],0,sizeof(ACTOR_SPAWN_INFO));

	m_SpawnInfo[ActorID].iSkin = iSkin;
	m_SpawnInfo[ActorID].vecPos.X = vecPos->X;
	m_SpawnInfo[ActorID].vecPos.Y = vecPos->Y;
	m_SpawnInfo[ActorID].vecPos.Z = vecPos->Z;
	m_SpawnInfo[ActorID].fRotation = fRotation;

	strcpy(m_szActorNames[ActorID], szName);

	return Spawn(ActorID);
}

//----------------------------------------------------

BOOL CActorPool::Delete(ACTORID ActorID)
{
	if(!GetSlotState(ActorID) || !m_pActors[ActorID])
	{
		return FALSE;
	}

	m_bActorSlotState[ActorID] = FALSE;
	delete m_pActors[ActorID];
	m_pActors[ActorID] = NULL;

	return TRUE;
}

//----------------------------------------------------

BOOL CActorPool::Spawn(ACTORID ActorID)
{	
	if(m_pActors[ActorID] != NULL) {
		Delete(ActorID);
	}

	/*m_pActors[ActorID] = pGame->NewActor(
		m_SpawnInfo[ActorID].iSkin,m_SpawnInfo[ActorID].vecPos.X,
		m_SpawnInfo[ActorID].vecPos.Y,m_SpawnInfo[ActorID].vecPos.Z,
		m_SpawnInfo[ActorID].fRotation);*/

	m_pActors[ActorID] = new CRemoteActor();
    
	if(m_pActors[ActorID])
	{	
		m_bActorSlotState[ActorID] = TRUE;
		return m_pActors[ActorID]->Spawn(ActorID, m_SpawnInfo[ActorID].iSkin, &m_SpawnInfo[ActorID].vecPos, m_SpawnInfo[ActorID].fRotation);
	}
	else
	{
		return FALSE;
	}
}

//----------------------------------------------------

ACTORID CActorPool::FindIDFromGtaPtr(PED_TYPE * pGtaPed)
{
	int x=1;
	
	while(x!=MAX_ACTORS) {
		if(pGtaPed == m_pGTAPed[x]) return x;
		x++;
	}

	return INVALID_ACTOR_ID;
}

//----------------------------------------------------

int CActorPool::FindGtaIDFromID(int iID)
{
	return GamePool_Ped_GetIndex(m_pGTAPed[iID]);
}

//----------------------------------------------------

int CActorPool::FindGtaIDFromGtaPtr(PED_TYPE * pGtaPed)
{
	return GamePool_Ped_GetIndex(pGtaPed);
}

//----------------------------------------------------

void CActorPool::Process()
{
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *m_pLocalPlayer = NULL;

	if (pPlayerPool) m_pLocalPlayer = pPlayerPool->GetLocalPlayer();

	for(BYTE bytePlayerID = 0; bytePlayerID < MAX_ACTORS; bytePlayerID++) {
		if(TRUE == m_bActorSlotState[bytePlayerID]) {
			try {
				m_pActors[bytePlayerID]->Process();
			} catch(...) {
				pChatWindow->AddDebugMessage("Warning: Error Processing Actor(%u)",bytePlayerID);
			}
		}
	}
}

//----------------------------------------------------

int CActorPool::FindNearestToLocalPlayerPed()
{
	float fLeastDistance=10000.0f;
	float fThisDistance;
	ACTORID ClosestSoFar=INVALID_ACTOR_ID;

	ACTORID x=0;
	while(x < MAX_ACTORS) {
		if(GetSlotState(x)) {
			fThisDistance = m_pActors[x]->GetAtPed()->GetDistanceFromLocalPlayerPed();
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