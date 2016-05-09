#include "../main.h"

void InstallSCMEventsProcessor();
void RelocateScanListHack();
void RelocatePedsListHack();

extern int iGtaVersion;
extern CNetGame* pNetGame;
extern CGame* pGame;
extern DWORD dwUIMode;

//----------------------------------------------------------

void UnFuckAndCheck(DWORD addr, int size, BYTE byteCheck)
{
	DWORD d;
	char s[256];
	VirtualProtect((PVOID)addr,size,PAGE_EXECUTE_READWRITE,&d);

	if(byteCheck != *(PBYTE)addr) {
#ifdef _DEBUG
		sprintf(s,"Failed Check At Addr: 0x%X",addr);
		OutputDebugString(s);
#endif
		while(byteCheck != *(PBYTE)addr) Sleep(1);
	}
}

//----------------------------------------------------------

void UnFuck(DWORD addr, int size)
{
	DWORD d;
	VirtualProtect((PVOID)addr,size,PAGE_EXECUTE_READWRITE,&d);
}

//----------------------------------------------------------

void InstallGameAndGraphicsLoopHooks();

void ApplyDebugLevelPatches()
{
	// hack to remove motion blur in high speed vehicle
	UnFuck(0x704E8A,5);
	memset((PVOID)0x704E8A,0x90,5);
	
	// Don't go back to player anims, use the peds IDE
	UnFuck(0x609A4E,6);
	memset((PVOID)0x609A4E, 0x90, 6);

	InstallGameAndGraphicsLoopHooks();
}

//----------------------------------------------------------

BOOL ApplyPreGamePatches()
{	
	BYTE * pbyteVersionDetermination = (PBYTE)ADDR_BYPASS_VIDS_USA10;
	int iCounter=0;

	// MAIN VERSION DETERMINING LOGIC
	while( (*pbyteVersionDetermination != 0x89) &&
		   (*pbyteVersionDetermination != 0xC8) )
	{
		if (*(PBYTE)ADDR_GAME_STARTED == 1) {
			return FALSE;
		} else {
			Sleep(10);
			iCounter++;
			if(iCounter>6000) { // 60 seconds have passed
				return FALSE;
			}
		}
	}

	if(*pbyteVersionDetermination == 0x89) {
		iGtaVersion = GTASA_VERSION_USA10;
	} 
	else if(*pbyteVersionDetermination == 0xC8) {
		iGtaVersion = GTASA_VERSION_EU10;
	}

	// (skip to starting screen)
	if(iGtaVersion == GTASA_VERSION_USA10) {
		UnFuck(ADDR_BYPASS_VIDS_USA10,6);
		*(BYTE *)ADDR_ENTRY = 5;
		memset((PVOID)ADDR_BYPASS_VIDS_USA10,0x90,6);
	} 
	else if (iGtaVersion == GTASA_VERSION_EU10) {
		UnFuck(ADDR_BYPASS_VIDS_EU10,6);
		*(BYTE *)ADDR_ENTRY = 5;
		memset((PVOID)ADDR_BYPASS_VIDS_EU10,0x90,6);
	}

	// Loading screens
	UnFuck(0x866CD8,10);
	UnFuck(0x866CCC,10);
	strcpy((PCHAR)0x866CD8,"title");
	strcpy((PCHAR)0x866CCC,"title");

	// Modify the streaming memory hardcoded values
	UnFuck(0x5B8E6A,4);
	*(DWORD *)0x5B8E6A = 134217728; // 128MB		

	return TRUE;
}

//----------------------------------------------------------

BYTE pbyteVehiclePoolAllocPatch[] = {0x6A,0x00,0x68,0xC6,0x2,0x00,0x00}; // 710
BYTE pbyteCollisionPoolAllocPatch[] = { 0x68,0xFF,0x7E,0x00,0x00 }; // 32511
BYTE pbyteEntryInfoPoolAllocPatch[] = { 0x68,0x00,0x8,0x00,0x00 }; // 2048
BYTE pbyteTrainDelrailmentPatch[] = {
	0xB8, 0x89, 0x8F, 0x6F, 0x00, 0xFF, 0xE0
};

