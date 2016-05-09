#include "../main.h"
#include "../game/util.h"
#include <math.h>

extern CGame		 *pGame;
extern CChatWindow   *pChatWindow;
extern CCmdWindow	 *pCmdWindow;

using namespace RakNet;
extern CNetGame* pNetGame;

#define IS_TARGETING(x) (x & 128)

//----------------------------------------------------

CRemotePlayer::CRemotePlayer()
{
	m_byteUpdateFromNetwork = UPDATE_TYPE_NONE;
	m_byteState = PLAYER_STATE_NONE;
	m_PlayerID = INVALID_PLAYER_ID;
	m_pPlayerPed = NULL;
	m_byteUpdateFromNetwork = UPDATE_TYPE_NONE;
	m_VehicleID = 0;
	m_dwWaitForEntryExitAnims = GetTickCount();
	ResetAllSyncAttributes();
	m_byteTeam = NO_TEAM;
	m_bShowNameTag = TRUE;
	m_dwLastHeadUpdate = GetTickCount();
	m_dwStreamUpdate = 0;
	m_bJetpack = FALSE;
}

//----------------------------------------------------

CRemotePlayer::~CRemotePlayer()
{
	if(m_pPlayerPed) {
		m_pPlayerPed->Destroy();
		delete m_pPlayerPed;
		m_pPlayerPed = NULL;
	}
}

//----------------------------------------------------

