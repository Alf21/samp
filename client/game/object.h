#pragma once

#include "game.h"
#include "entity.h"
#include "scripting.h"

//----------------------------------------------------

class CObject : public CEntity
{
public:

	MATRIX4X4				m_matTarget;
	MATRIX4X4				m_matCurrent;
	BYTE					m_byteMoving;
	float					m_fMoveSpeed;
	bool					m_bIsPlayerSurfing;

	VECTOR					m_vPlayerSurfOffs; // derive surf offsets

	
	CObject(int iModel, float fPosX, float fPosY, float fPosZ, VECTOR vecRot);
	~CObject();
	
	void Process(float fElapsedTime);
	float DistanceRemaining(MATRIX4X4* matPos);
	//void ToggleCollision(BYTE byteToggle) { ScriptCommand(&toggle_object_collision, m_dwGTAId, byteToggle); };
	
	void MoveTo(float X, float Y, float Z, float speed);
	
	void InstantRotate(float X, float Y, float Z);
};