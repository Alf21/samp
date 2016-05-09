#pragma once
#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <d3d9.h>
#include <d3dx9.h>

class CScoreBoard
{
private:
	IDirect3DDevice9* m_pDevice;
	IDirect3DStateBlock9* m_pOldStates;
	IDirect3DStateBlock9* m_pNewStates;
	ID3DXFont*	m_pFont;
	ID3DXSprite* m_pSprite;
	float m_fScalar;
	float m_fScreenOffsetX;
	float m_fScreenOffsetY;
public:
	int m_iOffset;
	BOOL m_bSorted;

	CScoreBoard(IDirect3DDevice9* pDevice, BOOL bScaleToScreen);
	~CScoreBoard();

	void Draw();
	void DeleteDeviceObjects();
	void RestoreDeviceObjects();
};

#endif