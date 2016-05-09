#include "../main.h"

using namespace RakNet;
extern CNetGame*	pNetGame;
extern CChatWindow*	pChatWindow;
extern CDeathWindow	*pDeathWindow;
extern CGame * pGame;

//----------------------------------------------------

void ScrSetSpawnInfo(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	PLAYER_SPAWN_INFO SpawnInfo;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	bsData.Read((PCHAR)&SpawnInfo, sizeof(PLAYER_SPAWN_INFO));

	/*
	pChatWindow->AddDebugMessage("Got Spawn Info: %i %i %f %f %f %f %i %i %i %i %i %i",
								SpawnInfo.byteTeam,
								SpawnInfo.iSkin,
								SpawnInfo.vecPos.X,
								SpawnInfo.vecPos.Y,
								SpawnInfo.vecPos.Z,
								SpawnInfo.fRotation,
								SpawnInfo.iSpawnWeapons[0],
								SpawnInfo.iSpawnWeaponsAmmo[0],
								SpawnInfo.iSpawnWeapons[1],
								SpawnInfo.iSpawnWeaponsAmmo[1],
								SpawnInfo.iSpawnWeapons[2],
								SpawnInfo.iSpawnWeaponsAmmo[2]);*/

	pPlayerPool->GetLocalPlayer()->SetSpawnInfo(&SpawnInfo);
}

//----------------------------------------------------

void ScrSetPlayerTeam(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	PLAYERID playerId;
	BYTE byteTeam;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	bsData.Read(playerId);
	bsData.Read(byteTeam);
	
	if (playerId == pPlayerPool->GetLocalPlayerID()) {
		pPlayerPool->GetLocalPlayer()->SetTeam(byteTeam);
	} else {
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
		if(pPlayer) pPlayer->SetTeam(byteTeam);
	}
}

//----------------------------------------------------

void ScrSetPlayerName(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	PLAYERID playerId;
	BYTE byteNickLen;
	char szNewName[MAX_PLAYER_NAME+1];
	BYTE byteSuccess;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	bsData.Read(playerId);
	bsData.Read(byteNickLen);

	if(byteNickLen > MAX_PLAYER_NAME) return;

	bsData.Read(szNewName, byteNickLen);
	bsData.Read(byteSuccess);

	szNewName[byteNickLen] = '\0';

	if (byteSuccess == 1) pPlayerPool->SetPlayerName(playerId, szNewName);
	
	// Extra addition which we need to do if this is the local player;
	if( pPlayerPool->GetLocalPlayerID() == playerId )
		pPlayerPool->SetLocalPlayerName( szNewName );
}

void ScrSetPlayerSkin(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	int iPlayerID;
	unsigned int uiSkin;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	bsData.Read(iPlayerID);
	bsData.Read(uiSkin);

	if (iPlayerID == pPlayerPool->GetLocalPlayerID()) {
		pPlayerPool->GetLocalPlayer()->GetPlayerPed()->SetModelIndex(uiSkin);
	}
	else {
		if(pPlayerPool->GetSlotState(iPlayerID) && pPlayerPool->GetAt(iPlayerID)->GetPlayerPed()) {
			pPlayerPool->GetAt(iPlayerID)->GetPlayerPed()->SetModelIndex(uiSkin);
		}
	}
}

//----------------------------------------------------

void ScrSetPlayerPos(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();

	VECTOR vecPos;

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	pLocalPlayer->GetPlayerPed()->TeleportTo(vecPos.X,vecPos.Y,vecPos.Z);
}

//----------------------------------------------------

void ScrSetPlayerPosFindZ(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();

	VECTOR vecPos;

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	vecPos.Z = pGame->FindGroundZForCoord(vecPos.X, vecPos.Y, vecPos.Z);
	vecPos.Z += 1.5f;

	pLocalPlayer->GetPlayerPed()->TeleportTo(vecPos.X, vecPos.Y, vecPos.Z);
}

//----------------------------------------------------

void ScrSetPlayerHealth(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	float fHealth;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(fHealth);

	//pChatWindow->AddDebugMessage("Setting your health to: %f", fHealth);
	pLocalPlayer->GetPlayerPed()->SetHealth(fHealth);
}

//----------------------------------------------------

void ScrPutPlayerInVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	VEHICLEID vehicleid;
	BYTE seatid;
	bsData.Read(vehicleid);
	bsData.Read(seatid);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	int iVehicleIndex = pNetGame->GetVehiclePool()->FindGtaIDFromID(vehicleid);
	CVehicle *pVehicle = pNetGame->GetVehiclePool()->GetAt(vehicleid);

	if(iVehicleIndex && pVehicle) {
		 pGame->FindPlayerPed()->PutDirectlyInVehicle(iVehicleIndex, seatid);
	}
}

//----------------------------------------------------

void ScrRemovePlayerFromVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	pPlayerPool->GetLocalPlayer()->GetPlayerPed()->ExitCurrentVehicle();
}

//----------------------------------------------------

void ScrSetPlayerColor(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	PLAYERID playerId;
	DWORD dwColor;

	bsData.Read(playerId);
	bsData.Read(dwColor);

	if(playerId == pPlayerPool->GetLocalPlayerID()) {
		pPlayerPool->GetLocalPlayer()->SetPlayerColor(dwColor);
	} else {
		CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
		if(pPlayer)	pPlayer->SetPlayerColor(dwColor);
	}
}

//----------------------------------------------------

void ScrDisplayGameText(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	char szMessage[512];
	int iType;
	int iTime;
	int iLength;

	bsData.Read(iType);
	bsData.Read(iTime);
	bsData.Read(iLength);

	if(iLength > 512) return;

	bsData.Read(szMessage,iLength);
	szMessage[iLength] = '\0';

	pGame->DisplayGameText(szMessage,iTime,iType);
}

//----------------------------------------------------

void ScrSetInterior(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteInterior;
	bsData.Read(byteInterior);
	
	//pChatWindow->AddDebugMessage("ScrSetInterior(%u)",byteInterior);

	pGame->FindPlayerPed()->SetInterior(byteInterior);	
}

//----------------------------------------------------

void ScrSetCameraPos(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	VECTOR vecPos;
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	pGame->GetCamera()->SetPosition(vecPos.X,vecPos.Y,vecPos.Z,0.0f,0.0f,0.0f);
}

//----------------------------------------------------

void ScrSetCameraLookAt(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	VECTOR vecPos;
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	pGame->GetCamera()->LookAtPoint(vecPos.X,vecPos.Y,vecPos.Z,2);	
}

//----------------------------------------------------