void CRemotePlayer::Process()
{
	CPlayerPool *pPool = pNetGame->GetPlayerPool();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	CLocalPlayer *pLocalPlayer = pPool->GetLocalPlayer();
	MATRIX4X4 matVehicle;
	VECTOR vecMoveSpeed;

	if(IsActive())
	{
		if(m_pPlayerPed)
		{
			m_pPlayerPed->ProcessMarkers(
				pNetGame->m_bLimitGlobalChatRadius,
				pNetGame->m_fGlobalChatRadius,TRUE);

			if(GetTickCount() > m_dwWaitForEntryExitAnims) {
				HandleVehicleEntryExit();
			}

			// ---- ONFOOT NETWORK PROCESSING ----
			if( GetState() == PLAYER_STATE_ONFOOT &&
				m_byteUpdateFromNetwork == UPDATE_TYPE_ONFOOT )
			{	
				if(GetTickCount() > m_dwWaitForEntryExitAnims) {
					HandleVehicleEntryExit();
				}

				// If the user has sent X, Y or Z offsets we need to 'attach' him to the car using these offsets
				if ( !IsSurfingOrTurretMode() )
				{
					// If the user hasn't sent these, they're obviously not on the car, so we just sync
					// their normal movement/speed/etc.
					UpdateOnFootPositionAndSpeed(&m_ofSync.vecPos,&m_ofSync.vecMoveSpeed);
					UpdateOnfootTargetPosition();
				}

				// UPDATE ROTATION
				m_pPlayerPed->SetTargetRotation(m_ofSync.fRotation);
		
				// UPDATE CURRENT WEAPON
				if(m_pPlayerPed->IsAdded() && m_pPlayerPed->GetCurrentWeapon() != m_ofSync.byteCurrentWeapon) {
					m_pPlayerPed->SetArmedWeapon(m_ofSync.byteCurrentWeapon);

					// double check
					if(m_pPlayerPed->GetCurrentWeapon() != m_ofSync.byteCurrentWeapon) {
						m_pPlayerPed->GiveWeapon(m_ofSync.byteCurrentWeapon,9999);
						m_pPlayerPed->SetArmedWeapon(m_ofSync.byteCurrentWeapon);
					}
				}

				// First person weapon action hack
				if (IS_TARGETING(m_ofSync.wKeys)) {
					if (m_pPlayerPed->GetCurrentWeapon() == 34 ||
						m_pPlayerPed->GetCurrentWeapon() == 35 ||
						m_pPlayerPed->GetCurrentWeapon() == 36)
					{
						m_pPlayerPed->SetActionTrigger(12);
					}
				} else if (m_pPlayerPed->GetActionTrigger() == 12) {
					m_pPlayerPed->SetActionTrigger(1);
				}

				m_pCurrentVehicle = NULL;
				m_byteUpdateFromNetwork = UPDATE_TYPE_NONE;
			}

			// ---- DRIVER NETWORK PROCESSING ----
			else if(GetState() == PLAYER_STATE_DRIVER &&
				m_byteUpdateFromNetwork == UPDATE_TYPE_INCAR)
			{
				if(!m_pCurrentVehicle) return;
               
				// MATRIX
				DecompressNormalVector(&matVehicle.right,&m_icSync.cvecRoll);
				DecompressNormalVector(&matVehicle.up,&m_icSync.cvecDirection);

				matVehicle.pos.X = m_icSync.vecPos.X;
				matVehicle.pos.Y = m_icSync.vecPos.Y;
				matVehicle.pos.Z = m_icSync.vecPos.Z;

				// MOVE SPEED
				vecMoveSpeed.X = m_icSync.vecMoveSpeed.X;
				vecMoveSpeed.Y = m_icSync.vecMoveSpeed.Y;
				vecMoveSpeed.Z = m_icSync.vecMoveSpeed.Z;

				// Note: Train Speed and Tire Popping values are mutually exclusive, which means
				//       if one is set, the other one will be affected.

				if( m_pCurrentVehicle->GetModelIndex() == TRAIN_PASSENGER_LOCO ||
					m_pCurrentVehicle->GetModelIndex() == TRAIN_FREIGHT_LOCO ||
					m_pCurrentVehicle->GetModelIndex() == TRAIN_TRAM) {
					// TRAIN MATRIX UPDATE
					UpdateTrainDriverMatrixAndSpeed(&matVehicle,&vecMoveSpeed, m_icSync.fTrainSpeed);
				} else {

					// GENERIC VEHICLE MATRIX UPDATE
					UpdateInCarMatrixAndSpeed(matVehicle,vecMoveSpeed);
					UpdateIncarTargetPosition();
					
					// FOR TIRE POPPING
					if (pNetGame->m_bTirePopping) {
						if (m_pCurrentVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_BIKE) {
							m_pCurrentVehicle->SetWheelPopped(0, m_icSync.byteTires[0]);
							m_pCurrentVehicle->SetWheelPopped(1, m_icSync.byteTires[1]);
						} 
						else if ( m_pCurrentVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR ) {
							m_pCurrentVehicle->SetWheelPopped(0, m_icSync.byteTires[0]);
							m_pCurrentVehicle->SetWheelPopped(1, m_icSync.byteTires[1]);
							m_pCurrentVehicle->SetWheelPopped(2, m_icSync.byteTires[2]);
							m_pCurrentVehicle->SetWheelPopped(3, m_icSync.byteTires[3]);
						}
					}
				}

				// HYDRA THRUSTERS
				if(m_pCurrentVehicle->GetModelIndex() == HYDRA) {
					m_pCurrentVehicle->SetHydraThrusters(m_icSync.dwHydraThrustAngle);
				}
		
				m_pCurrentVehicle->SetSirenOn(m_icSync.byteSirenOn);
				m_pCurrentVehicle->SetLandingGearState((eLandingGearState)m_icSync.byteLandingGearState);
		
				// VEHICLE HEALTH
				m_pCurrentVehicle->SetHealth(m_icSync.fCarHealth);

				// TRAILER AUTOMATIC ATTACHMENT AS THEY MOVE INTO IT
				if (m_pCurrentVehicle->GetDistanceFromLocalPlayerPed() < LOCKING_DISTANCE)
				{
					if (m_icSync.TrailerID && m_icSync.TrailerID < MAX_VEHICLES)
					{
						CVehicle* pRealTrailer = m_pCurrentVehicle->GetTrailer();
						CVehicle *pTrailerVehicle = pVehiclePool->GetAt(m_icSync.TrailerID);
						if (!pRealTrailer) {
							if (pTrailerVehicle) {
								m_pCurrentVehicle->SetTrailer(pTrailerVehicle);
								m_pCurrentVehicle->AttachTrailer();
							}
						}
					} 
					else
					{
						if (m_pCurrentVehicle->GetTrailer()) {
							m_pCurrentVehicle->DetachTrailer();
							m_pCurrentVehicle->SetTrailer(NULL);
						}
					}
				}
				else
				{
					m_pCurrentVehicle->SetTrailer(NULL);
				}

				// UPDATE CURRENT WEAPON (FOR DRIVER!)
				if( m_pPlayerPed->IsAdded() &&
					m_pPlayerPed->GetCurrentWeapon() != m_icSync.byteCurrentWeapon ) {
					m_pPlayerPed->SetArmedWeapon(m_icSync.byteCurrentWeapon);
					if(m_pPlayerPed->GetCurrentWeapon() != m_icSync.byteCurrentWeapon) {
						m_pPlayerPed->GiveWeapon(m_icSync.byteCurrentWeapon,9999);
						m_pPlayerPed->SetArmedWeapon(m_icSync.byteCurrentWeapon);
					}
				}
				m_byteUpdateFromNetwork = UPDATE_TYPE_NONE;
			}

			// ---- PASSENGER NETWORK PROCESSING ----
			else if(GetState() == PLAYER_STATE_PASSENGER && 
				m_byteUpdateFromNetwork == UPDATE_TYPE_PASSENGER)
			{
				if(!m_pCurrentVehicle) return;
				// UPDATE CURRENT WEAPON
				if( m_pPlayerPed->IsAdded() &&
					m_pPlayerPed->GetCurrentWeapon() != m_psSync.byteCurrentWeapon ) {
					m_pPlayerPed->SetArmedWeapon(m_psSync.byteCurrentWeapon);
					if(m_pPlayerPed->GetCurrentWeapon() != m_psSync.byteCurrentWeapon) {
						m_pPlayerPed->GiveWeapon(m_psSync.byteCurrentWeapon,9999);
						m_pPlayerPed->SetArmedWeapon(m_psSync.byteCurrentWeapon);
					}
				}
				// FOR INITIALISING PASSENGER DRIVEBY
				if(!m_bPassengerDriveByMode && m_pPlayerPed->GetActionTrigger() == ACTION_INCAR) {
					if(m_psSync.byteDriveBy) {
						if(m_pPlayerPed->IsAdded()) {
							if(m_pPlayerPed->StartPassengerDriveByMode()) {
								m_bPassengerDriveByMode = TRUE;
							}
						}
					}
				}
				m_byteUpdateFromNetwork = UPDATE_TYPE_NONE;
			}

			// ------ PROCESSED FOR ALL FRAMES ----- 
			if(GetState() == PLAYER_STATE_ONFOOT) {
				m_bPassengerDriveByMode = FALSE;
				ProcessSpecialActions(m_ofSync.byteSpecialAction);
				m_pPlayerPed->SetTargetRotation(m_ofSync.fRotation);
				m_pPlayerPed->SetKeys(m_ofSync.wKeys,m_ofSync.lrAnalog,m_ofSync.udAnalog);
				
				if(IsSurfingOrTurretMode()) {
					UpdateSurfing();
					m_pPlayerPed->SetGravityProcessing(0);
					m_pPlayerPed->SetCollisionChecking(0);
				} else {
					m_pPlayerPed->SetGravityProcessing(1);
					m_pPlayerPed->SetCollisionChecking(1);
				}
			}
			else if(GetState() == PLAYER_STATE_DRIVER) {
				m_bPassengerDriveByMode = FALSE;
				m_pPlayerPed->CheckVehicleParachute();
				
				// REMOVE PLAYER MODEL IN RC VEHICLE
				if(m_pCurrentVehicle && m_pCurrentVehicle->IsRCVehicle()) {
					m_pPlayerPed->Remove();
				}

				if(m_iIsInAModShop)	{
					VECTOR vec = {0.0f,0.0f,0.0f};
					m_pPlayerPed->SetKeys(0,0,0);
					if(m_pCurrentVehicle) m_pCurrentVehicle->SetMoveSpeedVector(vec);
				} else {
					m_pPlayerPed->SetKeys(m_icSync.wKeys,m_icSync.lrAnalog,m_icSync.udAnalog);
				}
			}
			else if(GetState() == PLAYER_STATE_PASSENGER) {
				m_pPlayerPed->CheckVehicleParachute();
				m_pPlayerPed->SetKeys(m_psSync.wKeys,0,0);
			}
			else {
				m_bPassengerDriveByMode = FALSE;
				m_pPlayerPed->SetKeys(0,0,0);
			}

			m_pPlayerPed->SetHealth(100.0f);
			//m_pPlayerPed->SetArmour(100.0f);
		}
	}
	else {
		// NOT ACTIVE
		if(m_pPlayerPed) {
			ResetAllSyncAttributes();
			m_pPlayerPed->Destroy();
			delete m_pPlayerPed;
			m_pPlayerPed = NULL;
		}
	}
}

