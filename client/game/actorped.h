#pragma once

#include "game.h"
#include "aimstuff.h"
#include "entity.h"

#define MOVETO_WALK		4	
#define MOVETO_JOG		6
#define MOVETO_SPRINT	7
#define MOVETO_DIRECT	8 // reverts to SetActorPos

//-----------------------------------------------------------

class CActorPed : public CEntity
{
public:

	// Constructor/Destructor.	
	CActorPed();
	CActorPed(int iSkin, float fPosX, float fPosY, float fPosZ, float fRotation = 0.0f);
	virtual ~CActorPed();
	void ResetPointers();

	void  Process();
	void  Destroy();
	void  ShowMarker();
	//void  ShowMarkerEx();
	void  HideMarker();
	int   GetCurrentVehicleID();
	BOOL  IsOnScreen();
	float GetHealth();
	void  SetHealth(float fHealth);
	float GetArmour();
	void  SetArmour(float fArmour);
	DWORD GetStateFlags();
	void  SetStateFlags(DWORD dwStateFlags);
	BOOL  IsDead();
	BOOL  IsInVehicle();
	BYTE  GetActionTrigger();
	void  SetActionTrigger(BYTE byteTrigger);
	void  SetAmmo(BYTE byteWeapon, WORD wordAmmo);
	void  ClearAllWeapons();

	void MoveTo(VECTOR vecPos, int iMoveType);

	int   GetFightingStyle();
	void  SetFightingStyle(int iStyle);

	float GetTargetRotation();
	void  SetTargetRotation(float fRotation);
	void  ForceTargetRotation(float fRotation);

	void GiveWeapon(int iWeaponID, int iAmmo);
	WEAPON_SLOT_TYPE * FindWeaponSlot(DWORD dwWeapon);
	BOOL HasAmmoForCurrentWeapon();

	void SetImmunities(BOOL bBullet, BOOL bFire, BOOL bExplosion, BOOL bDamage, BOOL bUnknown);
	
	void PutDirectlyInVehicle(int iVehicleID, int iSeat);
	void EnterVehicle(int iVehicleID, BOOL bPassenger);
	void ExitCurrentVehicle();
	void RemoveFromVehicleAndPutAt(float fX, float fY, float fZ);
	void DriveVehicleToPoint(int iGtaVehicleID, VECTOR *vecPoint, float fMaxSpeed, int iDriveStyle);
	void FlyHelicopterToPoint( int iGtaVehicleID, VECTOR *vecPoint, float fMaxSpeed, float fMinAltitude, float fMaxAltitude );


	BOOL IsAPassenger();

	VEHICLE_TYPE * GetGtaVehicle();
	PED_TYPE * GetGtaActor() { return m_pPed; };
	VEHICLE_TYPE * GetGtaContactVehicle();

	int GetVehicleSeatID();

	BOOL StartPassengerDriveByMode();
	void SetCollisionChecking(int iCheck);
	void SetDead();
	void ExtinguishFire();
	void SetAnimationSet(PCHAR szAnim);
	void SetMoney(int iAmount);
	void ApplyAnimation(char *szAnimName, char *szAnimFile, float fT,
						int opt1, int opt2, int opt3, int opt4, int iUnk);

	void KillPlayer(PLAYERID PlayerID, int iWeapon);

	void SetInterior(BYTE byteID);
	BOOL IsOnGround();
	void ResetDamageEntity();
	BOOL IsPerformingAnimation(char *szAnimName);
	BOOL IsPerformingCustomAnim();

	int			GetPedStat();
	void		HandsUp();
	BOOL		HasHandsUp();
	void		HoldItem(int iObject);

	void		ProcessMarkers(BOOL bMarkerStreamingEnabled, float fMarkerStreamRadius, BOOL bVisible);

	void		ApplyCommandTask(char *szTaskName, int p1, int p2, int p3, 
								VECTOR *p4, int p5, float p6, int p7, int p8, int p9);

	void		DestroyFollowPedTask();
	void		ToggleCellphone(int iOn);
    int			IsCellphoneEnabled();
	int			m_iCellPhoneEnabled;

	PED_TYPE    *m_pPed;
	DWORD		m_dwMarkerID;
	DWORD 		m_dwArrow;
	BYTE		m_byteCreateMarker;

};

//-----------------------------------------------------------
