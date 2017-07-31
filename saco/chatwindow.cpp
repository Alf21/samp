//----------------------------------------------------------
//
// SA:MP Multiplayer Modification For GTA:SA
// Copyright 2004-2005 SA:MP team
//
// Version: $Id: chatwindow.cpp,v 1.20 2006/05/08 14:31:50 kyeman Exp $
//
//----------------------------------------------------------

#include "main.h"
#include <string>

extern CGame *pGame;

//----------------------------------------------------

CChatWindow::CChatWindow(IDirect3DDevice9 *pD3DDevice, ID3DXFont *pFont)
{
	int x=0;

	m_pD3DDevice		= pD3DDevice;
	m_pD3DFont			= pFont;
	m_iEnabled			= CHAT_WINDOW_MODE_FULL;
	m_iCurrentPage		= 1;
	m_bTimestamp		= false;

	// Create a sprite to use when drawing text
	D3DXCreateSprite(pD3DDevice,&m_pChatTextSprite);

	// Init the chat window lines to 0
	while(x!=MAX_MESSAGES) {
		memset(&m_ChatWindowEntries[x],0,sizeof(CHAT_WINDOW_ENTRY));
		x++;
	}

	RECT rectSize;
	m_pD3DFont->DrawText(0,"Y",-1,&rectSize,DT_CALCRECT|DT_SINGLELINE|DT_LEFT,0xFF000000);
	m_lFontSizeY = rectSize.bottom - rectSize.top;

	m_dwChatTextColor = D3DCOLOR_ARGB(255,255,255,255);
	m_dwChatInfoColor = D3DCOLOR_ARGB(255,0,200,200);
	m_dwChatDebugColor = D3DCOLOR_ARGB(255,244,164,25);
	m_dwChatBackgroundColor = D3DCOLOR_ARGB(255,0,0,0);
}

//----------------------------------------------------

CChatWindow::~CChatWindow() {}

//----------------------------------------------------

void CChatWindow::ResetDialogControls(CDXUTDialog *pGameUI)
{
	m_pGameUI = pGameUI;

	if(pGameUI) {
		//pGameUI->AddEditBox(IDC_CHATBACK,"",5,5,420,170,true,&m_pEditBackground);

		m_pScrollBar = new CDXUTScrollBar(pGameUI);        
		pGameUI->AddControl(m_pScrollBar);
		m_pScrollBar->SetVisible(true);
		m_pScrollBar->SetEnabled(true);
		m_pScrollBar->SetLocation(5,40);
        m_pScrollBar->SetSize(15,((m_lFontSizeY+1)*DISP_MESSAGES)-60);
		m_pScrollBar->SetTrackRange(0,(MAX_MESSAGES-DISP_MESSAGES)-1);
		m_pScrollBar->SetPageSize(5);
		m_pScrollBar->ShowItem(MAX_MESSAGES-1);

		/*
		m_pEditBackground->GetElement(0)->TextureColor.Init(D3DCOLOR_ARGB( 170, 20, 20, 20 ));
		m_pEditBackground->GetElement(1)->TextureColor.Init(D3DCOLOR_ARGB( 200, 0, 0, 0 ));
		m_pEditBackground->GetElement(2)->TextureColor.Init(D3DCOLOR_ARGB( 200, 0, 0, 0 ));
		m_pEditBackground->GetElement(3)->TextureColor.Init(D3DCOLOR_ARGB( 200, 0, 0, 0 ));
		m_pEditBackground->GetElement(4)->TextureColor.Init(D3DCOLOR_ARGB( 200, 0, 0, 0 ));
		m_pEditBackground->GetElement(5)->TextureColor.Init(D3DCOLOR_ARGB( 200, 0, 0, 0 ));
		m_pEditBackground->GetElement(6)->TextureColor.Init(D3DCOLOR_ARGB( 200, 0, 0, 0 ));
		m_pEditBackground->GetElement(7)->TextureColor.Init(D3DCOLOR_ARGB( 200, 0, 0, 0 ));
		m_pEditBackground->GetElement(8)->TextureColor.Init(D3DCOLOR_ARGB( 200, 0, 0, 0 ));
		m_pEditBackground->SetEnabled(false);
		m_pEditBackground->SetVisible(true);*/

	}
}

