#include "main.h"
#include "game/util.h"

extern CChatWindow *pChatWindow;
extern CNetGame *pNetGame;
extern CGame *pGame;

#define NUDE void _declspec(naked) 

DWORD dwParams[4];

#define EVENT_TYPE_PAINTJOB			1
#define EVENT_TYPE_CARCOMPONENT		2
#define EVENT_TYPE_CARCOLOR			3
#define EVENT_ENTEREXIT_MODSHOP		4

void SendScmEvent(int iEventType, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);
void SendMoneyIncrease(DWORD dwIncreaseType, DWORD iAmount);

DWORD dwStack;

extern BOOL bFirstSpawn;

//----------------------------------------------------------

void ProcessIncommingEvent(PLAYERID playerId, int iEventType, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
	DWORD v;
	int iVehicleID;
	int iPaintJob;
	int iComponent;
	int iWait;
	CVehicle *pVehicle;
	CRemotePlayer *pRemote;

	if(!pNetGame) return;
	if(bFirstSpawn) return; // Local player has never spawned.

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	
#ifdef _DEBUG
	//pChatWindow->AddDebugMessage("get_event(%u,%u,%u,%u,%u)",playerId,iEventType,dwParam1,dwParam2,dwParam3);
#endif

	switch(iEventType) {

		case EVENT_TYPE_PAINTJOB:
			iVehicleID = pVehiclePool->FindGtaIDFromID(dwParam1);
			iPaintJob = (int)dwParam2;
			if(iVehicleID) ScriptCommand(&change_car_skin,iVehicleID,dwParam2);
			break;

		case EVENT_TYPE_CARCOMPONENT:
			iVehicleID = pVehiclePool->FindGtaIDFromID(dwParam1);
			iComponent = (int)dwParam2;
			pGame->RequestModel(iComponent);
			pGame->LoadRequestedModels();
			ScriptCommand(&request_car_component,iComponent);

			iWait = 10;
			while(!ScriptCommand(&is_component_available,iComponent) && iWait) {
				Sleep(5);
				iWait--;
			}
			if(!iWait) {
				//pChatWindow->AddDebugMessage("Timeout on car component.");
				break;
			}
			if (ScriptCommand(&is_component_available, iComponent)) ScriptCommand(&add_car_component, iVehicleID, iComponent, &v);
			//pChatWindow->AddDebugMessage("Added car component: %d",iComponent);
			break;

		case EVENT_TYPE_CARCOLOR:
			pVehicle = pVehiclePool->GetAt((VEHICLEID)dwParam1);
			if(pVehicle) pVehicle->SetColor((int)dwParam2,(int)dwParam3);
			break;

		case EVENT_ENTEREXIT_MODSHOP:
			if(playerId > MAX_PLAYERS) return;
			pVehicle = pVehiclePool->GetAt((VEHICLEID)dwParam1);
			pRemote = pNetGame->GetPlayerPool()->GetAt(playerId);
			if(pRemote && pVehicle) {
				//pVehicle->SetLockedState((int)dwParam2); // Results in a crash at 0x48CFC9 under certain conditions.
				pRemote->m_iIsInAModShop = (int)dwParam2;
			}
			break;

	}
}

//----------------------------------------------------------

