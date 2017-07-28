/*
Leaked by ZYRONIX.net.
*/

#include "main.h"

//----------------------------------------------------

BOOL	bRconSocketReply	= FALSE;

SOCKET	cur_sock			= INVALID_SOCKET;
char*	cur_data			= NULL;
int		cur_datalen			= 0;
sockaddr_in to;

extern CNetGame* pNetGame;
extern RakServerInterface* pRak;

//----------------------------------------------------

void RconSocketReply(char* szMessage);

//----------------------------------------------------
// int CheckQueryFlood()
// returns 1 if this query could flood
// returns 0 otherwise

DWORD dwLastQueryTick=0;
unsigned int lastBinAddr=0;

int CheckQueryFlood(unsigned int binaryAddress)
{
	if(!dwLastQueryTick) {
		dwLastQueryTick = (DWORD)GetTickCount();
		lastBinAddr = binaryAddress;
		return 0;
	}
	if(lastBinAddr == binaryAddress) {
		return 0;
	}
	if((GetTickCount() - dwLastQueryTick) < 25) {
		return 1;
	}
	dwLastQueryTick = GetTickCount();
	lastBinAddr = binaryAddress;
	return 0;
}

//----------------------------------------------------

// Fixes launcher not getting server data. (https://github.com/J0shES/samp/issues/21)

