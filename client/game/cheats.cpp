#include "../main.h"
#include "util.h"
#include <process.h>
#include <Tlhelp32.h>

#define WEAPON_DATA_SIZE (28*13)

int iMoneyStored;
BYTE byteWeaponData[WEAPON_DATA_SIZE];
DWORD dwHealth;
DWORD dwArmour;
MATRIX4X4 matStore;

DWORD dwCamTargetEnt1;
DWORD dwCamTargetEnt2;

extern CGame *pGame;
extern CChatWindow *pChatWindow;

int iTimesDataModified = 0;
extern BOOL bFirstSpawn;

void UnFuck(DWORD,int);

extern CChatWindow *pChatWindow;
extern CNetGame *pNetGame;

int iDupeD3D9DllsWasVerified=0;

DWORD dwD3D9DllBaseAddr=0;
DWORD dwD3D9DllSize=0;

//----------------------------------------------------------

void DoCheatDataStoring()
{
	if(!pNetGame) return;

	PED_TYPE *pPed = pGame->FindPlayerPed()->m_pPed;

	// Store the current money
	iMoneyStored = pGame->GetLocalMoney();
	_asm mov edx, iMoneyStored
	_asm ror edx, 5
	_asm mov iMoneyStored, edx

	// Store the weapon data
	memcpy(byteWeaponData,&pPed->WeaponSlots[0],WEAPON_DATA_SIZE);

	// Store the health and armour
	memcpy(&dwHealth,&pPed->fHealth,4);
	_asm mov edx, dwHealth
	_asm ror edx, 5
	_asm mov dwHealth, edx

	memcpy(&dwArmour,&pPed->fArmour,4);
	_asm mov edx, dwArmour
	_asm ror edx, 5
	_asm mov dwArmour, edx

	// Store the player matrix
	memcpy(&matStore,pPed->entity.mat,sizeof(MATRIX4X4));

	// Fuck with the Camera's target ent since
	// cheats use it to obtain the player addr.
	dwCamTargetEnt1 = *(PDWORD)0xB6F3B8;
	*(PDWORD)0xB6F3B8 = 0;

	dwCamTargetEnt2 = *(PDWORD)0xB6F5F0;
	*(PDWORD)0xB6F5F0 = 0;
}

//----------------------------------------------------------

void CheckDuplicateD3D9Dlls()
{
    HANDLE        hModuleSnap = NULL; 
	MODULEENTRY32 me32;
	int iFoundD3D9Dll=40736;
	char szModule[1024];
	DWORD dwCurImageSize=0;
	DWORD dwCurDllBase=0;

	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId()); 

	if(hModuleSnap == INVALID_HANDLE_VALUE) {
		iFoundD3D9Dll = 347983;
		return;
	}

	me32.dwSize = sizeof(MODULEENTRY32);

	if(!Module32First(hModuleSnap,&me32))
	{
		CloseHandle( hModuleSnap );     // Must clean up the snapshot object!
		iFoundD3D9Dll = 348403;
		return;
	}

	do
	{
		strcpy(szModule,me32.szModule);
		dwCurImageSize = me32.modBaseSize;
		dwCurDllBase = (DWORD)me32.modBaseAddr;
		
		if(		szModule[4] == '.'
			&&	szModule[6] == 'l'
			&&	szModule[7] == 'l'
			&&	szModule[1] == '3'
			&&	szModule[3] == '9'		
			&&	szModule[0] == 'd'
			&&	szModule[2] == 'd'
			&&	szModule[5] == 'd' ) {
				iFoundD3D9Dll++;
				dwD3D9DllSize = dwCurImageSize;
				dwD3D9DllBaseAddr = dwCurDllBase;

		}
	} while( Module32Next( hModuleSnap, &me32 ) );

	iDupeD3D9DllsWasVerified = 1;
	CloseHandle( hModuleSnap );

	// If there is more than 1 d3d9.dll or its image size is less than
	// 0x00100000 FORCE_EXIT.
	if(iFoundD3D9Dll >= 40738 || dwD3D9DllSize < 0x00100000) {
		FORCE_EXIT( 348403 );
	}

	return;
}

//----------------------------------------------------------

void DoCheatDataComparing()
{
	if(!pNetGame) return;

	PED_TYPE *pPed = pGame->FindPlayerPed()->m_pPed;
	DWORD dwTempHealth;
	DWORD dwTempArmour;

	// Repair the camera's target entity.
	*(PDWORD)0xB6F3B8 = dwCamTargetEnt1;
	*(PDWORD)0xB6F5F0 = dwCamTargetEnt2;

	if(!bFirstSpawn && !pGame->IsMenuActive()) {

		// MONEY CHECK
		_asm mov edx, iMoneyStored
		_asm rol edx, 5
		_asm mov iMoneyStored, edx

		if(pGame->GetLocalMoney() != iMoneyStored) {
			//if(pChatWindow) pChatWindow->AddDebugMessage("Money modified");
			iTimesDataModified++;
		}

		// WEAPON DATA CHECK
		if(memcmp(&pPed->WeaponSlots[0],byteWeaponData,WEAPON_DATA_SIZE) != 0) {
			//if(pChatWindow) pChatWindow->AddDebugMessage("Weapon Data modified");
			iTimesDataModified++;
		}

		// HEALTH CHECK
		memcpy(&dwTempHealth,&pPed->fHealth,4);

		_asm mov edx, dwHealth
		_asm rol edx, 5
		_asm mov dwHealth, edx

		if(dwTempHealth != dwHealth) {
			//if(pChatWindow) pChatWindow->AddDebugMessage("Health modified");
			iTimesDataModified++;
		}
	
		// ARMOUR CHECK
		memcpy(&dwTempArmour,&pPed->fArmour,4);

		_asm mov edx, dwArmour
		_asm rol edx, 5
		_asm mov dwArmour, edx

		if(dwTempArmour != dwArmour) {
			//if(pChatWindow) pChatWindow->AddDebugMessage("Armour modified");
			iTimesDataModified++;
		}

		// PLAYER MATRIX CHECK
		if(memcmp(&matStore,pPed->entity.mat,sizeof(MATRIX4X4)) != 0) {
			//if(pChatWindow) pChatWindow->AddDebugMessage("Matrix modified");
			iTimesDataModified++;
		}

	}
}

//----------------------------------------------------------