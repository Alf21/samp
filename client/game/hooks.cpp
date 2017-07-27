#include "../main.h"
#include "game.h"
#include "util.h"
#include "keystuff.h"
#include "aimstuff.h"

extern int iGtaVersion;
extern CNetGame* pNetGame;
extern CGame* pGame;
extern CChatWindow *pChatWindow;

extern DWORD dwGraphicsLoop; // Used for the external dll game loop.
extern DWORD dwGameLoop; // Used for the external dll game loop.

void DoCheatDataComparing();
void StartVehicleEngineAudio(DWORD dwAudioEnt);
void StopVehicleEngineAudio(DWORD dwAudioEnt);

int iSyncedRandomNumber=0;

#define NUDE void _declspec(naked) 

//-----------------------------------------------------------
// Globals which are used to avoid stack frame alteration
// inside the following hook procedures.

GTA_CONTROLSET *pGcsKeys;

DWORD	dwFarClipHookAddr=0;
DWORD	dwFarClipReturnAddr=0;

// used generically
PED_TYPE	*_pPlayer;
VEHICLE_TYPE *_pVehicle;
DWORD		TaskPtr;
VEHICLEID	VehicleID;
BOOL		bUsePassenger=FALSE;
CLocalPlayer *pLocalPlayer;
CVehiclePool *pVehiclePool;
CVehicle	 *pVehicleClass;

BOOL	bIgnoreNextEntry=FALSE;
BOOL	bIgnoreNextExit=FALSE;

int opt1,opt2,opt3,opt4; // for vehicle entry/exit.
 
BYTE	byteInternalPlayer=0;
DWORD	dwCurPlayerActor=0;
BYTE	byteCurPlayer=0;
BYTE	byteSavedCameraMode;
WORD	wSavedCameraMode2;
BYTE	*pbyteCameraMode = (BYTE *)0xB6F1A8;
BYTE	*pbyteCurrentPlayer = (BYTE *)0xB7CD74;
WORD    *wCameraMode2 = (WORD*)0xB6F858;
DWORD	*pdwVehicleEnginePed = (DWORD *)0xB6B990;
DWORD   dwRGBARadar=0;
DWORD	dwStackFrame=0;
DWORD	dwSavedEcx=0;
DWORD	dwRetAddr=0;
int		iRadarColor1=0;
DWORD	dwSavedCheatFn=0;

float   fHealth;

BOOL	bAllowVehicleCreation=FALSE;
BOOL    bVehicleProcessControlLocal=FALSE;

DWORD	vtbl;
DWORD	call_addr;

char s[256];

DWORD dwPedDamagePed=0;
DWORD dwPedDamage1=0;
DWORD dwIgnoreDamage=0;

float fFarClip=1400.0f;
float fBakUnk;
char *vt;

DWORD dwParam1;
DWORD dwParam2;
DWORD dwParamThis;

BYTE byteResetLocalVehicleAudio=0;
DWORD dwAudioEntToReset=0;
VEHICLE_TYPE *pVehicleToReset=0;
DWORD dwStartTime=0;
BYTE pbyteStorePlayerVehicleParams[512];
BYTE byteSavedGearNumber=0;

BYTE	byteTempPlayerID;
CRemotePlayer *pTempRemotePlayer;

//-----------------------------------------------------------
// x86 codes to perform our unconditional jmp for detour entries. 

//BYTE GraphicsLoop_HookJmpCode[]	= {0xFF,0x25,0x2C,0xE2,0x53,0x00,0x90,0x90}; //53E22C

BYTE GameProcess_HookJmpCode[]	= {0xFF,0x25,0xD1,0xBE,0x53,0x00}; //53BED1
BYTE TaskEnterVehicleDriver_HookJmpCode[]	= {0xFF,0x25,0xBB,0x19,0x69,0x00,0x90};//0x6919BB
BYTE TaskExitVehicle_HookJmpCode[]	= {0xFF,0x25,0xBA,0xB8,0x63,0x00,0x90};//0x63B8BA
BYTE RadarTranslateColor_HookJmpCode[] = {0xFF,0x25,0x79,0x4A,0x58,0x00,0x90}; // 584A79
BYTE CheatProcessHook_JmpCode[] = {0xFF,0x25,0xAA,0x85,0x43,0x00,0x90}; // 4385AA
BYTE AddVehicleHook_HookJmpCode[] = {0xFF,0x25,0x33,0x14,0x42,0x00}; // 421433
BYTE SetFarClip_HookJmpCode[] =  {0xFF,0x25,0x61,0x36,0x53,0x00,0x90,0x90,0x90}; // 533661
BYTE CGameShutdown_HookJmpCode[] = {0xFF,0x25,0xF1,0xC8,0x53,0x00,0x90}; // 53C8F1
BYTE PedDamage_HookJmpCode[] = {0xFF,0x25,0xBC,0x5A,0x4B,0x00}; // 4B5ABC
BYTE VehicleAudio_HookJmpCode[] = {0xFF,0x25,0x74,0x22,0x50,0x00,0x90,0x90,0x90,0x90}; // 502274
BYTE GenTaskAlloc_HookJmpCode[] = {0xFF,0x25,0x61,0x38,0x4C,0x00}; // 4C3861
BYTE GetText_HookJmpCode[]	= {0xFF, 0x25, 0x43, 0x00, 0x6A, 0x00, 0x90, 0x90, 0x90}; //, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}; // 0x6A004325
BYTE PedSay_HookJmpCode[]	= {0xFF, 0x25, 0xD8, 0xFF, 0x5E, 0x00, 0x90}; //5EFFD8
BYTE CProjectileInfo_Update_HookJmpCode[] = { 0xFF, 0x25, 0x1B, 0x8B, 0x73, 0x00 }; //00738B1B
BYTE CWeapon__Satchel__Activate_HookJmpCode[] = { 0xFF, 0x25, 0x5B, 0x88, 0x73, 0x00 }; // 0073885B
BYTE SetColor_HookJmpCode[] = {0xFF, 0x25, 0x36, 0xA0, 0x63, 0x00, 0x90 };
BYTE Rand_HookJmpCode[] = {0xFF, 0x25, 0x77, 0xAB, 0x5E, 0x00, 0x90 }; // 5EAB77
BYTE VehicleModel_SetEnvironmentMap_JmpCode[] = { 0xFF, 0x25, 0x39, 0x88, 0x4C, 0x00, 0x90, 0x90 }; // 4C8839

//-----------------------------------------------------------

DWORD dwLastExecTick = 0;

NUDE GameProcessHook()
{
	_asm pushad

	if(pGame && !pGame->IsMenuActive()) {
		if(pNetGame && pNetGame->GetTextDrawPool())	pNetGame->GetTextDrawPool()->Draw();
	}
	
	_asm popad
	
	_asm add esp, 190h
	_asm ret	
}

//-----------------------------------------------------------
// A hook function that switches keys for
// CPlayerPed::ProcessControl(void)

BYTE bytePatchPedRot[6] = { 0xD9,0x96,0x5C,0x05,0x00,0x00 };