void ScrSetVehiclePos(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	VEHICLEID VehicleId;
	float fX, fY, fZ;
	bsData.Read(VehicleId);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);

	if(pNetGame && pNetGame->GetVehiclePool()) {
		if(pNetGame->GetVehiclePool()->GetSlotState(VehicleId)) {
			pNetGame->GetVehiclePool()->GetAt(VehicleId)->TeleportTo(fX, fY, fZ);
		}
	}
}

//----------------------------------------------------

void ScrSetVehicleZAngle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	VEHICLEID VehicleId;
	float fZAngle;
	bsData.Read(VehicleId);
	bsData.Read(fZAngle);

	ScriptCommand(&set_car_z_angle, pNetGame->GetVehiclePool()->GetAt(VehicleId)->m_dwGTAId, fZAngle);
}

//----------------------------------------------------

void ScrVehicleParams(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	VEHICLEID VehicleID;
	BYTE byteObjectiveVehicle;
	BYTE byteDoorsLocked;

	bsData.Read(VehicleID);
	bsData.Read(byteObjectiveVehicle);
	bsData.Read(byteDoorsLocked);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	pVehiclePool->AssignSpecialParamsToVehicle(VehicleID,byteObjectiveVehicle,byteDoorsLocked);

}

//----------------------------------------------------

void ScrLinkVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	VEHICLEID VehicleID;
	BYTE byteInterior;

	bsData.Read(VehicleID);
	bsData.Read(byteInterior);

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	pVehiclePool->LinkToInterior(VehicleID, (int)byteInterior);
}

//----------------------------------------------------

void ScrSetCameraBehindPlayer(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	pGame->GetCamera()->SetBehindPlayer();	
}

//----------------------------------------------------

void ScrTogglePlayerControllable(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteControllable;
	bsData.Read(byteControllable);
	pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->TogglePlayerControllable((int)byteControllable);
}

//----------------------------------------------------

void ScrPlaySound(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	int iSound;
	float fX, fY, fZ;
	bsData.Read(iSound);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);

	pGame->PlaySound(iSound, fX, fY, fZ);
}

//----------------------------------------------------

void ScrSetWorldBounds(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(pNetGame->m_WorldBounds[0]);
	bsData.Read(pNetGame->m_WorldBounds[1]);
	bsData.Read(pNetGame->m_WorldBounds[2]);
	bsData.Read(pNetGame->m_WorldBounds[3]);
}

//----------------------------------------------------

void ScrHaveSomeMoney(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	int iAmount;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(iAmount);
	pGame->AddToLocalMoney(iAmount);
}

//----------------------------------------------------

void ScrSetPlayerFacingAngle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	float fAngle;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(fAngle);
	pGame->FindPlayerPed()->ForceTargetRotation(fAngle);
}

//----------------------------------------------------

void ScrResetMoney(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	pGame->ResetLocalMoney();
}

//----------------------------------------------------

void ScrResetPlayerWeapons(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	CPlayerPed *pPlayerPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
	pPlayerPed->ClearAllWeapons();
	//pChatWindow->AddDebugMessage("Cleared weapons");
}

//----------------------------------------------------

void ScrGivePlayerWeapon(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	int iWeaponID;
	int iAmmo;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(iWeaponID);
	bsData.Read(iAmmo);

	CPlayerPed *pPlayerPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
	pPlayerPed->GiveWeapon(iWeaponID, iAmmo);
	//pChatWindow->AddDebugMessage("Gave weapon: %d with ammo %d", iWeaponID, iAmmo);
}

//----------------------------------------------------

void ScrRespawnVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	VEHICLEID VehicleID;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(VehicleID);	
}

//----------------------------------------------------

void ScrDeathMessage(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	PLAYERID Killer, Killee;
	BYTE byteWeapon;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	PCHAR szKillerName = NULL;
	PCHAR szKilleeName = NULL;
	DWORD dwKillerColor = 0;
	DWORD dwKilleeColor = 0;

	bsData.Read(Killer);
	bsData.Read(Killee);
	bsData.Read(byteWeapon);

	//pChatWindow->AddDebugMessage("RawDeath: %u %u %u",Killer,Killee,byteWeapon);

	if(Killee == INVALID_PLAYER_ID) return;

	// Determine the killer's name and color
	if(Killer == INVALID_PLAYER_ID) {
		szKillerName = NULL; dwKillerColor = 0;
	} else {
		if(pPlayerPool->GetLocalPlayerID() == Killer) {
			szKillerName = pPlayerPool->GetLocalPlayerName();
			dwKillerColor = pPlayerPool->GetLocalPlayer()->GetPlayerColorAsARGB();
		} else {
			if(pPlayerPool->GetSlotState(Killer)) {
				szKillerName = pPlayerPool->GetPlayerName(Killer);
				dwKillerColor = pPlayerPool->GetAt(Killer)->GetPlayerColorAsARGB();
			} else {
				//pChatWindow->AddDebugMessage("Slot State Killer FALSE");
				szKillerName = NULL; dwKillerColor = 0;
			}
		}
	}

	// Determine the killee's name and color
	if(pPlayerPool->GetLocalPlayerID() == Killee) {
		szKilleeName = pPlayerPool->GetLocalPlayerName();
		dwKilleeColor = pPlayerPool->GetLocalPlayer()->GetPlayerColorAsARGB();
	} else {
		if(pPlayerPool->GetSlotState(Killee)) {
			szKilleeName = pPlayerPool->GetPlayerName(Killee);
			dwKilleeColor = pPlayerPool->GetAt(Killee)->GetPlayerColorAsARGB();
		} else {
			//pChatWindow->AddDebugMessage("Slot State Killee FALSE");
			szKilleeName = NULL; dwKilleeColor = 0;
		}
	}

	if(pDeathWindow) pDeathWindow->AddMessage(szKillerName,szKilleeName,dwKillerColor,dwKilleeColor,byteWeapon);
}

//----------------------------------------------------

void ScrSetMapIcon(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	BYTE byteIndex;
	BYTE byteIcon;
	DWORD byteColor;
	float fPos[3];

	bsData.Read(byteIndex);
	bsData.Read(fPos[0]);
	bsData.Read(fPos[1]);
	bsData.Read(fPos[2]);
	bsData.Read(byteIcon);
	bsData.Read(byteColor);

	pNetGame->SetMapIcon(byteIndex, fPos[0], fPos[1], fPos[2], byteIcon, byteColor);
}

void ScrDisableMapIcon(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	BYTE byteIndex;

	bsData.Read(byteIndex);

	pNetGame->DisableMapIcon(byteIndex);
}