//----------------------------------------------------

void CChatWindow::ScrollBarPosFromCurrentPage()
{
	if(m_pScrollBar) {
		if(m_iCurrentPage==1) m_pScrollBar->ShowItem(39);
		if(m_iCurrentPage==2) m_pScrollBar->ShowItem(29);
		if(m_iCurrentPage==3) m_pScrollBar->ShowItem(19);
		if(m_iCurrentPage==4) m_pScrollBar->ShowItem(9);
		if(m_iCurrentPage==5) m_pScrollBar->ShowItem(0);
	}
}

//----------------------------------------------------

void CChatWindow::PageUp()
{
	if(!m_iEnabled) return;

    m_iCurrentPage++;
	if(m_iCurrentPage > CHAT_WINDOW_PAGES) {
		m_iCurrentPage = CHAT_WINDOW_PAGES;
	} else {
		ScrollBarPosFromCurrentPage();
	}
	
}

//----------------------------------------------------

void CChatWindow::PageDown()
{
	if(!m_iEnabled) return;

    m_iCurrentPage--;
	if(m_iCurrentPage < 1) {
		m_iCurrentPage = 1;
	} else {
		ScrollBarPosFromCurrentPage();
	}
}

//----------------------------------------------------

void CChatWindow::Draw()
{
	DWORD dwColorChat=0;
	RECT rect;
	RECT rectSize;
	int x=0;
	int i=0;
	int iMessageAt;
		
	rect.top		= 10;
	rect.left		= 30;
	rect.bottom		= 110;
	rect.right		= 550;

	if(!m_iEnabled || m_iCurrentPage == 1) {
		m_pScrollBar->SetVisible(false);
	} else {
		m_pScrollBar->SetVisible(true);
	}

	iMessageAt = (m_iCurrentPage * DISP_MESSAGES) - 1;

	if(m_pD3DFont && m_iEnabled)
	{
		m_pChatTextSprite->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE );

		while(x!=DISP_MESSAGES) {

			switch(m_ChatWindowEntries[iMessageAt].eType) {

				case CHAT_TYPE_CHAT:

					i = strlen(m_ChatWindowEntries[iMessageAt].szNick);

					if (m_bTimestamp) {
						char tmp_timestamp[30];
						sprintf(tmp_timestamp, "[%02d:%02d:%02d]", m_ChatWindowEntries[iMessageAt].pTime.wHour, m_ChatWindowEntries[iMessageAt].pTime.wMinute, m_ChatWindowEntries[iMessageAt].pTime.wSecond);
						m_pD3DFont->DrawText(0, tmp_timestamp, -1, &rectSize, DT_CALCRECT | DT_LEFT, 0xFF000000);
						RenderText(tmp_timestamp, rect, m_ChatWindowEntries[iMessageAt].dwNickColor);
						rect.left = 35 + (rectSize.right - rectSize.left);
					}
					if (i) {
						m_pD3DFont->DrawText(0, m_ChatWindowEntries[iMessageAt].szNick, -1, &rectSize, DT_CALCRECT | DT_LEFT, 0xFF000000);
						RenderText(m_ChatWindowEntries[iMessageAt].szNick, rect, m_ChatWindowEntries[iMessageAt].dwNickColor);
						rect.left = m_bTimestamp ? rect.left + 35 + (rectSize.right - rectSize.left) : 35 + (rectSize.right - rectSize.left);
					}

					RenderText(m_ChatWindowEntries[iMessageAt].szMessage,rect,m_ChatWindowEntries[iMessageAt].dwTextColor);
	
					break;

				case CHAT_TYPE_INFO:
				case CHAT_TYPE_DEBUG:
					if (m_bTimestamp) {
						char tmp_timestamp[30];
						sprintf(tmp_timestamp, "[%02d:%02d:%02d]", m_ChatWindowEntries[iMessageAt].pTime.wHour, m_ChatWindowEntries[iMessageAt].pTime.wMinute, m_ChatWindowEntries[iMessageAt].pTime.wSecond);
						m_pD3DFont->DrawText(0, tmp_timestamp, -1, &rectSize, DT_CALCRECT | DT_LEFT, 0xFF000000);
						RenderText(tmp_timestamp, rect, m_ChatWindowEntries[iMessageAt].dwTextColor);
						rect.left = 35 + (rectSize.right - rectSize.left);
					}
					RenderText(m_ChatWindowEntries[iMessageAt].szMessage,rect,m_ChatWindowEntries[iMessageAt].dwTextColor);
					break;
			}		

			rect.top+=m_lFontSizeY+1;
			rect.bottom= rect.top + m_lFontSizeY+1;
			rect.left = 30;
			
			iMessageAt--;
			x++;
		}
		m_lChatWindowBottom = rect.bottom;

		m_pChatTextSprite->End();
	}	
}

