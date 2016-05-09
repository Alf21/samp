#pragma once
class CNetStats
{
private:

	DWORD m_dwLastTotalBytesSent;
	DWORD m_dwLastTotalBytesRecv;
	DWORD m_dwLastUpdateTick;
	DWORD m_dwBPSUpload;
	DWORD m_dwBPSDownload;
	IDirect3DDevice9 *m_pD3DDevice;

public:
	CNetStats(IDirect3DDevice9 *pD3DDevice);
	~CNetStats() {};

	void Draw();
};