extern DWORD dwFarClipHookAddr;
extern DWORD dwFarClipReturnAddr;

void ApplyInGamePatches()
{	
	if(GTASA_VERSION_USA10 == iGtaVersion) {
		dwFarClipHookAddr = 0x7EE2A0;
		dwFarClipReturnAddr = dwFarClipHookAddr+9;
	} else {
		dwFarClipHookAddr = 0x7EE2E0;
		dwFarClipReturnAddr = dwFarClipHookAddr+9;
	}

	RelocateScanListHack();
	RelocatePedsListHack(); // allows us to use all 300 ped model slots

	// Frame limiter default ~40 fps
	UnFuck(0xC1704C,1);
	*(PDWORD)0xC1704C = 55UL; // yes that means 40..

	// APPLY THE DAMN NOP PATCH AND QUIT ASCIING QUESTIONS!

	// Increase the vehicle pool limit (see top of proc for patch)
	UnFuckAndCheck(0x551024,sizeof(pbyteVehiclePoolAllocPatch),0x68);
	memcpy((PVOID)0x551024,pbyteVehiclePoolAllocPatch,sizeof(pbyteVehiclePoolAllocPatch));
	
	// Increase the ped pool limit (210)
	UnFuck(0x550FF2,1);
	*(PBYTE)0x550FF2 = 210;

	// And we need 210 ped intelligence too plz
	UnFuck(0x551283,1);
	*(PBYTE)0x551283 = 210;
	
	// And a larger task pool
	UnFuck(0x551140,1);
	*(PBYTE)0x551140 = 0x05;

	// And a larger event pool
	UnFuck(0x551178,1);
	*(PBYTE)0x551178 = 0x01; // 456

	// And we'd definitely need some more matrices.
	// Who doesn't need more matrices?
	UnFuck(0x54F3A2,1);
	*(PBYTE)0x54F3A2 = 0x10; // 4228

	// Increase the collision model ptr
	UnFuck(0x551106,sizeof(pbyteCollisionPoolAllocPatch));
	memcpy((PVOID)0x551106,pbyteCollisionPoolAllocPatch,sizeof(pbyteCollisionPoolAllocPatch));
	
	// Increase the entry info nodes (no need for more enter/exits really)
	// UnFuck(0x550FB9,sizeof(pbyteEntryInfoPoolAllocPatch));
	// memcpy((PVOID)0x550FB9,pbyteEntryInfoPoolAllocPatch,sizeof(pbyteEntryInfoPoolAllocPatch));

	// VehicleStruct increase
	UnFuck(0x5B8FDE, 7);
	*(BYTE*)0x5B8FDE = 0x6A;	// push imm8
	*(BYTE*)0x5B8FDF = 0x00;	// 0
	*(BYTE*)0x5B8FE0 = 0x68;	// push imm32
	*(BYTE*)0x5B8FE1 = 127;
	*(BYTE*)0x5B8FE2 = 0x00;
	*(BYTE*)0x5B8FE3 = 0x00;
	*(BYTE*)0x5B8FE4 = 0x00;

	// Increase the building pool
	UnFuck(0x551060,1);
	*(BYTE*)0x551060 = 0x42; // 17096 from 13000
	
	// Stop ped rotations from the camera
	UnFuck(0x6884C4,6);
	memset((PVOID)0x6884C4,0x90,6);

	/* clouds RenderEffects (needs checking)
	UnFuck(0x53E1AF,10);
	memset((PVOID)0x53E1AF,0x90,10);

	// 53E121 (15) low level cloud (needs checking)
	UnFuck(0x53E121,15);
	memset((PVOID)0x53E121,0x90,15);*/

	// Unknown from CGame::Process causes "country" crash (testing)
	// Update: Country crash is from procedural geometry like plants etc.
	// I don't want to force disable it.
	//UnFuck(0x53C159,5);
	//memset((PVOID)0x53C159,0x90,5);
	
	if (dwUIMode != 0) 
	{
		// Move the radar to the top left of the screen
		UnFuck(0x866B70,4);
		*(float *)0x866B70 = 430.0f;
	    
		// Scale down the hud slightly
		//UnFuck(0x859520,8);
		//*(float *)0x859520 = 0.0011f;
		//*(float *)0x859524 = 0.00172f;
	}

	//Cursor hiding
	UnFuck(0x7481CD,16);
	memset((PVOID)0x7481CD,0x90,16);
    
	UnFuck(0x747FEA,1);
	*(BYTE*)0x747FEA = 1;

	// A SetCursorPos proc internally
	UnFuck(0x6194A0,1);
	*(PBYTE)0x6194A0 = 0xC3;
	

	// No vehicle name rendering
	UnFuck(0x58FBE9,5);
    memset((PVOID)0x58FBE9,0x90,5);

	// No playidles anim loading.
	UnFuck(0x86D1EC,1);
	*(BYTE*)0x86D1EC = '\0';

	// Prevent replays
	UnFuck(0x53C090,5);
	memset((PVOID)0x53C090,0x90,5);

	// NO MORE INTERIOR PEDS
	UnFuck(0x440833,8);
	memset((PVOID)0x440833,0x90,8);

	// caused 0x706B2E crash (This seems to be ped shadow rendering)
	UnFuck(0x53EA08,10);
	memset((PVOID)0x53EA08,0x90,10);

	// Make the shadows slightly darker by increasing the alpha
	UnFuck(0x71162C,1);
	*(PBYTE)0x71162C = 80;

	// Anti-pause 
	UnFuck(0x561AF0,7);
	memset((PVOID)0x561AF0,0x90,7);
	/* A likely solution for the 0x crash
	UnFuck(0x411458,2);
	*(BYTE*)0x411458 = 0xF4;
	*(BYTE*)0x411459 = 0x01;*/

	/* Some main collision thing nulled
	UnFuck(0x616650,1);
	*(BYTE *)0x616650 = 0xC3;*/

	// Unknown from CPlayerPed::ProcessControl causes crash
	UnFuck(0x609C08,39);
	memset((PVOID)0x609C08,0x90,39);

	// FindPlayerVehicle (Always use nPlayerPed)
	UnFuck(0x56E0FA,18);
	memset((PVOID)0x56E0FA,0x90,18);

	// CMotorBike::ProcessControlInputs.. why oh why..
	UnFuck(0x6BC9EB,2);
	memset((PVOID)0x6BC9EB,0x90,2);

	// Smallish patch to fix the drown-in-vehicle crash
	UnFuck(0x4BC6C1,3);
	*(BYTE*)0x4BC6C1 = 0xB0;
	*(BYTE*)0x4BC6C2 = 0x00;
	*(BYTE*)0x4BC6C3 = 0x90;

	// Allow all vehicles to be sprayed;
	UnFuck(0x44AC75,5);
	memset((PVOID)0x44AC75,0x90,5);
	*(BYTE*)0x44AC75 = 0xB0;
	*(BYTE*)0x44AC76 = 0x01;

	// This removes the random weapon pickups (e.g. on the hill near chilliad)
	UnFuck(0x5B47B0,1);
	memset((PVOID)0x5B47B0,0xC3,1);

	// Removes the FindPlayerInfoForThisPlayerPed at these locations.
	UnFuck(0x5E63A6,19);
	memset((PVOID)0x5E63A6,0x90,19);

	UnFuck(0x621AEA,12);
	memset((PVOID)0x621AEA,0x90,12);

	UnFuck(0x62D331,11);
	memset((PVOID)0x62D331,0x90,11);

	UnFuck(0x741FFF,27);
	memset((PVOID)0x741FFF,0x90,27);
	
	// hack to remove motion blur in high speed vehicle
	UnFuck(0x704E8A,5);
	memset((PVOID)0x704E8A,0x90,5);

	// Respawn and Interior
	UnFuck(0x4090A0,1);
	*(BYTE*)0x4090A0 = 0xC3;
	
	// Respawn and Interior
	UnFuck(0x441482,5);
	memset((void*)0x441482, 0x90, 5);

	// No MessagePrint
	UnFuck(0x588BE0,1);
	*(BYTE*)0x588BE0 = 0xC3;

	// No IPL vehicle
	UnFuck(0x53C06A,5);
	memset((PVOID)0x53C06A,0x90,5);

	// SomeCarGenerator (0x41a8b3 crash)
	UnFuck(0x434272,5);
	memset((PVOID)0x434272,0x90,5);

	// CPlayerPed_CPlayerPed .. task system corrupts some shit
	UnFuck(0x60D64D,2);
	*(PBYTE)0x60D64E = 0x84; // jnz to jz
	
	// CPhysical Destructor (705b3b crash)
	UnFuck(0x542485,11);
	memset((PVOID)0x542485,0x90,11);
	
	// No peds kthx. (CPopulation::AddPed() nulled)
	UnFuck(0x612710,3);
	*(BYTE*)0x612710 = 0x33;
	*(BYTE*)0x612711 = 0xC0; // xor eax, eax
	*(BYTE*)0x612712 = 0xC3; // ret

	// Fuck the call to CPopulation::AddPed() for create_train off to kingdom kong	
	UnFuck(0x613BA7, 5);
	memset((PVOID)0x613BA7, 0x90, 5);

	// Don't go back to player anims, use the peds IDE
	UnFuck(0x609A4E,6);
	memset((PVOID)0x609A4E, 0x90, 6);

// note: siren issue not below here

	// Train derailment 
	UnFuck(0x006F8CF8, 12);
	memset((PVOID)0x006F8CF8, 0x90, 5); // (Actual hook is installed in hooks.cpp)
	memcpy((PVOID)(0x006F8CF8+5), pbyteTrainDelrailmentPatch, sizeof(pbyteTrainDelrailmentPatch));

	// Corrects train entry problems if on track 2
	UnFuck(0x6D5410,1);
	*(BYTE*)0x6D5410 = 0xFF; // test if for something it'll never be

	// CarCtl::GenerateRandomCars nulled from CGame::Process (firetrucks etc)
	UnFuck(0x53C1C1,5);
	memset((PVOID)0x53C1C1,0x90,5);

	// (540040 bug), test ecx for 0 instead of [ecx+270]
	UnFuck(0x540040,10);
	// nop the first 8 bytes
	memset((PVOID)0x540040,0x90,6);
	*(PBYTE)0x540046 = 0x85;
	*(PBYTE)0x540047 = 0xC9; // test ecx, ecx
	*(PBYTE)0x540048 = 0x74; // jz
	
	// No wasted message
	UnFuck(0x56E5AD,5);
	memset((PVOID)0x56E5AD,0x90,5);

	// For the input disabling in CGame.
	UnFuck(0x541DF5,5);

	/* Ret at CCamera::ClearPlayerWeaponMode
	UnFuck(0x50AB10,2);
	*(PBYTE)0x50AB10 = 0xC3;*/
	UnFuck(0x609CB4,5);
	memset((PVOID)0x609CB4,0x90,5);

	// PlayerInfo checks in CPlayerPed::ProcessControl
	UnFuck(0x60F2C4,25);
	memset((PVOID)0x60F2C4,0x90,25);

	// Enable full manual control of the Goggles (on and off), and
	// disable the little sound when putting on the goggles.
	UnFuck(0x5E3AD1,3);
	memset((PVOID)0x5E3AD1,0x90,3);
	UnFuck(0x5DF1EE,3);
	memset((PVOID)0x5DF1EE,0x90,3);
	UnFuck(0x634F6C,16);
	memset((PVOID)0x634F6C,0x90,16);

	// No Vehicle Audio Processing (done manually from the hooks)
	UnFuck(0x6B18F1,5);
	memset((PVOID)0x6B18F1,0x90,5);
	UnFuck(0x6B9298,5);
	memset((PVOID)0x6B9298,0x90,5);
	UnFuck(0x6F1793,5);
	memset((PVOID)0x6F1793,0x90,5);
	UnFuck(0x6F86B6,5);
	memset((PVOID)0x6F86B6,0x90,5);

	// TEMP VEHICLE AUDIO PATCHES -------------------
	/* Audio_SwitchRadioOff (nulled)
	UnFuck(0x4F5BA0,1);
	*(BYTE *)0x4F5BA0 = 0xC3;
	// Audio_SetRadioStation (nulled)
	UnFuck(0x4FD0B0,1);
	*(BYTE *)0x4FD0B0 = 0xC3;
	// Audio_SwitchRadioOn (whole proc nulled)
    UnFuck(0x4F5B20,1);
	*(BYTE *)0x4F5B20 = 0xC3;*/

	/* Nulling all locations that set AudioPed
	UnFuck(0x4FBAD6,6);
	memset((PVOID)0x4FBAD6,0x90,6);
	UnFuck(0x4FCF8F,6);
	memset((PVOID)0x4FCF8F,0x90,6);
	UnFuck(0x4FCFBF,6);
	memset((PVOID)0x4FCFBF,0x90,6);
	UnFuck(0x4FCFF5,6);
	memset((PVOID)0x4FCFF5,0x90,6);
	UnFuck(0x4F5716,6);
	memset((PVOID)0x4F5716,0x90,6);*/

	//-----------------------------------------------
	
	// camera_on_actor patch, tsk tsk R*
	UnFuck(0x0047C477,1);
	*(BYTE*)0x0047C477 = 0xEB;

	// CPushBike fires set on CPed patch
	UnFuck(0x0053A984,2);
	*(BYTE*)0x0053A984 = 0xEB;  // jmp
	*(BYTE*)0x0053A985 = 0x77;  // +77h = 0x0053A9FD

	// Stop sniper clicking
	UnFuck(0x0060F289, 8);
	memset((PVOID)0x0060F289, 0x90, 8);
	UnFuck(0x0060F29D, 19);
	memset((PVOID)0x0060F29D, 0x90, 19);
	
	// Automatic go-to-menu on alt+tab
	//UnFuck(0x748063, 5);
	//memset((PVOID)0x748063, 0x90, 5);

	// Wanted level hook
	UnFuck(0x58DB5F, 9);
	*(BYTE*)0x58DB5F = 0xBD;
	*(BYTE*)0x58DB60 = 0x00;
	*(BYTE*)0x58DB61 = 0x00;
	*(BYTE*)0x58DB62 = 0x00;
	*(BYTE*)0x58DB63 = 0x00;
	*(BYTE*)0x58DB64 = 0x90;
	*(BYTE*)0x58DB65 = 0x90;
	*(BYTE*)0x58DB66 = 0x90;
	*(BYTE*)0x58DB67 = 0x90;
	
	// Remove the blue(-ish) fog in the map
	UnFuck(0x00575B0E, 5);
	memset((PVOID)0x00575B0E, 0x90, 5);

	UnFuck(0x47BF54,4);
	InstallSCMEventsProcessor();
}