//----------------------------------------------------

BOOL CRemotePlayer::IsSurfingOrTurretMode()
{
	if(GetState() == PLAYER_STATE_ONFOOT) {
		if( m_ofSync.wSurfInfo != 0 && 
			m_ofSync.wSurfInfo != INVALID_VEHICLE_ID &&
			m_ofSync.wSurfInfo != INVALID_OBJECT_ID ) {
				return TRUE;
			}
	}
	return FALSE;
}

//----------------------------------------------------

void CRemotePlayer::UpdateSurfing()
{
	if ( m_ofSync.wSurfInfo > MAX_VEHICLES ) { // its an object

			m_ofSync.wSurfInfo -= MAX_VEHICLES; // derive proper object id
			CObjectPool* pObjectPool = pNetGame->GetObjectPool();
			CObject* pObject = pObjectPool->GetAt((BYTE)m_ofSync.wSurfInfo);

			if (pObject) {
				MATRIX4X4 objMat;
				pObject->GetMatrix(&objMat);
				objMat.pos.X += m_ofSync.vecSurfOffsets.X;
				objMat.pos.Y += m_ofSync.vecSurfOffsets.Y;
				objMat.pos.Z += m_ofSync.vecSurfOffsets.Z;
				m_pPlayerPed->SetMatrix(objMat);
			}

	} else { // must be a vehicle

		CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
		CVehicle* pVehicle = pVehiclePool->GetAt(m_ofSync.wSurfInfo);

		if (pVehicle) {
			MATRIX4X4 matPlayer, matVehicle;
			VECTOR vecMoveSpeed,vecTurnSpeed;

			pVehicle->GetMatrix(&matVehicle);
			pVehicle->GetMoveSpeedVector(&vecMoveSpeed);
			pVehicle->GetTurnSpeedVector(&vecTurnSpeed);

			m_pPlayerPed->GetMatrix(&matPlayer);
		
			matPlayer.pos.X = matVehicle.pos.X + m_ofSync.vecSurfOffsets.X;
			matPlayer.pos.Y = matVehicle.pos.Y + m_ofSync.vecSurfOffsets.Y;
			matPlayer.pos.Z = matVehicle.pos.Z + m_ofSync.vecSurfOffsets.Z;

			m_pPlayerPed->SetMatrix(matPlayer);
			m_pPlayerPed->SetMoveSpeedVector(vecMoveSpeed);
			//m_pPlayerPed->SetTurnSpeedVector(vecTurnSpeed);
		}
	}
}

//----------------------------------------------------

void CRemotePlayer::HandleVehicleEntryExit()
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	MATRIX4X4 mat;

	if(!m_pPlayerPed) return;

	if(GetState() == PLAYER_STATE_ONFOOT) {
		if(m_pPlayerPed->IsInVehicle()) {
			m_pPlayerPed->GetMatrix(&mat);
			m_pPlayerPed->RemoveFromVehicleAndPutAt(mat.pos.X,mat.pos.Y,mat.pos.Z);
		}
		return;
	}

	if( GetState() == PLAYER_STATE_DRIVER || GetState() == PLAYER_STATE_PASSENGER )
	{
		if(!m_pPlayerPed->IsInVehicle()) {
			// if they're onfoot, put them in the vehicle.
			CVehicle *pVehicle = pVehiclePool->GetAt(m_VehicleID);
			if(pVehicle) {
				int iCarID = pVehiclePool->FindGtaIDFromID(m_VehicleID);
				m_pPlayerPed->PutDirectlyInVehicle(iCarID,m_byteSeatID);
			}
		} else {
			// validate the vehicle they're driving is the one they're
			// reporting to be in.
			int iCurrentVehicle = m_pPlayerPed->GetCurrentVehicleID();
			int iReportedVehicle = pVehiclePool->FindGtaIDFromID(m_VehicleID);
			if(iCurrentVehicle != iReportedVehicle) {
				m_pPlayerPed->PutDirectlyInVehicle(iReportedVehicle,m_byteSeatID);
			}
		}
	}
}

//----------------------------------------------------