int queryLen = 0;
char queryBufferSend[4092];
void handleQueries(SOCKET sListen, int iAddrSize, struct sockaddr_in client, char *buffer)
{
	struct sockaddr_in *addrin = (struct sockaddr_in *)(struct sockaddr *)&client;
	char *ip_address = inet_ntoa(addrin->sin_addr);


	int queryLen = 0;
	if (buffer[10] == 0x70) // Ping query
	{
		memcpy(queryBufferSend, buffer, 10); queryLen += 10;
		*(unsigned char *)&queryBufferSend[10] = 0x70; queryLen += 1;
		*(unsigned int *)&queryBufferSend[11] = *(unsigned int *)&buffer[11]; queryLen += 4;
		sendto(sListen, queryBufferSend, queryLen, 0, (struct sockaddr *)&client, iAddrSize);
	}

	else if (buffer[10] == 0x69) // Server name, player count, game mode name, map name query
	{
		WORD wPlayerCount = 0;
		CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
		if (pPlayerPool)
			for (int i = 0; i<MAX_PLAYERS; i++)
				if (pPlayerPool->GetSlotState(i))
					wPlayerCount++;

		char* mapName = pConsole->GetStringVariable("mapname");
		char* serverName = pConsole->GetStringVariable("hostname");
		char* gmName = pConsole->GetStringVariable("gamemodetext");
		int MaxPlayers = pConsole->GetIntVariable("maxplayers");

		memcpy(queryBufferSend, buffer, 10); queryLen += 10;
		*(unsigned short *)&queryBufferSend[10] = 0x69; queryLen += 2;
		*(unsigned short *)&queryBufferSend[12] = wPlayerCount; queryLen += 2;
		*(unsigned short *)&queryBufferSend[14] = MaxPlayers; queryLen += 2;
		int serverNameLen = (int)strlen(serverName); *(int *)&queryBufferSend[16] = serverNameLen; queryLen += 4;
		strncpy(&queryBufferSend[20], serverName, serverNameLen); queryLen += serverNameLen;
		int gmNameLen = (int)strlen(gmName); *(int *)&queryBufferSend[20 + serverNameLen] = gmNameLen; queryLen += 4;
		strncpy(&queryBufferSend[20 + serverNameLen + 4], gmName, gmNameLen); queryLen += gmNameLen;
		int mapNameLen = (int)strlen(mapName); *(int *)&queryBufferSend[24 + serverNameLen + gmNameLen] = mapNameLen; queryLen += 4;
		strncpy(&queryBufferSend[24 + serverNameLen + gmNameLen + 4], mapName, mapNameLen); queryLen += mapNameLen;

		sendto(sListen, queryBufferSend, queryLen, 0, (struct sockaddr *)&client, iAddrSize);
		return;
	}

	else if (buffer[10] == 0x63) // Player list query
	{
		memcpy(queryBufferSend, buffer, 10); queryLen += 10;
		*(unsigned char *)&queryBufferSend[10] = 0x63; queryLen += 1;
		WORD wPlayerCount = 0;
		CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
		if (pPlayerPool)
			for (int i = 0; i<MAX_PLAYERS; i++)
				if (pPlayerPool->GetSlotState(i))
					wPlayerCount++;

		*(unsigned short *)&queryBufferSend[11] = wPlayerCount; queryLen += 2;
		char *curbufpos = &queryBufferSend[13];
		int bufcount = 0;
		for (unsigned short i = 0; i < wPlayerCount; i++)
		{
			if (pNetGame->GetPlayerPool()->GetSlotState(wPlayerCount)) {
				unsigned char pnamelen = (unsigned char)strlen(pNetGame->GetPlayerPool()->GetPlayerName(wPlayerCount));
				curbufpos[0] = pnamelen;
				strncpy(&curbufpos[1], pNetGame->GetPlayerPool()->GetPlayerName(wPlayerCount), pnamelen);
				*(int *)&curbufpos[1 + pnamelen] = pNetGame->GetPlayerPool()->GetPlayerScore(wPlayerCount);
				curbufpos += (1 + pnamelen + 4);
				bufcount += (1 + pnamelen + 4);
			}
		}
		queryLen += bufcount;

		sendto(sListen, queryBufferSend, queryLen, 0, (struct sockaddr *)&client, iAddrSize);

		return;
	}

	else if (buffer[10] == 0x72) // Rules query
	{
		pConsole->SendRules(sListen, buffer, addrin, iAddrSize);
		/*
		memcpy(queryBufferSend, buffer, 10); queryLen += 10;
		*(unsigned char *)&queryBufferSend[10] = 0x72; queryLen += 1;
		*(unsigned short *)&queryBufferSend[11] = pConsole->SendRules; queryLen += 2;
		char *curbufpos = &queryBufferSend[13];
		for (unsigned short i = 0; i < pConsole->SendRules; i++)
		{
			unsigned char rulelen = (unsigned char)strlen(rules[i].szRule);
			curbufpos[0] = rulelen;
			strncpy(&curbufpos[1], rules[i].szRule, rulelen);
			unsigned char valuelen = (unsigned char)strlen(rules[i].szValue);
			curbufpos[1 + rulelen] = valuelen;
			strncpy(&curbufpos[1 + rulelen + 1], rules[i].szValue, valuelen);
			curbufpos += (1 + rulelen + 1 + valuelen);
		}
		curbufpos = &queryBufferSend[13];
		queryLen += strlen(curbufpos);

		sendto(sListen, queryBufferSend, queryLen, 0, (struct sockaddr *)&client, iAddrSize);*/

		return;
	}
}
/*
int ProcessQueryPacket(unsigned int binaryAddress, unsigned short port, char* data, int length, SOCKET s)
{
	char* szPassword=NULL;
	WORD wStrLen=0;
	in_addr in;
	in.s_addr = binaryAddress;

	logprintf("ProcessQueryPacket");
	if ((length > 4) && (*(unsigned long*)(data) == 'PMAS')) // 0x504D4153 = "SAMP" as hex.
	{
		if(!pNetGame || (pNetGame->GetGameState() != GAMESTATE_RUNNING)) return 0;

		if (length >= 11)
		{
			to.sin_family = AF_INET;
			to.sin_port = htons(port);
			to.sin_addr.s_addr = binaryAddress;

			switch (data[10])
			{
				case 'p':	// ping
				{
					if (length == 15)
					{
						sendto(s, data, 15, 0, (sockaddr*)&to, sizeof(to));
					}
					break;
				}
				case 'i':	// info
				{
					// We do not process these queries 'query' is 0
					if(!pConsole->GetBoolVariable("query")) return 1;
					if(CheckQueryFlood(binaryAddress)) return 1;

					char* szHostname = pConsole->GetStringVariable("hostname");
					DWORD dwHostnameLen = strlen(szHostname);
					if (dwHostnameLen > 50) dwHostnameLen = 50;

					char* szGameMode = pConsole->GetStringVariable("gamemodetext");
					DWORD dwGameModeLen = strlen(szGameMode);
					if (dwGameModeLen > 30) dwGameModeLen = 30;

					char* szMapName = pConsole->GetStringVariable("mapname");
					DWORD dwMapNameLen = strlen(szMapName);
					if (dwMapNameLen > 30) dwMapNameLen = 30;

					WORD wPlayerCount = 0;
					CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
					if (pPlayerPool)
						for (int i=0; i<MAX_PLAYERS; i++)
							if (pPlayerPool->GetSlotState(i))
								wPlayerCount++;

					WORD wMaxPlayers = pConsole->GetIntVariable("maxplayers");

					BYTE byteIsPassworded = pConsole->GetStringVariable("password")[0] != 0;

					DWORD datalen = 28;	// Previous data = 11b
										// IsPassworded = 1b
										// Player count = 2b
										// Max player count = 2b
										// String-length bytes = 12b (3 * sizeof(DWORD))
					datalen += dwHostnameLen;
					datalen += dwGameModeLen;
					datalen += dwMapNameLen;


					char* newdata = (char*)malloc(datalen);
					char* keep_ptr = newdata;
					// Previous Data
					memcpy(newdata, data, 11);
					newdata += 11;

					// IsPassworded
					memcpy(newdata, &byteIsPassworded, sizeof(BYTE));
					newdata += sizeof(BYTE);

					// Player Count
					memcpy(newdata, &wPlayerCount, sizeof(WORD));
					newdata += sizeof(WORD);

					// Max Players
					memcpy(newdata, &wMaxPlayers, sizeof(WORD));
					newdata += sizeof(WORD);

					// Hostname
					memcpy(newdata, &dwHostnameLen, sizeof(DWORD));
					newdata += sizeof(DWORD);
					memcpy(newdata, szHostname, dwHostnameLen);
					newdata += dwHostnameLen;

					// Game Mode
					memcpy(newdata, &dwGameModeLen, sizeof(DWORD));
					newdata += sizeof(DWORD);
					memcpy(newdata, szGameMode, dwGameModeLen);
					newdata += dwGameModeLen;

					// Map Name
					memcpy(newdata, &dwMapNameLen, sizeof(DWORD));
					newdata += sizeof(DWORD);
					memcpy(newdata, szMapName, dwMapNameLen);
					newdata += dwMapNameLen;

					sendto(s, keep_ptr, datalen, 0, (sockaddr*)&to, sizeof(to));
					free(keep_ptr);
					break;
				}
				case 'c':	// players
				{
					// We do not process these queries 'query' is 0
					if(!pConsole->GetBoolVariable("query")) return 1;
					if(CheckQueryFlood(binaryAddress)) return 1;

					WORD wPlayerCount = 0;
					CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
					if (pPlayerPool) {
						for (int i=0; i<MAX_PLAYERS; i++)
							if (pPlayerPool->GetSlotState(i))
								wPlayerCount++;
					}

					char* newdata = (char*)malloc(13 + (wPlayerCount * (MAX_PLAYER_NAME + 5))); // 5 = 1b String len, and 4b Score
					char* keep_ptr = newdata;
					// Previous Data
					memcpy(newdata, data, 11);
					newdata += 11;

					// Player Count
					memcpy(newdata, &wPlayerCount, sizeof(WORD));
					newdata += sizeof(WORD);

					if (pPlayerPool) {
						char* szName;
						BYTE byteNameLen;
						DWORD dwScore;
						
						for (int r=0; r<MAX_PLAYERS; r++)
						{
							if (pPlayerPool->GetSlotState(r))
							{
								szName = pPlayerPool->GetPlayerName(r);
								byteNameLen = (BYTE)strlen(szName);
								memcpy(newdata, &byteNameLen, sizeof(BYTE));
								newdata += sizeof(BYTE);
								memcpy(newdata, szName, byteNameLen);
								newdata += byteNameLen;
								dwScore = pPlayerPool->GetPlayerScore(r);
								memcpy(newdata, &dwScore, sizeof(DWORD));
								newdata += sizeof(DWORD);							

							}
						}
					}

					sendto(s, keep_ptr, (int)(newdata - keep_ptr), 0, (sockaddr*)&to, sizeof(to));
					free(keep_ptr);
					break;
				}
				case 'd':	// detailed player list id.namelength.name.score.ping
				{
						// We do not process these queries 'query' is 0
						if(!pConsole->GetBoolVariable("query")) return 1;
						if(CheckQueryFlood(binaryAddress)) return 1;

						WORD wPlayerCount = 0;
						CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
						if (pPlayerPool) {
							for (int i=0; i<MAX_PLAYERS; i++)
								if (pPlayerPool->GetSlotState(i))
									wPlayerCount++;
						}
						char* newdata = (char*)malloc(13 + (wPlayerCount * (MAX_PLAYER_NAME + 10))); // 9 = 1b String len, 4b Score, 4b Ping, 1b Playerid
						char* keep_ptr = newdata;
						// Previous Data
						memcpy(newdata, data, 11);
						newdata += 11;

						// Player Count
						memcpy(newdata, &wPlayerCount, sizeof(WORD));
						newdata += sizeof(WORD);

						if (pPlayerPool) {
							char* szName;
							BYTE byteNameLen;
							DWORD dwScore, dwPing;

							for (int r=0; r<MAX_PLAYERS; r++)
							{
								if (pPlayerPool->GetSlotState(r))
								{
									memcpy(newdata, &r, sizeof(BYTE));
									newdata += sizeof(BYTE);
									szName = pPlayerPool->GetPlayerName(r);
									byteNameLen = (BYTE)strlen(szName);
									memcpy(newdata, &byteNameLen, sizeof(BYTE));
									newdata += sizeof(BYTE);
									memcpy(newdata, szName, byteNameLen);
									newdata += byteNameLen;
									dwScore = pPlayerPool->GetPlayerScore(r);
									memcpy(newdata, &dwScore, sizeof(DWORD));
									newdata += sizeof(DWORD);
									dwPing = pRak->GetLastPing( pRak->GetPlayerIDFromIndex( r ) );
									memcpy(newdata, &dwPing, sizeof(DWORD));
									newdata += sizeof(DWORD);
								}
							}
						}
						sendto(s, keep_ptr, (int)(newdata - keep_ptr), 0, (sockaddr*)&to, sizeof(to));
						free(keep_ptr);
						break;
				}

				case 'r':	// rules
				{
					// We do not process these queries 'query' is 0
					if(!pConsole->GetBoolVariable("query")) return 1;
					if(CheckQueryFlood(binaryAddress)) return 1;

					pConsole->SendRules(s, data, (sockaddr_in*)&to, sizeof(to));
					break;
				}
				case 'x':	// rcon
				{
					if(pRak && pRak->IsBanned(inet_ntoa(in))) return 1;

					// We do not process these queries 'query' is 0
					if(!pConsole->GetBoolVariable("query")) return 1;
					if(CheckQueryFlood(binaryAddress)) return 1;

					cur_sock = s;
					cur_data = data;
					cur_datalen = 11; // How much of the message we send back.
					int tmp_datalen = cur_datalen;

					data += cur_datalen;

					// Check there's enough data for another WORD
					tmp_datalen += sizeof(WORD);
					if (length < tmp_datalen)
						goto cleanup;	// Malformed packet! Haxy bastard!
					// Read the string length for the password
					wStrLen = *(WORD*)data;
					data += sizeof(WORD);
					// Check there's enough data for the password string
					tmp_datalen += wStrLen;
					if (length < tmp_datalen)
						goto cleanup;	// Malformed packet! Haxy bastard!
					// Read the password string
					szPassword = (char*)malloc(wStrLen + 1);
					memcpy(szPassword, data, wStrLen);
					szPassword[wStrLen] = 0;
					data += wStrLen;

					if (strcmp(szPassword, pConsole->GetStringVariable("rcon_password")) == 0)
					{
						// Check there's enough data for another WORD
						tmp_datalen += sizeof(WORD);
						if (length < tmp_datalen)
						{
							free(szPassword);
							goto cleanup;	// Malformed packet! Haxy bastard!
						}
						// Read the string length for the command
						wStrLen = *(WORD*)data;
						data += sizeof(WORD);
						tmp_datalen += wStrLen;
						if (length < tmp_datalen)
						{
							free(szPassword);
							goto cleanup;	// Malformed packet! Haxy bastard!
						}

						// Read the command string
						char* szCommand = (char*)malloc(wStrLen + 1);
						memcpy(szCommand, data, wStrLen);
						szCommand[wStrLen] = 0;

						if (pConsole)
						{
							bRconSocketReply = TRUE;
							// Execute the command
							pConsole->Execute(szCommand);
							bRconSocketReply = FALSE;
						}

						free(szCommand);
					} else {
						in_addr in;
						in.s_addr = binaryAddress;
						logprintf("BAD RCON ATTEMPT BY: %s", inet_ntoa(in));

						bRconSocketReply = TRUE;
						RconSocketReply("Invalid RCON password.");
						bRconSocketReply = FALSE;
					}
					free(szPassword);

				cleanup:
					cur_datalen = 0;
					cur_data = NULL;
					cur_sock = INVALID_SOCKET;
					break;
				}
			}
		}
		return 1;
	} else {
		return 0;
	}
}*/

//----------------------------------------------------

void RconSocketReply(char* szMessage)
{
	// IMPORTANT!
	// Don't use logprintf from here... You'll cause an infinite loop.
	if (bRconSocketReply)
	{
		char* newdata = (char*)malloc(cur_datalen + strlen(szMessage) + sizeof(WORD));
		char* keep_ptr = newdata;
		memcpy(newdata, cur_data, cur_datalen);
		newdata += cur_datalen;
		*(WORD*)newdata = (WORD)strlen(szMessage);
		newdata += sizeof(WORD);
		memcpy(newdata, szMessage, strlen(szMessage));
		newdata += strlen(szMessage);
		sendto(cur_sock, keep_ptr, (int)(newdata - keep_ptr), 0, (sockaddr*)&to, sizeof(to));
		free(keep_ptr);
	}
}

//----------------------------------------------------
// EOF