//----------------------------------------------------------

#pragma pack(1)
typedef struct _PED_MODEL
{
	DWORD func_tbl;
	BYTE  data[64];
} PED_MODEL;

PED_MODEL PedModelsMemory[300];

void RelocatePedsListHack()
{
    BYTE *aPedsListMemory = (BYTE*)&PedModelsMemory[0];

	// Init the mem
	int x=0;
	while(x!=300) {
		PedModelsMemory[x].func_tbl = 0x85BDC0;
		memset(PedModelsMemory[x].data,0,64);
		x++;
	}
	// Patch the GetPedsModelInfo to use us
	// instead of the gta_sa.exe mem.
	UnFuck(0x4C67AD,4);
	*(DWORD *)0x4C67AD = (DWORD)aPedsListMemory;
}

//----------------------------------------------------------
// FOLLOWING IS TO RELOCATE THE SCANLIST MEMORY, A BIG
// HACK THAT ALLOWS US TO HAVE MORE THAN 2 CPlayerInfo STRUCTURES.

#define SCANLIST_SIZE 8*20000

unsigned char ScanListMemory[SCANLIST_SIZE];

// Pointers to actual code addresses to patch. The first list
// has taken into account the instruction bytes, second list
// does not. The second list is guaranteed to have 3 byte
// instructions before the new address.

