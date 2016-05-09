#pragma once

#include "game.h"

//----------------------------------------------------------

class CEntity
{
public:
	CEntity() {};
	virtual ~CEntity() {};

	void  GetMatrix(PMATRIX4X4 Matrix);
	void  SetMatrix(MATRIX4X4 Matrix);
	void  GetMoveSpeedVector(PVECTOR Vector);
	void  SetMoveSpeedVector(VECTOR Vector);
	void  GetTurnSpeedVector(PVECTOR Vector);
	void  SetTurnSpeedVector(VECTOR Vector);
	UINT  GetModelIndex();
	void  SetModelIndex(UINT uiModel);
	void  TeleportTo(float x, float y, float z);
	float GetDistanceFromLocalPlayerPed();
	
	float GetDistanceFromPoint(float X, float Y, float Z);
	BOOL  IsStationary();
	void  ApplyMoveSpeed();
	
	BOOL  EnforceWorldBoundries(float fPX, float fZX, float fPY, float fNY);
	BOOL  HasExceededWorldBoundries(float fPX, float fZX, float fPY, float fNY);

	virtual void  Add();
	virtual void  Remove();
	BOOL  IsAdded();

	ENTITY_TYPE *m_pEntity;
	DWORD		m_dwGTAId;
};

//----------------------------------------------------------