void CRemotePlayer::ProcessSpecialActions(BYTE byteSpecialAction)
{
	if(!m_pPlayerPed || !m_pPlayerPed->IsAdded()) return;

	if(GetState() != PLAYER_STATE_ONFOOT) {
		byteSpecialAction = SPECIAL_ACTION_NONE;
		m_ofSync.byteSpecialAction = SPECIAL_ACTION_NONE;
	}

	// headsync:always
	if(GetState() == PLAYER_STATE_ONFOOT && m_pPlayerPed->IsAdded()) {
		if((GetTickCount() - m_dwLastHeadUpdate) > 500) {
            VECTOR LookAt;
			CAMERA_AIM *Aim = GameGetRemotePlayerAim(m_pPlayerPed->m_bytePlayerNumber);
            LookAt.X = Aim->pos1x + (Aim->f1x * 20.0f);
			LookAt.Y = Aim->pos1y + (Aim->f1y * 20.0f);
			LookAt.Z = Aim->pos1z + (Aim->f1z * 20.0f);
			m_pPlayerPed->ApplyCommandTask("FollowPedSA",0,2000,-1,&LookAt,0,0.1f,500,3,0);
			m_dwLastHeadUpdate = GetTickCount();
		}
	}

	// cellphone:start
	if( byteSpecialAction == SPECIAL_ACTION_USECELLPHONE &&
		!m_pPlayerPed->IsCellphoneEnabled() ) {
			//pChatWindow->AddDebugMessage("Player enabled Cell");
			m_pPlayerPed->ToggleCellphone(1);
			return;
	}

	// cellphone:stop
	if( byteSpecialAction != SPECIAL_ACTION_USECELLPHONE &&
		m_pPlayerPed->IsCellphoneEnabled() ) {
			m_pPlayerPed->ToggleCellphone(0);
			return;
	}

	// goggles:start
	if(byteSpecialAction == SPECIAL_ACTION_NIGHTVISION ||
	   byteSpecialAction == SPECIAL_ACTION_THERMALVISION) {
		if(!m_pPlayerPed->HasGoggles()) {
			m_pPlayerPed->StartGoggles();
			return;
		}
	}

	// goggles:stop
	if(byteSpecialAction != SPECIAL_ACTION_NIGHTVISION &&
	   byteSpecialAction != SPECIAL_ACTION_THERMALVISION) {
		if (m_pPlayerPed->HasGoggles()) {
			m_pPlayerPed->StopGoggles();
			return;
		}
	}

	// jetpack:start
	if(byteSpecialAction == SPECIAL_ACTION_USEJETPACK) {
		if(!m_pPlayerPed->IsInJetpackMode()) {
			//pChatWindow->AddDebugMessage("[%i] CRemotePlayer->StartJetpack();", m_pPlayerPed->m_bytePlayerNumber);
			m_bJetpack=true;
			m_pPlayerPed->StartJetpack();
			return;
		}
	}

	// jetpack:stop
	if(byteSpecialAction != SPECIAL_ACTION_USEJETPACK) {
		if(m_pPlayerPed->IsInJetpackMode()) {
			//pChatWindow->AddDebugMessage("[%i] CRemotePlayer->StopJetpack();", m_pPlayerPed->m_bytePlayerNumber);
			m_bJetpack=false;
			m_pPlayerPed->StopJetpack();
			return;
		}
	}

	// handsup:start
	if(byteSpecialAction == SPECIAL_ACTION_HANDSUP && !m_pPlayerPed->HasHandsUp()) {
		m_pPlayerPed->HandsUp();
	}

	// handsup:stop
	if(byteSpecialAction != SPECIAL_ACTION_HANDSUP && m_pPlayerPed->HasHandsUp()) {
		m_pPlayerPed->StopDancing(); // has the same effect
	}

	// dancing:start
	if(!m_pPlayerPed->IsDancing() && byteSpecialAction == SPECIAL_ACTION_DANCE1) {
		m_pPlayerPed->StartDancing(0);
	}
	if(!m_pPlayerPed->IsDancing() && byteSpecialAction == SPECIAL_ACTION_DANCE2) {
		m_pPlayerPed->StartDancing(1);
	}
	if(!m_pPlayerPed->IsDancing() && byteSpecialAction == SPECIAL_ACTION_DANCE3) {
		m_pPlayerPed->StartDancing(2);
	}
	if(!m_pPlayerPed->IsDancing() && byteSpecialAction == SPECIAL_ACTION_DANCE4) {
		m_pPlayerPed->StartDancing(3);
	}

	// dancing:stop
	if( m_pPlayerPed->IsDancing() && 
		(byteSpecialAction != SPECIAL_ACTION_DANCE1 &&
		byteSpecialAction != SPECIAL_ACTION_DANCE2 && 
		byteSpecialAction != SPECIAL_ACTION_DANCE3 && 
		byteSpecialAction != SPECIAL_ACTION_DANCE4) ) {
			m_pPlayerPed->StopDancing();
	}

	if(m_pPlayerPed->IsDancing()) m_pPlayerPed->ProcessDancing();

	// parachutes:we don't have any network indicators for this yet
	m_pPlayerPed->ProcessParachutes();
}

//----------------------------------------------------

void CRemotePlayer::UpdateOnfootTargetPosition()
{
	MATRIX4X4 matEnt;

	if(!m_pPlayerPed) return;
	m_pPlayerPed->GetMatrix(&matEnt);

     // check for an already valid position
	if( (FloatOffset(m_vecOnfootTargetPos.X,matEnt.pos.X) <= 0.025f) &&
		(FloatOffset(m_vecOnfootTargetPos.Y,matEnt.pos.Y) <= 0.025f) &&
		(FloatOffset(m_vecOnfootTargetPos.Z,matEnt.pos.Z) <= 0.025f) )
	{
		// nothing to do
		return;
	}

	// Directly translate way-out position
	if( !m_pPlayerPed->IsAdded() ||
		(FloatOffset(m_vecOnfootTargetPos.X,matEnt.pos.X) > 8.0f) ||
		(FloatOffset(m_vecOnfootTargetPos.Y,matEnt.pos.Y) > 8.0f) ||
		(FloatOffset(m_vecOnfootTargetPos.Z,matEnt.pos.Z) > 8.0f) ) {

		matEnt.pos.X = m_vecOnfootTargetPos.X;
		matEnt.pos.Y = m_vecOnfootTargetPos.Y;
		matEnt.pos.Z = m_vecOnfootTargetPos.Z;

		m_pPlayerPed->SetMatrix(matEnt);
        m_pPlayerPed->SetMoveSpeedVector(m_vecOnfootTargetSpeed);

		return;
	}

	// Avoid direct translation by increasing velocity
	// towards the target
	
	VECTOR vec;
	m_pPlayerPed->GetMoveSpeedVector(&vec);

	float fMultiplyAmount = 0.08f;
	if(!m_pPlayerPed->IsOnGround()) {
		fMultiplyAmount = 0.03f;
	}

	if( FloatOffset(m_vecOnfootTargetPos.X,matEnt.pos.X) > 0.025f ) {
		vec.X += (m_vecOnfootTargetPos.X - matEnt.pos.X) * fMultiplyAmount;
	}
	if( FloatOffset(m_vecOnfootTargetPos.Y,matEnt.pos.Y) > 0.025f ) {
		vec.Y += (m_vecOnfootTargetPos.Y - matEnt.pos.Y) * fMultiplyAmount;
	}
	if( FloatOffset(m_vecOnfootTargetPos.Z,matEnt.pos.Z) > 0.025f ) {
		vec.Z += (m_vecOnfootTargetPos.Z - matEnt.pos.Z) * fMultiplyAmount;
	}

	m_pPlayerPed->SetMoveSpeedVector(vec);

	if(m_pPlayerPed->IsOnGround()) {
		m_pPlayerPed->ApplyMoveSpeed();
	}
}

//----------------------------------------------------

