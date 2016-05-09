#pragma once

#define PLAYER_STATE_NONE						0
#define PLAYER_STATE_ONFOOT						1
#define PLAYER_STATE_DRIVER						2
#define PLAYER_STATE_PASSENGER					3
#define PLAYER_STATE_EXIT_VEHICLE				4
#define PLAYER_STATE_ENTER_VEHICLE_DRIVER		5
#define PLAYER_STATE_ENTER_VEHICLE_PASSENGER	6
#define PLAYER_STATE_WASTED						7
#define PLAYER_STATE_SPAWNED					8
#define PLAYER_STATE_SPECTATING					9


#define UPDATE_TYPE_NONE		0
#define UPDATE_TYPE_ONFOOT		1
#define UPDATE_TYPE_INCAR		2
#define UPDATE_TYPE_PASSENGER	3

#define VALID_KILL						1
#define TEAM_KILL						2
#define SELF_KILL						3

//----------------------------------------------------

class CRemotePlayer
{
public:
	CPlayerPed			*m_pPlayerPed;
	CVehicle			*m_pCurrentVehicle;

	PLAYERID			m_PlayerID;
	BYTE				m_byteUpdateFromNetwork;
	BYTE				m_byteState;
	VEHICLEID			m_VehicleID;
	BYTE				m_byteSeatID;
	BYTE				m_byteTeam;
	BOOL				m_bPassengerDriveByMode;
	DWORD				m_dwLastHeadUpdate;
	DWORD				m_dwStreamUpdate;

	BOOL				m_bShowNameTag;
	BOOL				m_bJetpack; 

	// Information that is synced.
	ONFOOT_SYNC_DATA		m_ofSync;
	INCAR_SYNC_DATA			m_icSync;
	PASSENGER_SYNC_DATA		m_psSync;
	AIM_SYNC_DATA			m_aimSync;
	TRAILER_SYNC_DATA		m_trSync;

	VECTOR				m_vecOnfootTargetPos;
	VECTOR				m_vecOnfootTargetSpeed;
	VECTOR				m_vecIncarTargetPos;
	VECTOR				m_vecIncarTargetSpeed;
	
	float				m_fReportedHealth;
	float				m_fReportedArmour;
	DWORD				m_dwWaitForEntryExitAnims;
	VECTOR				m_vecReferencePosition;

	int					m_iIsInAModShop;

	CRemotePlayer();
	~CRemotePlayer();

	void StateChange(BYTE byteNewState, BYTE byteOldState);

	void SetState(BYTE byteState) {	
		if(byteState != m_byteState) {
			StateChange(byteState,m_byteState);
			m_byteState = byteState;	
		}
	};

	BYTE GetState() { return m_byteState; };

	// Process this player during the server loop.
	void Process();
	void ProcessSpecialActions(BYTE byteSpecialAction);

	void HandleVehicleEntryExit();
	void Say(unsigned char * szText);
	void Privmsg(PCHAR szText);
	void TeamPrivmsg(PCHAR szText);

	void SetID(PLAYERID playerId) { m_PlayerID = playerId; };
	PLAYERID GetID() { return m_PlayerID; };
	CPlayerPed * GetPlayerPed() { return m_pPlayerPed; };

	BOOL IsActive() {
		if(m_pPlayerPed && m_byteState != PLAYER_STATE_NONE && m_byteState != PLAYER_STATE_SPECTATING) {
			return TRUE;
		}
		return FALSE;
	};

	void Deactivate() {
		m_byteState = PLAYER_STATE_NONE;
		m_bShowNameTag = TRUE;
	};

	BOOL IsSurfingOrTurretMode();
	void UpdateSurfing();

	void UpdateIncarTargetPosition();
	void UpdateOnfootTargetPosition();

	void UpdateOnFootPositionAndSpeed(VECTOR * vecPos, VECTOR *vecMoveSpeed);
	void UpdateInCarMatrixAndSpeed(MATRIX4X4 mat, VECTOR vecMove);
	void UpdateTrainDriverMatrixAndSpeed(MATRIX4X4 *matWorld,VECTOR *vecMoveSpeed, float fTrainSpeed);
	void UpdateAimFromSyncData(AIM_SYNC_DATA *paimSync);
	void UpdateTrailerMatrixAndSpeed(CVehicle* Trailer, MATRIX4X4 mat);
		
	void StoreOnFootFullSyncData(ONFOOT_SYNC_DATA *pofSync);
	void StoreInCarFullSyncData(INCAR_SYNC_DATA *picSync);
	void StorePassengerFullSyncData(PASSENGER_SYNC_DATA *ppsSync);
	void StoreTrailerFullSyncData(TRAILER_SYNC_DATA *trSync);
	void StoreUnoccupiedSyncData(UNOCCUPIED_SYNC_DATA *unocSync);

	BOOL Spawn(BYTE byteTeam, int iSkin, VECTOR * vecPos, float fRotation, DWORD dwColor, BYTE byteFightingStyle = 4, BOOL bVisible = TRUE);
	void Remove();

	void HandleDeath();
	void ResetAllSyncAttributes();

	float GetDistanceFromRemotePlayer(CRemotePlayer *pFromPlayer);
	float GetDistanceFromLocalPlayer();

	void SetPlayerColor(DWORD dwColor);
	DWORD GetPlayerColorAsRGBA();
	DWORD GetPlayerColorAsARGB();
	float GetReportedHealth() { return m_fReportedHealth; };
	float GetReportedArmour() { return m_fReportedArmour; };

	void EnterVehicle(VEHICLEID VehicleID, BOOL bPassenger);
	void ExitVehicle();
	
	BYTE GetTeam() { return m_byteTeam; };
	void SetTeam(BYTE byteTeam) { m_byteTeam = byteTeam; };
	void ShowNameTag(BYTE byteShow) { if (byteShow) m_bShowNameTag = TRUE; else m_bShowNameTag = FALSE; };
};

//----------------------------------------------------