//----------------------------------------------------

void CChatWindow::AddChatMessage(CHAR *szNick, DWORD dwNickColor, CHAR *szMessage)
{
	FilterInvalidChars(szMessage);

	if(strlen(szMessage) > MAX_MESSAGE_LENGTH) return;

	AddToChatWindowBuffer(CHAT_TYPE_CHAT,szMessage,szNick,m_dwChatTextColor,dwNickColor);
}

//----------------------------------------------------

void CChatWindow::AddInfoMessage(CHAR * szFormat, ...)
{
	char tmp_buf[512];
	memset(tmp_buf,0,512);

	va_list args;
	va_start(args, szFormat);
	vsprintf(tmp_buf, szFormat, args);
	va_end(args);

	FilterInvalidChars(tmp_buf);

	if(strlen(tmp_buf) > MAX_MESSAGE_LENGTH) return;

	AddToChatWindowBuffer(CHAT_TYPE_INFO,tmp_buf,NULL,m_dwChatInfoColor,0);
}

//----------------------------------------------------

void CChatWindow::AddDebugMessage(CHAR * szFormat, ...)
{
	char tmp_buf[512];
	memset(tmp_buf,0,512);

	va_list args;
	va_start(args, szFormat);
	vsprintf(tmp_buf, szFormat, args);
	va_end(args);

	FilterInvalidChars(tmp_buf);

	if(strlen(tmp_buf) > MAX_MESSAGE_LENGTH) return;

	AddToChatWindowBuffer(CHAT_TYPE_DEBUG,tmp_buf,NULL,m_dwChatDebugColor,0);
	OutputDebugString(tmp_buf);
}

//----------------------------------------------------

void CChatWindow::AddClientMessage(DWORD dwColor, PCHAR szStr)
{
	dwColor = (dwColor >> 8) | 0xFF000000; // convert to ARGB

	FilterInvalidChars(szStr);

	if(strlen(szStr) > MAX_MESSAGE_LENGTH) return;

	AddToChatWindowBuffer(CHAT_TYPE_INFO,szStr,NULL,dwColor,0);
}

//----------------------------------------------------

void CChatWindow::FilterInvalidChars(PCHAR szString)
{
	while(*szString) {
		if(*szString > 0 && *szString < ' ') {
			*szString = ' ';
		}
		szString++;
	}
}

//----------------------------------------------------