void ScrSetPlayerArmour(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	float fArmour;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(fArmour);

	pLocalPlayer->GetPlayerPed()->SetArmour(fArmour);
}

void ScrSetWeaponAmmo(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	BYTE byteWeapon;
	WORD wordAmmo;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(byteWeapon);
	bsData.Read(wordAmmo);

	pLocalPlayer->GetPlayerPed()->SetAmmo(byteWeapon, wordAmmo);
}

//----------------------------------------------------

void ScrSetGravity(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	float fGravity;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(fGravity);

	pGame->SetGravity(fGravity);
}

//----------------------------------------------------

void ScrSetVehicleHealth(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	float fVehicleHealth;
	VEHICLEID VehicleID;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(VehicleID);
	bsData.Read(fVehicleHealth);

	if (pNetGame->GetVehiclePool()->GetSlotState(VehicleID))
	{
		pNetGame->GetVehiclePool()->GetAt(VehicleID)->SetHealth(fVehicleHealth);
	}
}

//----------------------------------------------------

void ScrAttachTrailerToVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	VEHICLEID TrailerID, VehicleID;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(TrailerID);
	bsData.Read(VehicleID);
	CVehicle* pTrailer = pNetGame->GetVehiclePool()->GetAt(TrailerID);
	CVehicle* pVehicle = pNetGame->GetVehiclePool()->GetAt(VehicleID);
	
	pVehicle->SetTrailer(pTrailer);
	pVehicle->AttachTrailer();
}

//----------------------------------------------------

void ScrDetachTrailerFromVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	VEHICLEID VehicleID;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(VehicleID);
	CVehicle* pVehicle = pNetGame->GetVehiclePool()->GetAt(VehicleID);

	pVehicle->DetachTrailer();
	pVehicle->SetTrailer(NULL);
}

//----------------------------------------------------

void ScrCreateObject(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	byte byteObjectID;
	int iModel;
	VECTOR vecPos, vecRot;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(byteObjectID);
	bsData.Read(iModel);

	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	bsData.Read(vecRot.X);
	bsData.Read(vecRot.Y);
	bsData.Read(vecRot.Z);

	CObjectPool* pObjectPool =	pNetGame->GetObjectPool();
	pObjectPool->New(byteObjectID, iModel, vecPos, vecRot);
}

//----------------------------------------------------

void ScrSetObjectPos(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	byte byteObjectID;
	float fX, fY, fZ, fRotation;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(byteObjectID);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	bsData.Read(fRotation);

	CObjectPool*	pObjectPool =	pNetGame->GetObjectPool();
	CObject*		pObject		=	pObjectPool->GetAt(byteObjectID);
	if (pObject)
	{
		pObject->TeleportTo(fX, fY, fZ);
	}
}

//----------------------------------------------------

void ScrSetObjectRotation(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	byte byteObjectID;
	float fX, fY, fZ;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(byteObjectID);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);

	CObjectPool*	pObjectPool =	pNetGame->GetObjectPool();
	CObject*		pObject		=	pObjectPool->GetAt(byteObjectID);
	if (pObject)
	{
		pObject->InstantRotate(fX, fY, fZ);
	}
}

//----------------------------------------------------

void ScrDestroyObject(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	byte byteObjectID;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(byteObjectID);

	CObjectPool* pObjectPool =	pNetGame->GetObjectPool();
	if (pObjectPool->GetAt(byteObjectID))
	{
		pObjectPool->Delete(byteObjectID);
	}
}

void ScrCreateExplosion(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	float X, Y, Z, Radius;
	int   iType;

	bsData.Read(X);
	bsData.Read(Y);
	bsData.Read(Z);
	bsData.Read(iType);
	bsData.Read(Radius);

	ScriptCommand(&create_explosion_with_radius, X, Y, Z, iType, Radius);
}

void ScrShowNameTag(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	PLAYERID playerId;
	BYTE byteShow;

	bsData.Read(playerId);
	bsData.Read(byteShow);

	if (pNetGame->GetPlayerPool()->GetSlotState(playerId))
	{
		pNetGame->GetPlayerPool()->GetAt(playerId)->ShowNameTag(byteShow);
	}
}

void ScrMoveObject(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteObjectID;
	float curx, cury, curz, newx, newy, newz, speed;

	bsData.Read(byteObjectID);
	bsData.Read(curx);
	bsData.Read(cury);
	bsData.Read(curz);
	bsData.Read(newx);
	bsData.Read(newy);
	bsData.Read(newz);
	bsData.Read(speed);

	CObject* pObject = pNetGame->GetObjectPool()->GetAt(byteObjectID);
	if (pObject)
	{
		pObject->TeleportTo(curx, cury, curz);
		pObject->MoveTo(newx, newy, newz, speed);
	}
}

void ScrStopObject(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BYTE byteObjectID;
	float newx, newy, newz;

	bsData.Read(byteObjectID);
	bsData.Read(newx);
	bsData.Read(newy);
	bsData.Read(newz);

	CObject* pObject = pNetGame->GetObjectPool()->GetAt(byteObjectID);
	if (pObject)
	{
		pObject->MoveTo(newx, newy, newz, pObject->m_fMoveSpeed);
	}
}

void ScrNumberPlate(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	VEHICLEID Vehicle;
	CHAR cNumberPlate[9];
	
	bsData.Read(Vehicle);
	bsData.Read(cNumberPlate, 9);
}

//----------------------------------------------------

void ScrTogglePlayerSpectating(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	BOOL bToggle;
	bsData.Read(bToggle);
	pPlayerPool->GetLocalPlayer()->ToggleSpectating(bToggle);
}

void ScrSetPlayerSpectating(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	PLAYERID playerId;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	bsData.Read(playerId);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	if (pPlayerPool->GetSlotState(playerId)) {
		pPlayerPool->GetAt(playerId)->SetState(PLAYER_STATE_SPECTATING);
	}
}

#define SPECTATE_TYPE_NORMAL	1
#define SPECTATE_TYPE_FIXED		2
#define SPECTATE_TYPE_SIDE		3

void ScrPlayerSpectatePlayer(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	PLAYERID playerId;
    BYTE byteMode;
	
	bsData.Read(playerId);
	bsData.Read(byteMode);

	switch (byteMode) {
		case SPECTATE_TYPE_FIXED:
			byteMode = 15;
			break;
		case SPECTATE_TYPE_SIDE:
			byteMode = 14;
			break;
		default:
			byteMode = 4;
	}
	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	pLocalPlayer->m_byteSpectateMode = byteMode;
	pLocalPlayer->SpectatePlayer(playerId);
}