NUDE CPlayerPed_ProcessControl_Hook()
{
	_asm mov dwCurPlayerActor, ecx // store the passed actor
	_asm pushad

	_pPlayer = (PED_TYPE *)dwCurPlayerActor;
	byteInternalPlayer = *pbyteCurrentPlayer; // get the current internal player number
	byteCurPlayer = FindPlayerNumFromPedPtr(dwCurPlayerActor); // get the ordinal of the passed actor

	if( dwCurPlayerActor && 
		(byteCurPlayer != 0) &&
		(byteInternalPlayer == 0) ) // not local player and local player's keys set.
	{	
		// key switching
		GameStoreLocalPlayerKeys(); // remember local player's keys
		GameSetRemotePlayerKeys(byteCurPlayer); // set remote player's keys

		// save the internal cammode, apply the context.
		byteSavedCameraMode = *pbyteCameraMode;
		*pbyteCameraMode = GameGetPlayerCameraMode(byteCurPlayer);

		// save the second internal cammode, apply the context
		wSavedCameraMode2 = *wCameraMode2;
		*wCameraMode2 = GameGetPlayerCameraMode(byteCurPlayer);
		if(*wCameraMode2 == 4) *wCameraMode2 = 0;

		// save the camera zoom factor, apply the context
		GameStoreLocalPlayerCameraExtZoom();
		GameSetRemotePlayerCameraExtZoom(byteCurPlayer);

		// aim switching
		GameStoreLocalPlayerAim();
		GameSetRemotePlayerAim(byteCurPlayer);

		*pbyteCurrentPlayer = byteCurPlayer; // Set the internal player to the passed actor

		fHealth = _pPlayer->fHealth;
       
		// call the internal CPlayerPed:ProcessControl
		_asm popad
		_asm mov edx, 0x60EA90
		_asm call edx
		_asm pushad

		// restore the camera modes.
		*pbyteCameraMode = byteSavedCameraMode;
		*wCameraMode2 = wSavedCameraMode2;
		
		// remote the local player's camera zoom factor
		GameSetLocalPlayerCameraExtZoom();

		// restore the local player's keys and the internal ID.
		*pbyteCurrentPlayer = 0;
		GameSetLocalPlayerKeys();
		GameSetLocalPlayerAim();
	}
	else // it's the local player or keys have already been set.
	{

		if( pNetGame &&
			pNetGame->GetPlayerPool()->GetLocalPlayer()->IsActive() && 
			pGame->FindPlayerPed()->HasExceededWorldBoundries(
				pNetGame->m_WorldBounds[0],pNetGame->m_WorldBounds[1],
				pNetGame->m_WorldBounds[2],pNetGame->m_WorldBounds[3]) &&
			(pGame->GetActiveInterior() == 0)
			)
		{

			// Make them jump so they're effected by movespeed
			if(!IN_VEHICLE(_pPlayer) && (_pPlayer->dwStateFlags & 3)) {
				pGcsKeys = GameGetInternalKeys();
				pGcsKeys->wKeys1[14] = 0xFF;
				pGcsKeys->wKeys2[14] = 0;
			}

		}

		// Apply the original code to set ped rot from Cam
		memcpy((PVOID)0x6884C4,bytePatchPedRot,6);

		_asm popad
		_asm mov edx, 0x60EA90
		_asm call edx
		_asm pushad	

		// Reapply the no ped rots from Cam patch
		memset((PVOID)0x6884C4,0x90,6);
	}
	
	_asm popad
	_asm ret
}

//-----------------------------------------------------------
// For switching the camera aim per context

NUDE TaskUseGun_Hook()
{
	//_asm mov dwStackFrame, esp
	_asm mov dwSavedEcx, ecx
	_asm mov eax, [esp+4]
	_asm mov dwCurPlayerActor, eax // store the passed actor

	byteInternalPlayer = *pbyteCurrentPlayer; // get the current internal player number
	byteCurPlayer = FindPlayerNumFromPedPtr(dwCurPlayerActor); // get the ordinal of the passed actor

	//OutputDebugString("TaskUseGun");

	if( dwCurPlayerActor && 
		(byteCurPlayer != 0) &&
		(byteInternalPlayer == 0) ) // not local player and local player's keys set.
	{	
		// key switching
		GameStoreLocalPlayerKeys(); // remember local player's keys
		GameSetRemotePlayerKeys(byteCurPlayer); // set remote player's keys

		// save the internal cammode, apply the context.
		byteSavedCameraMode = *pbyteCameraMode;
		*pbyteCameraMode = GameGetPlayerCameraMode(byteCurPlayer);

		// save the second internal cammode, apply the context
		wSavedCameraMode2 = *wCameraMode2;
		*wCameraMode2 = GameGetPlayerCameraMode(byteCurPlayer);
		if(*wCameraMode2 == 4) *wCameraMode2 = 0;

		// save the camera zoom factor, apply the context
		GameStoreLocalPlayerCameraExtZoom();
		GameSetRemotePlayerCameraExtZoom(byteCurPlayer);

		// aim switching
		GameStoreLocalPlayerAim();
		GameSetRemotePlayerAim(byteCurPlayer);
		*pbyteCurrentPlayer = byteCurPlayer; // Set the internal player to the passed actor

		// call the internal TaskUseGun
		_asm mov ecx, dwSavedEcx
		_asm push dwCurPlayerActor
		_asm mov eax, 0x624ED0
		_asm call eax

		// restore the camera modes, internal id and local player's aim
		*pbyteCameraMode = byteSavedCameraMode;
		*wCameraMode2 = wSavedCameraMode2;

		// remote the local player's camera zoom factor
		GameSetLocalPlayerCameraExtZoom();

		*pbyteCurrentPlayer = 0;
		GameSetLocalPlayerAim();
		GameSetLocalPlayerKeys();
	}
	else // it's the local player or keys have already been set.
	{
		_asm mov ecx, dwSavedEcx
		_asm push dwCurPlayerActor
		_asm mov eax, 0x624ED0
		_asm call eax
	}

	_asm retn 4
}

//-----------------------------------------------------------

NUDE TaskOnFoot1_Hook()
{
	_asm mov eax, [esp+4]
	_asm pushad
	_asm mov dwCurPlayerActor, eax // store the passed actor

	byteInternalPlayer = *pbyteCurrentPlayer; // get the current internal player number
	byteCurPlayer = FindPlayerNumFromPedPtr(dwCurPlayerActor); // get the ordinal of the passed actor

	if( dwCurPlayerActor && 
		(byteCurPlayer != 0) &&
		(byteInternalPlayer == 0) )
	{
		_asm popad
		_asm retn 12
	}
	else // it's the local player or keys have already been set.
	{
		_asm popad
		_asm mov edx, 0x6857E0
		_asm call edx
		_asm ret
	}
}

//-----------------------------------------------------------

NUDE TaskOnFoot2_Hook()
{
    _asm mov eax, [esp+4]
	_asm pushad
	_asm mov dwCurPlayerActor, eax // store the passed actor

	byteInternalPlayer = *pbyteCurrentPlayer; // get the current internal player number
	byteCurPlayer = FindPlayerNumFromPedPtr(dwCurPlayerActor); // get the ordinal of the passed actor

	if( dwCurPlayerActor && 
		(byteCurPlayer != 0) &&
		(byteInternalPlayer == 0) )
	{
		_asm popad
		_asm retn 4
	}
	else // it's the local player or keys have already been set.
	{
		_asm popad
		_asm mov edx, 0x688810
		_asm call edx
		_asm ret
	}
}

//-----------------------------------------------------------

