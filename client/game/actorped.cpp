#include <windows.h>
#include <assert.h>
#define _ASSERT assert

#include "../main.h"
#include "game.h"
#include "util.h"
#include "task.h"

extern CGame *pGame;
extern CChatWindow *pChatWindow;
extern CNetGame *pNetGame;

//-----------------------------------------------------------
// This is the constructor for creating new player.

CActorPed::CActorPed(int iSkin, float fX, float fY,float fZ,float fRotation)
{
	m_pPed=0;
	m_dwGTAId=0;
	DWORD dwActorID=0;

	pGame->RequestModel(iSkin);
	pGame->LoadRequestedModels();
	while(!pGame->IsModelLoaded(iSkin)) Sleep(5);

	ScriptCommand(&create_actor,5,iSkin,fX,fY,fZ+0.5f,&dwActorID);
	ScriptCommand(&set_actor_z_angle,dwActorID,fRotation);

	m_dwGTAId = dwActorID;
	m_pPed = GamePool_Ped_GetAt(m_dwGTAId);
	m_pEntity = (ENTITY_TYPE *)m_pPed;
	ScriptCommand(&set_actor_immunities,m_dwGTAId,1,1,1,1,1);	
	ScriptCommand(&set_actor_dicision,m_dwGTAId,65542);

	//m_pPed->dwPlayerInfoOffset = GamePool_FindPlayerPed()->dwPlayerInfoOffset;
	//m_pPed->dwPedType = 5;
}

//-----------------------------------------------------------

CActorPed::~CActorPed()
{
	Destroy();
}

//-----------------------------------------------------------

void CActorPed::Destroy()
{
	DWORD dwPedPtr = (DWORD)m_pPed;

	// If it points to the CPlaceable vtable it's not valid
	if(!m_pPed || !GamePool_Ped_GetAt(m_dwGTAId) || m_pPed->entity.vtable == 0x863C40)
	{
		m_pPed = NULL;
		m_pEntity = NULL;
		m_dwGTAId = 0;
		return;
	}

	if(IN_VEHICLE(m_pPed)) {
		RemoveFromVehicleAndPutAt(100.0f,100.0f,10.0f);
	}

	// DESTROY METHOD
	_asm mov ecx, dwPedPtr
	_asm mov ebx, [ecx] ; vtable
	_asm push 1
	_asm call [ebx] ; destroy

	m_pPed = NULL;
	m_pEntity = NULL;
}

//-----------------------------------------------------------
// If the game has internally destroyed the ped
// during this frame, the ped pointer should become 0

void CActorPed::ResetPointers()
{
	m_pPed = GamePool_Ped_GetAt(m_dwGTAId);
	m_pEntity = (ENTITY_TYPE *)m_pPed;
}

//-----------------------------------------------------------

// Shows the normal marker
void CActorPed::ShowMarker() //int iMarkerColor)
{	
	if (m_dwArrow) HideMarker();
	ScriptCommand(&create_arrow_above_actor, m_dwGTAId, &m_dwArrow);
	ScriptCommand(&set_marker_color, m_dwArrow, 0);
	ScriptCommand(&show_on_radar2, m_dwArrow, 2);
}

//-----------------------------------------------------------

void CActorPed::HideMarker()
{	
	if (m_dwArrow) ScriptCommand(&disable_marker, m_dwArrow);
	m_dwArrow = NULL; // Just make sure
}

//-----------------------------------------------------------

BOOL CActorPed::IsOnScreen()
{
	if(m_pPed)	return GameIsEntityOnScreen((DWORD *)m_pPed);
	return FALSE;
}

//-----------------------------------------------------------

float CActorPed::GetHealth()
{	
	if(!m_pPed) return 0.0f;
	return m_pPed->fHealth;
}

//-----------------------------------------------------------

void CActorPed::SetHealth(float fHealth)
{	
	if(!m_pPed) return;
	m_pPed->fHealth = fHealth;
}	

//-----------------------------------------------------------

float CActorPed::GetArmour()
{
	if(!m_pPed) return 0.0f;
	return m_pPed->fArmour;
}

//-----------------------------------------------------------

void CActorPed::SetArmour(float fArmour)
{
	if(!m_pPed) return;
	m_pPed->fArmour = fArmour;
}	

//-----------------------------------------------------------

DWORD CActorPed::GetStateFlags()
{	
	if(!m_pPed) return 0;
	return m_pPed->dwStateFlags;
}

//-----------------------------------------------------------

void CActorPed::SetStateFlags(DWORD dwState)
{	
	if(!m_pPed) return;
	m_pPed->dwStateFlags = dwState;
}	

