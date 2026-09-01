#include "FloatingTextManager.h"
#include "Renderable.h"
#include "Text.h"

FloatingTextManager::FloatingTextManager()
{
}


FloatingTextManager::~FloatingTextManager()
{
	Cleanup();
}

// AoTv4: owned here, used by every Text. See the note in Text.cpp for why it is shared.
ID3DXSprite* g_pFtSharedSprite = nullptr;

// AoTv4: player-tunable through /fct, defined in core_floatingtext.cpp. Upstream hardcoded 150 px,
// 1000 ms and a 1.0 ceiling on scale.
extern int   g_ftRisePixels;
extern int   g_ftDurationMs;
extern float g_ftScalePct;

// AoTv4: display mode and the two anchors, all set through the Combat Text window.
// 📌 Anchors are stored as a FRACTION of the viewport, not pixels, so they stay where the player put
// them when the resolution or window size changes -- and so the same ini is portable between machines.
extern int   g_ftMode;            // 0 = over the target, 1 = two fixed screen anchors
extern float g_ftAnchorDmgX, g_ftAnchorDmgY;    // damage: dealt and taken
extern float g_ftAnchorHealX, g_ftAnchorHealY;  // healing: dealt and taken
extern int   g_ftCascade;         // 0 = straight stack, 1 = fanned, 2 = straight with stagger
extern int   g_ftFanWidth;        // px of horizontal travel at the widest, for the fanned cascade
extern char  g_ftFontFace[64];    // a Windows face name; D3DXCreateFont falls back if it is absent
extern int   g_ftFontSize;        // font height in points

// AoTv4 colours, 0xRRGGBB, set in the Combat Text window. Indexed by FT_COLOUR_*.
extern unsigned int g_ftColour[FT_COLOUR_COUNT];
extern int          g_ftLaneGapPx;   // vertical separation forced between numbers arriving together

// AoTv4: one RNG for the fan. EQ::Random is defined in this header and seeds itself.
static EQ::Random g_ftRandom;

void FloatingTextManager::Initialize()
{
	if (!g_pFtSharedSprite) { D3DXCreateSprite(g_pDevice, &g_pFtSharedSprite); }


	// AoTv4: face and size are settings. Upstream hardcoded Arial 24.
	// ⚠️ D3DXCreateFont does NOT fail on an unknown face name -- GDI substitutes a default and it
	// SUCCEEDS. So a typo in the face gives working text in the wrong typeface, never an error, and the
	// window offers a fixed list rather than free text for exactly that reason.
	ID3DXFont* ArialFont;
	HRESULT hr = D3DXCreateFont(g_pDevice, g_ftFontSize, 0, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, g_ftFontFace, &ArialFont);
	if (SUCCEEDED(hr))
	{
		fontMap[EFonts::EFontArial] = ArialFont;
	}
	else
	{
		fontMap[EFonts::EFontArial] = nullptr;
	}

	ID3DXFont* ArialBFont;
	hr = D3DXCreateFont(g_pDevice, g_ftFontSize, 0, FW_BOLD, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, g_ftFontFace, &ArialBFont);
	if (SUCCEEDED(hr))
	{
		fontMap[EFonts::EFontArialBold] = ArialBFont;
	}
	else
	{
		fontMap[EFonts::EFontArialBold] = nullptr;
	}
}

void FloatingTextManager::Cleanup()
{
	// AoTv4: numbers first, sprite second. Every live Text points at the shared sprite, so releasing it
	// before they are destroyed leaves each of them holding a dead pointer. Deleting HudObjects here is
	// also what keeps the FONT release below safe -- a Text outliving its font would call DrawTextA on a
	// released ID3DXFont.

	for (auto delIt : HudObjects)
	{
		delete delIt;
		delIt = nullptr;
	}
	HudObjects.clear();

	for (auto fntItem : fontMap)
	{
		if (fntItem.second)
		{
			fntItem.second->Release();
			fntItem.second = nullptr;
		}
	}
	fontMap.clear();

	for (auto delIt : DeadHudObjects)
	{
		if (delIt)
		{
			delete delIt;
			delIt = nullptr;
		}
	}

	DeadHudObjects.clear();

	for (auto spellText : spellTextures)
	{
		if (spellText.second)
		{
			delete spellText.second;
			spellText.second = nullptr;
		}
	}

	spellTextures.clear();

	// AoTv4: last, once nothing references it.
	if (g_pFtSharedSprite) { g_pFtSharedSprite->Release(); g_pFtSharedSprite = nullptr; }
}