DWORD dwPatchAddrScanReloc1USA[14] = {
0x5DC7AA,0x41A85D,0x41A864,0x408259,0x711B32,0x699CF8,
0x4092EC,0x40914E,0x408702,0x564220,0x564172,0x563845,
0x84E9C2,0x85652D };

DWORD dwPatchAddrScanReloc1EU[14] = {
0x5DC7AA,0x41A85D,0x41A864,0x408261,0x711B32,0x699CF8,
0x4092EC,0x40914E,0x408702,0x564220,0x564172,0x563845,
0x84EA02,0x85656D };

// Lots of hex.. that's why they call us a "determined group of hackers"

DWORD dwPatchAddrScanReloc2USA[56] = {
0x0040D68C,0x005664D7,0x00566586,0x00408706,0x0056B3B1,0x0056AD91,0x0056A85F,0x005675FA,
0x0056CD84,0x0056CC79,0x0056CB51,0x0056CA4A,0x0056C664,0x0056C569,0x0056C445,0x0056C341,
0x0056BD46,0x0056BC53,0x0056BE56,0x0056A940,0x00567735,0x00546738,0x0054BB23,0x006E31AA,
0x0040DC29,0x00534A09,0x00534D6B,0x00564B59,0x00564DA9,0x0067FF5D,0x00568CB9,0x00568EFB,
0x00569F57,0x00569537,0x00569127,0x0056B4B5,0x0056B594,0x0056B2C3,0x0056AF74,0x0056AE95,
0x0056BF4F,0x0056ACA3,0x0056A766,0x0056A685,0x0070B9BA,0x0056479D,0x0070ACB2,0x006063C7,
0x00699CFE,0x0041A861,0x0040E061,0x0040DF5E,0x0040DDCE,0x0040DB0E,0x0040D98C,0x01566855 };