void CChatWindow::AddToChatWindowBuffer(eChatMessageType eType, 
										PCHAR szString, 
										PCHAR szNick,
										DWORD dwTextColor,
										DWORD dwChatColor)
{
	int iBestLineLength=0;

	PushBack();

	m_ChatWindowEntries[0].eType = eType;
	m_ChatWindowEntries[0].dwTextColor = dwTextColor;
	m_ChatWindowEntries[0].dwNickColor = dwChatColor;
	GetSystemTime(&m_ChatWindowEntries[0].pTime);
	if(szNick) {
		strcpy(m_ChatWindowEntries[0].szNick,szNick);
		strcat(m_ChatWindowEntries[0].szNick,":");
	} else {
		m_ChatWindowEntries[0].szNick[0] = '\0';
	}

	if(m_ChatWindowEntries[0].eType == CHAT_TYPE_CHAT && strlen(szString) > MAX_LINE_LENGTH)
	{
		iBestLineLength = MAX_LINE_LENGTH;
		// see if we can locate a space.
		while(szString[iBestLineLength] != ' ' && iBestLineLength)
			iBestLineLength--;

		if((MAX_LINE_LENGTH - iBestLineLength) > 12) {
			// we should just take the whole line
			strncpy(m_ChatWindowEntries[0].szMessage,szString,MAX_LINE_LENGTH);
			m_ChatWindowEntries[0].szMessage[MAX_LINE_LENGTH] = '\0';

			PushBack();
			
			m_ChatWindowEntries[0].eType = eType;
			m_ChatWindowEntries[0].dwTextColor = dwTextColor;
			m_ChatWindowEntries[0].dwNickColor = dwChatColor;
			m_ChatWindowEntries[0].szNick[0] = '\0';

			strcpy(m_ChatWindowEntries[0].szMessage,szString+MAX_LINE_LENGTH);
		}
		else {
			// grab upto the found space.
			strncpy(m_ChatWindowEntries[0].szMessage,szString,iBestLineLength);
			m_ChatWindowEntries[0].szMessage[iBestLineLength] = '\0';

			PushBack();

			m_ChatWindowEntries[0].eType = eType;
			m_ChatWindowEntries[0].dwTextColor = dwTextColor;
			m_ChatWindowEntries[0].dwNickColor = dwChatColor;
			m_ChatWindowEntries[0].szNick[0] = '\0';

			strcpy(m_ChatWindowEntries[0].szMessage,szString+(iBestLineLength+1));
		}
	}
	else {
		strncpy(m_ChatWindowEntries[0].szMessage,szString,MAX_MESSAGE_LENGTH);
		m_ChatWindowEntries[0].szMessage[MAX_MESSAGE_LENGTH] = '\0';
	}
	
}

//----------------------------------------------------

void CChatWindow::PushBack()
{
	int x=MAX_MESSAGES-1;
	while(x) {
		memcpy(&m_ChatWindowEntries[x],&m_ChatWindowEntries[x-1],sizeof(CHAT_WINDOW_ENTRY));
		x--;
	}
}

// From (https://github.com/IgnatBantserov/sacm/blob/master/client/chatwindow.cpp)
// With minor edits.
//----------------------------------------------------
typedef struct _COLOR_TAG {
	DWORD _color;
	bool _alpha;
	bool _valid;
} COLOR_TAG;

typedef struct _PARTS_OF_MESSAGE {
	DWORD dwColor;
	int iStartPos;
	int iEndPos;
} PARTS_OF_MESSAGE;

//----------------------------------------------------
static signed char HexToDec(signed char ch) {
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	if (ch >= 'a' && ch <= 'f')
		return ch - 'A' + 10;

	return -1;
}

//----------------------------------------------------
bool IsValidHex(const std::string& hex) {
	if (hex.empty())
		return false;

	for (size_t i = 0, len = hex.length(); i < len; i++)
	{
		if (HexToDec(hex[i]) == -1)
			return false;
	}
	return true;
}

//----------------------------------------------------
COLOR_TAG GetColorTag(const char *text, size_t maxLen) {
	COLOR_TAG color = { 0, false, false };
	const int minLen = 8;

	if (text == nullptr || *text != '{' || maxLen < minLen)
		return color;

	if (text[7] == '}') {
		color._alpha = false;
		color._valid = true;
	}
	else if (minLen + 2 <= maxLen && text[9] == '}') {
		color._alpha = true;
		color._valid = true;
	}
	if (color._valid) {
		if (IsValidHex(std::string(text).substr(1, color._alpha ? 8 : 6)))
			color._color = strtol(&text[1], nullptr, 16);
		else
			color._valid = false;
	}
	return color;
}