EQ::Random randomNums;

int zPosition = 0;

extern bool addedTestText;
void FloatingTextManager::AddDamageText(PSPAWNINFO actor, int damage, int spellId, uint8_t nHitType, bool isHeal, int colour)
{

	DamageText* tmp = new DamageText();

	tmp->Damage = damage;
	tmp->LastTick = MQGetTickCount64();
	tmp->SpellID = spellId;
	tmp->hitType = nHitType;
	tmp->IsHeal = isHeal;

	// AoTv4 cascade. Decided ONCE, here, not per frame -- rolling it in Render would reseed every number
	// on every frame and turn a fan into a jitter.
	switch (g_ftCascade)
	{
		case 1:  // fanned: spread sideways and vary the climb, so a multi-hit round separates visually
			tmp->DriftX    = (float)g_ftRandom.Int(-g_ftFanWidth, g_ftFanWidth);
			tmp->RiseScale = (float)g_ftRandom.Real(0.70, 1.30);
			break;
		case 2:  // straight with a small stagger -- what it did before any of this was configurable
			tmp->DriftX    = (float)g_ftRandom.Int(-12, 12);
			tmp->RiseScale = 1.0f;
			break;
		default: // straight stack
			tmp->DriftX    = 0.0f;
			tmp->RiseScale = 1.0f;
			break;
	}
	bool isLocalActor = (PSPAWNINFO)pLocalPlayer == actor;

	// AoTv4 colour. Four configurable colours, chosen by what the number IS.
	// ⚠️⚠️ UPSTREAM FORCED RED FOR ANYTHING ON THE LOCAL PLAYER, WHICH MADE EVERY HEAL ON YOURSELF RED.
	// That is the single most misleading thing floating text can do -- a healer watching their own bar
	// sees red numbers and reads them as damage. "On me" is now just one of the four colours, and a
	// heal is coloured as a heal wherever it lands.
	const int ci = (colour >= 0 && colour < FT_COLOUR_COUNT) ? colour : FT_COLOUR_MELEE;
	const unsigned int rgb = g_ftColour[ci];
	(void)isLocalActor;   // kept for the offset below; it no longer decides the colour
	tmp->fontColor = D3DXCOLOR(((rgb >> 16) & 0xFF) / 255.0f,
	                           ((rgb >>  8) & 0xFF) / 255.0f,
	                           ( rgb        & 0xFF) / 255.0f, 1.0f);
	tmp->fontSizePct = GetFontSizePctFromHitType(nHitType);
	auto rOffsetX = 0.0f;
	auto rOffsetY = 0.0f;
	auto rOffsetZ = zPosition - 5;
	zPosition = (zPosition + 1) % 10;

	if (isLocalActor)
		rOffsetZ -= -5;


	// AoTv4 lane. Reported from play: damage dealt and damage taken land at the same instant and print
	// exactly on top of each other, which is unreadable however they are coloured.
	// ⚠️⚠️ COUNTING LIVE NUMBERS IS WHAT FIXES IT, NOT A BIGGER SPREAD. The fan is RANDOM, so widening
	// it lowers the odds of a collision without ever removing it -- and two numbers arriving in the same
	// frame can still roll the same offset. This gives each arrival its own row, and only among numbers
	// that share an anchor, so an unrelated old number does not push a new one off screen.
	int lane = 0;
	for (auto ex : HudObjects) {
		if (ex->IsHeal == isHeal) { ++lane; }
	}
	if (lane > 5) { lane = lane % 6; }   // wrap rather than climb forever during a heavy burst
	tmp->Lane = lane;

	tmp->InitialActorLocation = D3DXVECTOR3(actor->X + rOffsetX, actor->Y + rOffsetY, actor->Z - rOffsetZ);
	// AoTv4: rise distance and lifetime are /fct settings rather than the hardcoded 150 px / 1000 ms.
	// \u26a0\u26a0 THE TWO DURATIONS MUST MATCH. They are stepped by the same delta each frame and Render
	// retires a number when its OPACITY tween reaches 0 -- so a location tween longer than the opacity one
	// leaves numbers frozen mid-rise, and a shorter one snaps them to the top and holds them there.
	tmp->LocationTween = tweeny::from(0).to(g_ftRisePixels).during(g_ftDurationMs);
	tmp->OpacityTween  = tweeny::from(1.0f).to(0.0f).during(g_ftDurationMs);
	if (spellId)
		tmp->Icon = LoadSpellImage(spellId);



	tmp->m_Text->Initialize(fontMap[GetFontTypeFromHitType(nHitType)], std::to_string(damage));

	HudObjects.push_back(tmp);
}