NUDE AllVehicles_ProcessControl_TankTurret_Hook()
{
	_asm mov _pVehicle, ecx
	_asm mov eax, [ecx]
	_asm mov vtbl, eax

	if(!bVehicleProcessControlLocal) // not player's car
	{
		// The player keys are already context switched since
		// this function called from CAutomobile_ProcessControl
		// DON'T CONTEXT SWITCH THE KEYS AGAIN BECAUSE THE CURRENT KEYS WOULD
		// BE THE REMOTE PLAYERS KEYS AND NOT THE LOCAL PLAYERS!!

		// save the internal cammode, apply the context.
		byteSavedCameraMode = *pbyteCameraMode;
		*pbyteCameraMode = GameGetPlayerCameraMode(byteCurPlayer);

		// save the second internal cammode, apply the context
		wSavedCameraMode2 = *wCameraMode2;
		*wCameraMode2 = GameGetPlayerCameraMode(byteCurPlayer);
		if(*wCameraMode2 == 4) *wCameraMode2 = 0;
		
		// aim switching
		GameStoreLocalPlayerAim();
		GameSetRemotePlayerAim(byteCurPlayer);

		// call internal function
		_asm mov ecx, _pVehicle
		_asm mov eax, 0x6AE850 // CAutomobile_ProcessControl_Tank
		_asm call eax
		
		// restore the camera modes, internal id and local player's aim
		*pbyteCameraMode = byteSavedCameraMode;
		*wCameraMode2 = wSavedCameraMode2;

		GameSetLocalPlayerAim();

	}
	else
	{	
		_asm mov ecx, _pVehicle
		_asm mov eax, 0x6AE850 // CAutomobile_ProcessControl_Tank
		_asm call eax
	}

	_asm ret

}


//-----------------------------------------------------------

NUDE AllVehicles_ProcessControl_WaterTurret_Hook()
{
	_asm mov _pVehicle, ecx
	_asm mov eax, [ecx]
	_asm mov vtbl, eax

	if(!bVehicleProcessControlLocal) // not player's car
	{

		// The player keys are already context switched since
		// this function called from CAutomobile_ProcessControl
		// DON'T CONTEXT SWITCH THE KEYS AGAIN BECAUSE THE CURRENT KEYS WOULD
		// BE THE REMOTE PLAYERS KEYS AND NOT THE LOCAL PLAYERS!!

		// save the internal cammode, apply the context.
		byteSavedCameraMode = *pbyteCameraMode;
		*pbyteCameraMode = GameGetPlayerCameraMode(byteCurPlayer);

		// save the second internal cammode, apply the context
		wSavedCameraMode2 = *wCameraMode2;
		*wCameraMode2 = GameGetPlayerCameraMode(byteCurPlayer);
		if(*wCameraMode2 == 4) *wCameraMode2 = 0;
		
		// aim switching
		GameStoreLocalPlayerAim();
		GameSetRemotePlayerAim(byteCurPlayer);

		// call internal function
		_asm push 0
		_asm mov ecx, _pVehicle
		_asm mov eax, 0x729B60 // CAutomobile_ProcessControl_WaterTurret
		_asm call eax
		
		// restore the camera modes, internal id and local player's aim
		*pbyteCameraMode = byteSavedCameraMode;
		*wCameraMode2 = wSavedCameraMode2;

		GameSetLocalPlayerAim();

	}
	else
	{	
		_asm push 0
		_asm mov ecx, _pVehicle
		_asm mov eax, 0x729B60 // CAutomobile_ProcessControl_WaterTurret
		_asm call eax
	}

	_asm retn 4

}

//-----------------------------------------------------------
// These functions process the vehicle audio for all
// vehicles since the R* proc is not condusive to what
// we want.
/*
void StopVehicleEngineAudio(DWORD dwAudioEnt)
{
    _asm mov ecx, dwAudioEnt
	_asm mov edx, 0x4FCF40
	_asm call edx
    _asm mov ecx, dwAudioEnt
	_asm mov [ecx+165], 0
}

//-----------------------------------------------------------
//  Saving this incase I want to reset the vehicle audio banks.
	*(PBYTE)0x4F5762 = Bank;
	*(PBYTE)0x4F577E = Bank;
	*(PBYTE)0x4F57CC = Bank;
	*(PBYTE)0x4F57EA = Bank;
	*(PBYTE)0x4F585A = Bank;
	*(PBYTE)0x4F5876 = Bank;
	*(PBYTE)0x4F58AE = Bank;
	*(PBYTE)0x4F58CC = Bank;
	*(PBYTE)0x4F58F3 = Bank;
	*(PBYTE)0x4F590F = Bank;
	*(PBYTE)0x4F5981 = Bank;
	*(PBYTE)0x4F5992 = Bank;
	*(PBYTE)0x4F59A4 = Bank;
	*(PBYTE)0x4F59B9 = Bank;
	*(PBYTE)0x4F5A31 = Bank;
	*(PBYTE)0x4F5A4D = Bank;
	*(PBYTE)0x4F5A9A = Bank;
	*(PBYTE)0x4F5AB4 = Bank; */

//-----------------------------------------------------------
/*
void StartVehicleEngineAudio(DWORD dwAudioEnt)
{
	_asm mov ecx, dwAudioEnt
	_asm mov edx, 0x4F5700
	_asm call edx
  	_asm mov ecx, dwAudioEnt
	_asm mov [ecx+165], 1
}

//-----------------------------------------------------------

void ProcessVehicleEngineAudio(DWORD dwAudioEnt, VEHICLE_TYPE * pVehicle)
{
	_asm push pVehicle
    _asm mov ecx, dwAudioEnt
	_asm mov edx, 0x501E10
	_asm call edx
}

//-----------------------------------------------------------

void VehicleAudioProcessor(VEHICLE_TYPE *pVehicle)
{
	DWORD dwAudioEnt=0;
	DWORD dwAudioEnt2=0;
	DWORD dwLocalPlayerVeh=0;
	BYTE byteAudioActive=0;
	BYTE byteProcessPlayerAudio=0;
	BYTE byteAccelStateAudio=0;
	PED_TYPE *pPlayerPed = GamePool_FindPlayerPed();
	
	DWORD dwRealVehiclePtr=(DWORD)pVehicle;
	DWORD dwListedVehiclePtr=0;

	BYTE bLocalPlayerDriving = IN_VEHICLE(pPlayerPed) && pPlayerPed->pVehicle &&
		  ((VEHICLE_TYPE *)pPlayerPed->pVehicle)->pDriver == pPlayerPed;

	_asm mov esi, pVehicle
    _asm lea ecx, [esi+312]
	_asm mov dwAudioEnt, ecx
	_asm mov eax, [ecx+4]
	_asm mov dwListedVehiclePtr, eax

	if(!dwAudioEnt) return;

	_asm mov ecx, dwAudioEnt
	_asm mov bl, [ecx+164]
	_asm mov byteAudioActive, bl
	_asm mov bl, [ecx+165]
	_asm mov byteProcessPlayerAudio, bl
	_asm mov bl, [ecx+169]
	_asm mov byteAccelStateAudio, bl

	if(!byteAudioActive) return;

	// Handle engine needs turning off
	if((!pVehicle->pDriver || !IN_VEHICLE(pVehicle->pDriver))
		&& byteProcessPlayerAudio) {
		StopVehicleEngineAudio(dwAudioEnt);
		pChatWindow->AddDebugMessage("StoppingAudio");
	}

	// Handle engine needs turning on
	if( pVehicle->pDriver && 
		IN_VEHICLE(pVehicle->pDriver) &&
		!byteProcessPlayerAudio) {

		if(pVehicle->pDriver == pPlayerPed) {
			StartVehicleEngineAudio(dwAudioEnt);
		} else {
			if(bLocalPlayerDriving) {
				// Save an restore local player vehicle params
				dwAudioEnt2 = (DWORD)(pPlayerPed->pVehicle)+312;
                memcpy(&pbyteStorePlayerVehicleParams[0],(BYTE *)(pPlayerPed->pVehicle)+312,512);
      			StartVehicleEngineAudio(dwAudioEnt);
		                
				byteResetLocalVehicleAudio = 1;
				dwStartTime = GetTickCount();
				dwAudioEntToReset = dwAudioEnt2;
				pVehicleToReset = (VEHICLE_TYPE *)pPlayerPed->pVehicle;
				_asm mov esi, dwAudioEnt2
				_asm mov al, [esi+170]
				_asm mov byteSavedGearNumber, al

				//ProcessVehicleEngineAudio(dwAudioEnt2,pVehicleToReset);
				//StartVehicleEngineAudio(dwAudioEnt2);

				pChatWindow->AddDebugMessage("StartedRemoteAudioWithRestore");
                return;
			} else {
				StartVehicleEngineAudio(dwAudioEnt);
			}
		}	
	}

	ProcessVehicleEngineAudio(dwAudioEnt,_pVehicle);
}
*/
//-----------------------------------------------------------

