#include "../main.h"
#include "font.h"
extern CGame *pGame;

//char ProvideTmp[1024];

CTextDraw::CTextDraw(TEXT_DRAW_TRANSMIT *TextDrawTransmit, PCHAR szText)
{
	memset(&m_TextDrawData,0,sizeof(TEXT_DRAW_DATA));

	// Set standard parameters
	m_TextDrawData.fLetterWidth = TextDrawTransmit->fLetterWidth;
	m_TextDrawData.fLetterHeight = TextDrawTransmit->fLetterHeight;
	m_TextDrawData.dwLetterColor = TextDrawTransmit->dwLetterColor;
	m_TextDrawData.byteUnk12 = 0;
	m_TextDrawData.byteCentered = TextDrawTransmit->byteCenter;
	m_TextDrawData.byteBox = TextDrawTransmit->byteBox;
	m_TextDrawData.fLineWidth = TextDrawTransmit->fLineWidth;
	m_TextDrawData.fLineHeight = TextDrawTransmit->fLineHeight;
	m_TextDrawData.dwBoxColor = TextDrawTransmit->dwBoxColor;
	m_TextDrawData.byteProportional = TextDrawTransmit->byteProportional;
	m_TextDrawData.dwBackgroundColor = TextDrawTransmit->dwBackgroundColor;
	m_TextDrawData.byteShadow = TextDrawTransmit->byteShadow;
	m_TextDrawData.byteOutline = TextDrawTransmit->byteOutline;
	m_TextDrawData.byteAlignLeft = TextDrawTransmit->byteLeft;
	m_TextDrawData.byteAlignRight = TextDrawTransmit->byteRight;
	m_TextDrawData.dwStyle = TextDrawTransmit->byteStyle;
	m_TextDrawData.fX = TextDrawTransmit->fX;
	m_TextDrawData.fY = TextDrawTransmit->fY;
	m_TextDrawData.dwParam1 = 0xFFFFFFFF;
	m_TextDrawData.dwParam2 = 0xFFFFFFFF;
	
	strncpy(m_szText, szText, MAX_TEXT_DRAW_LINE);
	m_szText[MAX_TEXT_DRAW_LINE - 1] = '\0';
}

void CTextDraw::SetText(char* szText)
{
	strncpy(m_szText, szText, MAX_TEXT_DRAW_LINE);
	m_szText[MAX_TEXT_DRAW_LINE - 1] = '\0';
}


void CTextDraw::Draw()
{
	if(!m_szText || !strlen(m_szText)) return;

	strcpy(m_szString,m_szText);

	int iScreenWidth, iScreenHeight;
	float fVertHudScale, fHorizHudScale;

    iScreenWidth = pGame->GetScreenWidth();
	iScreenHeight = pGame->GetScreenHeight();
	fVertHudScale = pGame->GetHudVertScale();
	fHorizHudScale = pGame->GetHudHorizScale();

    float fScaleY = (float)iScreenHeight * fVertHudScale * m_TextDrawData.fLetterHeight * 0.5f;
	float fScaleX = (float)iScreenWidth * fHorizHudScale * m_TextDrawData.fLetterWidth;

	Font_SetScale(fScaleX,fScaleY);

	Font_SetColor(m_TextDrawData.dwLetterColor);
    Font_Unk12(0);

	if(m_TextDrawData.byteAlignRight) Font_SetJustify(2);
	else if(m_TextDrawData.byteCentered) Font_SetJustify(0);
    else Font_SetJustify(1);

	//Font_SetRightJustifyWrap(0.0f);

	float fLineWidth = iScreenWidth * fHorizHudScale * m_TextDrawData.fLineWidth;
	Font_SetLineWidth(fLineWidth);

	float fLineHeight = iScreenWidth * fHorizHudScale * m_TextDrawData.fLineHeight;
	Font_SetLineHeight(fLineHeight);

	Font_UseBox(m_TextDrawData.byteBox,0);
	Font_UseBoxColor(m_TextDrawData.dwBoxColor);
	Font_SetProportional(m_TextDrawData.byteProportional);
	Font_SetDropColor(m_TextDrawData.dwBackgroundColor);

	if(m_TextDrawData.byteOutline) {
		Font_SetOutline(m_TextDrawData.byteOutline);
	} else {
		Font_SetShadow(m_TextDrawData.byteShadow);
	}

	Font_SetFontStyle(m_TextDrawData.dwStyle);

	Font_UnkConv(m_szString,-1,-1,-1,-1,-1,-1,m_szString);
	Font_UnkConv2(m_szString);

	float fPS2Height = 448.0f;
	float fPS2Width = 640.0f;
	float fScriptY =  m_TextDrawData.fY;
	float fScriptX =  m_TextDrawData.fX;
	float fUseX,fUseY;

	/*
	_asm fild	iScreenHeight  ; st(2)
	_asm fild	iScreenWidth   ; st(1)
	_asm fld	fPS2Height     ; st
	_asm fsub	fScriptY       ; fPS2Height - fScriptY
	_asm fld    st(2)		   ; st = iScreenHeight
	_asm sub    esp, 8
	_asm fmul	fVertHudScale  ; 
	_asm fmulp  st(1), st	   ; (fPS2Height - fScriptY) * (iScreenHeight * fVertHudScale)
	_asm fsubr  st, st(2)	   ; iScreenHeight - ((fPS2Height - fScriptY) * (iScreenHeight * fVertHudScale))
	_asm fstp	fUseY		   ; fUseY = ^

	_asm fld	fPS2Width
	_asm fsub	fScriptX
	_asm fld    st(1)
	_asm fmul	fHorizHudScale
	_asm fmulp  st(1), st
	_asm fsubr  st, st(1)
	_asm fstp	fUseX*/

	fUseY = iScreenHeight - ((fPS2Height - fScriptY) * (iScreenHeight * fVertHudScale));
    fUseX = iScreenWidth - ((fPS2Width - fScriptX) * (iScreenWidth * fHorizHudScale));
    
    Font_PrintString(fUseX,fUseY,m_szString);
	Font_SetOutline(0);
}
