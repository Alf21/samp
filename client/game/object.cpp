#include "../main.h"
#include "util.h"
#include "playerped.h"

extern CGame		*pGame;
extern CNetGame		*pNetGame;
extern CChatWindow  *pChatWindow;

CObject::CObject(int iModel, float fPosX, float fPosY, float fPosZ, VECTOR vecRot)
{

	DWORD dwRetID	= 0;
	m_pEntity		= 0;
	m_dwGTAId		= 0;

	ScriptCommand(&create_object, iModel, fPosX, fPosY, fPosZ, &dwRetID);
	ScriptCommand(&put_object_at, dwRetID, fPosX, fPosY, fPosZ);
	
	m_pEntity	=	GamePool_Object_GetAt(dwRetID);
	m_dwGTAId	=	dwRetID;
	m_byteMoving = 0;
	m_fMoveSpeed = 0.0;

	m_bIsPlayerSurfing = false;

	InstantRotate(vecRot.X, vecRot.Y, vecRot.Z);
}

CObject::~CObject()
{
	m_pEntity	=	GamePool_Object_GetAt(m_dwGTAId);
	if (m_pEntity)
	{
		ScriptCommand(&destroy_object, m_dwGTAId);
	}
}


void CObject::Process(float fElapsedTime)
{
	if (m_byteMoving & 1)
	{
		// Calculate new position based on elapsed time (interpolate)
		// distance = speed * time
		// time = fElapsedTime
		// speed = m_fMoveSpeed
		MATRIX4X4 matPos;
		GetMatrix(&matPos);

		m_matCurrent = matPos;

		float distance = fElapsedTime * m_fMoveSpeed;
		float remaining = DistanceRemaining(&matPos);
		if (distance >= remaining)
		{
			m_byteMoving &= ~1; // Stop it moving
			// Force the final location so we don't overshoot slightly
			TeleportTo(m_matTarget.pos.X, m_matTarget.pos.Y, m_matTarget.pos.Z);
		}
		else
		{

			// Else interpolate the new position between the current and final positions
			remaining /= distance; // Calculate ratio

			m_vPlayerSurfOffs.X = (m_matTarget.pos.X - matPos.pos.X) / remaining;
			m_vPlayerSurfOffs.Y = (m_matTarget.pos.Y - matPos.pos.Y) / remaining;
			m_vPlayerSurfOffs.Z = (m_matTarget.pos.Z - matPos.pos.Z) / remaining;

			matPos.pos.X += m_vPlayerSurfOffs.X;
			matPos.pos.Y += m_vPlayerSurfOffs.Y;
			matPos.pos.Z += m_vPlayerSurfOffs.Z;

			TeleportTo(matPos.pos.X, matPos.pos.Y, matPos.pos.Z);

			// basic object surfing - local player
			CPlayerPed *pPed = pGame->FindPlayerPed();
			if (pPed) {
				if (pPed->GetGtaContactEntity() == m_pEntity) {
					PED_TYPE *pPedType = pPed->m_pPed;
					if ( (pPedType) && (pPed->IsOnGround()) ) {
						m_bIsPlayerSurfing=true;
						MATRIX4X4 *matPlayer = pPed->m_pPed->entity.mat;
						matPlayer->pos.X += m_vPlayerSurfOffs.X;
						matPlayer->pos.Y += m_vPlayerSurfOffs.Y;
						matPlayer->pos.Z += m_vPlayerSurfOffs.Z;
						pPed->SetMatrix(*matPlayer);
					} else { // player fell off object or jumped
						m_bIsPlayerSurfing=false;
					}
				} else { // contact entity does not match this object
					m_bIsPlayerSurfing=false;
				}
			}

			// basic object surfing - remote player (this method sucks, moving onto remoteplayer)
/*			for (int i=1; i<=MAX_PLAYERS-1; i++)
			{
				CRemotePlayer *pRemotePlayer = pNetGame->GetPlayerPool()->GetAt(i);
				if (pRemotePlayer) {
					CPlayerPed *pPed = pRemotePlayer->GetPlayerPed();
					if (pPed) {
						if (pPed->GetGtaContactEntity() == m_pEntity) {
							VECTOR vMoveSpeed;
							VECTOR vMoveTo;
							pPed->GetMoveSpeedVector(&vMoveSpeed);
							MATRIX4X4 *matPlayer = NULL;
							PED_TYPE *pPedType = pPed->m_pPed;
							if ((pPedType)&&(pPed->IsOnGround())) {
								matPlayer = pPed->m_pPed->entity.mat;
								matPlayer->pos.X += (m_matTarget.pos.X - matPlayer->pos.X) / remaining;
								matPlayer->pos.Y += (m_matTarget.pos.Y - matPlayer->pos.Y) / remaining;
								matPlayer->pos.Z += (m_matTarget.pos.Z - matPlayer->pos.Z) / remaining;
								pPed->SetMatrix(*matPlayer);

							}
						}
					}
				}
			}*/

		}
	}
}

float CObject::DistanceRemaining(MATRIX4X4* matPos)
{
	float	fSX,fSY,fSZ;
	fSX = (matPos->pos.X - m_matTarget.pos.X) * (matPos->pos.X - m_matTarget.pos.X);
	fSY = (matPos->pos.Y - m_matTarget.pos.Y) * (matPos->pos.Y - m_matTarget.pos.Y);
	fSZ = (matPos->pos.Z - m_matTarget.pos.Z) * (matPos->pos.Z - m_matTarget.pos.Z);
	return (float)sqrt(fSX + fSY + fSZ);
}

void CObject::MoveTo(float X, float Y, float Z, float speed)
{
	m_matTarget.pos.X = X;
	m_matTarget.pos.Y = Y;
	m_matTarget.pos.Z = Z;
	m_fMoveSpeed = speed;
	m_byteMoving |= 1;
}

void CObject::InstantRotate(float X, float Y, float Z)
{
	ScriptCommand(&set_object_rotation, m_dwGTAId, X, Y, Z);
}