BYTE bytePatchRadioForPlayer[5] = { 0xE8,0x9F,0x37,0xFF,0xFF };

extern BOOL bDontProcessVehiclePool;
BYTE byteSaveControlFlags;

NUDE AllVehicles_ProcessControl_Hook()
{
	_asm mov _pVehicle, ecx
	_asm mov eax, [ecx]
	_asm mov vtbl, eax

	if(vtbl == 0x871120) { // AUTOMOBILE
		call_addr = 0x6B1880;
	}
	else if(vtbl == 0x8721A0) { // BOAT
		call_addr = 0x6F1770;
	}
	else if(vtbl == 0x871360) {	// MOTORBIKE
		call_addr = 0x6B9250;
	}
	else if(vtbl == 0x871948) { // PLANE
		call_addr = 0x6C9260;
	}
	else if(vtbl == 0x871680) { // HELI
		call_addr = 0x6C7050;
	}
	else if(vtbl == 0x871528) { // PUSHBIKE (BMX?)
		call_addr = 0x6BFA30;
	}
	else if(vtbl == 0x8717d8) { // UNKNOWN2
		call_addr = 0x6C8250;
	}
	else if(vtbl == 0x871AE8) { // UNKNOWN1
		call_addr = 0x6CDCC0;
	}
	else if(vtbl == 0x872370) { // TRAIN
		call_addr = 0x6F86A0;
	}

	byteInternalPlayer = *(BYTE *)0xB7CD74;
	
	if( (_pVehicle->pDriver) && (_pVehicle->pDriver->dwPedType == 0) &&
		(_pVehicle->pDriver != GamePool_FindPlayerPed()) &&
		(byteInternalPlayer==0) ) // not player's car
	{
		// Note:
		// If you ever want to context switch the AIM over here, you must (must!) remove the 
		// appropriate context switching code from AllVehicles_ProcessControl_WaterTurret_Hook 
		// and AllVehicles_ProcessControl_TankTurret_Hook.

		bVehicleProcessControlLocal = FALSE;

		byteCurPlayer = FindPlayerNumFromPedPtr((DWORD)_pVehicle->pDriver);

		GameStoreLocalPlayerKeys(); // save local player keys
		GameSetRemotePlayerKeys(byteCurPlayer); // set remote player keys.
		
		*pbyteCurrentPlayer = 0; // set internal ID to this remote player

		// VEHICLE ENGINE AUDIO/RADIO
		//*pdwVehicleEnginePed = (DWORD)_pVehicle->pDriver;
		//VehicleAudioProcessor(_pVehicle);
		
		_pVehicle->pDriver->dwPedType = 4; // So CPed::IsPlayer returns FALSE
		//byteSaveControlFlags = _pVehicle->entity.nControlFlags;
		//_pVehicle->entity.nControlFlags = 0x1A;

		// ProcessVehicleAudio
		_asm mov edx, _pVehicle
		_asm lea ecx, [edx+312]
		_asm mov edx, 0x502280
		_asm call edx

		//_pVehicle->entity.nControlFlags = byteSaveControlFlags;
		_pVehicle->pDriver->dwPedType = 0;
	
		// CVehicle*::ProcessControl
		_asm mov ecx, _pVehicle
		_asm mov eax, call_addr
		_asm call eax

		*pbyteCurrentPlayer = 0;
		GameSetLocalPlayerKeys();

		_asm ret
	}
	else
	{	
		bVehicleProcessControlLocal = TRUE;

		// VEHICLE ENGINE AUDIO/RADIO
		_asm mov edx, _pVehicle
		_asm lea ecx, [edx+312]
		_asm mov edx, 0x502280
		_asm call edx

		// CVehicle*::ProcessControl
		_asm mov ecx, _pVehicle
		_asm mov eax, call_addr
		_asm call eax
		_asm ret
	}
}

//-----------------------------------------------------------
// Returns FALSE if the entering should be cancelled.

BOOL NotifyEnterVehicle()
{
	if(pNetGame) {
		pVehiclePool=pNetGame->GetVehiclePool();
		VehicleID=pVehiclePool->FindIDFromGtaPtr(_pVehicle);

		if(VehicleID == INVALID_VEHICLE_ID) return FALSE;
		if(!pVehiclePool->GetSlotState(VehicleID)) return FALSE;
		pVehicleClass = pVehiclePool->GetAt(VehicleID);
		if(pVehicleClass->m_bDoorsLocked) return FALSE;
		if(pVehicleClass->m_pVehicle->entity.nModelIndex == TRAIN_PASSENGER) return FALSE;

		/*
		if(pVehicleClass->m_pVehicle->entity.nModelIndex == TRAIN_PASSENGER_LOCO)
		{
			ScriptCommand(&put_actor_in_car,GamePool_Ped_GetIndex(GamePool_FindPlayerPed()),GamePool_Vehicle_GetIndex(_pVehicle));
            return FALSE;
		}*/

		// if there's a ped driver, prevent entry
		if( pVehicleClass->m_pVehicle->pDriver &&
			pVehicleClass->m_pVehicle->pDriver->dwPedType != 0 )
			return FALSE;

		pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();

		if(pLocalPlayer->GetPlayerPed() && pLocalPlayer->GetPlayerPed()->GetCurrentWeapon() == WEAPON_PARACHUTE) {
			pLocalPlayer->GetPlayerPed()->SetArmedWeapon(0);
		}

		pLocalPlayer->SendEnterVehicleNotification(VehicleID,FALSE);
	}

	return TRUE;
}

//-----------------------------------------------------------
// Used for gaining an event as to when the player enters
// a vehicle gracefully.