DWORD dwPatchAddrScanReloc2EU[56] = {
0x0040D68C,0x005664D7,0x00566586,0x00408706,0x0056B3B1,0x0056AD91,0x0056A85F,0x005675FA,
0x0056CD84,0x0056CC79,0x0056CB51,0x0056CA4A,0x0056C664,0x0056C569,0x0056C445,0x0056C341,
0x0056BD46,0x0056BC53,0x0056BE56,0x0056A940,0x00567735,0x00546738,0x0054BB23,0x006E31AA,
0x0040DC29,0x00534A09,0x00534D6B,0x00564B59,0x00564DA9,0x0067FF5D,0x00568CB9,0x00568EFB,
0x00569F57,0x00569537,0x00569127,0x0056B4B5,0x0056B594,0x0056B2C3,0x0056AF74,0x0056AE95,
0x0056BF4F,0x0056ACA3,0x0056A766,0x0056A685,0x0070B9BA,0x0056479D,0x0070ACB2,0x006063C7,
0x00699CFE,0x0041A861,0x0040E061,0x0040DF5E,0x0040DDCE,0x0040DB0E,0x0040D98C,0x01566845 };

DWORD dwPatchAddrScanReloc3[11] = {
0x004091C5,0x00409367,0x0040D9C5,0x0040DB47,0x0040DC61,0x0040DE07,0x0040DF97,
0x0040E09A,0x00534A98,0x00534DFA,0x0071CDB0 };