//-----------------------------------------------------------

BOOL CActorPed::IsDead()
{
	if(!m_pPed) return TRUE;
	if(m_pPed->fHealth > 0.0f) return FALSE;
	return TRUE;
}

//-----------------------------------------------------------

BYTE CActorPed::GetActionTrigger()
{
	return (BYTE)m_pPed->dwAction;
}

//-----------------------------------------------------------

void CActorPed::SetActionTrigger(BYTE byteTrigger)
{
	if(!m_pPed) return;

	m_pPed->dwAction = byteTrigger;
}

//-----------------------------------------------------------

BOOL CActorPed::IsInVehicle()
{
	if(!m_pPed) return FALSE;

	if(IN_VEHICLE(m_pPed)) {
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------

float CActorPed::GetTargetRotation()
{
	float fRotation;
	ScriptCommand(&get_actor_z_angle,m_dwGTAId,&fRotation);
	return fRotation;

	//return m_pPed->fRotation2;
}

//-----------------------------------------------------------

void CActorPed::SetTargetRotation(float fRotation)
{	
	//pChatWindow->AddDebugMessage("deg: %f rad: %f", fRotation, DegToRad(fRotation));
	
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	m_pPed->fRotation2 = DegToRad(fRotation);

	if(!IsOnGround()) {
		m_pPed->fRotation1 = DegToRad(fRotation);
		ScriptCommand(&set_actor_z_angle,m_dwGTAId,fRotation);
	}
}

//-----------------------------------------------------------

void CActorPed::ForceTargetRotation(float fRotation)
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	m_pPed->fRotation1 = DegToRad(fRotation);
	m_pPed->fRotation2 = DegToRad(fRotation);

	ScriptCommand(&set_actor_z_angle,m_dwGTAId,fRotation);    
}

//-----------------------------------------------------------

BOOL CActorPed::IsAPassenger()
{
	if( m_pPed->pVehicle && IN_VEHICLE(m_pPed) )
	{
		VEHICLE_TYPE * pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle;

		if( pVehicle->pDriver != m_pPed || 
			pVehicle->entity.nModelIndex == TRAIN_PASSENGER ||
			pVehicle->entity.nModelIndex == TRAIN_FREIGHT ) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}

	return FALSE;
}

//-----------------------------------------------------------

VEHICLE_TYPE * CActorPed::GetGtaVehicle()
{
	return (VEHICLE_TYPE *)m_pPed->pVehicle;
}

//-----------------------------------------------------------

void CActorPed::GiveWeapon(int iWeaponID, int iAmmo)
{
	int iModelID = 0;
	iModelID = GameGetWeaponModelIDFromWeaponID(iWeaponID);

	if(iModelID == -1) return;

	if(!pGame->IsModelLoaded(iModelID)) {
		pGame->RequestModel(iModelID);
		pGame->LoadRequestedModels();
		while(!pGame->IsModelLoaded(iModelID)) Sleep(5);
	}

	ClearAllWeapons();
	ScriptCommand(&give_actor_weapon,m_dwGTAId,iWeaponID,iAmmo);
	
	pGame->RemoveModel(iModelID);
}

//-----------------------------------------------------------

void CActorPed::ClearAllWeapons()
{
	DWORD dwPedPtr = (DWORD)m_pPed;

	if(dwPedPtr)
	{
		_asm mov ecx, dwPedPtr
		_asm mov eax, 0x5E6320
		_asm call eax
	}
}

//-----------------------------------------------------------

int CActorPed::GetVehicleSeatID()
{
	VEHICLE_TYPE *pVehicle;

	if( GetActionTrigger() == ACTION_INCAR && (pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle) != 0 ) {
		if(pVehicle->pDriver == m_pPed) return 0;
		if(pVehicle->pPassengers[0] == m_pPed) return 1;
		if(pVehicle->pPassengers[1] == m_pPed) return 2;
		if(pVehicle->pPassengers[2] == m_pPed) return 3;
		if(pVehicle->pPassengers[3] == m_pPed) return 4;
		if(pVehicle->pPassengers[4] == m_pPed) return 5;
		if(pVehicle->pPassengers[5] == m_pPed) return 6;
		if(pVehicle->pPassengers[6] == m_pPed) return 7;
	}

	return (-1);
}

//-----------------------------------------------------------

void CActorPed::PutDirectlyInVehicle(int iVehicleID, int iSeat)
{
	if(!m_pPed) return;
	if(!GamePool_Vehicle_GetAt(iVehicleID)) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	VEHICLE_TYPE *pVehicle = GamePool_Vehicle_GetAt(iVehicleID);

	if(pVehicle->fHealth == 0.0f) return;

	// Check to make sure internal data structure of the vehicle hasn't been deleted
	// by checking if the vtbl points to CPlaceable_vtbl
	if (pVehicle->entity.vtable == 0x863C40) return;

	if(iSeat==0) {
		if(pVehicle->pDriver && IN_VEHICLE(pVehicle->pDriver)) return;
		ScriptCommand(&put_actor_in_car,m_dwGTAId,iVehicleID);
	} else {
		iSeat--;
		ScriptCommand(&put_actor_in_car2,m_dwGTAId,iVehicleID,iSeat);
	}	
}

//-----------------------------------------------------------

void CActorPed::EnterVehicle(int iVehicleID, BOOL bPassenger)
{
	if(!m_pPed) return;
	if(!GamePool_Vehicle_GetAt(iVehicleID)) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	if(bPassenger) {
		//pChatWindow->AddDebugMessage("Passenger: %u 0x%X",m_bytePlayerNumber,iVehicleID);
		ScriptCommand(&send_actor_to_car_passenger,m_dwGTAId,iVehicleID,3000,-1);
	} else {
		//pChatWindow->AddDebugMessage("Driver: %u 0x%X",m_bytePlayerNumber,iVehicleID);
		ScriptCommand(&send_actor_to_car_driverseat,m_dwGTAId,iVehicleID,3000);
	}				
}

//-----------------------------------------------------------

void CActorPed::KillPlayer(PLAYERID PlayerID, int iWeapon)
{
	CActorPool*  pActorPool = pNetGame->GetActorPool();
	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	CPlayerPed*  pPed = NULL;
	CTaskKillPedOnFootArmed* pKill = NULL;

	if ( PlayerID == pPlayerPool->GetLocalPlayerID() )
	{
		pPed = pGame->FindPlayerPed();	
	} else {
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(PlayerID);
		if (pPlayer) {
			pPed = pPlayer->GetPlayerPed();
		}
	}
		
	if (pPed) {
		pKill = new CTaskKillPedOnFootArmed( 1, 0, 0, 0, pPed->m_pPed );
		GiveWeapon( iWeapon, 9999 );
		pKill->ApplyToPed( this );
	}
}

//-----------------------------------------------------------

void CActorPed::MoveTo(VECTOR vecPos, int iMoveType)
{
	if (iMoveType != MOVETO_DIRECT) { // we use the task
		CTaskGoToPoint* pMoveTo = new CTaskGoToPoint( iMoveType, &vecPos, 1.0, 0, 0 );
		pMoveTo->ApplyToPed( this );
		//pChatWindow->AddDebugMessage("CTaskGoToPoint(%i, %f, %f, %f, 1.0, 0, 0);", iMoveType, vecPos.X, vecPos.Y, vecPos.Z);
	} else { // teleport directly
		MATRIX4X4 mat;
		GetMatrix(&mat);
		mat.pos.X = vecPos.X;
		mat.pos.Y = vecPos.Y;
		mat.pos.Z = vecPos.Z;
		SetMatrix(mat);
	}
}

//-----------------------------------------------------------

int CActorPed::GetCurrentVehicleID()
{
	if(!m_pPed) return 0;

	VEHICLE_TYPE *pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle;
	return GamePool_Vehicle_GetIndex(pVehicle);
}

//-----------------------------------------------------------
// Graceful vehicle exit.

void CActorPed::ExitCurrentVehicle()
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(IN_VEHICLE(m_pPed)) {
		ScriptCommand(&make_actor_leave_car,m_dwGTAId,GetCurrentVehicleID());
	}
}