void CRemotePlayer::UpdateOnFootPositionAndSpeed(VECTOR * vecPos, VECTOR * vecMove)
{
    m_vecOnfootTargetPos.X = vecPos->X;
	m_vecOnfootTargetPos.Y = vecPos->Y;
	m_vecOnfootTargetPos.Z = vecPos->Z;
    m_vecOnfootTargetSpeed.X = vecMove->X;
	m_vecOnfootTargetSpeed.Y = vecMove->Y;
	m_vecOnfootTargetSpeed.Z = vecMove->Z;
}

//----------------------------------------------------

void CRemotePlayer::UpdateIncarTargetPosition()
{
	MATRIX4X4 matEnt;
	VECTOR vec = {0.0f,0.0f,0.0f};

	if(!m_pCurrentVehicle) return;

	m_pCurrentVehicle->GetMatrix(&matEnt);

  	// Don't apply any updates from vehicle mod shops
	if(m_iIsInAModShop)
	{
		m_pPlayerPed->SetKeys(0,0,0);
		m_pCurrentVehicle->SetMoveSpeedVector(vec);
		m_icSync.udAnalog = 0;
		m_icSync.lrAnalog = 0;
		m_icSync.wKeys = 0;
		m_icSync.vecMoveSpeed.X = 0.0f;
		m_icSync.vecMoveSpeed.Y = 0.0f;
		m_icSync.vecMoveSpeed.Z = 0.0f;
		return;
	}

    // check for an already valid position
	if( (FloatOffset(m_vecIncarTargetPos.X,matEnt.pos.X) <= 0.05f) &&
		(FloatOffset(m_vecIncarTargetPos.Y,matEnt.pos.Y) <= 0.05f) &&
		(FloatOffset(m_vecIncarTargetPos.Z,matEnt.pos.Z) <= 0.05f) )
	{
		// nothing to do
		return;
	}

	// Directly translate way-out position
	if( !m_pCurrentVehicle->IsAdded() ||
		(FloatOffset(m_vecIncarTargetPos.X,matEnt.pos.X) > 8.0f) ||
		(FloatOffset(m_vecIncarTargetPos.Y,matEnt.pos.Y) > 8.0f) ||
		(FloatOffset(m_vecIncarTargetPos.Z,matEnt.pos.Z) > 8.0f) ) {

		matEnt.pos.X = m_vecIncarTargetPos.X;
		matEnt.pos.Y = m_vecIncarTargetPos.Y;
		matEnt.pos.Z = m_vecIncarTargetPos.Z;

		m_pCurrentVehicle->SetMatrix(matEnt);
        m_pCurrentVehicle->SetMoveSpeedVector(m_vecIncarTargetSpeed);

		return;
	}

	// Avoid direct translation by increasing velocity
	// towards the target
	
	m_pCurrentVehicle->GetMoveSpeedVector(&vec);

	if( FloatOffset(m_vecIncarTargetPos.X,matEnt.pos.X) > 0.05f ) {
		vec.X += (m_vecIncarTargetPos.X - matEnt.pos.X) * 0.05f;
	}
	if( FloatOffset(m_vecIncarTargetPos.Y,matEnt.pos.Y) > 0.05f ) {
		vec.Y += (m_vecIncarTargetPos.Y - matEnt.pos.Y) * 0.05f;
	}
	if( FloatOffset(m_vecIncarTargetPos.Z,matEnt.pos.Z) > 0.05f ) {
		vec.Z += (m_vecIncarTargetPos.Z - matEnt.pos.Z) * 0.05f;
	}

	m_pCurrentVehicle->SetMoveSpeedVector(vec);
}

//----------------------------------------------------

void CRemotePlayer::UpdateInCarMatrixAndSpeed(MATRIX4X4 mat, VECTOR vecMove)
{	
	MATRIX4X4 matEnt;
	m_pCurrentVehicle->GetMatrix(&matEnt);

	CPlayerPed* pPlayer = pGame->FindPlayerPed();

	matEnt.right.X = mat.right.X;
	matEnt.right.Y = mat.right.Y;
	matEnt.right.Z = mat.right.Z;

	matEnt.up.X = mat.up.X;
	matEnt.up.Y = mat.up.Y;
	matEnt.up.Z = mat.up.Z;

	m_vecIncarTargetPos.X = mat.pos.X;
	m_vecIncarTargetPos.Y = mat.pos.Y;
	m_vecIncarTargetPos.Z = mat.pos.Z;

	m_vecIncarTargetSpeed.X = vecMove.X;
	m_vecIncarTargetSpeed.Y = vecMove.Y;
	m_vecIncarTargetSpeed.Z = vecMove.Z;

	m_pCurrentVehicle->SetMoveSpeedVector(vecMove);	
	m_pCurrentVehicle->SetMatrix(matEnt);	
}

//----------------------------------------------------

void CRemotePlayer::UpdateTrainDriverMatrixAndSpeed(MATRIX4X4 *matWorld,VECTOR *vecMoveSpeed, float fTrainSpeed)
{
	MATRIX4X4 matVehicle;
	VECTOR vecInternalMoveSpeed;
	BOOL bTeleport=FALSE;
	float fDif;

	if(!m_pPlayerPed || !m_pCurrentVehicle) return;

	m_pCurrentVehicle->GetMatrix(&matVehicle);

	if(matWorld->pos.X >= matVehicle.pos.X) {
		fDif = matWorld->pos.X - matVehicle.pos.X;
	} else {
		fDif = matVehicle.pos.X - matWorld->pos.X;
	}
	if(fDif > 10.0f) bTeleport=TRUE;

	if(matWorld->pos.Y >= matVehicle.pos.Y) {
		fDif = matWorld->pos.Y - matVehicle.pos.Y;
	} else {
		fDif = matVehicle.pos.Y - matWorld->pos.Y;
	}
	if(fDif > 10.0f) bTeleport=TRUE;

	if(bTeleport) m_pCurrentVehicle->TeleportTo(matWorld->pos.X,matWorld->pos.Y,matWorld->pos.Z);
	
	m_pCurrentVehicle->GetMoveSpeedVector(&vecInternalMoveSpeed);
	vecInternalMoveSpeed.X = vecMoveSpeed->X;
	vecInternalMoveSpeed.Y = vecMoveSpeed->Y;
	vecInternalMoveSpeed.Z = vecMoveSpeed->Z;
	m_pCurrentVehicle->SetMoveSpeedVector(vecInternalMoveSpeed);
	m_pCurrentVehicle->SetTrainSpeed(fTrainSpeed);
}

