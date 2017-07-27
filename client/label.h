#pragma once
#ifndef LABEL_H
#define LABEL_H

#include <d3d9.h>
#include <d3dx9.h>

class CLabel
{
private:
	IDirect3DDevice9* m_pDevice;
	ID3DXFont* m_pFont;

	char* m_szFontFace;
	float m_fSize;
	bool m_bFontBold;
public:
	CLabel(IDirect3DDevice9* pDevice, char* szFontFace, bool bFontBold, float fSize = 12);
	~CLabel();

	void Draw(D3DXVECTOR3* ppos, char* szText, DWORD dwColor);

	void SetFont(char *szFontFace);
	void SetSize(float fSize);

	void DeleteDeviceObjects();
	void RestoreDeviceObjects();
};

#endif