//-----------------------------------------------------------
// Forceful removal

void CActorPed::RemoveFromVehicleAndPutAt(float fX, float fY, float fZ)
{
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(m_pPed && IN_VEHICLE(m_pPed)) {
		ScriptCommand(&remove_actor_from_car_and_put_at,m_dwGTAId,fX,fY,fZ);
	}
}

//-----------------------------------------------------------

void CActorPed::DriveVehicleToPoint(int iGtaVehicleID, VECTOR *vecPoint, float fMaxSpeed, int iDriveStyle)
{
	VEHICLE_TYPE *pVehicle = GamePool_Vehicle_GetAt(iGtaVehicleID);
    if(!pVehicle) return;

    ScriptCommand(&drive_car_to_point,m_dwGTAId,iGtaVehicleID,
		vecPoint->X,vecPoint->Y,vecPoint->Z,fMaxSpeed,0,0,iDriveStyle);
}

//-----------------------------------------------------------

BOOL CActorPed::HasHandsUp()
{
	if(!m_pPed || IN_VEHICLE(m_pPed)) return FALSE;
	if(!IsAdded()) return FALSE;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return FALSE;
	if(m_pPed->Tasks->pdwJumpJetPack == NULL) return FALSE;
	DWORD dwJmpVtbl = m_pPed->Tasks->pdwJumpJetPack[0];
	if(dwJmpVtbl == 0x85A29C) return TRUE;
	
	return FALSE;
}

