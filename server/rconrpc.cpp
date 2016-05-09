/*
Leaked by ZYRONIX.net.
*/

#include "main.h"
#ifdef RAKRCON

#include "rconrpc.h"

extern CNetGame *pNetGame;
extern CRcon *pRcon;

void RconConnect(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	logprintf("RconConnect");
}

void RconCommand(RPCParameters *rpcParams)
{
	
	int iBitLength = rpcParams->numberOfBitsOfData;
	PlayerID sender = rpcParams->sender;

	BYTE byteLength;
	char szCommand[256];
	RakNet::BitStream bsData(rpcParams->input, (iBitLength/8)+1, false);
	bsData.Read(byteLength);
	bsData.Read(szCommand, byteLength);
	szCommand[byteLength] = '\0';
	logprintf("[RCON] Command: %s", szCommand);
	pNetGame->SendClientMessageToAll(0x2587CEAA, "* Admin: %s", szCommand);
}

#endif