NUDE TaskEnterVehicleDriver_Hook()
{
	_asm mov eax, [esp]
	_asm mov dwRetAddr, eax
	_asm mov eax, [esp+4]
	_asm mov _pVehicle, eax

	_asm pushad

	if(dwRetAddr == 0x570A20 || dwRetAddr == 0x570A99) {
		// It's coming from CPlayerInfo::Process, so do some upcalls
	
		if(!NotifyEnterVehicle()) {
			_asm popad
			_asm mov ebx, [ecx]
			_asm test ebx, ebx
			_asm jz no_destruct
			_asm push 1
			_asm call [ebx] ; destroy the task
no_destruct:
			_asm pop eax ; param thiscall
			_asm pop eax ; param thiscall
			_asm mov eax, 0x570A9E ; after vehicle entering
			_asm jmp eax ; bye now
		}
	}

	_asm popad

	_asm mov eax, [esp+4]
	_asm push esi
	_asm push 0
	_asm mov ebx, 0x6402F7
	_asm jmp ebx
}

//-----------------------------------------------------------

NUDE TaskExitVehicle()
{
	_asm mov ebp, esp

	_asm mov TaskPtr, ecx
	_asm mov eax, [ebp]
	_asm mov dwRetAddr, eax
	_asm mov eax, [ebp+4]
	_asm mov _pVehicle, eax

	if(!bIgnoreNextExit && (dwRetAddr == 0x5704A1 || dwRetAddr == 0x5703FC)) {
		if(pNetGame) {
			if(GamePool_FindPlayerPed()->pVehicle == (DWORD)_pVehicle) {
				pVehiclePool=pNetGame->GetVehiclePool();
				VehicleID=pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE *)GamePool_FindPlayerPed()->pVehicle);
				pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
				pLocalPlayer->SendExitVehicleNotification(VehicleID);
			}
		}
	} else {
		bIgnoreNextExit = FALSE;
	}

	_asm push 0xFFFFFFFF
	_asm push 0x841618
	_asm mov ecx, TaskPtr
	_asm mov edx, 0x63B8C0
	_asm add edx, 7
	_asm jmp edx
}

//-----------------------------------------------------------

NUDE AddVehicleHook()
{
	_asm pushad

	if(!bAllowVehicleCreation) {
		_asm popad
		_asm xor eax, eax
		_asm ret
	}

	_asm popad
	_asm mov eax, 0x421446
	_asm jmp eax
}

//-----------------------------------------------------------

NUDE RadarTranslateColor()
{
	_asm mov eax, [esp+4]
	_asm mov iRadarColor1, eax
	TranslateColorCodeToRGBA(iRadarColor1); // return will still be in eax.
	_asm ret
}

//-----------------------------------------------------------