//-----------------------------------------------------------

void CActorPed::HoldItem(int iObject)
{
	if(!m_pPed) return;
	if(!IsAdded()) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	DWORD dwPed = (DWORD)m_pPed;
	_asm push 1
	_asm push iObject
	_asm mov ecx, dwPed
	_asm mov ebx, 0x5E4390
	_asm call ebx

}

//-----------------------------------------------------------

BOOL CActorPed::StartPassengerDriveByMode()
{
	return FALSE;
}

//-----------------------------------------------------------

void CActorPed::SetCollisionChecking(int iCheck)
{
    if(m_pPed) ScriptCommand(&actor_set_collision,m_dwGTAId,iCheck);
}


//-----------------------------------------------------------

void CActorPed::SetDead()
{
	if(m_dwGTAId && m_pPed) {
		MATRIX4X4 mat;
		GetMatrix(&mat);

		// will reset the tasks
		TeleportTo(mat.pos.X,mat.pos.Y,mat.pos.Z);
		ScriptCommand(&kill_actor,m_dwGTAId);
	}
}

//-----------------------------------------------------------

void CActorPed::SetAmmo(BYTE byteWeapon, WORD wordAmmo)
{
	if(m_pPed)
	{
		//WEAPON_SLOT_TYPE * WeaponSlot = GetCurrentWeaponSlot();
		WEAPON_SLOT_TYPE * WeaponSlot = FindWeaponSlot((DWORD)byteWeapon);
		if(!WeaponSlot) return;
		WeaponSlot->dwAmmo = (DWORD)wordAmmo;
		//WeaponSlot->dwAmmoInClip = 0;
	}
}

//-----------------------------------------------------------

WEAPON_SLOT_TYPE * CActorPed::FindWeaponSlot(DWORD dwWeapon)
{
	if (m_pPed)
	{
		BYTE i;
		for (i = 0; i < 13; i++)
		{
			if (m_pPed->WeaponSlots[i].dwType == dwWeapon) return &m_pPed->WeaponSlots[i];
		}
	}
	return NULL;
}

//-----------------------------------------------------------

void CActorPed::SetAnimationSet(PCHAR szAnim)
{
	if(m_pPed) {
		ScriptCommand(&set_actor_animation_set,m_dwGTAId,szAnim);
	}
}

//-----------------------------------------------------------

void CActorPed::SetMoney(int iAmount)
{
	ScriptCommand(&set_actor_money,m_dwGTAId,0);
	ScriptCommand(&set_actor_money,m_dwGTAId,iAmount);
}

//-----------------------------------------------------------

void CActorPed::ApplyAnimation( char *szAnimName, char *szAnimFile, float fT,
								 int opt1, int opt2, int opt3, int opt4, int iUnk )
{
	BOOL bLoaded = FALSE;
	int iWaitAnimLoad=0;

	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	// Can't allow 'naughty' anims!
	if( !stricmp(szAnimFile,"SEX") || !stricmp(szAnimFile,"SNM") ||
		!stricmp(szAnimFile,"BLOWJOBZ") || !stricmp(szAnimFile,"PAULNMAC") )
		return;

#ifdef _DEBUG
	//if(pChatWindow) pChatWindow->AddDebugMessage("Anim(%s,%s,%f,%d,%d,%d,%d,%d)",
		//szAnimName,szAnimFile,fT,opt1,opt2,opt3,opt4,iUnk);
#endif

	if (!pGame->IsAnimationLoaded(szAnimFile)) {
		pGame->RequestAnimation(szAnimFile);
		while(!pGame->IsAnimationLoaded(szAnimFile)) {
			Sleep(5);
			iWaitAnimLoad++;
			if(iWaitAnimLoad == 10) return; // we can't wait forever
		}		
		bLoaded = TRUE;
	}

	ScriptCommand(&apply_animation,m_dwGTAId,szAnimName,szAnimFile,fT,opt1,opt2,opt3,opt4,iUnk);

	if (bLoaded) {
		// not sure about this (how many anim files can we keep loaded?)
		//pGame->ReleaseAnimation(szAnimFile);
	}
}

