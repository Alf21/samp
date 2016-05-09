#include "main.h"
#include <stdio.h>

extern CGame* pGame;
extern CNetGame* pNetGame;
extern GAME_SETTINGS tSettings;
extern CChatWindow *pChatWindow;
extern CFontRender *pDefaultFont;

char szDispBuf[16384];
char szStatBuf[16384];
char szDrawLine[1024];

CNetStats::CNetStats(IDirect3DDevice9 *pD3DDevice)
{
	m_dwLastUpdateTick = GetTickCount();
	m_dwLastTotalBytesRecv = 0;
	m_dwLastTotalBytesSent = 0;
	m_dwBPSDownload = 0;
	m_dwBPSUpload = 0;
	m_pD3DDevice = pD3DDevice;
}

void CNetStats::Draw()
{
	RakNetStatisticsStruct *pRakStats = pNetGame->GetRakClient()->GetStatistics();
	float fDown,fUp;

	if((GetTickCount() - m_dwLastUpdateTick) > 1000) {
		m_dwLastUpdateTick = GetTickCount();
		
		m_dwBPSDownload = ((UINT)(pRakStats->bitsReceived / 8)) - m_dwLastTotalBytesRecv;
		m_dwLastTotalBytesRecv = (UINT)(pRakStats->bitsReceived / 8);

		m_dwBPSUpload = ((UINT)(pRakStats->totalBitsSent / 8)) - m_dwLastTotalBytesSent;
		m_dwLastTotalBytesSent = (UINT)(pRakStats->totalBitsSent / 8);
	}

	if(m_dwBPSDownload != 0) {
		fDown = (float)m_dwBPSDownload / 1024;
	} else {
		fDown = 0.0f;
	}

	if(m_dwBPSUpload != 0) {
		fUp = (float)m_dwBPSUpload / 1024;
	} else {
		fUp = 0.0f;
	}

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	CPlayerPed *pPlayerPed = pLocalPlayer->GetPlayerPed();
	int iPlayersInRange = pLocalPlayer->DetermineNumberOfPlayersInLocalRange();
	if(!iPlayersInRange) iPlayersInRange = 20;

	sprintf(szDispBuf,"Download Rate: %.2f kbps\nUpload Rate: %.2f kbps\n",
		fDown,fUp);

	// VEHICLE COUNTING STUFF
	int x=0, iVehiclesCount=0, iVehicleModelsCount=0;
	BYTE bModelInUse[212];
	memset(&bModelInUse[0],0,212);
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	char szVehicleStats[256];

	if(pVehiclePool) {
		while(x!=MAX_VEHICLES) {
			if(pVehiclePool->GetSlotState(x)) {
				iVehiclesCount++;
				if(pVehiclePool->GetAt(x)->m_pVehicle) {
					bModelInUse[pVehiclePool->GetAt(x)->m_pVehicle->entity.nModelIndex - 400]++;
				}
			}
			x++;
		}
		x=0;
		while(x!=212) {
			if(bModelInUse[x]) iVehicleModelsCount++;
			x++;
		}
		sprintf(szVehicleStats,"Vehicles: %u\nVehicle Models: %u\n",iVehiclesCount,iVehicleModelsCount);
		strcat(szDispBuf,szVehicleStats);
	}

	if(pPlayerPed) {
		if(!pPlayerPed->IsInVehicle()) {
			sprintf(szStatBuf,"OnFoot Send Rate: %u\n",pLocalPlayer->GetOptimumOnFootSendRate(iPlayersInRange));
		} else {
			sprintf(szStatBuf,"InCar Send Rate: %u\n",pLocalPlayer->GetOptimumInCarSendRate(iPlayersInRange));
		}
		strcat(szDispBuf,szStatBuf);
	}

#ifdef _DEBUG
	StatisticsToString(pRakStats,szStatBuf,2);
#else
	StatisticsToString(pRakStats,szStatBuf,1);
#endif
	strcat(szDispBuf,szStatBuf);

	//m_pD3DDevice->GetDisplayMode(0,&dDisplayMode);

	SIZE size = pDefaultFont->MeasureText(szDispBuf);

	RECT rect;
	rect.top		= 10;
	rect.right		= pGame->GetScreenWidth() - 150;
	rect.left		= 10;
	rect.bottom		= rect.top + 16;

	PCHAR pBuf = szDispBuf;
	
	x=0;

	pDefaultFont->RenderText("Client Network Stats",rect,0xFF0000AA);
	rect.top += 16;
	rect.bottom += 16;

	while(*pBuf) {
		szDrawLine[x] = *pBuf;
		if(szDrawLine[x] == '\n') {
			szDrawLine[x] = '\0';
			pDefaultFont->RenderText(szDrawLine,rect,0xFFFFFFFF);
			rect.top += 16;
			rect.bottom += 16;
			x=0;
		} else {
			x++;
		}
		pBuf++;
	}
}