NUDE CheatProcessorHook()
{
	_asm mov dwSavedCheatFn, eax
	_asm pushad

	//sprintf(s,"CheatFn: 0x%X\n",dwSavedCheatFn);
	//OutputDebugString(s);

	_asm mov edx, 0x96918C
	_asm mov byte ptr [edx], 1
	_asm popad
	//_asm mov edx, 0x43857D ;(process cheat)
	_asm mov edx, 0x438581 ;(don't process cheat)
	_asm cmp eax, 0
	_asm jmp edx
}

//-----------------------------------------------------------

NUDE SetFarClipHook()
{
	_asm fld fFarClip
	_asm push esi
	_asm mov esi, [esp+8]
	_asm mov edx, dwFarClipReturnAddr
	_asm jmp edx	
}

//-----------------------------------------------------------
// We use this to trap and exit the game

DWORD dwShutDownTick;
void QuitGame();

NUDE CGameShutdownHook()
{
	dwShutDownTick = GetTickCount() + 2000;
	QuitGame();

	while(GetTickCount() < dwShutDownTick) {
		Sleep(100);
	}

	ExitProcess(0);
}

//-----------------------------------------------------------


BOOL _stdcall IsFriendlyFire(PED_TYPE *pIssuer,PED_TYPE *pPlayer)
{
	BYTE byteLocalTeam=0, byteRemoteTeam=0;
	VEHICLEID RemoteVehicleID=-1;
	PED_TYPE * pPedPlayer = GamePool_FindPlayerPed();
	PLAYERID RemotePlayerID=0;
#ifdef DEBUG
	sprintf(s,"IsFriendlyFire(0x%X,0x%X)\n",pIssuer,pPlayer);
	OutputDebugString(s);
#endif

	if(pIssuer && (pPlayer == pPedPlayer)) {
		if(pNetGame && pNetGame->m_byteFriendlyFire) {
			CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
			byteLocalTeam = pPlayerPool->GetLocalPlayer()->GetTeam();

			if((byteLocalTeam == NO_TEAM)) return FALSE;

			RemotePlayerID = pPlayerPool->FindRemotePlayerIDFromGtaPtr((PED_TYPE *)pIssuer);
			if (RemotePlayerID == INVALID_PLAYER_ID) {
				RemoteVehicleID = pNetGame->GetVehiclePool()->FindIDFromGtaPtr((VEHICLE_TYPE *)pIssuer);
				if (RemoteVehicleID != INVALID_VEHICLE_ID) {
					for (int i=0; i<MAX_PLAYERS; i++) {
						if (pPlayerPool->GetSlotState(i)) {
							if (pPlayerPool->GetAt(i)->m_VehicleID == RemoteVehicleID) {
								RemotePlayerID = i;
								break;
							}
						}
					}
				}
			}
			if(RemotePlayerID != INVALID_PLAYER_ID) {
				byteRemoteTeam = pNetGame->GetPlayerPool()->GetAt(RemotePlayerID)->GetTeam();
				//pChatWindow->AddDebugMessage("Teams: me%d you%d", byteLocalTeam, byteRemoteTeam);
				if(byteRemoteTeam == byteLocalTeam) {
					return TRUE;
				} else {
					return FALSE;
				}
			}
		}
	}

	return FALSE;
}

void _stdcall ProcessInstagib(PED_TYPE *pIssuer,PED_TYPE *pPlayer) {
	PED_TYPE * pPedPlayer = GamePool_FindPlayerPed();
	if (pNetGame) {
		if (pNetGame->m_bInstagib) {
			if (pIssuer && (pPedPlayer == pPlayer)) {
				if (pNetGame->GetPlayerPool()->FindRemotePlayerIDFromGtaPtr((PED_TYPE *)pIssuer) != INVALID_PLAYER_ID) {
					pPedPlayer->fHealth = 0.1f;
				}
			}
		}
	}
}

//-----------------------------------------------------------

NUDE PedDamage_Hook()
{
	_asm mov eax, fs:0
	_asm test ecx, ecx
	_asm jz borked
	_asm mov dwPedDamagePed, ecx
	_asm test esp, esp
	_asm jz borked
	_asm mov ebx, [esp+4]
	_asm test ebx, ebx
	_asm jz borked
	_asm mov dwPedDamage1, ebx
	//_asm mov dwStackFrame, esp
	_asm pushad
	
	if(IsFriendlyFire((PED_TYPE *)*(DWORD*)dwPedDamagePed,(PED_TYPE *)dwPedDamage1)) {
		_asm popad
		_asm retn 12
	}
	ProcessInstagib((PED_TYPE *)*(DWORD*)dwPedDamagePed,(PED_TYPE *)dwPedDamage1);
	
	_asm  popad
borked:
	_asm  mov edx, 0x4B5AC6
	_asm  jmp edx
}

/*
// Not really required anymore
NUDE Tank_Call_To_IsPlayer()
{
	_asm mov dwCurPlayerActor, ecx // store the passed actor

	if (pGame->FindPlayerPed()->m_pEntity != (ENTITY_TYPE*)dwCurPlayerActor) // If its not our tank, then why let it move?!?!
	{
		_asm xor al, al
		_asm retn
	} else {
		_asm mov al, 1
		_asm retn
	}
}
*/

BOOL _stdcall IsLocalPlayerFiring() {
	if (pNetGame) {
		WORD lrAnalog,udAnalog;
		WORD wKeys = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->GetKeys(&lrAnalog,&udAnalog);
		if (wKeys & 4) {
			return TRUE;
		}
	} else return TRUE;
	return FALSE;
}

NUDE CCameraCamShake_Sniper_Hook()
{
	//_asm mov dwCurPlayerActor, esi
	//if (dwCurPlayerActor == (DWORD)GamePool_FindPlayerPed()) {
	//	_asm mov eax, 0x50A970
	//	_asm jmp eax
	//}
	_asm ret
}

//-----------------------------------------------------------

NUDE CWeapon_FireCamera_Hook()
{
	_asm mov ebx, [esp+8]
	_asm mov dwCurPlayerActor, ebx
	_asm pushad

	if (dwCurPlayerActor == (DWORD)GamePool_FindPlayerPed()) {
		*(BYTE*)0xC8A7C0 = 1;
		*(BYTE*)0xC8A7C1 = 1;
	}

	_asm popad
	_asm mov ebx, 0x73C260
	_asm jmp ebx
}

//-----------------------------------------------------------

NUDE CTrain_ProcessControl_Derailment()
{
	_asm {
		mov eax, [esi+1444];	// get the speed
		and eax, 0x80000000;	// get the upper bit
		or eax, 0x3F800000;		// or it with 1.0f (it'll be -ve if bit is set)
		mov [esi+1444], eax;	// set the new speed
		ret;
	}
}

//-----------------------------------------------------------

NUDE AnimCrashFixHook()
{
	__asm
	{
		push edi;
		mov edi, [esp+8];	// arg0
		test edi, edi;
		jz exitFn
		mov eax, 0x4D41C5;	// continuation for function
		jmp eax;
exitFn:
		pop edi;
		ret;
	}
}


//-----------------------------------------------------------

NUDE SetForegroundWindowCrashFixHook()
{
	__asm
	{
		mov eax, ds:0xC17054;
		test eax, eax;
		jz exitFn;
		push 0x746929;	// return address;
		ret;
exitFn:
		xor eax, eax;
		ret;
	}
}


//-----------------------------------------------------------

NUDE GenTaskAlloc_Hook()
{
	_asm mov edi, [esp+4]
	_asm mov dwParam1, edi
	//_asm mov edx, [esp+8]
	//_asm mov dwParam2, edx
	_asm mov dwParamThis, ecx

	_asm pushad
		_asm mov edx, [edi]
		_asm mov ecx, edi
		_asm call dword ptr [edx+4]
		_asm mov dwParam2, eax
	
		pChatWindow->AddDebugMessage("TaskAlloc: 0x%X, 0x%X, 0x%X",dwParamThis,dwParam1,dwParam2);
	_asm popad

	_asm mov eax, fs:0
	_asm mov edx, 0x4C3876
	_asm jmp edx
}

//-----------------------------------------------------------
extern DWORD dwFogEnabled;
void SetupD3DFog(BOOL bEnable);

NUDE CPed_Render_Hook()
{
	_asm pushad
	SetupD3DFog(FALSE);
	_asm popad

	_asm mov edx, 0x5E7680
	_asm call edx

	_asm pushad

	if(dwFogEnabled) {
		SetupD3DFog(TRUE);
	} else {
		SetupD3DFog(FALSE);
	}

	_asm popad
	_asm ret
}

//-----------------------------------------------------------

DWORD dwSayParam1;
DWORD dwSayParam2;
float fSayParam3;
DWORD dwSayParam4;
DWORD dwSayParam5;
DWORD dwSayParam6;

NUDE CPed_Say_Hook()
{
    _asm mov eax, [esp+4]
	_asm mov dwSayParam1, eax
	_asm mov eax, [esp+8]
	_asm mov dwSayParam2, eax
	_asm mov eax, [esp+12]
	_asm mov fSayParam3, eax
	_asm mov eax, [esp+16]
	_asm mov dwSayParam4, eax
	_asm mov eax, [esp+20]
	_asm mov dwSayParam5, eax
	_asm mov eax, [esp+24]
	_asm mov dwSayParam6, eax

	_asm pushad

	if(dwSayParam1 != 45) {
		if(pChatWindow) pChatWindow->AddDebugMessage("CPed::Say(%u,%u,%f,%u,%u,%u)",
			dwSayParam1,dwSayParam2,fSayParam3,dwSayParam4,dwSayParam5,dwSayParam6);
	}

	_asm popad

    _asm mov eax, [esp+4]
    _asm test ax, ax
	_asm mov edx, 0x5EFFE7
	_asm jmp edx
}

//-----------------------------------------------------------

NUDE ZoneOverlay_Hook()
{
	_asm pushad;
	if (pNetGame && pNetGame->GetGangZonePool()) pNetGame->GetGangZonePool()->Draw();
	_asm popad;
	_asm ret;
}

//-----------------------------------------------------------

NUDE PlayerWalk_Hook()
{
	_asm pushad;
	if (pNetGame && pNetGame->GetWalkStyle())
	{
		_asm popad;
		_asm mov [esi + 0x4D4], eax;
	}
	else
	{
		_asm popad;
	}
	_asm ret;
}

//-----------------------------------------------------------

NUDE PickUpPickup_Hook()
{
	_asm mov dwParam1, esi
	_asm pushad;

	if (pNetGame && pNetGame->GetPickupPool()) {
		CPickupPool* pPickups = pNetGame->GetPickupPool( );
		pPickups->PickedUp( ((dwParam1-0x9788C0) / 0x20) );
	}

	_asm popad;
	_asm mov al, [esi+0x1C]
	_asm cmp al, 6
	_asm push 0x4579CB
	_asm ret
}

//-----------------------------------------------------------

DWORD dwProjectile = NULL;
DWORD dwProjectile_Pool_Entry = NULL;

NUDE CProjectile_Update_Hook()
{
	_asm mov dwProjectile, ESI
	_asm mov dwProjectile_Pool_Entry, EBX
	_asm pushad
	
	if ( *(DWORD*)(dwProjectile_Pool_Entry + 4) == NULL ) // If a crash would have happened
	{
		__asm
		{
			PUSH dwProjectile
			MOV EAX, 0x739A40
			CALL EAX			// CProjectileInfo::Remove
			ADD ESP,4

			_asm popad

			mov dl, 0xFF
		}
	} else {
		_asm popad

		_asm mov ecx, [ebx+4]
		_asm mov dl, [ecx+36h]
	}

	_asm PUSH 0x00738F40
	_asm RETN
}

//-----------------------------------------------------------

NUDE CWeapon__Satchel__Activate_Hook()
{
	__asm
	{
		CMP DWORD PTR DS:[ESI-0x20], 0x27
		MOV EDI, DWORD PTR SS:[EBP]
		JNZ skip
		MOV EAX, [ESP]
		CMP DWORD PTR DS:[ESI-0x1C], EAX
		JNZ skip
		MOV EAX, 0x738880
		JMP EAX

skip:
		MOV EAX, 0x7388DB
		JMP EAX
	}
}

//-----------------------------------------------------------

NUDE PathProcessingCrashFixHook()
{
	__asm
	{
		push esi
		test ecx, ecx;
		jz exitFn
		mov esi,dword ptr ss:[esp+8]
		mov eax,dword ptr ds:[esi]
		mov edx, 0x63A047;
		jmp edx;

exitFn:
		mov esi, 0x63a04d;
		jmp esi;
	}
}

//-----------------------------------------------------------

NUDE SetCarColorCrashFix()
{
	__asm
	{
		test eax, eax;
		jz exitFn;						// CPool_CVehicle_GetAt(scmParam1) == NULL
		mov ecx, 0x47eab8;				
		mov cl,byte ptr ds:[0xA43C7C];	// get color1 (was replaced by jmpcode)
		jmp ecx;						// continue function

exitFn:
		mov esi, 0x47eaca;
		jmp esi;
	}
}

//-----------------------------------------------------------

PCHAR szGotText;

NUDE GetText_Hook()
{
	_asm sub esp, 0x20
	_asm push esi
	_asm push edi
	_asm mov edi, [esp + 0x2C]
	_asm pushad
	_asm mov dwSavedEcx, edi
	
	szGotText = NULL;
	// Faster than doing strncmp
	if (pNetGame && *(PCHAR)(dwSavedEcx) == 'S' && *(PCHAR)(dwSavedEcx + 1) == 'A' && *(PCHAR)(dwSavedEcx + 2) == 'M' && *(PCHAR)(dwSavedEcx + 3) == 'P')
	{
		if (pNetGame->GetMenuPool()) szGotText = pNetGame->GetMenuPool()->GetTextPointer((PCHAR)(dwSavedEcx + 4));
	}

	_asm popad
	
	if (szGotText) {
		_asm mov eax, szGotText
		_asm pop edi
		_asm pop esi
		_asm add esp, 0x20
		_asm retn 4
	}

	_asm mov eax, 0x6A0059
	_asm jmp eax
}

//-----------------------------------------------------------

DWORD dwRandCaller;

NUDE Rand_Hook()
{
	_asm mov eax, [esp+0]
	_asm mov dwRandCaller, eax

	/*
	if(dwRandCaller > 0x73FB10 && dwRandCaller < 0x74132E) {
		_asm mov eax, iSyncedRandomNumber
		_asm ret
	}*/
		
	rand();
	_asm ret
}

//-----------------------------------------------------------
// We use a special bit (32) on dwProcFlags (+28) to indicate
// whether we should process gravity/collisions on this PlayerPed.

NUDE CPlayerPed_ProcessCollision_Hook()
{
	_asm test ecx, ecx
	_asm jnz ptr_is_ok
	_asm ret
ptr_is_ok:
	_asm mov eax, [ecx+28]
	_asm shr eax, 31
	_asm cmp eax, 1
	_asm jne do_process_cols
	_asm ret // we set top bit so don't process this
do_process_cols:
    _asm mov edx, 0x54DFB0
	_asm jmp edx
}

//-----------------------------------------------------------
DWORD dwMat;
DWORD dwMatEffects;
DWORD dwDataParam;

NUDE VehicleModel_SetEnvironmentMapHook()
{
    _asm mov eax, [esp+4]
	_asm mov dwMat, eax
	_asm mov eax, [esp+8]
	_asm mov dwDataParam, eax

	UnFuck(0x6D64F0,1);
	*(PBYTE)0x6D64F0 = 0xC3;

	/*
	_asm push dwMat
	_asm mov edx, 0x812140	// _RpMatFXMaterialGetEffects
	_asm call edx
	_asm pop edx
	_asm mov dwMatEffects, eax*/

	_asm push 0
	_asm push dwMat
	_asm mov edx, 0x811C80	// _RpMatFXMaterialSetEffects
	_asm call edx
	_asm pop edx
	_asm pop edx

    pChatWindow->AddDebugMessage("SetEnvironmentMapCB(0x%X,0x%X,%d)",dwMat,dwDataParam,dwMatEffects);
    
	_asm mov edx, 0x4C8848
    _asm cmp [esp+8], 0FFFFh
	_asm jmp edx
}

//-----------------------------------------------------------

NUDE SCM_TogglePlayerControllable() {
	_asm mov edx, 0x47DF56;
	_asm jmp edx;
}

//-----------------------------------------------------------

void InstallMethodHook(	DWORD dwInstallAddress,
						DWORD dwHookFunction )
{
	DWORD oldProt, oldProt2;
	VirtualProtect((LPVOID)dwInstallAddress,4,PAGE_EXECUTE_READWRITE,&oldProt);
	*(PDWORD)dwInstallAddress = (DWORD)dwHookFunction;
	VirtualProtect((LPVOID)dwInstallAddress,4,oldProt,&oldProt2);
}

//-----------------------------------------------------------

void InstallHook( DWORD dwInstallAddress,
				  DWORD dwHookFunction,
				  DWORD dwHookStorage,
				  BYTE * pbyteJmpCode,
				  int iJmpCodeSize )
{
	DWORD oldProt, oldProt2;

	// Install the pointer to procaddr.
	VirtualProtect((PVOID)dwHookStorage,4,PAGE_EXECUTE_READWRITE,&oldProt);
	*(PDWORD)dwHookStorage = (DWORD)dwHookFunction;
	VirtualProtect((PVOID)dwHookStorage,4,oldProt,&oldProt2);

	// Install the Jmp code.
	VirtualProtect((PVOID)dwInstallAddress,iJmpCodeSize,PAGE_EXECUTE_READWRITE,&oldProt);		
	memcpy((PVOID)dwInstallAddress,pbyteJmpCode,iJmpCodeSize);
	VirtualProtect((PVOID)dwInstallAddress,iJmpCodeSize,oldProt,&oldProt2);	
}

//-----------------------------------------------------------

void InstallCallHook(DWORD dwInstallAddress, DWORD dwHookFunction, BYTE byteJumpCode = 0xE8)
{
	DWORD oldProt, oldProt2;
	DWORD disp = dwHookFunction - (dwInstallAddress + 5);

	VirtualProtect((LPVOID)dwInstallAddress,5,PAGE_EXECUTE_READWRITE,&oldProt);
	*(PBYTE)(dwInstallAddress) = byteJumpCode;
	*(PDWORD)(dwInstallAddress+1) = (DWORD)disp;
	VirtualProtect((LPVOID)dwInstallAddress,5,oldProt,&oldProt2);
}

//-----------------------------------------------------------

void InstallGameAndGraphicsLoopHooks()
{	
	UnFuck(0x53EB13,4);
	*(int *)0x53EB13 = dwGraphicsLoop - 0x53EB12 - 5; // relative addr

	InstallHook(0x58C246,(DWORD)GameProcessHook,
		0x53BED1,GameProcess_HookJmpCode,sizeof(GameProcess_HookJmpCode));

	// For fixing fogging issues (needed for both debug and net)
	InstallMethodHook(0x86D1B0,(DWORD)CPed_Render_Hook); // This is PlayerPed
	InstallMethodHook(0x86C0F0,(DWORD)CPed_Render_Hook);
	InstallMethodHook(0x86C168,(DWORD)CPed_Render_Hook);
	InstallMethodHook(0x86C248,(DWORD)CPed_Render_Hook);
	InstallMethodHook(0x86C3A0,(DWORD)CPed_Render_Hook);

	/*
	InstallHook(0x5EFFE0,(DWORD)CPed_Say_Hook,
		0x5EFFD8,PedSay_HookJmpCode,sizeof(PedSay_HookJmpCode));*/
}

//-----------------------------------------------------------

void GameInstallHooks()
{
	//InstallHook(ADDR_RENDER2DSTUFF,(DWORD)GraphicsLoopHook,
		//ADDR_RENDER2DSTUFF_STORAGE,GraphicsLoop_HookJmpCode,
		//sizeof(GraphicsLoop_HookJmpCode));
	// Above code replaced by a new method which should avoid
	// stack corruption of the return value
	
	InstallGameAndGraphicsLoopHooks();

	/* Hook rand()
	if(iGtaVersion == GTASA_VERSION_USA10) {
		InstallHook(0x821B1E,(DWORD)Rand_Hook,0x5EAB77,Rand_HookJmpCode,sizeof(Rand_HookJmpCode));
		OutputDebugString("GTASA US 1.0 rand() hook");
	} else {
		InstallHook(0x821B5E,(DWORD)Rand_Hook,0x5EAB77,Rand_HookJmpCode,sizeof(Rand_HookJmpCode));
		OutputDebugString("GTASA EU 1.0 rand() hook");
	}*/

	//InstallMethodHook(0x86D190,(DWORD)CPlayerPed_ProcessControl_Hook);
	InstallMethodHook(0x86D744,(DWORD)TaskUseGun_Hook);
	InstallMethodHook(0x86D194,(DWORD)CPlayerPed_ProcessCollision_Hook);
	
	//InstallMethodHook(0x870904,(DWORD)TaskOnFoot1_Hook);
	//InstallMethodHook(0x870908,(DWORD)TaskOnFoot2_Hook);

	InstallMethodHook(0x871148,(DWORD)AllVehicles_ProcessControl_Hook); // Automobile
	InstallMethodHook(0x8721C8,(DWORD)AllVehicles_ProcessControl_Hook); // boat
	InstallMethodHook(0x871388,(DWORD)AllVehicles_ProcessControl_Hook); // motorbike1
	InstallMethodHook(0x871970,(DWORD)AllVehicles_ProcessControl_Hook); // plane
	InstallMethodHook(0x8716A8,(DWORD)AllVehicles_ProcessControl_Hook); // heli
	InstallMethodHook(0x871550,(DWORD)AllVehicles_ProcessControl_Hook); // pushbike
	InstallMethodHook(0x871800,(DWORD)AllVehicles_ProcessControl_Hook); // truck
	InstallMethodHook(0x871B10,(DWORD)AllVehicles_ProcessControl_Hook); // quad
	InstallMethodHook(0x872398,(DWORD)AllVehicles_ProcessControl_Hook); // train

	// Radar and map hooks for gang zones
	InstallCallHook(0x5869BF,(DWORD)ZoneOverlay_Hook);
	InstallCallHook(0x5759E4,(DWORD)ZoneOverlay_Hook);
	
	InstallCallHook(0x609A4E,(DWORD)PlayerWalk_Hook);
	InstallCallHook(0x4579C6,(DWORD)PickUpPickup_Hook, 0xE9);
	InstallCallHook(0x73C252,(DWORD)CWeapon_FireCamera_Hook, 0xE9);

	InstallCallHook(0x6B2028,(DWORD)AllVehicles_ProcessControl_TankTurret_Hook);
	InstallCallHook(0x6B1F5E,(DWORD)AllVehicles_ProcessControl_WaterTurret_Hook);

	// Hook the call to CCamera::CamShake when called for sniper fire
	InstallCallHook(0x73ACE2,(DWORD)CCameraCamShake_Sniper_Hook);

	// Hook the train derailment code
	InstallCallHook(0x6F8CF8,(DWORD)CTrain_ProcessControl_Derailment);

	InstallHook(0x6402F0,(DWORD)TaskEnterVehicleDriver_Hook,
		0x6919BB,TaskEnterVehicleDriver_HookJmpCode,sizeof(TaskEnterVehicleDriver_HookJmpCode));

	InstallHook(0x63B8C0,(DWORD)TaskExitVehicle,
		0x63B8BA,TaskExitVehicle_HookJmpCode,sizeof(TaskExitVehicle_HookJmpCode));

	//InstallHook(0x47D3C8, (DWORD)SCM_TogglePlayerControllable, 0x47DF56, (BYTE*)0x47DF56, sizeof((BYTE*)0x47DF56));

	/*
	InstallHook(0x421440,(DWORD)AddVehicleHook,
		0x421433,AddVehicleHook_HookJmpCode,sizeof(AddVehicleHook_HookJmpCode));*/

	InstallHook(0x438576,(DWORD)CheatProcessorHook,
		0x4385AA,CheatProcessHook_JmpCode,sizeof(CheatProcessHook_JmpCode));

	InstallHook(0x584770,(DWORD)RadarTranslateColor,0x584A79,
		RadarTranslateColor_HookJmpCode,sizeof(RadarTranslateColor_HookJmpCode));

	//InstallHook(dwFarClipHookAddr,(DWORD)SetFarClipHook,0x533661,
		//SetFarClip_HookJmpCode,sizeof(SetFarClip_HookJmpCode));

	InstallHook(0x53C900,(DWORD)CGameShutdownHook,0x53C8F1,
		CGameShutdown_HookJmpCode,sizeof(CGameShutdown_HookJmpCode));

	InstallHook(0x4B5AC0,(DWORD)PedDamage_Hook,0x4B5ABC,
		PedDamage_HookJmpCode,sizeof(PedDamage_HookJmpCode));

	// Fix for 0x004D41C5 crash
	InstallCallHook(0x4D41C0, (DWORD)AnimCrashFixHook, 0xE9);

	// Fix for 0x00746929 crash
	//InstallCallHook(0x00746924, (DWORD)SetForegroundWindowCrashFixHook, 0xE9);

	//InstallHook(0x4C3870,(DWORD)GenTaskAlloc_Hook,0x4C3861, 
		//GenTaskAlloc_HookJmpCode, sizeof(GenTaskAlloc_HookJmpCode));

	// Fix for crash when the player who threw the satchel died
	InstallHook(0x738F3A, (DWORD)CProjectile_Update_Hook, 0x738B1B, CProjectileInfo_Update_HookJmpCode, sizeof(CProjectileInfo_Update_HookJmpCode));
	// Fix for all satchels blowing up when someone activated their satchel
	InstallHook(0x738877, (DWORD)CWeapon__Satchel__Activate_Hook, 0x73885B, CWeapon__Satchel__Activate_HookJmpCode, sizeof(CWeapon__Satchel__Activate_HookJmpCode));

	InstallHook(0x6A0050, (DWORD)GetText_Hook, 0x6A0043, GetText_HookJmpCode, sizeof (GetText_HookJmpCode));

	// Fix for ActorRunToPoint crash on invalid path 
	//InstallHook(0x63a040, (DWORD)PathProcessingCrashFixHook, 0x63a036, PathProcessing_HookJmpCode, sizeof(PathProcessing_HookJmpCode));

#	// Fix for opcode 0x229 (set_car_color) crash - modshop related 
	//InstallHook(0x47EAB2, (DWORD)SetCarColorCrashFix, 0x63a036, SetColor_HookJmpCode, sizeof(SetColor_HookJmpCode));

	//InstallHook(0x4C8840,(DWORD)VehicleModel_SetEnvironmentMapHook,0x4C8839,
		//VehicleModel_SetEnvironmentMap_JmpCode,sizeof(VehicleModel_SetEnvironmentMap_JmpCode));

}

//-----------------------------------------------------------