//-----------------------------------------------------------

BOOL CActorPed::IsPerformingAnimation(char *szAnimName)
{
	if(m_pPed && ScriptCommand(&is_actor_performing_anim,m_dwGTAId,szAnimName)) {
		return TRUE;
	}
	return FALSE;	
}

//-----------------------------------------------------------

void CActorPed::SetInterior(BYTE byteID)
{
	if(!m_pPed) return;
	ScriptCommand(&link_actor_to_interior,m_dwGTAId,byteID);
}

//-----------------------------------------------------------

BOOL CActorPed::IsOnGround()
{
	if(m_pPed) {
		if(m_pPed->dwStateFlags & 3) {
			return TRUE;
		}
	}
	return FALSE;
}

//-----------------------------------------------------------

void CActorPed::ResetDamageEntity()
{
	if(m_pPed) {
		m_pPed->pdwDamageEntity = 0;
		m_pPed->dwWeaponUsed = 255;
	}
}

//-----------------------------------------------------------

VEHICLE_TYPE* CActorPed::GetGtaContactVehicle()
{
	return (VEHICLE_TYPE*)m_pPed->pContactVehicle;
}

//-----------------------------------------------------------

int CActorPed::GetPedStat()
{
	if(!m_pPed) return -1;
	return Game_PedStatPrim(m_pPed->entity.nModelIndex);
}

//-----------------------------------------------------------

BOOL CActorPed::IsPerformingCustomAnim()
{
	if(!m_pPed) return FALSE;
	if(!IsAdded()) return FALSE;

	if(m_pPed->Tasks->pdwJumpJetPack) {
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------

void CActorPed::ProcessMarkers(BOOL bMarkerStreamingEnabled, float fMarkerStreamRadius, BOOL bVisible)
{
	if(!m_pPed) return;

	// Add or remove player markers.
	if (bVisible && (!bMarkerStreamingEnabled || GetDistanceFromLocalPlayerPed() < fMarkerStreamRadius)) {
		if (m_byteCreateMarker && !m_dwArrow) ShowMarker(); 
	}
	else {
		if (m_dwArrow) HideMarker();
	}
}

//-----------------------------------------------------------

void CActorPed::ApplyCommandTask(char *szTaskName, int p1, int p2, int p3, 
								  VECTOR *p4, int p5, float p6, int p7, int p8, int p9)
{
	DWORD dwPed = (DWORD)m_pPed;
	if(!dwPed) return;

    _asm push p9
	_asm push p8
	_asm push p7
	_asm push p6
	_asm push p5
	_asm push p4
	_asm push p3
	_asm push p2
	_asm push p1
	_asm push dwPed
	_asm push szTaskName
	_asm mov ecx, 0xC15448
	_asm mov edx, 0x618970
	_asm call edx
}

//-----------------------------------------------------------

void CActorPed::DestroyFollowPedTask()
{    
	DWORD dwExt4 = (DWORD)m_pPed->Tasks->pdwIK;

	_asm mov ecx, dwExt4
	_asm mov edx, 0x639330
	_asm push 1
	_asm call edx

	m_pPed->Tasks->pdwIK = 0;
}

//-----------------------------------------------------------

void CActorPed::ToggleCellphone(int iOn)
{
	if(!m_pPed) return;
	m_iCellPhoneEnabled = iOn;
	ScriptCommand(&toggle_actor_cellphone,m_dwGTAId,iOn);
}

//-----------------------------------------------------------

int CActorPed::IsCellphoneEnabled()
{
    return m_iCellPhoneEnabled;
}

//-----------------------------------------------------------

int CActorPed::GetFightingStyle()
{
	if (!m_pPed) return 0;
	return m_pPed->byteFightingStyle;
}

//-----------------------------------------------------------

void CActorPed::SetFightingStyle(int iStyle)
{
	if (!m_pPed) return;
	ScriptCommand( &set_fighting_style, m_dwGTAId, iStyle, 6 );
}

//-----------------------------------------------------------

void CActorPed::FlyHelicopterToPoint( int iGtaVehicleID, VECTOR *vecPoint, float fMaxSpeed, float fMinAltitude, float fMaxAltitude )
{
	VEHICLE_TYPE* pVehicle = GamePool_Vehicle_GetAt( iGtaVehicleID );
	if ( !pVehicle )
		return;

	ScriptCommand( &set_car_max_speed, iGtaVehicleID );
	ScriptCommand( &set_helicopter_fly_to, iGtaVehicleID, vecPoint->X, vecPoint->Y, vecPoint->Z, fMinAltitude, fMaxAltitude );
}