//----------------------------------------------------

void CRemotePlayer::StoreOnFootFullSyncData(ONFOOT_SYNC_DATA *pofSync)
{
	if(GetTickCount() < m_dwWaitForEntryExitAnims) return;

	m_pCurrentVehicle = NULL;
	memcpy(&m_ofSync,pofSync,sizeof(ONFOOT_SYNC_DATA));
	m_fReportedHealth = (float)pofSync->byteHealth;
	m_fReportedArmour = (float)pofSync->byteArmour;
	m_byteUpdateFromNetwork = UPDATE_TYPE_ONFOOT;
	
	SetState(PLAYER_STATE_ONFOOT);
}

//----------------------------------------------------

void CRemotePlayer::StoreInCarFullSyncData(INCAR_SYNC_DATA *picSync)
{	
	if(GetTickCount() < m_dwWaitForEntryExitAnims) return;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	memcpy(&m_icSync,picSync,sizeof(INCAR_SYNC_DATA));
	m_VehicleID = picSync->VehicleID;
	if (pVehiclePool) m_pCurrentVehicle = pVehiclePool->GetAt(m_VehicleID);

	m_byteSeatID = 0;
	m_fReportedHealth = (float)picSync->bytePlayerHealth;
	m_fReportedArmour = (float)picSync->bytePlayerArmour;
	m_byteUpdateFromNetwork = UPDATE_TYPE_INCAR;
	
	SetState(PLAYER_STATE_DRIVER);
}

//----------------------------------------------------

void CRemotePlayer::StorePassengerFullSyncData(PASSENGER_SYNC_DATA *ppsSync)
{
	if(GetTickCount() < m_dwWaitForEntryExitAnims) return;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	memcpy(&m_psSync,ppsSync,sizeof(PASSENGER_SYNC_DATA));
	m_VehicleID = ppsSync->VehicleID;
	m_byteSeatID = ppsSync->byteSeatFlags & 127;
	if (pVehiclePool) m_pCurrentVehicle = pVehiclePool->GetAt(m_VehicleID);
	m_fReportedHealth = (float)ppsSync->bytePlayerHealth;
	m_fReportedArmour = (float)ppsSync->bytePlayerArmour;
	m_byteUpdateFromNetwork = UPDATE_TYPE_PASSENGER;
	
	SetState(PLAYER_STATE_PASSENGER);
}

//----------------------------------------------------

void CRemotePlayer::UpdateAimFromSyncData(AIM_SYNC_DATA *paimSync)
{
	if(!m_pPlayerPed) return;
	m_pPlayerPed->SetCameraMode(paimSync->byteCamMode);

	CAMERA_AIM Aim;

	Aim.f1x = paimSync->vecAimf1.X;
	Aim.f1y = paimSync->vecAimf1.Y;
	Aim.f1z = paimSync->vecAimf1.Z;
	Aim.f2x = paimSync->vecAimf2.X;
	Aim.f2y = paimSync->vecAimf2.Y;
	Aim.f2z = paimSync->vecAimf2.Z;
	Aim.pos1x = paimSync->vecAimPos.X;
	Aim.pos1y = paimSync->vecAimPos.Y;
	Aim.pos1z = paimSync->vecAimPos.Z;
	Aim.pos2x = paimSync->vecAimPos.X;
	Aim.pos2y = paimSync->vecAimPos.Y;
	Aim.pos2z = paimSync->vecAimPos.Z;

	m_pPlayerPed->SetCurrentAim(&Aim);
	m_pPlayerPed->SetAimZ(paimSync->fAimZ);

	float fExtZoom = (float)(paimSync->byteCamExtZoom)/63.0f;
	m_pPlayerPed->SetCameraExtendedZoom(fExtZoom);

	WEAPON_SLOT_TYPE* pwstWeapon = m_pPlayerPed->GetCurrentWeaponSlot();
	if (paimSync->byteWeaponState == WS_RELOADING)
		pwstWeapon->dwState = 2;		// Reloading
	else
		if (paimSync->byteWeaponState != WS_MORE_BULLETS) 
			pwstWeapon->dwAmmoInClip = (DWORD)paimSync->byteWeaponState;
		else
			if (pwstWeapon->dwAmmoInClip < 2)
				pwstWeapon->dwAmmoInClip = 2;


}

//----------------------------------------------------

void CRemotePlayer::StoreUnoccupiedSyncData(UNOCCUPIED_SYNC_DATA * unocSync)
{

	VEHICLEID UnocID = unocSync->VehicleID;
	if (!UnocID || UnocID == INVALID_VEHICLE_ID) return;
	
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	CVehicle *pVehicle = NULL;
	if (pVehiclePool) {
		pVehicle = pVehiclePool->GetAt(UnocID);
		pVehiclePool->SetLastUndrivenID(UnocID, m_PlayerID);
	}

	if(pVehicle && !pVehicle->HasADriver()) 
	{
		MATRIX4X4 matWorld;

		pVehicle->GetMatrix(&matWorld);

		DecompressNormalVector(&matWorld.up, &unocSync->cvecDirection);
		DecompressNormalVector(&matWorld.right, &unocSync->cvecRoll);

		// if we're pretty already there.. no translation. 
		if( (FloatOffset(unocSync->vecPos.X,matWorld.pos.X) <= 0.1f) &&
			(FloatOffset(unocSync->vecPos.Y,matWorld.pos.Y) <= 0.1f) &&
			(FloatOffset(unocSync->vecPos.Z,matWorld.pos.Z) <= 0.1f) )
		{
			return;
		}

		// if difference is over 8 units, direct translation
		if( !pVehicle->IsAdded() ||
			(FloatOffset(unocSync->vecPos.X,matWorld.pos.X) > 8.0f) ||
			(FloatOffset(unocSync->vecPos.Y,matWorld.pos.Y) > 8.0f) ||
			(FloatOffset(unocSync->vecPos.Z,matWorld.pos.Z) > 8.0f) ) {

			matWorld.pos.X = unocSync->vecPos.X;
			matWorld.pos.Y = unocSync->vecPos.Y;
			matWorld.pos.Z = unocSync->vecPos.Z;

			pVehicle->SetMatrix(matWorld);
			pVehicle->SetMoveSpeedVector(unocSync->vecMoveSpeed);
			pVehicle->SetTurnSpeedVector(unocSync->vecTurnSpeed);
			return;
		}

		// gradually increase/decrease velocities towards the target								
		pVehicle->SetMatrix(matWorld);							// rotation
		pVehicle->SetMoveSpeedVector(unocSync->vecMoveSpeed);	// move velocity
		pVehicle->SetTurnSpeedVector(unocSync->vecTurnSpeed);	// turn velocity

		VECTOR vec = {0.0f, 0.0f, 0.0f};		
		pVehicle->GetMoveSpeedVector(&vec);

		if( FloatOffset(unocSync->vecPos.X,matWorld.pos.X) > 0.05 ) {
			vec.X += (unocSync->vecPos.X - matWorld.pos.X) * 0.05f;
		}
		if( FloatOffset(unocSync->vecPos.Y,matWorld.pos.Y) > 0.05 ) {
			vec.Y += (unocSync->vecPos.Y - matWorld.pos.Y) * 0.05f;
		}
		if( FloatOffset(unocSync->vecPos.Z,matWorld.pos.Z) > 0.05 ) {
			vec.Z += (unocSync->vecPos.Z - matWorld.pos.Z) * 0.05f;
		}

		pVehicle->SetMoveSpeedVector(vec);				
		pVehicle->m_bRemoteUnocSync = true;
	}
}

