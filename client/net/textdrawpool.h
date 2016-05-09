#pragma once

//----------------------------------------------------

class CTextDrawPool
{
private:

	CTextDraw		*m_pTextDraw[MAX_TEXT_DRAWS];
	BOOL			m_bSlotState[MAX_TEXT_DRAWS];

public:
	CTextDrawPool();
	~CTextDrawPool();
	
	CTextDraw * New(WORD wText, TEXT_DRAW_TRANSMIT *TextDrawTransmit, PCHAR szText);
	void Delete(WORD wText);
	void Draw();

	CTextDraw * GetAt(WORD wText) {
		if (wText >= MAX_TEXT_DRAWS) return NULL;
		if (!m_bSlotState[wText]) return NULL;
		return m_pTextDraw[wText];
	};
};

//----------------------------------------------------