void ScrPlayerSpectateVehicle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	VEHICLEID VehicleID;
    BYTE byteMode;
	
	bsData.Read(VehicleID);
	bsData.Read(byteMode);

	//pChatWindow->AddDebugMessage("ScrPlayerSpectateVehicle(%u,%u)",VehicleID,byteMode);
    
	switch (byteMode) {
		case SPECTATE_TYPE_FIXED:
			byteMode = 15;
			break;
		case SPECTATE_TYPE_SIDE:
			byteMode = 14;
			break;
		default:
			byteMode = 3;
	}
	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	pLocalPlayer->m_byteSpectateMode = byteMode;
	pLocalPlayer->SpectateVehicle(VehicleID);
}

void ScrRemoveComponent(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	VEHICLEID VehicleID;
    DWORD dwComponent;
	
	bsData.Read(VehicleID);
	bsData.Read(dwComponent);

	int iVehicleID;
	//int iComponent;

	if(!pNetGame) return;

	iVehicleID = pNetGame->GetVehiclePool()->FindGtaIDFromID(VehicleID);
	if(iVehicleID) ScriptCommand(&remove_component, iVehicleID, (int)dwComponent);
}

void ScrForceSpawnSelection(RPCParameters *rpcParams)
{
	pNetGame->GetPlayerPool()->GetLocalPlayer()->ReturnToClassSelection();
}

void ScrAttachObjectToPlayer(RPCParameters *rpcParams)
{
	int iBitLength	= rpcParams->numberOfBitsOfData;
	PlayerID sender	= rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input, (iBitLength/8)+1, false);

	BYTE byteObjectID;
	PLAYERID playerId;

	float OffsetX, OffsetY, OffsetZ, rX, rY, rZ;

	bsData.Read( byteObjectID );
	bsData.Read( playerId );

	bsData.Read( OffsetX );
	bsData.Read( OffsetY );
	bsData.Read( OffsetZ );

	bsData.Read( rX );
	bsData.Read( rY );
	bsData.Read( rZ );

	try {

	CObject* pObject =	pNetGame->GetObjectPool()->GetAt(	byteObjectID );

	if ( !pObject )
		return;

	if ( playerId == pNetGame->GetPlayerPool()->GetLocalPlayerID() )
	{
		CLocalPlayer* pPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
		ScriptCommand( &attach_object_to_actor, pObject->m_dwGTAId, pPlayer->GetPlayerPed()->m_dwGTAId,
			OffsetX,
			OffsetY,
			OffsetZ, 
			rX,
			rY,
			rZ);
	} else {
		CRemotePlayer* pPlayer = pNetGame->GetPlayerPool()->GetAt(playerId);

		if ( !pPlayer )
			return;

		ScriptCommand( &attach_object_to_actor, pObject->m_dwGTAId, pPlayer->GetPlayerPed()->m_dwGTAId,
																	OffsetX,
																	OffsetY,
																	OffsetZ, 
																	rX,
																	rY,
																	rZ);
	}

	} catch(...) {}
}

void ScrInitMenu(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	if(!pNetGame) return;
	CMenuPool* pMenuPool = pNetGame->GetMenuPool();

	BYTE byteMenuID;
	BOOL bColumns; // 0 = 1, 1 = 2
	CHAR cText[MAX_MENU_LINE];
	float fX;
	float fY;
	float fCol1;
	float fCol2 = 0.0;
	MENU_INT MenuInteraction;
	
	bsData.Read(byteMenuID);
	bsData.Read(bColumns);
	bsData.Read(cText, MAX_MENU_LINE);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fCol1);
	if (bColumns) bsData.Read(fCol2);
	bsData.Read(MenuInteraction.bMenu);
	for (BYTE i = 0; i < MAX_MENU_ITEMS; i++)
	{
		bsData.Read(MenuInteraction.bRow[i]);
	}

	CMenu* pMenu;
	
	if (pMenuPool->GetSlotState(byteMenuID))
	{
		pMenuPool->Delete(byteMenuID);
	}
	
	pMenu = pMenuPool->New(byteMenuID, cText, fX, fY, ((BYTE)bColumns) + 1, fCol1, fCol2, &MenuInteraction);
	
	if (!pMenu) return;
	
	BYTE byteColCount;
	bsData.Read(cText, MAX_MENU_LINE);
	pMenu->SetColumnTitle(0, cText);
	
	bsData.Read(byteColCount);
	for (BYTE i = 0; i < byteColCount; i++)
	{
		bsData.Read(cText, MAX_MENU_LINE);
		pMenu->AddMenuItem(0, i, cText);
	}
	
	if (bColumns)
	{
		bsData.Read(cText, MAX_MENU_LINE);
		pMenu->SetColumnTitle(1, cText);
		
		bsData.Read(byteColCount);
		for (BYTE i = 0; i < byteColCount; i++)
		{
			bsData.Read(cText, MAX_MENU_LINE);
			pMenu->AddMenuItem(1, i, cText);
		}
	}
}

void ScrShowMenu(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	if(!pNetGame) return;
	CMenuPool* pMenuPool = pNetGame->GetMenuPool();

	BYTE byteMenuID;
	bsData.Read(byteMenuID);
	pNetGame->GetMenuPool()->ShowMenu(byteMenuID);
}

void ScrHideMenu(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	if(!pNetGame) return;
	CMenuPool* pMenuPool = pNetGame->GetMenuPool();

	BYTE byteMenuID;
	bsData.Read(byteMenuID);
	pNetGame->GetMenuPool()->HideMenu(byteMenuID);
}

void ScrSetPlayerWantedLevel(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	if(!pGame) return;

	BYTE byteLevel;
	bsData.Read(byteLevel);
	pGame->SetWantedLevel(byteLevel);
}

void ScrShowTextDraw(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	
	CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
	if (pTextDrawPool)
	{
		WORD wTextID;
		TEXT_DRAW_TRANSMIT TextDrawTransmit;
		CHAR cText[MAX_TEXT_DRAW_LINE];
		bsData.Read(wTextID);
		bsData.Read((PCHAR)&TextDrawTransmit, sizeof (TEXT_DRAW_TRANSMIT));
		bsData.Read(cText, MAX_TEXT_DRAW_LINE);

		char szDebug[256];
		sprintf(szDebug,"New TextDraw: %u\n",wTextID);
		OutputDebugString(szDebug);

		pTextDrawPool->New(wTextID, &TextDrawTransmit, cText);
	}
}