D3DXCOLOR FloatingTextManager::GetFontColorFromHitType(uint8_t nHitType)
{
	if (nHitType & 4)
		return D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f);
	return D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
}

float FloatingTextManager::GetFontSizePctFromHitType(uint8_t nHitType)
{
	if (nHitType & 2)
		return 0.6f;

	return 1.0f;
}

EFonts FloatingTextManager::GetFontTypeFromHitType(uint8_t nHitType)
{
	if (nHitType & 1)
		return EFonts::EFontArialBold;
	return EFonts::EFontArial;
}

float mapValue(float mainValue, float inValueMin, float inValueMax, float outValueMin, float outValueMax)
{
	return (mainValue - inValueMin) * (outValueMax - outValueMin) / (inValueMax - inValueMin) + outValueMin;
}

void FloatingTextManager::Render()
{

	if (!g_pDevice)
		return;

	auto current = MQGetTickCount64();
	int index = 0;
	for (auto it : HudObjects)
	{
		it->Display = true;
		if (it->LastTick == 0)
			it->LastTick = current;

		D3DXVECTOR3 screen;

		D3DXVECTOR3 world;
		world.x = it->InitialActorLocation.y;
		world.y = it->InitialActorLocation.x;
		world.z = it->InitialActorLocation.z;

		// AoTv4: anchored mode never projects. The number is pinned to a fixed fraction of the viewport
		// and cascades from there, so it cannot be hidden by geometry, walk off screen behind you, or
		// vanish when the thing you hit dies -- which is the whole reason for offering it.
		// 📌 Read from the viewport every frame rather than cached: the player can alt-enter to fullscreen
		// or resize the window at any point, and a cached size would strand both anchors off screen.
		bool anchored = (g_ftMode == 1);
		if (anchored)
		{
			D3DVIEWPORT9 vp;
			if (g_pDevice->GetViewport(&vp) == D3D_OK)
			{
				const float ax = it->IsHeal ? g_ftAnchorHealX : g_ftAnchorDmgX;
				const float ay = it->IsHeal ? g_ftAnchorHealY : g_ftAnchorDmgY;
				screen.x = vp.X + ax * vp.Width;
				screen.y = vp.Y + ay * vp.Height;
				screen.z = 0.5f;   // inside the frustum test below, which anchored text must always pass
			}
			else
			{
				anchored = false;   // no viewport: fall back to projecting rather than drawing at 0,0
			}
		}

		if (!anchored)
		{
			WorldToScreen(world, &screen);
		}

		// AoTv4: skip anything not in front of the camera.
		// ⚠⚠ D3DXVec3Project DOES NOT FAIL on a point behind the viewer -- it divides by a negative w
		// and returns a MIRRORED on-screen position, so the number draws confidently in the wrong place
		// instead of not drawing. Projected z outside [0,1] is the frustum test, and it is the only thing
		// separating "behind me" from "in front of me" here.
		// 📌 This matters most for damage taken: those numbers are placed on the LOCAL player, who in
		// first person sits essentially at the camera origin, so without this every hit on you throws a red
		// number to a mirrored spot on screen. With it, they simply do not draw in first person and do draw
		// in third -- which is the right behaviour and needs no special case for the local player.
		// ⚠ The tween is still stepped below regardless, so a number hidden behind the camera still ages
		// and expires on schedule rather than freezing and reappearing if you spin round.
		const bool ft_on_screen = (screen.z >= 0.0f && screen.z <= 1.0f);

		auto res = it->LocationTween.step((int)(current - it->LastTick));
		auto opacity = it->OpacityTween.step((int)(current - it->LastTick));
		it->LastTick = current;

		RECTF spriteRect = RECTF(), textRect = RECTF();

		// AoTv4: our GetDistance3D is the six-float form (MQ2Inlines.h:260); upstream had an overload
		// taking a spawn plus y/x/z. `world` already has x and y swapped (see above), so world.y/world.x
		// are the actor's real EQ coordinates.
		// ⚠ `dist` is UNUSED -- textScale and iconScale below are constants and the mapValue calls that
		// consumed it are commented out upstream. Kept, correctly, so re-enabling them just works.
		PSPAWNINFO ftMe = (PSPAWNINFO)pLocalPlayer;
		float dist = ftMe ? GetDistance3D(ftMe->X, ftMe->Y, ftMe->Z, world.y, world.x, world.z) : 0.0f;
		(void)dist;

		//Convert 50 and 500 distance range to 0f and 1f range. Scale text accordingly
		float textScale = 0.85f;//mapValue(dist, 50.f, 500.f, 0.85f, 1.1f);
		float iconScale = 0.5f;//mapValue(dist, 50.f, 500.f, 0.65f, 0.85f);

		textScale = Clamp<float>(textScale, 0.45f, 1.0f);
		iconScale = Clamp<float>(iconScale, 0.45f, 1.0f);

		textScale *= it->fontSizePct;
		iconScale *= it->fontSizePct;

		// AoTv4: the /fct scale goes on AFTER the clamp, deliberately. That clamp has a ceiling of 1.0, so
		// applying the setting before it would silently cap "/fct size 150" at 100 percent and the command
		// would look broken for every value above the default.
		textScale *= g_ftScalePct;
		iconScale *= g_ftScalePct;


		// AoTv4 cascade: progress through this number's life, 0 at the anchor and 1 as it fades out.
		// ⚠️ Guarded against a rise of 0 -- the /fct and window clamps stop that, but the ini is a text
		// file and this divides.
		const float ft_t     = (g_ftRisePixels > 0) ? ((float)res / (float)g_ftRisePixels) : 0.0f;
		const float ft_drift = it->DriftX * ft_t;
		const float ft_rise  = (float)res * it->RiseScale;

		if (it->Icon)
			it->Icon->GetRect(&spriteRect,   iconScale);

		it->m_Text->GetRect(&textRect, textScale);

		auto totalWidth = spriteRect.right + textRect.right;

		auto startx = screen.x - (totalWidth / 2) + ft_drift;
		auto starty = screen.y + (abs(textRect.bottom - spriteRect.bottom) / 2.0f)
		            + (float)(it->Lane * g_ftLaneGapPx);

		if (it->Icon && ft_on_screen)
		{
			it->Icon->Render(D3DXVECTOR3(startx, starty - ft_rise, 0), D3DXVECTOR3(iconScale, iconScale, 1.0f), D3DXCOLOR(1, 1, 1, opacity), 0);
			startx += 50 * iconScale;
		}

		if (ft_on_screen)
		{
			it->m_Text->Render(D3DXVECTOR3(startx, starty - ft_rise - 2.5f, 0), D3DXVECTOR3(textScale, textScale, 1.0f), D3DXCOLOR(it->fontColor.r, it->fontColor.g, it->fontColor.b, opacity), 2);
		}

		if (opacity == 0)
		{
			DeadHudObjects.push_back(it);
		}
	}

	for (auto delIt : DeadHudObjects)
	{
		HudObjects.remove(delIt);
		delete delIt;
		delIt = nullptr;
	}
	DeadHudObjects.clear();
	
}

Sprite* FloatingTextManager::LoadSpellImage(int id)
{

	auto find = spellTextures.find(id);

	if (spellTextures.find(id) != spellTextures.end())
		return (*find).second;

	auto spell = GetSpellByID(id);

	if (!spell)
		return false;

	char tmp[MAX_STRING] = { 0 };

	sprintf_s(tmp, "%s\\SpellIcons\\%d.png", gszEQPath, spell->SpellIcon);

	Sprite* sprite = new Sprite();
	auto res = sprite->Initialize(g_pDevice, std::string(tmp), 40, 40);

	if (res)
		spellTextures.insert(pair<int, Sprite*>(id, sprite));
	else
		return nullptr;

	return sprite;
}
