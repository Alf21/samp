#pragma once

/*
Leaked by ZYRONIX.net.
*/

//----------------------------------------------------

class CGangZonePool
{
private:
	float			m_fGangZone[MAX_GANG_ZONES][4];
	BOOL			m_bSlotState[MAX_GANG_ZONES];
public:
	CGangZonePool();
	~CGangZonePool() {};
	WORD New(float fMinX, float fMinY, float fMaxX, float fMaxY);
	void Delete(WORD wZone);
	void ShowForPlayer(BYTE bytePlayer, WORD wZone, DWORD dwColor);
	void ShowForAll(WORD wZone, DWORD dwColor);
	void HideForPlayer(BYTE bytePlayer, WORD wZone);
	void HideForAll(WORD wZone);
	void FlashForPlayer(BYTE bytePlayer, WORD wZone, DWORD dwColor);
	void FlashForAll(WORD wZone, DWORD dwColor);
	void StopFlashForPlayer(BYTE bytePlayer, WORD wZone);
	void StopFlashForAll(WORD wZone);
	BOOL GetSlotState(WORD wZone)
	{
		if (wZone >= MAX_GANG_ZONES) return FALSE;
		return m_bSlotState[wZone];
	};
};

//----------------------------------------------------