void ScrHideTextDraw(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();

	if (pTextDrawPool) {
		WORD wTextID;
		bsData.Read(wTextID);
		
		char szDebug[256];
		sprintf(szDebug,"Deleteing TextDraw: %u\n",wTextID);
		OutputDebugString(szDebug);

		pTextDrawPool->Delete(wTextID);
	}
}

void ScrEditTextDraw(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
	if (pTextDrawPool)
	{
		WORD wTextID;
		CHAR cText[MAX_TEXT_DRAW_LINE];
		bsData.Read(wTextID);
		bsData.Read(cText, MAX_TEXT_DRAW_LINE);
		CTextDraw* pText = pTextDrawPool->GetAt(wTextID);
		if (pText) pText->SetText(cText);
	}
}

void ScrAddGangZone(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CGangZonePool* pGangZonePool = pNetGame->GetGangZonePool();
	if (pGangZonePool)
	{
		WORD wZoneID;
		float minx, miny, maxx, maxy;
		DWORD dwColor;
		bsData.Read(wZoneID);
		bsData.Read(minx);
		bsData.Read(miny);
		bsData.Read(maxx);
		bsData.Read(maxy);
		bsData.Read(dwColor);
//		pChatWindow->AddDebugMessage("called %d %f %f %f %f %x", wZoneID, minx, miny, maxx, maxy, dwColor);
		pGangZonePool->New(wZoneID, minx, miny, maxx, maxy, dwColor);
	}
}

void ScrRemoveGangZone(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CGangZonePool* pGangZonePool = pNetGame->GetGangZonePool();
	if (pGangZonePool)
	{
		WORD wZoneID;
		bsData.Read(wZoneID);
		pGangZonePool->Delete(wZoneID);
	}
}

void ScrFlashGangZone(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CGangZonePool* pGangZonePool = pNetGame->GetGangZonePool();
	if (pGangZonePool)
	{
		WORD wZoneID;
		DWORD dwColor;
		bsData.Read(wZoneID);
		bsData.Read(dwColor);
		pGangZonePool->Flash(wZoneID, dwColor);
	}
}

void ScrStopFlashGangZone(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);
	CGangZonePool* pGangZonePool = pNetGame->GetGangZonePool();
	if (pGangZonePool)
	{
		WORD wZoneID;
		bsData.Read(wZoneID);
		pGangZonePool->StopFlash(wZoneID);
	}
}

//----------------------------------------------------

void ScrApplyAnimation(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	PLAYERID playerId;
	BYTE byteAnimLibLen;
	BYTE byteAnimNameLen;
	char szAnimLib[256];
	char szAnimName[256];
	float fS;
	bool opt1,opt2,opt3,opt4;
	int  opt5;
	CPlayerPool *pPlayerPool=NULL;
	CPlayerPed *pPlayerPed=NULL;

	memset(szAnimLib,0,256);
	memset(szAnimName,0,256);

	bsData.Read(playerId);
	bsData.Read(byteAnimLibLen);
	bsData.Read(szAnimLib,byteAnimLibLen);
	bsData.Read(byteAnimNameLen);
	bsData.Read(szAnimName,byteAnimNameLen);
	bsData.Read(fS);
	bsData.Read(opt1);
	bsData.Read(opt2);
	bsData.Read(opt3);
	bsData.Read(opt4);
	bsData.Read(opt5);

	szAnimLib[byteAnimLibLen] = '\0';
	szAnimName[byteAnimNameLen] = '\0';

	pPlayerPool = pNetGame->GetPlayerPool();

	if(pPlayerPool) {
		// Get the CPlayerPed for this player
		if(playerId == pPlayerPool->GetLocalPlayerID()) {
			pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
		}
		else {
			if(pPlayerPool->GetSlotState(playerId)) {
				pPlayerPed = pPlayerPool->GetAt(playerId)->GetPlayerPed();
			}
		}
		if(pPlayerPed) {
			try {
				pPlayerPed->ApplyAnimation(szAnimName,szAnimLib,fS,
					(int)opt1,(int)opt2,(int)opt3,(int)opt4,(int)opt5);
			} catch(...) {}
		}
	}
}

//----------------------------------------------------

void ScrClearAnimations(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	PLAYERID playerId;
	bsData.Read(playerId);
	MATRIX4X4 mat;

	CPlayerPool *pPlayerPool=NULL;
	CPlayerPed *pPlayerPed=NULL;

	pPlayerPool = pNetGame->GetPlayerPool();

	if(pPlayerPool) {
		// Get the CPlayerPed for this player
		if(playerId == pPlayerPool->GetLocalPlayerID()) {
			pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
		}
		else {
			if(pPlayerPool->GetSlotState(playerId)) {
				pPlayerPed = pPlayerPool->GetAt(playerId)->GetPlayerPed();
			}
		}
		if(pPlayerPed) {
			try {
				
				pPlayerPed->GetMatrix(&mat);
				pPlayerPed->TeleportTo(mat.pos.X,mat.pos.Y,mat.pos.Z);

			} catch(...) {}
		}
	}
}

//----------------------------------------------------

void ScrSetSpecialAction(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	BYTE byteSpecialAction;
	bsData.Read(byteSpecialAction);
	
	CPlayerPool *pPool=pNetGame->GetPlayerPool();
	if(pPool) pPool->GetLocalPlayer()->ApplySpecialAction(byteSpecialAction);
}

//----------------------------------------------------

void ScrEnableStuntBonus(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	bool bStuntBonusEnabled;
	bsData.Read(bStuntBonusEnabled);
	pGame->EnableStuntBonus(bStuntBonusEnabled);
}

//----------------------------------------------------

void ScrSetFightingStyle(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	PLAYERID playerId;
	BYTE byteFightingStyle = 0;

	bsData.Read(playerId);
	bsData.Read(byteFightingStyle);
	
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CPlayerPed *pPlayerPed = 0;

	if(pPlayerPool) {
		if(playerId == pPlayerPool->GetLocalPlayerID()) {
			pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
		}
		else {
			if(pPlayerPool->GetSlotState(playerId)) {
				pPlayerPed = pPlayerPool->GetAt(playerId)->GetPlayerPed();
			}
		}
		if(pPlayerPed) {
			try {
				
				pPlayerPed->SetFightingStyle(byteFightingStyle);

			} catch(...) {}
		}
	}
}

//-----------------------------------------------------------