// For End
// 0xB992B8 is reffed for checking end of scanlist... rewrite this to point to end of new list
DWORD dwPatchAddrScanRelocEnd[4] = { 0x005634A6, 0x005638DF, 0x0056420F, 0x00564283 };


//-----------------------------------------------------------

void RelocateScanListHack()
{
	DWORD oldProt;
	memset(&ScanListMemory[0], 0, SCANLIST_SIZE);
	unsigned char *aScanListMemory = &ScanListMemory[0];

	// FIRST PREPARED LIST OF ACCESSORS
	int x=0;
	while(x!=14) {
		if(iGtaVersion == GTASA_VERSION_USA10) {
			VirtualProtect((PVOID)dwPatchAddrScanReloc1USA[x],4,PAGE_EXECUTE_READWRITE,&oldProt);
			*(PDWORD)dwPatchAddrScanReloc1USA[x] = (DWORD)aScanListMemory;
		}
		else if(iGtaVersion == GTASA_VERSION_EU10) {
			VirtualProtect((PVOID)dwPatchAddrScanReloc1EU[x],4,PAGE_EXECUTE_READWRITE,&oldProt);
			*(PDWORD)dwPatchAddrScanReloc1EU[x] = (DWORD)aScanListMemory;
		}
		x++;
	}

	// SECOND PREPARED LIST OF ACCESSORS <G>
	x=0;
	while(x!=56) {
		if(iGtaVersion == GTASA_VERSION_USA10) {
			VirtualProtect((PVOID)dwPatchAddrScanReloc2USA[x],8,PAGE_EXECUTE_READWRITE,&oldProt);
			*(PDWORD)(dwPatchAddrScanReloc2USA[x] + 3) = (DWORD)aScanListMemory;
		}
		else if(iGtaVersion == GTASA_VERSION_EU10) {
			VirtualProtect((PVOID)dwPatchAddrScanReloc2EU[x],8,PAGE_EXECUTE_READWRITE,&oldProt);
			*(PDWORD)(dwPatchAddrScanReloc2EU[x] + 3) = (DWORD)aScanListMemory;
		}
		x++;
	}

	// THIRD LIST THAT POINTS TO THE BASE SCANLIST MEMORY + 4
	x=0;
	while(x!=11) {
		VirtualProtect((PVOID)dwPatchAddrScanReloc3[x],8,PAGE_EXECUTE_READWRITE,&oldProt);
		*(PDWORD)(dwPatchAddrScanReloc3[x] + 3) = (DWORD)(aScanListMemory+4);
		x++;
	}	

	// FOURTH LIST THAT POINTS TO THE END OF THE SCANLIST
	x=0;
	while(x!=4) {
		VirtualProtect((PVOID)dwPatchAddrScanRelocEnd[x],4,PAGE_EXECUTE_READWRITE,&oldProt);
		*(PDWORD)(dwPatchAddrScanRelocEnd[x]) = (DWORD)(aScanListMemory+sizeof(ScanListMemory));
		x++;
	}	

	// Others that didn't fit.
	VirtualProtect((PVOID)0x40936A,4,PAGE_EXECUTE_READWRITE,&oldProt);
	*(PDWORD)0x40936A = (DWORD)(aScanListMemory+4);

	// Reset the exe scanlist mem for playerinfo's
	memset((BYTE*)0xB7D0B8,0,8*14400);
}

//----------------------------------------------------------

