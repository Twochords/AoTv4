#pragma once
#include "tweeny/tweeny.h"

#include <map>
#include <vector>
#include <list>
#include <random>      // AoTv4: std::mt19937 / uniform_*_distribution / random_device.
                       // Upstream got this transitively from its MQ2Main.h; ours does not.
#include <algorithm>   // AoTv4: std::min / std::max, used by EQClamp and Clamp below.
#include <d3d9.h>
#include <d3dx9math.h>


#include "MQ2Main.h"

// AoTv4 colour slots. Indices into g_ftColour, defined in core_floatingtext.cpp.
// ⚠️ Your MELEE and your SPELL damage are separate slots. Reported from play: with one "your damage"
// colour there is no way to read at a glance whether a number came from a swing or a nuke, which is
// most of what the display is for on a caster.
enum EFtColour {
	FT_COLOUR_MELEE   = 0,   // an ordinary weapon swing
	FT_COLOUR_SPELL   = 1,   // a spell, including damage over time
	FT_COLOUR_ABILITY = 2,   // an activated combat ability (Bash, Kick, Reckless Cleave, ...)
	FT_COLOUR_TAKEN   = 3,   // damage dealt to you, from any source
	FT_COLOUR_HEAL    = 4,
	FT_COLOUR_CRIT    = 5,
	FT_COLOUR_COUNT   = 6
};

#include "TickCount.h"
#include "Sprite.h"
#include "Text.h"

enum EFonts {
	EFontArial = 0,
	EFontArialBold = 1,
	EFontMAX = 2
};

// AoTv4: free Clamp, used by FloatingTextManager::Render as Clamp<float>(...). Upstream relies on
// one existing in its MQ2 base; ours has none by that name.
template<typename T>
inline T Clamp(const T& value, const T& lower, const T& upper)
{
	return (std::max)(lower, (std::min)(value, upper));
}

extern void WorldToScreen(D3DXVECTOR3 world, D3DXVECTOR3* screen);
extern IDirect3DDevice9* g_pDevice;
namespace EQ {
	class Random {

	public:

		// AoTv4: the render loop calls a FREE Clamp<float>, which upstream never defines -- EQClamp
		// below is a member and unreachable from there. Same body, file scope.
		template<typename T>
		T EQClamp(const T& value, const T& lower, const T& upper)
		{
			return std::max(lower, std::min(value, upper));
		}

		// AKA old MakeRandomInt
		int Int(int low, int high)
		{
			if (low > high)
				std::swap(low, high);
			return int_dist(m_gen, int_param_t(low, high)); // [low, high]
		}

		// AKA old MakeRandomFloat
		double Real(double low, double high)
		{
			if (low > high)
				std::swap(low, high);
			return real_dist(m_gen, real_param_t(low, high)); // [low, high)
		}

		// example Roll(50) would have a 50% success rate
		// Roll(100) 100%, etc
		// valid values 0-100 (well, higher works too but ...)
		bool Roll(const int required)
		{
			return Int(0, 99) < required;
		}

		// valid values 0.0 - 1.0
		bool Roll(const double required)
		{
			return Real(0.0, 1.0) <= required;
		}

		// same range as client's roll0
		// This is their main high level RNG function
		int Roll0(int max)
		{
			if (max - 1 > 0)
				return Int(0, max - 1);
			return 0;
		}

		// std::shuffle requires a RNG engine passed to it, so lets provide a wrapper to use our engine
		template<typename RandomAccessIterator>
		void Shuffle(RandomAccessIterator first, RandomAccessIterator last)
		{
			static_assert(std::is_same<std::random_access_iterator_tag,
				typename std::iterator_traits<RandomAccessIterator>::iterator_category>::value,
				"EQ::Random::Shuffle requires random access iterators");
			std::shuffle(first, last, m_gen);
		}

		template<typename Iter, typename RandomGenerator>
		Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
			std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
			std::advance(start, dis(g));
			return start;
		}

		template<typename Iter>
		Iter select_randomly(Iter start, Iter end) {
			static std::random_device rd;
			static std::mt19937 gen(rd());
			return select_randomly(start, end, gen);
		}

		void Reseed()
		{
			// We could do the seed_seq thing here too if we need better seeding
			// but that is mostly overkill for us, so just seed once
			std::random_device rd;
			m_gen.seed(rd());
		}

		Random()
		{
			Reseed();
		}

	private:
		typedef std::uniform_int_distribution<int>::param_type int_param_t;
		typedef std::uniform_real_distribution<double>::param_type real_param_t;
		std::mt19937 m_gen;
		std::uniform_int_distribution<int> int_dist;
		std::uniform_real_distribution<double> real_dist;
	};
}

class DamageText
{
public:
	DamageText()
	{
		SpellID = -1;
		LastTick = 0;
		Damage = 0;
		hitType = 0;
		InitialActorLocation = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		Display = true;
		IsHeal = false;
		DriftX = 0.0f;
		RiseScale = 1.0f;
		Lane = 0;
		m_Text = new Text();
		Icon = nullptr;
		fontSizePct = 1.0f;
		fontColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
	};

	~DamageText() 
	{
		if (m_Text)
		{
			delete m_Text;
			m_Text = nullptr;
		}
	};

	int SpellID;
	unsigned __int64 LastTick;
	int Damage;
	uint8_t hitType;

	// AoTv4 additions.
	bool  IsHeal;      // picks which screen anchor this belongs to, and nothing else
	float DriftX;      // horizontal travel over the number's life, for the fanned cascade
	float RiseScale;   // per-number multiplier on the vertical travel, so a burst does not move as one
	int   Lane;        // row this number starts on, so simultaneous arrivals never overlap
	D3DXCOLOR fontColor;
	float fontSizePct;
	tweeny::tween<int> LocationTween;
	tweeny::tween<float> OpacityTween;
	D3DXVECTOR3 InitialActorLocation;
	Text* m_Text;
	Sprite* Icon;

	bool Display;
};


class FloatingTextManager
{
public:
	FloatingTextManager();
	~FloatingTextManager();

	void Initialize();

	void Cleanup();

	// AoTv4: isHeal decides which anchor the number cascades from in anchored mode, and nothing else --
	// colour still comes from hitType, so a heal is coloured by its flags like any other number.
	// AoTv4: `colour` is an EFtColour chosen by the caller, because only the caller knows whether the
	// number came from a swing, a nuke or a heal -- the renderer cannot tell them apart.
	void AddDamageText(PSPAWNINFO actor, int damage, int spellId, uint8_t hitType, bool isHeal = false,
	                   int colour = FT_COLOUR_MELEE);

	D3DXCOLOR GetFontColorFromHitType(uint8_t nHitType);

	float GetFontSizePctFromHitType(uint8_t nHitType);

	EFonts GetFontTypeFromHitType(uint8_t nHitType);

	void Render();


private:

	Sprite* LoadSpellImage(int id);
	std::list<DamageText*> HudObjects;
	std::list<DamageText*> DeadHudObjects;
	std::map<EFonts, ID3DXFont*> fontMap;
	std::map<int, Sprite*> spellTextures;

};

