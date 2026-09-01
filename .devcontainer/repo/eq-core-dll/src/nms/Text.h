#pragma once

#include <string>
#include <d3d9.h>
#include <d3dx9math.h>
#include "Renderable.h"

extern IDirect3DDevice9* g_pDevice;

class Text : Renderable
{
public:
	Text();
	~Text();
	bool Initialize(ID3DXFont* fontPtr, std::string text);

	virtual void Render(D3DXVECTOR3 position, D3DXVECTOR3 scale, D3DXCOLOR color, int outlineOffset) override;

	virtual void GetRect(RECTF * rect, float scale) override;

	ID3DXFont* _font;
	ID3DXSprite* _uiSprite;

private:
	// AoTv4: true only when this Text created its own sprite. See Text::Initialize.
	bool _ownsSprite;
	RECT _rect;
	std::string _text;
};

