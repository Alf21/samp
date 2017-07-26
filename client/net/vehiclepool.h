#pragma once

#define INVALID_VEHICLE_ID			0xFFFF
#define UNOCCUPIED_SYNC_RADIUS		70.0f
#define MAX_VEHICLE_WAITING_SLOTS	100


typedef struct _NEW_VEHICLE {
    VEHICLEID VehicleId;
	int		  iVehicleType;
	VECTOR	  vecPos;
	float	  fRotation;
	char	  aColor1;
	char	  aColor2;
	float	  fHealth;
	BYTE	  byteInterior;
	BYTE	  byteDoorsLocked;
	DWORD	  dwDoorDamageStatus;
	DWORD	  dwPanelDamageStatus;
	BYTE	  byteLightDamageStatus;
} NEW_VEHICLE;

//----------------------------------------------------


class CVehiclePool
{
private:
	
	BOOL				m_bVehicleSlotState[MAX_VEHICLES];
	CVehicle			*m_pVehicles[MAX_VEHICLES];
	VEHICLE_TYPE		*m_pGTAVehicles[MAX_VEHICLES];
	BOOL				m_bIsActive[MAX_VEHICLES];
	BOOL				m_bIsWasted[MAX_VEHICLES];
	int					m_iRespawnDelay[MAX_VEHICLES];
	PLAYERID			m_LastUndrivenID[MAX_VEHICLES];

	NEW_VEHICLE			m_NewVehicleWaiting[MAX_VEHICLE_WAITING_SLOTS];
	BOOL				m_bWaitingSlotState[MAX_VEHICLE_WAITING_SLOTS];

public:

	VECTOR				m_vecSpawnPos[MAX_VEHICLES];
	
	CVehiclePool();
	~CVehiclePool();

	BOOL New(NEW_VEHICLE *pNewVehicle);

	BOOL Delete(VEHICLEID VehicleID);	
	
	// Retrieve a vehicle
	CVehicle* GetAt(VEHICLEID VehicleID) {
		if(VehicleID >= MAX_VEHICLES || !m_bVehicleSlotState[VehicleID]) { return NULL; }
		return m_pVehicles[VehicleID];
	};

	// Find out if the slot is inuse.
	BOOL GetSlotState(VEHICLEID VehicleID) {
		if(VehicleID >= MAX_VEHICLES) { return FALSE; }
		return m_bVehicleSlotState[VehicleID];
	};

	PLAYERID GetLastUndrivenID(VEHICLEID VehicleID) {
		return m_LastUndrivenID[VehicleID];
	};

	void SetLastUndrivenID(VEHICLEID VehicleID, PLAYERID PlayerID) {
		m_LastUndrivenID[VehicleID] = PlayerID;
	};

	VEHICLEID FindIDFromGtaPtr(VEHICLE_TYPE * pGtaVehicle);

	int FindGtaIDFromID(int iID);
	int FindGtaIDFromGtaPtr(VEHICLE_TYPE * pGtaVehicle);
	void LinkToInterior(VEHICLEID VehicleID, int iInterior);

	void ProcessUnoccupiedSync(VEHICLEID VehicleID, CVehicle *pVehicle);
	void Process();
	
	void NotifyVehicleDeath(VEHICLEID VehicleID);

	int FindNearestToLocalPlayerPed();
	void AssignSpecialParamsToVehicle(VEHICLEID VehicleID, BYTE byteObjective, BYTE byteDoorsLocked);

	void ProcessWaitingList();
	void NewWhenModelLoaded(NEW_VEHICLE *pNewVehicle);

};

//----------------------------------------------------
