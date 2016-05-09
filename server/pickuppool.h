/*
Leaked by ZYRONIX.net.
*/

#ifndef SAMPSRV_PICKUPPOOL_H
#define SAMPSRV_PICKUPPOOL_H

#define MAX_PICKUPS 400

#pragma pack(1)
typedef struct _PICKUP
{
	int iModel;
	int iType;
	float fX;
	float fY;
	float fZ;
} PICKUP;

//----------------------------------------------------

class CPickupPool
{
private:

	PICKUP  m_Pickups[MAX_PICKUPS];
	int		m_iPickupCount;
	BYTE	m_bActive[MAX_PICKUPS];

public:
	
	CPickupPool() {
		m_iPickupCount = 0;
		for (int i = 0; i < MAX_PICKUPS; i++)
		{
			m_bActive[i] = false;
		}
	};

	~CPickupPool() {};

	int New(int iModel, int iType, float fX, float fY, float fZ, BYTE staticp = 0);
	int Destroy(int iPickup);
	void InitForPlayer(BYTE bytePlayerID);
};

//----------------------------------------------------

#endif