void ScrSetPlayerVelocity(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	VECTOR vecMoveSpeed;

	bsData.Read( vecMoveSpeed.X );
	bsData.Read( vecMoveSpeed.Y );
	bsData.Read( vecMoveSpeed.Z );

	CPlayerPed* pPlayerPed = pGame->FindPlayerPed();

	if ( pPlayerPed )
	{
		if ( pPlayerPed->IsOnGround() )
		{
			DWORD dwStateFlags = pPlayerPed->GetStateFlags();
			dwStateFlags ^= 3; // Make the game think the ped is off the ground so SetMoveSpeed works
			pPlayerPed->SetStateFlags( dwStateFlags );
		}

		pPlayerPed->SetMoveSpeedVector( vecMoveSpeed );
	}
}

//-----------------------------------------------------------

void ScrSetVehicleVelocity(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	VECTOR vecMoveSpeed;

	bsData.Read( vecMoveSpeed.X );
	bsData.Read( vecMoveSpeed.Y );
	bsData.Read( vecMoveSpeed.Z );

	CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
	CPlayerPed* pPlayerPed = pGame->FindPlayerPed();

	if ( pPlayerPed )
	{
		CVehicle* pVehicle = pVehiclePool->GetAt( pVehiclePool->FindIDFromGtaPtr( pPlayerPed->GetGtaVehicle() ) );

		if ( pVehicle )
			pVehicle->SetMoveSpeedVector( vecMoveSpeed );
	}
}

//-----------------------------------------------------------

void ScrCreateActor(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	ACTORID ActorID;
	BYTE byteNameLen;
	char szActorName[MAX_PLAYER_NAME];
	VECTOR vecPos;
	float fRotation;
	int iSkin;

	bsData.Read( ActorID );
	bsData.Read( byteNameLen );
	bsData.Read( szActorName, byteNameLen );
	bsData.Read( iSkin );
	bsData.Read( vecPos.X );
	bsData.Read( vecPos.Y );
	bsData.Read( vecPos.Z );
	bsData.Read( fRotation );

	szActorName[byteNameLen] = 0;
	pNetGame->GetActorPool()->New( ActorID, iSkin, &vecPos, fRotation, szActorName );
}

//-----------------------------------------------------------

void ScrDestroyActor(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	ACTORID ActorID;

	bsData.Read( ActorID );

	pNetGame->GetActorPool()->Delete( ActorID );
}

//-----------------------------------------------------------

void ScrMoveActorTo(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	ACTORID ActorID;
	int iMoveType;
	VECTOR vecPos;

	bsData.Read( ActorID );
	bsData.Read( iMoveType );
	bsData.Read( vecPos.X );
	bsData.Read( vecPos.Y );
	bsData.Read( vecPos.Z );

	CRemoteActor *pRemoteActor = pNetGame->GetActorPool()->GetAt( ActorID );

	if (pRemoteActor) {
		CActorPed *pActorPed = pRemoteActor->GetAtPed();
		if (pActorPed) pActorPed->MoveTo(vecPos, iMoveType);
	}
}

//-----------------------------------------------------------

void ScrActorKillPlayer(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	ACTORID ActorID;
	int iWeapon;
	PLAYERID playerId;

	bsData.Read( ActorID );
	bsData.Read( playerId );
	bsData.Read( (int)iWeapon );

	CRemoteActor *pRemoteActor = pNetGame->GetActorPool()->GetAt ( ActorID );
	if (pRemoteActor && playerId && playerId != INVALID_PLAYER_ID && iWeapon) {
		CActorPed *pActorPed = pRemoteActor->GetAtPed();
		if (pActorPed) pActorPed->KillPlayer(playerId, iWeapon);
	}

}

//----------------------------------------------------

void ScrActorEnterVehicle(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	ACTORID ActorID;
    VEHICLEID VehicleID;
	bool bPassenger;

	bsData.Read(ActorID);
	bsData.Read(VehicleID);
	bsData.Read(bPassenger);
    
	CActorPool *pActorPool = pNetGame->GetActorPool();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(pActorPool && pVehiclePool) {
		CRemoteActor *pRemoteActor = pActorPool->GetAt(ActorID);
		CVehicle *pVehicle = pVehiclePool->GetAt(VehicleID);
		if(pRemoteActor && pVehicle) {
			pRemoteActor->EnterVehicle(VehicleID, bPassenger);
		}
	}

	//pChatWindow->AddDebugMessage("ActorEnterVehicle: %u %u %u",ActorID,VehicleID,(BOOL)bPassenger);

}

//----------------------------------------------------

void ScrActorDriveVehicleToPoint(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	ACTORID ActorID;
    VEHICLEID VehicleID;
	VECTOR vecPoint;
	float fMaxSpeed;
	int iDriveType;
	float fAltitudeMin;
	float fAltitudeMax;

	bsData.Read(ActorID);
	bsData.Read(VehicleID);
	bsData.Read(vecPoint.X);
	bsData.Read(vecPoint.Y);
	bsData.Read(vecPoint.Z);
	bsData.Read(fMaxSpeed);
	bsData.Read(iDriveType);
	bsData.Read(fAltitudeMin);
	bsData.Read(fAltitudeMax);
    
	CActorPool *pActorPool = pNetGame->GetActorPool();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(pActorPool && pVehiclePool) {
		CRemoteActor *pRemoteActor = pActorPool->GetAt(ActorID);
		CVehicle *pVehicle = pVehiclePool->GetAt(VehicleID);
		if(pRemoteActor && pVehicle) {
			if ( pVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_HELI )
				pRemoteActor->GetAtPed()->FlyHelicopterToPoint( pVehicle->m_dwGTAId, &vecPoint, fMaxSpeed, fAltitudeMin, fAltitudeMax );
			else
				pRemoteActor->GetAtPed()->DriveVehicleToPoint(pVehicle->m_dwGTAId,&vecPoint,fMaxSpeed,iDriveType);
		}
	}

	//pChatWindow->AddDebugMessage("DriveTo: %u %u %f %f %f %f %u",ActorID,VehicleID,
		//vecPoint.X,vecPoint.Y,vecPoint.Z,fMaxSpeed,iDriveType);	
}

//----------------------------------------------------

void ScrActorExitVehicle(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	ACTORID ActorID;
    VEHICLEID VehicleID;

	bsData.Read(ActorID);
	bsData.Read(VehicleID);
    
	CActorPool *pActorPool = pNetGame->GetActorPool();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(pActorPool && pVehiclePool) {
		CRemoteActor *pRemoteActor = pActorPool->GetAt(ActorID);
		CVehicle *pVehicle = pVehiclePool->GetAt(VehicleID);
		if(pRemoteActor && pVehicle) {
			pRemoteActor->ExitVehicle();
		}
	}

	//pChatWindow->AddDebugMessage("ActorExitVehicle: %u %u",ActorID,VehicleID);
}

//----------------------------------------------------