//----------------------------------------------------

void CRemotePlayer::StoreTrailerFullSyncData(TRAILER_SYNC_DATA *trSync)
{
	VEHICLEID TrailerID = m_icSync.TrailerID;
	if (!TrailerID) return;

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	CVehicle *pVehicle = NULL;

	if (pVehiclePool) pVehicle = pVehiclePool->GetAt(TrailerID);
	if (pVehicle) 
	{
		MATRIX4X4 matWorld;

		memcpy(&matWorld.pos, &trSync->vecPos, sizeof(VECTOR));
		DecompressNormalVector(&matWorld.up, &trSync->cvecDirection);
		DecompressNormalVector(&matWorld.right, &trSync->cvecRoll);
		
		pVehicle->SetMatrix(matWorld);
		pVehicle->SetMoveSpeedVector(trSync->vecMoveSpeed);
	}
}

//----------------------------------------------------

BOOL CRemotePlayer::Spawn(BYTE byteTeam, int iSkin, VECTOR * vecPos, float fRotation, DWORD dwColor, BYTE byteFightingStyle, BOOL bVisible)
{
	if(!pGame->IsGameLoaded()) return FALSE;

	if(m_pPlayerPed != NULL) {
		m_pPlayerPed->Destroy();
		delete m_pPlayerPed;
	}

	CPlayerPed *pPlayer = pGame->NewPlayer(iSkin,vecPos->X,vecPos->Y,vecPos->Z,fRotation);
	
	if(pPlayer)
	{		
		if(dwColor!=0) SetPlayerColor(dwColor);
		if(pNetGame->m_bShowPlayerMarkers) pPlayer->ShowMarker(m_PlayerID);

		m_pPlayerPed = pPlayer;
		m_fReportedHealth = 100.0f;
		pPlayer->SetKeys(0,0,0);
		pPlayer->SetFightingStyle(byteFightingStyle);
		pPlayer->SetVisible(bVisible);

		SetState(PLAYER_STATE_SPAWNED);

		return TRUE;
	}

	SetState(PLAYER_STATE_NONE);
	return FALSE;
}

//----------------------------------------------------

void CRemotePlayer::Remove()
{
	if(m_pPlayerPed != NULL) {
		m_pPlayerPed->Destroy();
		delete m_pPlayerPed;
		m_pPlayerPed = NULL;
	}
	Deactivate();
}

//----------------------------------------------------

void CRemotePlayer::ResetAllSyncAttributes()
{	
	m_VehicleID = 0;
	memset(&m_ofSync,0,sizeof(ONFOOT_SYNC_DATA));
	memset(&m_icSync,0,sizeof(INCAR_SYNC_DATA));
	memset(&m_psSync,0,sizeof(PASSENGER_SYNC_DATA));
	memset(&m_aimSync,0,sizeof(AIM_SYNC_DATA));
	m_fReportedHealth = 0.0f;
	m_fReportedArmour = 0.0f;
	m_pCurrentVehicle = NULL;
	m_byteSeatID = 0;
	m_bPassengerDriveByMode = FALSE;
	m_iIsInAModShop = 0;
}

//----------------------------------------------------

void CRemotePlayer::HandleDeath()
{
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer *pLocalPlayer = NULL;

	if (pPlayerPool) pLocalPlayer = pPlayerPool->GetLocalPlayer();

	if (pLocalPlayer) {
		if (pLocalPlayer->IsSpectating() && pLocalPlayer->m_SpectateID == m_PlayerID) {
				//pLocalPlayer->ToggleSpectating(FALSE);
		}
	}

	if(m_pPlayerPed) {
		m_pPlayerPed->SetKeys(0,0,0);
		m_pPlayerPed->SetDead();
	}

	// Dead weapon pickups
	if (m_pPlayerPed && m_pPlayerPed->GetDistanceFromLocalPlayerPed() <= 100.0f)
	{
		MATRIX4X4 matPlayer;
		m_pPlayerPed->GetMatrix(&matPlayer);

		WEAPON_SLOT_TYPE * WeaponSlot = m_pPlayerPed->GetCurrentWeaponSlot();

		if (WeaponSlot->dwType != 0 &&
			WeaponSlot->dwType != WEAPON_GRENADE &&
			WeaponSlot->dwType != WEAPON_TEARGAS &&
			WeaponSlot->dwType != WEAPON_MOLTOV)
		{
			if(pNetGame->GetPickupPool()) {
				pNetGame->GetPickupPool()->New(pGame->GetWeaponModelIDFromWeapon(WeaponSlot->dwType),
					matPlayer.pos.X, matPlayer.pos.Y, matPlayer.pos.Z, WeaponSlot->dwAmmoInClip, m_PlayerID);
			}
		}
	}

	SetState(PLAYER_STATE_WASTED);
	ResetAllSyncAttributes();
}

