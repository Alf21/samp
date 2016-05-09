#pragma once

#define MAX_PICKUPS 400

typedef struct _PICKUP
{
	int iModel;
	int iType;
	float fX;
	float fY;
	float fZ;
} PICKUP;

typedef struct _DROPPED_WEAPON
{
	bool bDroppedWeapon;
	PLAYERID fromPlayer;
} DROPPED_WEAPON;

//----------------------------------------------------

class CPickupPool
{
private:

	PICKUP  m_Pickups[MAX_PICKUPS];
	int		m_iPickupCount;
	DWORD	m_dwHnd[MAX_PICKUPS];
	DWORD   m_iTimer[MAX_PICKUPS];
	DROPPED_WEAPON	m_droppedWeapon[MAX_PICKUPS];

public:
	
	CPickupPool() {
		memset(&m_Pickups[0],0,sizeof(PICKUP) * MAX_PICKUPS);
		m_iPickupCount = 0;
		for (int i = 0; i < MAX_PICKUPS; i++)
		{
			m_dwHnd[i] = NULL;
			m_iTimer[i] = NULL;
		}
	};

	~CPickupPool();

	void New(int iModel, float fX, float fY, float fZ, DWORD dwAmmo, PLAYERID DeadPlayer = INVALID_PLAYER_ID);
	void New(PICKUP* pPickup, int iPickup);
	void Destroy(int iPickup);
	void DestroyDropped(PLAYERID playerId);
	void PickedUp(int iPickup);
	void Process();
};

//----------------------------------------------------