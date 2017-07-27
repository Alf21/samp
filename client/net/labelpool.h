#pragma once

typedef struct _E_LABEL_INFO {
	VECTOR vecPosition;
	float fDistance;
	DWORD dwColour;
	char *szFontFace;
	int	iAttachedPlayerID;
	char *szText;
	float fFontSize;
} LABEL_INFO;

class CLabelPool 
{
private:
	BOOL			m_bLabelSlotState[MAX_LABELS];
	CLabel			*m_pLabels[MAX_LABELS];
	LABEL_INFO		*m_pLabelInfo[MAX_LABELS];

public:

	CLabelPool();
	~CLabelPool();

	BOOL New(LABEL_INFO *pNewLabel);
	BOOL Delete(int iLabelID);
	void Draw();

	CLabel *GetAt(int iLabelID) {
		if (iLabelID >= MAX_LABELS) return FALSE;
		return m_pLabels[iLabelID];
	}

	BOOL GetSlotState(int iLabelID) {
		if (iLabelID >= MAX_LABELS) return FALSE;
		return m_bLabelSlotState[iLabelID];
	}

	void SetLabelColour(int iLabelID, DWORD dwColour);
	void SetLabelText(int iLabelID, char *szText);
	void SetLabelPosition(int iLabelID, VECTOR vPosition);
	void SetLabelFontName(int iLabelID, char *szFontName);
	//void SetLabelAttachedPlayer(int iLabelID, BYTE iPlayerID); todo
	void SetLabelSize(int iLabelID, float fSize);

	void DeleteDeviceObjects();
	void RestoreDeviceObjects();
	
	float GetDistanceFromPlayerPed(int iLabelID);
};