//----------------------------------------------------

void CRemotePlayer::Say(unsigned char *szText)
{
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if (pPlayerPool) {
		char * szPlayerName = pPlayerPool->GetPlayerName(m_PlayerID);
		pChatWindow->AddChatMessage(szPlayerName,GetPlayerColorAsARGB(),(char*)szText);
	}
}


//----------------------------------------------------

void CRemotePlayer::Privmsg(char *szText)
{
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if (pPlayerPool) {
		CHAR szStr[256];
		sprintf(szStr, "PM from %s(%d): %s", pPlayerPool->GetPlayerName(m_PlayerID), m_PlayerID, szText);
		pChatWindow->AddClientMessage(D3DCOLOR_ARGB(255,220,24,26), szStr);
	}
}


//----------------------------------------------------

void CRemotePlayer::TeamPrivmsg(char *szText)
{
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if (pPlayerPool) {
		CHAR szStr[256];
		sprintf(szStr, "Team PM from %s(%d): %s", pPlayerPool->GetPlayerName(m_PlayerID), m_PlayerID, szText);
		pChatWindow->AddClientMessage(D3DCOLOR_ARGB(255,220,24,26), szStr);
	}
}


//----------------------------------------------------

float CRemotePlayer::GetDistanceFromRemotePlayer(CRemotePlayer *pFromPlayer)
{
	MATRIX4X4	matFromPlayer;
	MATRIX4X4	matThisPlayer;
	CPlayerPed *pPlayerPed = NULL;

	float		fSX,fSY,fSZ;

	if(!m_pPlayerPed) return 10000.0f; // very far away
	
	m_pPlayerPed->GetMatrix(&matThisPlayer);
	pPlayerPed = pFromPlayer->GetPlayerPed();

	if (pPlayerPed) {
		pPlayerPed->GetMatrix(&matFromPlayer);
		fSX = (matThisPlayer.pos.X - matFromPlayer.pos.X) * (matThisPlayer.pos.X - matFromPlayer.pos.X);
		fSY = (matThisPlayer.pos.Y - matFromPlayer.pos.Y) * (matThisPlayer.pos.Y - matFromPlayer.pos.Y);
		fSZ = (matThisPlayer.pos.Z - matFromPlayer.pos.Z) * (matThisPlayer.pos.Z - matFromPlayer.pos.Z);
		return (float)sqrt(fSX + fSY + fSZ);	
	} else {
		return 10000.0f;
	}
}

//----------------------------------------------------

float CRemotePlayer::GetDistanceFromLocalPlayer()
{
	if(!m_pPlayerPed) return 10000.0f; // very far away

	if(GetState() == PLAYER_STATE_DRIVER && m_pCurrentVehicle) {
		return m_pCurrentVehicle->GetDistanceFromLocalPlayerPed();
	} else {
		return m_pPlayerPed->GetDistanceFromLocalPlayerPed();
	}
}

//----------------------------------------------------

void CRemotePlayer::SetPlayerColor(DWORD dwColor)
{
	SetRadarColor(m_PlayerID,dwColor);	
}

//----------------------------------------------------

DWORD CRemotePlayer::GetPlayerColorAsRGBA()
{
	return TranslateColorCodeToRGBA(m_PlayerID);
}

//----------------------------------------------------

DWORD CRemotePlayer::GetPlayerColorAsARGB()
{
	return (TranslateColorCodeToRGBA(m_PlayerID) >> 8) | 0xFF000000;	
}

//----------------------------------------------------

void CRemotePlayer::EnterVehicle(VEHICLEID VehicleID, BOOL bPassenger)
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(m_pPlayerPed && !m_pPlayerPed->IsInVehicle()) {

		if(bPassenger) {
			SetState(PLAYER_STATE_ENTER_VEHICLE_PASSENGER);
		} else {
			SetState(PLAYER_STATE_ENTER_VEHICLE_DRIVER);
		}
		
		m_pPlayerPed->SetKeys(0,0,0);
		if(m_pPlayerPed->GetDistanceFromLocalPlayerPed() < 120.0f) {
			int iGtaVehicleID = pVehiclePool->FindGtaIDFromID(VehicleID);
			if(iGtaVehicleID && iGtaVehicleID != INVALID_VEHICLE_ID) {	
				m_pPlayerPed->EnterVehicle(iGtaVehicleID,bPassenger);
			}
		}
	}
	m_dwWaitForEntryExitAnims = GetTickCount() + 500;
}

//----------------------------------------------------

void CRemotePlayer::ExitVehicle()
{
	if(m_pPlayerPed && m_pPlayerPed->IsInVehicle()) {
		SetState(PLAYER_STATE_EXIT_VEHICLE);
		m_pPlayerPed->SetKeys(0,0,0);
		if(m_pPlayerPed->GetDistanceFromLocalPlayerPed() < 120.0f) {
			m_pPlayerPed->ExitCurrentVehicle();
		}
	}
	m_dwWaitForEntryExitAnims = GetTickCount() + 500;
}

//----------------------------------------------------
// callback. called when this player's state changes.

void CRemotePlayer::StateChange(BYTE byteNewState, BYTE byteOldState)
{

	if(byteNewState == PLAYER_STATE_DRIVER && byteOldState == PLAYER_STATE_ONFOOT)
	{
		// If their new vehicle is the one the local player
		// is driving, we'll have to kick the local player out
		CPlayerPed *pLocalPlayerPed = pGame->FindPlayerPed();
		CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
		VEHICLEID LocalVehicle=0xFFFF;
		MATRIX4X4 mat;

		if(pLocalPlayerPed && pLocalPlayerPed->IsInVehicle() && !pLocalPlayerPed->IsAPassenger() && pVehiclePool)
		{
			LocalVehicle = pVehiclePool->FindIDFromGtaPtr(pLocalPlayerPed->GetGtaVehicle());
			if(LocalVehicle == m_VehicleID) {
				pLocalPlayerPed->GetMatrix(&mat);
				pLocalPlayerPed->RemoveFromVehicleAndPutAt(mat.pos.X,mat.pos.Y,mat.pos.Z + 1.0f);	
				pGame->DisplayGameText("~r~Car Jacked~w~!",1000,5);
			}
		}
	}	
}

//----------------------------------------------------