//----------------------------------------------------
LONG CChatWindow::GetWidth(const char *szString) {
	RECT rect = { 0, 0, 0, 0 };
	if (m_pD3DFont)
		m_pD3DFont->DrawText(NULL, szString, strlen(szString), &rect, DT_CALCRECT, D3DCOLOR_XRGB(0, 0, 0));

	return rect.right - rect.left;
}

//----------------------------------------------------
void CChatWindow::RenderText(CHAR *sz, RECT rect, DWORD dwColor)
{
	PARTS_OF_MESSAGE partOfMessage[32];
	int iCounter = 1, iLen = strlen(sz);

	for (int i = 0; i < 32; i++) {
		partOfMessage[i].dwColor = dwColor;
		partOfMessage[i].iStartPos = 0;
		partOfMessage[i].iEndPos = 0;

	}

	for (size_t i = 0; i < iLen; i++)
	{
		if (sz[i] == '{')
		{
			COLOR_TAG tag = GetColorTag(&sz[i], iLen - i);
			if (tag._valid)
			{
				partOfMessage[iCounter].dwColor = tag._color;
				if (!tag._alpha)
					partOfMessage[iCounter].dwColor |= 0xFF000000;
				i += tag._alpha ? 9 : 7;
				partOfMessage[iCounter].iStartPos = i + 1;
				for (size_t j = i; j < iLen; j++) {
					if (sz[j] == '{') {
						partOfMessage[iCounter].iEndPos = j;
					}
				}
				iCounter++;
				continue;
			}
		}
	}
	for (int i = 0; i < iCounter; i++) {
		std::string tmpTxt = &sz[partOfMessage[i].iStartPos];
		if (partOfMessage[i].iStartPos > 0 && partOfMessage[i].iStartPos < iLen) {

			size_t pos = tmpTxt.find('{');

			if (pos > 0 && pos < iLen) {
				tmpTxt.resize(pos);
			}

			if (m_iEnabled == CHAT_WINDOW_MODE_FULL) {
				rect.top -= 1;
				m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, 0xFF000000);
				rect.top += 1;

				// below
				rect.top += 1;
				m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, 0xFF000000);
				rect.top -= 1;

				// left
				rect.left -= 1;
				m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, 0xFF000000);
				rect.left += 1;

				// right
				rect.left += 1;
				m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, 0xFF000000);
				rect.left -= 1;
			}

			m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, partOfMessage[i].dwColor);

			if (tmpTxt.c_str()[tmpTxt.length() - 1] == ' ') {
				rect.left += GetWidth(tmpTxt.c_str()) + GetWidth("{ }") - GetWidth("{}");
			}
			else {
				rect.left += GetWidth(tmpTxt.c_str());
			}
		}
		else if (sz[0] != '{') {
			std::string tmpTxt = sz;
			size_t pos = tmpTxt.find('{');

			if (pos > 0 && pos < iLen) {
				tmpTxt.resize(pos);
			}

			if (m_iEnabled == CHAT_WINDOW_MODE_FULL) {
				rect.top -= 1;
				m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, 0xFF000000);
				rect.top += 1;

				// below
				rect.top += 1;
				m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, 0xFF000000);
				rect.top -= 1;

				// left
				rect.left -= 1;
				m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, 0xFF000000);
				rect.left += 1;

				// right
				rect.left += 1;
				m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, 0xFF000000);
				rect.left -= 1;
			}

			m_pD3DFont->DrawTextA(m_pChatTextSprite, tmpTxt.c_str(), -1, &rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT, partOfMessage[i].dwColor);

			if (tmpTxt.c_str()[tmpTxt.length() - 1] == ' ') {
				rect.left += GetWidth((char*)tmpTxt.c_str()) + GetWidth("{ }") - GetWidth("{}");
			}
			else {
				rect.left += GetWidth((char*)tmpTxt.c_str());
			}
			continue;
		}
	}

}


//----------------------------------------------------
// EOF