void ScrToggleWidescreen(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	BYTE bToggle;
	bsData.Read(bToggle);

	pChatWindow->AddDebugMessage("Widescreen = %i", bToggle);

	ScriptCommand(&toggle_widescreen, bToggle);
}


//----------------------------------------------------

void ScrSetActorPos(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	ACTORID ActorID;
	VECTOR vecPos;

	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	bsData.Read(ActorID);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	CRemoteActor *pRemoteActor = pNetGame->GetActorPool()->GetAt( ActorID );

	if (pRemoteActor && pRemoteActor->GetAtPed()) {
		pRemoteActor->GetAtPed()->MoveTo(vecPos, MOVETO_DIRECT);
	}
}

//----------------------------------------------------

void ScrSetVehicleTireStatus(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	VEHICLEID vehicleID;
	BYTE byteTireStatus;
	BYTE byteTireID;
	bsData.Read(vehicleID);
	bsData.Read(byteTireID);
	bsData.Read(byteTireStatus);

	CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
	CVehicle* pVehicle = pVehiclePool->GetAt( vehicleID );

	if ( pVehiclePool && pVehicle )
	{
		pVehicle->SetWheelPopped( byteTireID, byteTireStatus );
	}
}

//----------------------------------------------------

void ScrSetPlayerDrunkVisuals(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	int iVisuals;
	bsData.Read(iVisuals);

	CPlayerPed* pPlayer = pGame->FindPlayerPed();
	if (pPlayer) {
		ScriptCommand(&set_player_drunk_visuals, 0, iVisuals);
	}
}

void ScrSetPlayerDrunkHandling(RPCParameters* rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData(rpcParams->input,(iBitLength/8)+1,false);

	int iVisuals;
	bsData.Read(iVisuals);

	CPlayerPed* pPlayer = pGame->FindPlayerPed();
	if (pPlayer) {
		ScriptCommand(&handling_responsiveness, 0, iVisuals);
	}
}


void RegisterScriptRPCs(RakClientInterface* pRakClient)
{
	/*pRakClient->RegisterAsRemoteProcedureCall()
	REGISTER_CLASS_MEMBER_RPC(pRakClient, ScrSetSpawnInfo);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerTeam);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerSkin);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerName);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerPos);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerPosFindZ);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerHealth);
	REGISTER_STATIC_RPC(pRakClient, ScrPutPlayerInVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrRemovePlayerFromVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerColor);
	REGISTER_STATIC_RPC(pRakClient, ScrDisplayGameText);
	REGISTER_STATIC_RPC(pRakClient, ScrSetInterior);
	REGISTER_STATIC_RPC(pRakClient, ScrSetCameraPos);
	REGISTER_STATIC_RPC(pRakClient, ScrSetCameraLookAt);
	REGISTER_STATIC_RPC(pRakClient, ScrSetVehiclePos);
	REGISTER_STATIC_RPC(pRakClient, ScrSetVehicleZAngle);
	REGISTER_STATIC_RPC(pRakClient, ScrVehicleParams);
	REGISTER_STATIC_RPC(pRakClient, ScrSetCameraBehindPlayer);
	REGISTER_STATIC_RPC(pRakClient, ScrTogglePlayerControllable);
	REGISTER_STATIC_RPC(pRakClient, ScrPlaySound);
	REGISTER_STATIC_RPC(pRakClient, ScrSetWorldBounds);
	REGISTER_STATIC_RPC(pRakClient, ScrHaveSomeMoney);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerFacingAngle);
	REGISTER_STATIC_RPC(pRakClient, ScrResetMoney);
	REGISTER_STATIC_RPC(pRakClient, ScrResetPlayerWeapons);
	REGISTER_STATIC_RPC(pRakClient, ScrGivePlayerWeapon);
	REGISTER_STATIC_RPC(pRakClient, ScrRespawnVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrLinkVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerArmour);
	REGISTER_STATIC_RPC(pRakClient, ScrDeathMessage);
	//REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerMarker);
	REGISTER_STATIC_RPC(pRakClient, ScrSetMapIcon);
	REGISTER_STATIC_RPC(pRakClient, ScrDisableMapIcon);
	REGISTER_STATIC_RPC(pRakClient, ScrSetWeaponAmmo);
	REGISTER_STATIC_RPC(pRakClient, ScrSetGravity);
	REGISTER_STATIC_RPC(pRakClient, ScrSetVehicleHealth);
	REGISTER_STATIC_RPC(pRakClient, ScrAttachTrailerToVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrDetachTrailerFromVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrCreateObject);
	REGISTER_STATIC_RPC(pRakClient, ScrSetObjectPos);
	REGISTER_STATIC_RPC(pRakClient, ScrSetObjectRotation);
	REGISTER_STATIC_RPC(pRakClient, ScrDestroyObject);
	REGISTER_STATIC_RPC(pRakClient, ScrCreateExplosion);
	REGISTER_STATIC_RPC(pRakClient, ScrShowNameTag);
	REGISTER_STATIC_RPC(pRakClient, ScrMoveObject);
	REGISTER_STATIC_RPC(pRakClient, ScrStopObject);
	REGISTER_STATIC_RPC(pRakClient, ScrNumberPlate);
	REGISTER_STATIC_RPC(pRakClient, ScrTogglePlayerSpectating);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerSpectating);
	REGISTER_STATIC_RPC(pRakClient, ScrPlayerSpectatePlayer);
	REGISTER_STATIC_RPC(pRakClient, ScrPlayerSpectateVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrRemoveComponent);
	REGISTER_STATIC_RPC(pRakClient, ScrForceSpawnSelection);
	REGISTER_STATIC_RPC(pRakClient, ScrAttachObjectToPlayer);
	REGISTER_STATIC_RPC(pRakClient, ScrInitMenu);
	REGISTER_STATIC_RPC(pRakClient, ScrShowMenu);
	REGISTER_STATIC_RPC(pRakClient, ScrHideMenu);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerWantedLevel);
	REGISTER_STATIC_RPC(pRakClient, ScrShowTextDraw);
	REGISTER_STATIC_RPC(pRakClient, ScrHideTextDraw);
	REGISTER_STATIC_RPC(pRakClient, ScrEditTextDraw);
	REGISTER_STATIC_RPC(pRakClient, ScrAddGangZone);
	REGISTER_STATIC_RPC(pRakClient, ScrRemoveGangZone);
	REGISTER_STATIC_RPC(pRakClient, ScrFlashGangZone);
	REGISTER_STATIC_RPC(pRakClient, ScrStopFlashGangZone);
	REGISTER_STATIC_RPC(pRakClient, ScrApplyAnimation);
	REGISTER_STATIC_RPC(pRakClient, ScrClearAnimations);
	REGISTER_STATIC_RPC(pRakClient, ScrSetSpecialAction);
	REGISTER_STATIC_RPC(pRakClient, ScrEnableStuntBonus);
	REGISTER_STATIC_RPC(pRakClient, ScrSetFightingStyle);
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerVelocity);
	REGISTER_STATIC_RPC(pRakClient, ScrSetVehicleVelocity);
	REGISTER_STATIC_RPC(pRakClient, ScrCreateActor);
	REGISTER_STATIC_RPC(pRakClient, ScrDestroyActor);
	REGISTER_STATIC_RPC(pRakClient, ScrMoveActorTo);
	REGISTER_STATIC_RPC(pRakClient, ScrActorKillPlayer);
	REGISTER_STATIC_RPC(pRakClient, ScrActorEnterVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrActorExitVehicle);
	REGISTER_STATIC_RPC(pRakClient, ScrActorDriveVehicleToPoint);
	REGISTER_STATIC_RPC(pRakClient, ScrSetActorPos);
	REGISTER_STATIC_RPC(pRakClient, ScrToggleWidescreen);
	REGISTER_STATIC_RPC(pRakClient, ScrSetVehicleTireStatus );
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerDrunkVisuals );
	REGISTER_STATIC_RPC(pRakClient, ScrSetPlayerDrunkHandling );*/
}

