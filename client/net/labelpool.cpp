#include "../main.h"
#include "../game/util.h"

extern CNetGame *pNetGame;
extern CGame *pGame;
extern CChatWindow *pChatWindow;

//----------------------------------------------------------------------------------
CLabelPool::CLabelPool() {
	for (int i = 0; i < MAX_LABELS; i++) {
		m_bLabelSlotState[i] = FALSE;
		m_pLabels[i] = NULL;
	}
	memset(&m_pLabelInfo[0], 0, MAX_LABELS);
}

//----------------------------------------------------------------------------------
CLabelPool::~CLabelPool() {
	for (int i = 0; i < MAX_LABELS; i++) {
		Delete(i);
	}
}

//----------------------------------------------------------------------------------
BOOL CLabelPool::New(LABEL_INFO *pNewLabel) {
	int i = 0;

	for (i; i < MAX_LABELS; i++)
		if (!GetSlotState(i)) break;
	
	if (m_pLabels[i] != NULL) return FALSE;

	m_pLabels[i] = new CLabel((IDirect3DDevice9*)pGame->GetD3DDevice(), pNewLabel->szFontFace, true);

	if (!m_pLabels[i]) return FALSE;
	memcpy(m_pLabelInfo, pNewLabel, sizeof(LABEL_INFO));
	return TRUE;
}

//----------------------------------------------------------------------------------
BOOL CLabelPool::Delete(int iLabelID) {
	if (!GetSlotState(iLabelID) || !m_pLabels[iLabelID]) return FALSE;
	m_bLabelSlotState[iLabelID] = FALSE;
	delete m_pLabels[iLabelID];
	m_pLabels[iLabelID] = NULL;
	memcpy(m_pLabelInfo[iLabelID], 0, sizeof(LABEL_INFO));
	return TRUE;
}

//----------------------------------------------------------------------------------
void CLabelPool::Draw() {
	for (int i = 0; i < MAX_LABELS; i++) {
		if (GetSlotState(i) && m_pLabels[i]) {
			float distance = GetDistanceFromPlayerPed(i);
			if (m_pLabelInfo[i]->fDistance >= distance) 
				m_pLabels[i]->Draw(&D3DXVECTOR3(m_pLabelInfo[i]->vecPosition.X, m_pLabelInfo[i]->vecPosition.Y, m_pLabelInfo[i]->vecPosition.Z), m_pLabelInfo[i]->szText, m_pLabelInfo[i]->dwColour);
		}
	}
}

//----------------------------------------------------------------------------------
void CLabelPool::SetLabelColour(int iLabelID, DWORD dwColour) {
	if (!GetSlotState(iLabelID) || !m_pLabels[iLabelID]) return;
	m_pLabelInfo[iLabelID]->dwColour = dwColour;
}

//----------------------------------------------------------------------------------
void CLabelPool::SetLabelText(int iLabelID, char *szText) {
	if (!GetSlotState(iLabelID) || !m_pLabels[iLabelID]) return;
	m_pLabelInfo[iLabelID]->szText = szText;
}

//----------------------------------------------------------------------------------
void CLabelPool::SetLabelFontName(int iLabelID, char *szFontName) {
	if (!GetSlotState(iLabelID) || !m_pLabels[iLabelID]) return;
	m_pLabelInfo[iLabelID]->szFontFace = szFontName;
	m_pLabels[iLabelID]->SetFont(szFontName);
}

//----------------------------------------------------------------------------------
void CLabelPool::SetLabelPosition(int iLabelID, VECTOR vPosition) {
	if (!GetSlotState(iLabelID) || !m_pLabels[iLabelID]) return;
	m_pLabelInfo[iLabelID]->vecPosition = vPosition;
}

//----------------------------------------------------------------------------------
void CLabelPool::SetLabelSize(int iLabelID, float fSize) {
	if (!GetSlotState(iLabelID) || !m_pLabels[iLabelID]) return;
	m_pLabelInfo[iLabelID]->fFontSize = fSize;
}

//----------------------------------------------------------------------------------
void CLabelPool::DeleteDeviceObjects() {
	for (int i = 0; i < MAX_LABELS; i++)  {
		if (GetSlotState(i))
			m_pLabels[i]->DeleteDeviceObjects();
	}
}

//----------------------------------------------------------------------------------
void CLabelPool::RestoreDeviceObjects() {
	for (int i = 0; i < MAX_LABELS; i++)  {
		if (GetSlotState(i))
			m_pLabels[i]->RestoreDeviceObjects();
	}
}

//----------------------------------------------------------------------------------
// Took from CEntity 
float CLabelPool::GetDistanceFromPlayerPed(int iLabelID)
{
	if (!GetSlotState(iLabelID) || !m_pLabels[iLabelID]) return 1000.0f;

	MATRIX4X4	matFromPlayer;
	float		fSX, fSY, fSZ;

	CPlayerPed *pLocalPlayerPed = pGame->FindPlayerPed();
	CLocalPlayer *pLocalPlayer = NULL;

	if (!pLocalPlayerPed) return 10000.0f;


	if (pNetGame) {
		pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
		if (pLocalPlayer && (pLocalPlayer->IsSpectating() || pLocalPlayer->IsInRCMode())) {
			pGame->GetCamera()->GetMatrix(&matFromPlayer);
		}
		else {
			pLocalPlayerPed->GetMatrix(&matFromPlayer);
		}
	}
	else {
		pLocalPlayerPed->GetMatrix(&matFromPlayer);
	}

	fSX = (m_pLabelInfo[iLabelID]->vecPosition.X - matFromPlayer.pos.X) * (m_pLabelInfo[iLabelID]->vecPosition.X - matFromPlayer.pos.X);
	fSY = (m_pLabelInfo[iLabelID]->vecPosition.Y - matFromPlayer.pos.Y) * (m_pLabelInfo[iLabelID]->vecPosition.Y - matFromPlayer.pos.Y);
	fSZ = (m_pLabelInfo[iLabelID]->vecPosition.Z - matFromPlayer.pos.Z) * (m_pLabelInfo[iLabelID]->vecPosition.Z - matFromPlayer.pos.Z);

	return (float)sqrt(fSX + fSY + fSZ);
}