void ProcessOutgoingEvent(int iEventType, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
	if(!pNetGame) return;

	int iVehicleID;
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	//pChatWindow->AddDebugMessage("set_event(%u,%u,%u,%u)",iEventType,dwParam1,dwParam2,dwParam3);


	switch(iEventType) {

		case EVENT_TYPE_PAINTJOB:
			iVehicleID = pVehiclePool->FindIDFromGtaPtr(GamePool_Vehicle_GetAt(dwParam1));
			if(iVehicleID != INVALID_VEHICLE_ID) {
				SendScmEvent(EVENT_TYPE_PAINTJOB,iVehicleID,dwParam2,0);		
			}
			break;

		case EVENT_TYPE_CARCOMPONENT:
			iVehicleID = pVehiclePool->FindIDFromGtaPtr(GamePool_Vehicle_GetAt(dwParam1));
			if(iVehicleID != INVALID_VEHICLE_ID) {
				SendScmEvent(EVENT_TYPE_CARCOMPONENT,iVehicleID,dwParam2,0);
			}
			break;

		case EVENT_TYPE_CARCOLOR:
			iVehicleID = pVehiclePool->FindIDFromGtaPtr(GamePool_Vehicle_GetAt(dwParam1));
			if(iVehicleID != INVALID_VEHICLE_ID) {
				SendScmEvent(EVENT_TYPE_CARCOLOR,iVehicleID,dwParam2,dwParam3);
			}
			break;

		case EVENT_ENTEREXIT_MODSHOP:
			iVehicleID = pVehiclePool->FindIDFromGtaPtr(GamePool_Vehicle_GetAt(dwParam1));
			if(iVehicleID != INVALID_VEHICLE_ID) {
				SendScmEvent(EVENT_ENTEREXIT_MODSHOP,iVehicleID,dwParam2,0);
			}
			break;

	}
}

//----------------------------------------------------------

NUDE SetEventCustomOpcode()
{
	_asm pushad

	_asm push 4
	_asm mov ecx, esi
	_asm mov ebx, 0x464080 // CRunningScript::CollectParamaters()
	_asm call ebx

	dwParams[0] = *(PDWORD)0xA43C78;
	dwParams[1] = *(PDWORD)0xA43C7C;
	dwParams[2] = *(PDWORD)0xA43C80;
	dwParams[3] = *(PDWORD)0xA43C84;

	ProcessOutgoingEvent(dwParams[0],dwParams[1],dwParams[2],dwParams[3]);
		
	_asm popad

	_asm mov ebx, 0x47BF2B // return to ProcessCommands2500To2600
	_asm jmp ebx
}

//----------------------------------------------------------

NUDE ScriptMoneyIncrease()
{
	_asm pushad

	_asm push 2
	_asm mov ecx, esi
	_asm mov ebx, 0x464080 // CRunningScript::CollectParamaters()
	_asm call ebx

	dwParams[0] = *(PDWORD)0xA43C78;	// Amount of money
	dwParams[1] = *(PDWORD)0xA43C7C;	// Type of increase

	if (dwParams[0] != 0)
		SendMoneyIncrease( dwParams[1], dwParams[0] );

	_asm popad

	_asm mov ebx, 0x47BF2B // return to ProcessCommands2500To2600
	_asm jmp ebx
}

//----------------------------------------------------------

void SendMoneyIncrease(DWORD dwIncreaseType, DWORD dwAmount)
{
	if (!pNetGame) return;
	if (!pNetGame->GetPlayerPool()) return;
	int iAmount = dwAmount;

	// "Increase" types 4 and five for negative value's
	if (dwIncreaseType >= 4) {
		iAmount = 0 - iAmount;
		dwIncreaseType -= 2;
	}

	RakNet::BitStream bsSend;
	bsSend.Write(iAmount);
	bsSend.Write(dwIncreaseType);
	//pNetGame->GetRakClient()->RPC(&RPC_ScriptCash,&bsSend,HIGH_PRIORITY,RELIABLE_SEQUENCED,0,false);
}

//----------------------------------------------------------

void SendScmEvent(int iEventType, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
	RakNet::BitStream bsSend;
	bsSend.Write(iEventType);
	bsSend.Write(dwParam1);
	bsSend.Write(dwParam2);
	bsSend.Write(dwParam3);
	pNetGame->GetRakClient()->RPC(&RPC_ScmEvent,&bsSend,HIGH_PRIORITY,RELIABLE_SEQUENCED,0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

//----------------------------------------------------------

void InstallSCMEventsProcessor()
{
	*(DWORD *)0x47BF54 = (DWORD)SetEventCustomOpcode; // opcode 09C6 - 2502
	*(DWORD *)0x47BF88 = (DWORD)ScriptMoneyIncrease; // opcode 09D3 - 2515
}

//----------------------------------------------------------