//----------------------------------------------------

void UnRegisterScriptRPCs(RakClientInterface* pRakClient)
{
	/*UNREGISTER_STATIC_RPC(pRakClient, ScrSetSpawnInfo); // 32
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerTeam);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerName);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerSkin);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerPos);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerPosFindZ);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerHealth);
	UNREGISTER_STATIC_RPC(pRakClient, ScrPutPlayerInVehicle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrRemovePlayerFromVehicle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerColor);
	UNREGISTER_STATIC_RPC(pRakClient, ScrDisplayGameText);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetInterior);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetCameraPos);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetCameraLookAt);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetVehiclePos);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetVehicleZAngle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrVehicleParams);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetCameraBehindPlayer);
	UNREGISTER_STATIC_RPC(pRakClient, ScrTogglePlayerControllable); // 50
	UNREGISTER_STATIC_RPC(pRakClient, ScrPlaySound);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetWorldBounds);
	UNREGISTER_STATIC_RPC(pRakClient, ScrHaveSomeMoney);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerFacingAngle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrResetMoney);
	UNREGISTER_STATIC_RPC(pRakClient, ScrResetPlayerWeapons);
	UNREGISTER_STATIC_RPC(pRakClient, ScrGivePlayerWeapon);
	UNREGISTER_STATIC_RPC(pRakClient, ScrRespawnVehicle); // 58
	UNREGISTER_STATIC_RPC(pRakClient, ScrLinkVehicle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrDeathMessage);
	//UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerMarker);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetMapIcon);
	UNREGISTER_STATIC_RPC(pRakClient, ScrDisableMapIcon);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetWeaponAmmo);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetGravity);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetVehicleHealth);
	UNREGISTER_STATIC_RPC(pRakClient, ScrAttachTrailerToVehicle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrDetachTrailerFromVehicle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrCreateObject);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetObjectPos);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetObjectRotation);
	UNREGISTER_STATIC_RPC(pRakClient, ScrDestroyObject); // Possible problem source
	UNREGISTER_STATIC_RPC(pRakClient, ScrCreateExplosion);
	UNREGISTER_STATIC_RPC(pRakClient, ScrShowNameTag);
	UNREGISTER_STATIC_RPC(pRakClient, ScrMoveObject);
	UNREGISTER_STATIC_RPC(pRakClient, ScrStopObject);
	UNREGISTER_STATIC_RPC(pRakClient, ScrNumberPlate);
	UNREGISTER_STATIC_RPC(pRakClient, ScrTogglePlayerSpectating);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerSpectating);
	UNREGISTER_STATIC_RPC(pRakClient, ScrPlayerSpectatePlayer);
	UNREGISTER_STATIC_RPC(pRakClient, ScrPlayerSpectateVehicle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrRemoveComponent);
	UNREGISTER_STATIC_RPC(pRakClient, ScrForceSpawnSelection);
	UNREGISTER_STATIC_RPC(pRakClient, ScrAttachObjectToPlayer);
	UNREGISTER_STATIC_RPC(pRakClient, ScrInitMenu);
	UNREGISTER_STATIC_RPC(pRakClient, ScrShowMenu);
	UNREGISTER_STATIC_RPC(pRakClient, ScrHideMenu);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerWantedLevel);
	UNREGISTER_STATIC_RPC(pRakClient, ScrShowTextDraw);
	UNREGISTER_STATIC_RPC(pRakClient, ScrHideTextDraw);
	UNREGISTER_STATIC_RPC(pRakClient, ScrEditTextDraw);
	UNREGISTER_STATIC_RPC(pRakClient, ScrAddGangZone);
	UNREGISTER_STATIC_RPC(pRakClient, ScrRemoveGangZone);
	UNREGISTER_STATIC_RPC(pRakClient, ScrFlashGangZone);
	UNREGISTER_STATIC_RPC(pRakClient, ScrStopFlashGangZone);
	UNREGISTER_STATIC_RPC(pRakClient, ScrApplyAnimation);
	UNREGISTER_STATIC_RPC(pRakClient, ScrClearAnimations);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetSpecialAction);
	UNREGISTER_STATIC_RPC(pRakClient, ScrEnableStuntBonus);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetFightingStyle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerVelocity);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetVehicleVelocity);
	UNREGISTER_STATIC_RPC(pRakClient, ScrCreateActor);
	UNREGISTER_STATIC_RPC(pRakClient, ScrDestroyActor);
	UNREGISTER_STATIC_RPC(pRakClient, ScrMoveActorTo);
	UNREGISTER_STATIC_RPC(pRakClient, ScrActorKillPlayer);
	UNREGISTER_STATIC_RPC(pRakClient, ScrActorEnterVehicle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrActorExitVehicle);
	UNREGISTER_STATIC_RPC(pRakClient, ScrActorDriveVehicleToPoint);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetActorPos);
	UNREGISTER_STATIC_RPC(pRakClient, ScrToggleWidescreen);
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetVehicleTireStatus );
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerDrunkVisuals );
	UNREGISTER_STATIC_RPC(pRakClient, ScrSetPlayerDrunkHandling );*/
}

//----------------------------------------------------
