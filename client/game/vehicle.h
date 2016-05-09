#pragma once

#include "game.h"
#include "entity.h"

//-----------------------------------------------------------

enum eLandingGearState 
{
	LGS_CHANGING = 0,
	LGS_UP = 1,
	LGS_DOWN = 2,
};

class CVehicle : public CEntity
{
public:

	CVehicle( int iType, float fPosX, float fPosY, float fPosZ, float fRotation = 0.0f, PCHAR szNumberPlate = NULL, BOOL bKeepModelLoaded = FALSE);
	virtual ~CVehicle();

	virtual void Add();
	virtual void Remove();

	void  ResetPointers();

	void  ProcessMarkers();
	int	  m_dwMarkerID;

	void  SetLockedState(int iLocked);
	UINT  GetVehicleSubtype();

	float GetHealth();
	void  SetHealth(float fHealth);
	void  SetColor(int iColor1, int iColor2);

	BOOL  HasSunk();
	BOOL  IsWrecked();
	BOOL  IsDriverLocalPlayer();
	BOOL  IsATrainPart();
	BOOL  HasTurret();
	BOOL  HasADriver();

	void  SetHydraThrusters(DWORD dwDirection);
	DWORD GetHydraThrusters();

	void  SetTankRot(float X, float Y);
	float GetTankRotX();
	float GetTankRotY();

	float GetTrainSpeed();
	void  SetTrainSpeed(float fSpeed);

	void  Explode();

	void  ClearLastWeaponDamage();
	UINT  GetPassengersMax();

	void    SetSirenOn(BOOL state);
	BOOL    IsSirenOn();
	void    SetLandingGearState(eLandingGearState state);
	eLandingGearState	GetLandingGearState();
	void	SetInvulnerable(BOOL bInv);
	BOOL	IsInvulnerable() { return m_bIsInvulnerable; };
	void    SetEngineState(BOOL bState);
	void	SetDoorState(int iState);
	void	UpdateDamageStatus(DWORD dwPanelDamage, DWORD dwDoorDamage, BYTE byteLightDamage);
	DWORD	GetPanelDamageStatus();
	DWORD	GetDoorDamageStatus();
	BYTE	GetLightDamageStatus();

	void	LinkToInterior(int iInterior);
	void	SetWheelPopped(DWORD wheelid, DWORD popped);
	BYTE	GetWheelPopped(DWORD wheelid);
	void	AttachTrailer();
	void	DetachTrailer();
	void    SetTrailer(CVehicle *pTrailer);
	CVehicle* GetTrailer();
	BOOL	IsRCVehicle();

	void	ToggleLights(BYTE bEnable = 25);
	bool	AreLightsEnabled();

	BOOL	IsOccupied();
	void    Recreate();
	BOOL	UpdateLastDrivenTime();

	/*
	void	ProcessEngineAudio(BYTE byteDriverID);
	void	ResetAudio();*/

	void	RemoveEveryoneFromVehicle();

	VEHICLE_TYPE	*m_pVehicle;
	BOOL		m_bIsInvulnerable;
	BOOL		m_bIsLocked; // No control state
	BOOL		m_bDoorsLocked; // Vehicle is enterable TRUE/FALSE
	BYTE		m_byteObjectiveVehicle; // Is this a special objective vehicle? 0/1
	BOOL		m_bSpecialMarkerEnabled;
	DWORD		m_dwTimeSinceLastDriven;
	BOOL		m_bHasBeenDriven;
	CVehicle*   m_pTrailer;

	BOOL		m_bUnoccupiedSync;		
	BOOL		m_bRemoteUnocSync;	
	BOOL		m_bKeepModelLoaded;
};

//-----------------------------------------------------------
