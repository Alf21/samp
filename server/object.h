/*
Leaked by ZYRONIX.net.
*/

#ifndef SAMPSRV_OBJECT_H
#define SAMPSRV_OBJECT_H

//----------------------------------------------------

class CObject
{
public:

	BYTE					m_byteObjectID;
	int						m_iModel;
	BOOL					m_bIsActive;
	MATRIX4X4				m_matWorld;
	MATRIX4X4				m_matTarget;
	BYTE					m_byteMoving;
	float					m_fMoveSpeed;
	float					m_fRotation;

	CObject(int iModel, VECTOR * vecPos, VECTOR * vecRot);
	~CObject(){};

	BOOL IsActive() { return m_bIsActive; }

	void SetID(BYTE byteObjectID) { m_byteObjectID = byteObjectID; };

	void SpawnForPlayer(BYTE byteForPlayerID);
	int Process(float fElapsedTime);
	void Stop() { m_byteMoving &= ~1; };
	float DistanceRemaining();
	
	float MoveTo(float X, float Y, float Z, float speed);
};

#endif