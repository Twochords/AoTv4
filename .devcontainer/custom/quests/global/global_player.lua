-- items: 67704, 72091, 62621, 62622, 62844, 62827, 62828, 62836, 62883, 62876, 47100, 62878, 62879

local don = require("dragons_of_norrath")
local spell_choice = require("spell_choice")
local aa_choice = require("aa_choice")
local pok_travel = require("pok_travel")
local death_loss = require("death_loss")
local era_system = require("era_system")   -- server-wide expansion unlock progression
local bazaar_broker = require("bazaar_broker")  -- player-shop vendor window (vpset/vshop/vclose)
local autobuff = require("autobuff")            -- memmed beneficial buffs/songs become permanent auras
local loot_tracker = require("loot_tracker")    -- Advanced Loot System (ALS) personal loot window

-- AA-on-death tuning (roguelite: death banks AA and resets the character to level 1).
-- Reward tracks XP EFFORT below the cap, then JUMPS at the era level cap to reward the last push.
-- NO owned-AA taper -- the per-death reward is FIXED by how far you got, so these values are exact:
--     below cap:  banked = floor( (run_xp / cap_exp) * cap * DEATH_AA_SUB_CAP )   -- XP-effort ramp
--     at cap:     banked = floor( cap * DEATH_AA_AT_CAP )                          -- big final-push bonus
-- Classic (cap 50): L30 ~2, L40 ~8, L49 ~17, and dying AT 50 banks 20 -- the last push always wins,
-- but only slightly (SUB_CAP is kept just under AT_CAP so at-cap is the best death, not a huge cliff).
-- run_xp is per-run (reset on death); everything scales with the era cap (Kunark cap 60 -> 24 at cap).
local DEATH_AA_AT_CAP  = 0.40   -- AA for dying AT the era cap = cap * this  (L50 -> 20, L60 -> 24)
local DEATH_AA_SUB_CAP = 0.38   -- XP-effort scale below the cap = cap * this; ceiling = 0.95 * at-cap
                                --   so a near-cap death is ~95% of dying AT cap (L59 Kunark ~19 vs 24)

function event_enter_zone(e)
	mysterious_voice(e)
	era_system.sync_zone()                          -- point this zone's expansion rule at the unlocked era
	e.self:Message(MT.NPCQuestSay, "PORTALCLOSE")   -- dismiss the Portal window on any zone change
	autobuff.sync(e.self)                           -- memmed beneficial buffs/songs -> permanent auras
	e.self:SetTimer("autobuff", 3)                  -- keep them in sync every 3s (catches mem changes)

	if eq.is_lost_dungeons_of_norrath_enabled() and eq.get_zone_short_name() == "lavastorm" and e.self:GetGMStatus() >= 80 then 
		e.self:Message(MT.DimGray, "There are GM commands available for Dragons of Norrath, use " .. eq.say_link("#don") .. " to get started")
	end
end

function mysterious_voice(e)
	if not eq.is_lost_dungeons_of_norrath_enabled() then
		return
	end
	local qglobals = eq.get_qglobals(e.self);
	if e.self:GetLevel() < 15 then
		return
	end
	if qglobals.Wayfarer ~= nil then
		return
	end
	local zone_id = eq.get_zone_id();

	local voice_zones = {
		Zone.qeynos,
		Zone.qeynos2,
		Zone.qrg,
		Zone.freportn,
		Zone.freportw,
		Zone.freporte,
		Zone.rivervale,
		Zone.ecommons,
		Zone.erudnint,
		Zone.erudnext,
		Zone.halas,
		Zone.everfrost,
		Zone.nro,
		Zone.sro,
		Zone.neriaka,
		Zone.neriakb,
		Zone.neriakc,
		Zone.qcat,
		Zone.oggok,
		Zone.grobb,
		Zone.gfaydark,
		Zone.akanon,
		Zone.kaladima,
		Zone.felwithea,
		Zone.felwitheb,
		Zone.kaladimb,
		Zone.butcher,
		Zone.paineel,
		Zone.cabwest,
		Zone.cabeast,
		Zone.sharvahl,
		Zone.poknowledge,
		Zone.freeporteast,
		Zone.freeportwest,
		Zone.northro,
		Zone.southro,
		Zone.commonlands
	};

	for _, zone in pairs(voice_zones) do
		if zone == zone_id then
			e.self:Message(MT.Yellow,
			"A mysterious voice whispers to you, \'If you can feel me in your thoughts, know this -- "
			.. "something is changing in the world and I reckon you should be a part of it. I do not know much, but I do know "
			.. "that in every home city and the wilds there are agents of an organization called the Wayfarers Brotherhood. They "
			.. "are looking for recruits . . . If you can hear this message, you are one of the chosen. Rush to your home city, or "
			.. "search the West Karanas and Rathe Mountains for a contact if you have been exiled from your home for your deeds, "
			.. "and find out more. Adventure awaits you, my friend.\'");
			return
		end
	end
end

function event_combine_validate(e)
	-- e.validate_type values = { "check_zone", "check_tradeskill" }
	-- criteria exports:
	--	["check_zone"].         = e.zone_id
	--	["check_tradeskill"]    = e.tradeskill_id (not active)
	if (e.recipe_id == 10344) then
		if (e.validate_type:find("check_zone")) then
			if (e.zone_id ~= Zone.tipt and e.zone_id ~= Zone.vxed) then
				return 1;
			end
		end
	end

	return 0;
end

function event_combine_success(e)
	if (e.recipe_id == 10904 or e.recipe_id == 10905 or e.recipe_id == 10906 or e.recipe_id == 10907) then
		e.self:Message(MT.Default,
		"The gem resonates with power as the shards placed within glow unlocking some of the stone's power. "
		.. "You were successful in assembling most of the stone but there are four slots left to fill, "
		.. "where could those four pieces be?"
		);
	elseif(e.recipe_id == 10903 or e.recipe_id == 10346 or e.recipe_id == 10334) then
		local reward = { };
		reward["melee"] =  { ["10903"] = 67665, ["10346"] = 67660, ["10334"] = 67653 };
		reward["hybrid"] = { ["10903"] = 67666, ["10346"] = 67661, ["10334"] = 67654 };
		reward["priest"] = { ["10903"] = 67667, ["10346"] = 67662, ["10334"] = 67655 };
		reward["caster"] = { ["10903"] = 67668, ["10346"] = 67663, ["10334"] = 67656 };

		local ctype = eq.ClassType(e.self:GetClass());
		e.self:SummonItem(reward[ctype][tostring(e.recipe_id)]);
		e.self:SummonItem(67704); -- Item: Vaifan's Clockwork Gemcutter Tools
		e.self:Message(MT.Default, "Success");
	--cleric 1.5
	elseif(e.recipe_id == 19460) then
		e.self:AddEXP(25000);
		e.self:AddAAPoints(5);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 5 ability points!');
		eq.set_global("cleric_epic","7",5,"F");
	--rogue 1.5
	elseif(e.recipe_id == 13402 or e.recipe_id == 13403 or e.recipe_id == 13404 or e.recipe_id == 13405) then
		e.self:Message(MT.Yellow,"The piece of the metal orb fuses together with the blue diamonds under the intense heat of the forge. As it does, a flurry of images flash through your mind... A ranger and his bear side by side, stoic and unafraid, in a war-torn forest. A bitter tattooed woman with bluish skin wallowing in misery in a waterfront tavern. An endless barrage of crashing thunder and lightning illuminating a crimson brick ampitheater. Two halflings locked in a battle of wits using a checkered board. The images then fade from your mind");
	--ranger 1.5 tree
	elseif(e.recipe_id ==13412) then
		eq.set_global("ranger_epic","3",5,"F");
		if(eq.get_zone_short_name()=="jaggedpine") then
			e.self:Message(MT.Yellow,"The seed grows rapidly the moment you push it beneath the soil. It appears at first as a mere shoot, but within moments grows into a stout sapling and then into a gigantic tree. The tree is one you've never seen before. It is the coloration and thick bark of a redwood with the thick bole indicative of the species. The tree is, however, far too short and has spindly branches sprouting from it with beautiful flowers that you would expect on a dogwood. You take all of this in at a glance. It takes you a moment longer to realize that the tree is moving.");			
			eq.spawn2(181222, 0, 0, e.self:GetX()+3,e.self:GetY()+3,e.self:GetZ(),0); -- NPC: Red_Dogwood_Treant
		else
			e.self:Message(MT.Yellow,"The soil conditions prohibit the seed from taking hold");
			e.self:SummonItem(72091); -- Item: Fertile Earth
			e.self:SummonItem(62621); -- Item: Senvial's Blessing
			e.self:SummonItem(62622); -- Item: Grinbik's Blessing
			e.self:SummonItem(62844); -- Item: Red Dogwood Seed
		end
	--ranger 1.5 final
	elseif(e.recipe_id ==13413) then
		e.self:AddEXP(25000);
		e.self:AddAAPoints(5);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 5 ability points!');
		eq.set_global("ranger_epic","5",5,"F");
	--ranger 2.0
	elseif(e.recipe_id ==19914 or e.recipe_id==19915) then
		e.self:Message(MT.Yellow,'Very Good. Now we must attune the cage to the specific element we wish to free. You will need two items, one must protect from the element and the other must be able to absorb an incredible amount of that element. This is not a simple task. You must first discover the nature of the spirit that you wish to free and then find such items that will allow you to redirect its power. You must know that each spirit represents a specific area within their element and that is what you must focus on, not their element specifically. For example, Grinbik was an earth spirit, but his area of power was fertility. Senvial was a spirit of Water, but his power was in mist and fog.');
		eq.set_global("ranger_epic","8",5,"F");
	elseif(e.recipe_id ==19916) then
		e.self:Message(MT.Yellow,"The Red Dogwood Treant speaks to you from within your sword. 'Well done. This should allow me to free a spirit with power over cold and ice. Now you need to find the power that binds the spirit and unleash it where that spirit is bound.'");	
	elseif(e.recipe_id ==19917) then
		if(eq.get_zone_short_name()=="anguish") then
			eq.spawn2(317113, 0, 0, e.self:GetX(),e.self:GetY(),e.self:GetZ(),0); -- NPC: #Oshimai_Spirit_of_the_High_Air
		end
	-- paladin 1.5 final
	elseif(e.recipe_id ==19880) then
		e.self:AddEXP(25000);
		e.self:AddAAPoints(5);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 5 ability points!');	
		eq.set_global("paladin_epic","8",5,"F");
		e.self:Message(MT.Gray,"As the four soulstones come together, a soft blue light eminates around the dark sword. The soulstones find themselves at home within the sword. A flash occurs and four voices in unison speak in your mind, 'Thank you for saving us and giving us a purpose again. You are truly our savior and our redeemer, and we shall serve you from now on. Thank you, noble knight!")
	--bard 1.5 final	
	elseif(e.recipe_id == 19882) then
		e.self:AddEXP(25000);
		e.self:AddAAPoints(5);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 5 ability points!');	
		eq.set_global("bard15","6",5,"F");
	--druid 1.5 feerrott
	elseif(e.recipe_id == 19888) then
		if(eq.get_zone_short_name()=="feerrott") then
			eq.spawn2(47209, 0, 0, e.self:GetX()+10,e.self:GetY()+10,e.self:GetZ(),0); -- NPC: corrupted_spirit
			e.self:Message(MT.White,"compelled spirit screams as his essences is forced back into the world of the living. 'What is this? Where am I? Who are you? What do you want from me?");
		else
			e.self:SummonItem(62827); -- Item: Mangled Head
			e.self:SummonItem(62828); -- Item: Animating Heads
			e.self:SummonItem(62836); -- Item: Soul Stone
		end
	-- druid 1.5 final
	elseif(e.recipe_id ==19892) then
		e.self:AddAAPoints(5);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 5 ability points!');	
		eq.set_global("druid_epic","8",5,"F");	
		e.self:SendMarqueeMessage(MT.Yellow, 510, 1, 100, 10000, "You plant the Mind Crystal and the Seed of Living Brambles in the pot. The pot grows warm and immediately you see a vine sprouting from the soil. The vine continues to grow at a tremendous rate. Brambles grow into the heart of the crystal where the core impurity is and split it. They continue to grow at an astounding speed and soon burst the pot and form the Staff of Living Brambles");
	--druid 2.0 sub final
	elseif(e.recipe_id ==19908) then
		if(eq.get_zone_short_name()=="anguish") then
			eq.spawn2(317115, 0, 0, e.self:GetX()+3,e.self:GetY()+3,e.self:GetZ(),0); -- NPC: #Yuisaha
			e.self:SummonItem(62883); -- Item: Essence of Rainfall
			e.self:SummonItem(62876); -- Item: Insulated Container
		else
			e.self:Message(MT.Yellow,"The rain spirit cannot be reached here");
			e.self:SummonItem(47100); -- Item: Globe of Discordant Energy
			e.self:SummonItem(62876); -- Item: Insulated Container
			e.self:SummonItem(62878); -- Item: Frozen Rain Spirit
			e.self:SummonItem(62879); -- Item: Everburning Jagged Tree Limb
		end
	--druid 2.0 final
	elseif(e.recipe_id ==19909) then	
		e.self:AddEXP(50000);
		e.self:AddAAPoints(10);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 10 ability points!');	
		eq.set_global("druid_epic","13",5,"F");	
		--e.self:SendMarqueeMessage(MT.Yellow, 510, 1, 100, 10000, "You plant the Mind Crystal and the Seed of Living Brambles in the pot. The pot grows warm and immediately you see a vine sprouting from the soil. The vine continues to grow at a tremendous rate. Brambles grow into the heart of the crystal where the core impurity is and split it. They continue to grow at an astounding speed and soon burst the pot and form the Staff of Living Brambles");
	--warrior 2.0
	elseif(e.recipe_id ==19902) then	
		e.self:AddEXP(50000);
		e.self:AddAAPoints(10);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 10 ability points!');	
		eq.set_global("warrior_epic","21",5,"F");		
	-- CLR 2.0
	elseif (e.recipe_id == 19893) then
		e.self:Message(MT.Red, "Omat should probably see this.");
	--ench 2.0
	elseif (e.recipe_id == 19919) then
		eq.set_global("ench_epic","9",5,"F");
		e.self:Message(MT.Yellow,"Your Oculus of Persuasion gleams with a blinding light for a moment, dimming quickly to its previous understated beauty. The light has left an image burned into your mind, a strangely tattooed woman chanting by a waterfall.");
	--ench 2.0 final
	elseif (e.recipe_id == 19920) then
		e.self:Message(MT.Yellow,"The discordant energy shoots through the staff, sending a shower of sparks through the air. The crystal shatters before you, and as the sparks fade away you notice the changes in your staff.");
		e.self:AddEXP(50000);
		e.self:AddAAPoints(10);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 10 ability points!');
		eq.set_global("ench_epic","10",5,"F");
	--pal 2.0 final
	elseif (e.recipe_id == 19925) then
		e.self:Message(MT.Yellow,"As you combine all six tokens in the scabbard with Redemption, you feel a tugging at your soul. An energy flows through you as you feel the virtues of your inner self being tugged and tempered into the weapon. For a second you feel drained, but now that feeling has subsided. A final flash of light occurs and a new sword is tempered; Nightbane, Sword of the Valiant");
		e.self:AddEXP(50000);
		e.self:AddAAPoints(10);
		e.self:Ding();
		e.self:Message(MT.Yellow,'You have gained 10 ability points!');
		eq.set_global("paladin_epic","11",5,"F");
		eq.delete_global("paladin_epic_mmcc");
		eq.delete_global("paladin_epic_hollowc");
	elseif (e.recipe_id == 2182) then -- Pumpkin Pie
		if (eq.is_task_activity_active(8013, 0)) then -- The Hungry Halfling
			eq.update_task_activity(8013, 0, 1);
		end
	elseif (e.recipe_id == 2181) then -- Pumpkin Bread
		if (eq.is_task_activity_active(8013, 1)) then -- The Hungry Halfling
			eq.update_task_activity(8013, 1, 1);
		end
	elseif (e.recipe_id == 7811) then -- Spiced Pumpkin Cider
		if (eq.is_task_activity_active(8013, 2)) then -- The Hungry Halfling
			eq.update_task_activity(8013, 2, 1);
		end
	elseif (e.recipe_id == 2183) then -- Pumpkin Shake
		if (eq.is_task_activity_active(8013, 3)) then -- The Hungry Halfling
			eq.update_task_activity(8013, 3, 1);
		end
	end
end

function event_command(e)
	return eq.DispatchCommands(e);
end

--[[ the main key is the ID of the AA
--   the first set is the age required in seconds
--   the second is if to ignore the age and grant anyways live test server style
--   the third is enabled
--]]
vet_aa = {
    [481]  = { 31536000, true, true}, -- Lesson of the Devote 1 yr
    [482]  = { 63072000, true, true}, -- Infusion of the Faithful 2 yr
    [483]  = { 94608000, true, true}, -- Chaotic Jester 3 yr
    [484]  = {126144000, true, true}, -- Expedient Recovery 4 yr
    [485]  = {157680000, true, true}, -- Steadfast Servant 5 yr
    [486]  = {189216000, true, true}, -- Staunch Recovery 6 yr
    [487]  = {220752000, true, true}, -- Intensity of the Resolute 7 yr
    [511]  = {252288000, true, true}, -- Throne of Heroes 8 yr
    [2000] = {283824000, true, true}, -- Armor of Experience 9 yr
    [8081] = {315360000, true, true}, -- Summon Resupply Agent 10 yr
    [8130] = {346896000, true, true}, -- Summon Clockwork Banker 11 yr
    [453]  = {378432000, true, true}, -- Summon Permutation Peddler 12 yr
    [182]  = {409968000, true, true}, -- Summon Personal Tribute Master 13 yr
    [600]  = {441504000, true, true}, -- Blessing of the Devoted 14 yr
}


-- Auto-grant casting/singing/utility + the PASSIVE combat skills whose skill_caps cap is already > 0 at
-- the player's current level. Combat ACTIVATED skills stay reward-gated (skill_pool.lua). The cap curve
-- is the level gate: Dual Wield (22) caps from level 1, Double Attack (20) from 32, Triple Attack (76)
-- from 58 -- so a skill is only granted once the player is high enough (MaxSkill > 0). Run on BOTH connect
-- (so a level-1 char has Dual Wield immediately, not only after their first ding) and level up.
local FREE_SKILLS = {
	4,5,13,14,18,24,          -- casting (Abjuration/Alteration/Channeling/Conjuration/Divination/Evocation)
	43,44,45,46,47,           -- casting specializations
	12,41,49,54,70,           -- singing + instruments (Bard)
	9,17,25,29,31,32,39,42,67,-- utility (Bind Wound/Disarm Traps/Feign Death/Hide/Meditate/Mend/Safe Fall/Sneak/Begging)
	0,1,2,3,36,28,7,51,       -- weapon proficiencies (1H/2H Blunt+Slash, 1H Pierce, H2H, Archery, Throwing)
	33,15,19,34,37,11,20,76,22,71 -- passive combat: Offense/Defense/Dodge/Parry/Riposte/Block/Double(20)+Triple(76) Attack/Dual Wield(22)/Intimidation
}
local function grant_free_skills(c)
	for _, v in ipairs(FREE_SKILLS) do
		if c:MaxSkill(v) > 0 and c:GetRawSkill(v) < 1 and c:CanHaveSkill(v) then
			c:SetSkill(v, 1)
		end
	end
end

function event_connect(e)
	loot_tracker.reset(e.self)   -- ALS: last session's loot list is stale (corpses gone) -- start empty
	-- Bard-only server: every character is a Bard (native spellbook + gems + mana + melee).
	-- Force-converts anyone not already a Bard; takes full effect on their next relog.
	if e.self:GetClass() ~= Class.BARD then
		e.self:SetBaseClass(Class.BARD)
		e.self:Message(MT.Yellow, "This world bends all souls to the Bard's path. Relog to complete your transformation.")
	end

	grant_veteran_aa(e)
	don.fix_invalid_faction_state(e.self)
	pok_travel.send_list(e.self, true)   -- warm the dll Portal window with discovered zones

	-- tell the client which combat skills are already earned (so the unlock hook reveals them)
	spell_choice.send_unlocks(e.self)

	-- hand over coin earned while the player's (permanent) shop sold items offline
	bazaar_broker.pay_escrow(e.self)

	grant_free_skills(e.self)            -- level-1 chars get Dual Wield etc. now, not only after first ding
	autobuff.sync(e.self)                -- apply permanent auras for already-memmed beneficial buffs/songs

	-- daily Hot Zone welcome: the midnight roll publishes the list to the GLOBAL bucket
	-- "aotv4_hotzone_list" (read live, never cached). Printed as clean per-line chat because the RoF2
	-- MOTD control won't render line breaks. Bucket format: longname|mult^longname|mult^...
	local hz = eq.get_data("aotv4_hotzone_list")
	if hz and hz ~= "" then
		e.self:Message(MT.LightBlue, "----- Welcome to AoTv4!  Today's Hot Zones (bonus EXP) -----")
		for entry in string.gmatch(hz, "([^%^]+)") do
			local name, mult = entry:match("^(.-)|(%d+)$")
			if name then
				e.self:Message(mult == "3" and MT.Lime or MT.Yellow,
					string.format("   %s  -  %sx EXP", name, mult))
			end
		end
	end
end

-- repeating "autobuff" timer (started in event_enter_zone): reconcile permanent buff auras with the bar
function event_timer(e)
	if e.timer == "autobuff" then
		autobuff.sync(e.self)
	end
end

function grant_veteran_aa(e)
	if not eq.is_dragons_of_norrath_enabled() then
		return
	end

    local age = e.self:GetAccountAge();
    for aa, v in pairs(vet_aa) do
        if v[3] and (v[2] or age >= v[1]) then
            e.self:GrantAlternateAdvancementAbility(aa, 1)
        end
    end
end

--[[
0  /*13855*/ Skill1HBlunt = 0,
1  /*13856*/ Skill1HSlashing,
2  /*13857*/ Skill2HBlunt,
3  /*13858*/ Skill2HSlashing,
4  /*13859*/ SkillAbjuration,
5  /*13861*/ SkillAlteration,
6  /*13862*/ SkillApplyPoison, X
7  /*13863*/ SkillArchery, X
8  /*13864*/ SkillBackstab,
9  /*13866*/ SkillBindWound,
10 /*13867*/ SkillBash,
11 /*13871*/ SkillBlock,
12 /*13872*/ SkillBrassInstruments,
13 /*13874*/ SkillChanneling,
14 /*13875*/ SkillConjuration,
15 /*13876*/ SkillDefense,
16 /*13877*/ SkillDisarm,
17 /*13878*/ SkillDisarmTraps, 
18 /*13879*/ SkillDivination,
19 /*13880*/ SkillDodge,
20 /*13881*/ SkillDoubleAttack,
21 /*13882*/ SkillDragonPunch,
21 /*13924*/ SkillTailRake = SkillDragonPunch, // Iksar Monk equivilent
22 /*13883*/ SkillDualWield,
23 /*13884*/ SkillEagleStrike,
24 /*13885*/ SkillEvocation,
25 /*13886*/ SkillFeignDeath,
26 /*13888*/ SkillFlyingKick,
27 /*13889*/ SkillForage, X
28 /*13890*/ SkillHandtoHand,
29 /*13891*/ SkillHide,
30 /*13893*/ SkillKick,
31 /*13894*/ SkillMeditate,
32 /*13895*/ SkillMend,
33 /*13896*/ SkillOffense,
34 /*13897*/ SkillParry,
35 /*13899*/ SkillPickLock, X
36 /*13900*/ Skill1HPiercing,        // Changed in RoF2(05-10-2013)
37 /*13903*/ SkillRiposte,
38 /*13904*/ SkillRoundKick,
39 /*13905*/ SkillSafeFall, 
40 /*13906*/ SkillSenseHeading, X
41 /*13908*/ SkillSinging,
42 /*13909*/ SkillSneak,
43 /*13910*/ SkillSpecializeAbjure,      // No idea why they truncated this one..especially when there are longer ones...
44 /*13911*/ SkillSpecializeAlteration,
45 /*13912*/ SkillSpecializeConjuration,
46 /*13913*/ SkillSpecializeDivination,
47 /*13914*/ SkillSpecializeEvocation,
48 /*13915*/ SkillPickPockets, X
49 /*13916*/ SkillStringedInstruments,
50 /*13917*/ SkillSwimming, X
51 /*13919*/ SkillThrowing,
52 /*13920*/ SkillTigerClaw,
53 /*13921*/ SkillTracking, X
54 /*13923*/ SkillWindInstruments,
55 /*13854*/ SkillFishing, X
56 /*13853*/ SkillMakePoison, X
57 /*13852*/ SkillTinkering, X
58 /*13851*/ SkillResearch, X
59 /*13850*/ SkillAlchemy, X
60 /*13865*/ SkillBaking, X
61 /*13918*/ SkillTailoring, X
62 /*13907*/ SkillSenseTraps, X
63 /*13870*/ SkillBlacksmithing, X
64 /*13887*/ SkillFletching, X
65 /*13873*/ SkillBrewing, X
66 /*13860*/ SkillAlcoholTolerance, X
67 /*13868*/ SkillBegging, 
68 /*13892*/ SkillJewelryMaking, X
69 /*13901*/ SkillPottery, X
70 /*13898*/ SkillPercussionInstruments,
71 /*13922*/ SkillIntimidation,
72 /*13869*/ SkillBerserking,
73 /*13902*/ SkillTaunt,
74 /*05837*/ SkillFrenzy,
75 /*03670*/  SkillRemoveTraps,  X
76 /*13049*/  SkillTripleAttack,
]]--

function event_level_up(e)
  -- Era level cap: hold the character at the current expansion's cap. Clamping returns early so we
  -- don't offer spells above the cap (the spell picker is already level-bounded, so the cap also
  -- gates the spell pool). At the cap, XP keeps flowing into the death-bank (held at the threshold).
  if era_system.clamp_level(e.self) then
    return
  end

  -- Auto-grant casting/singing/utility + passive combat skills newly unlocked at this level (their
  -- skill_caps cap just crossed 0 -- e.g. Double Attack @32, Triple Attack @58). Combat ACTIVATED
  -- skills stay reward-gated (skill_pool.lua). Same grant used on connect. See grant_free_skills above.
  grant_free_skills(e.self)

  if e.self:GetLevel() == 5 then
    eq.popup("", "<c \"#F0F000\">Welcome to level 5.</c><br><br>You have just been granted a new ability called '<c \"#F0F000\">Origin</c>' which allows you to teleport back to your starting city.<br><br>Open the Alternate Advancement window by pressing the '<c \"#F0F000\">V</c>' key, look in the '<c \"#F0F000\">General' tab</c>, and find the '<c \"#F0F000\">Origin</c>' ability and select it.<br><br>Now press the '<c \"#F0F000\">Hotkey</c>' button to create a hotkey you can place on your hot bar.");
  end

  if e.self:GetLevel() == 10 and eq.is_dragons_of_norrath_enabled() then
    eq.popup("", "<c \"#F0F000\">Welcome to level 10.</c><br><br>You are now able to begin the new player armor and weapon quests.  Speak with Castlen and Barrenzin or V`Lynn Renloe in the <c \"#66CCFF\">Plane of Knowledge</c> to begin.  One additional quest will become available to you at each level past level 10, so be sure to check back with these NPCs as you continue to gain experience.");
  end

  -- offer a choice of 3 level-appropriate spells to learn
  spell_choice.offer(e)
end

-- On death, bank ALL experience as Alternate Advancement and restart at level 1 (roguelite).
-- The random AA window then pops; banked AA scales with how far the run got.
function event_death(e)
  local client = e.self
  local death_level = client:GetLevel()   -- capture BEFORE the roguelite reset to level 1 below
  local run_xp      = client:GetEXP()     -- XP earned THIS run (effort); also captured pre-reset

  -- server-wide death announcement: who died, what killed them, at what level, and lifetime deaths.
  -- Death count persists in the per-character bucket "deaths_<charid>".
  local cid    = client:CharacterID()
  local deaths = (tonumber(eq.get_data("deaths_" .. cid)) or 0) + 1
  eq.set_data("deaths_" .. cid, tostring(deaths))
  local killer = (e.other and e.other:GetCleanName()) or "the world"
  local times  = deaths == 1 and "time" or "times"
  -- server-wide broadcast; MT.Yellow reliably renders in the main chat window (MT.Red can get filtered)
  eq.world_emote(MT.Yellow, string.format("%s has been slain by %s at level %d!  They have now died %d %s.",
    client:GetCleanName(), killer, death_level, deaths, times))

  -- ROGUELITE: every death banks AA (scaled by the level reached) and restarts the run at level 1.
  -- SetEXP takes (normal_exp, aa_exp) -- zero normal XP, keep AA exp.
  client:SetLevel(1)
  client:SetEXP(0, client:GetAAExp())

  -- XP-effort bank below the cap, with a big bonus for dying AT the cap (see the tuning block on top).
  -- No owned-AA taper: the reward is fixed by the level/XP you reached this run.
  local cap     = era_system.level_cap()
  local cap_exp = era_system.def(era_system.current()).cap_exp or 1
  local base
  if death_level >= cap then
    base = cap * DEATH_AA_AT_CAP                              -- reached the cap: big final-push reward
  else
    base = (run_xp / cap_exp) * (cap * DEATH_AA_SUB_CAP)     -- XP-effort ramp below the cap
  end
  -- CATCH-UP: +25% per era the server is ahead of this player's AA tier (0 in Classic; see era_system).
  local banked = math.floor(base * era_system.catchup_bonus(client))
  if banked >= 1 then                 -- (no farming: low-XP early deaths bank little/nothing)
    -- Bank into aa_choice's PRIVATE bucket (not the native unspent pool) so the random picker
    -- is the ONLY way to spend -- the native AA window shows 0 spendable points.
    aa_choice.grant_picks(e, banked)
  end

  -- ROGUELITE LOSS: destroy carried gear/inventory/money + wipe spells (bank + epics are safe),
  -- then report what was lost (chat + the "what you lost" dll window via LOSTDATA).
  local lost = death_loss.process(client)
  death_loss.announce(client, lost)
  spell_choice.clear_pending(client)  -- drop any un-picked offers so the new run starts clean
  spell_choice.send_unlocks(client)   -- re-hide the now-reset combat skills on the client

  -- RESET QUESTS: the roguelite wipe destroys quest items, which would otherwise strand a player
  -- mid-quest (e.g. a half-finished Tutorial task they can no longer hand in). Cancel every active
  -- task on death so the quest is cleared and can be re-taken fresh on the new run.
  client:CancelAllTasks()

  -- ALS: clear the loot window on death -- a dead run's loot must not stay claimable at level 1.
  loot_tracker.reset(client)

  -- Personal death recap: the dying player's own client can swallow the world_emote during its death
  -- sequence, so guarantee they see their own line here (after the reset settles).
  client:Message(MT.Yellow, string.format("You were slain by %s at level %d.  You have now died %d %s.",
    killer, death_level, deaths, times))

  -- NOTE: the STARTER WEAPON is granted in event_death_complete, NOT here. event_death fires BEFORE
  -- the death corpse is built (attack.cpp: EVENT_DEATH ~1833, `new Corpse` ~2065) -- anything summoned
  -- to the cursor here gets swept into that corpse and lost. death_complete fires after the corpse.
end

-- STARTER WEAPON: the wipe (death_loss) strips equipped weapons (epics excepted), so a fresh run would
-- otherwise be fists-only. Grant a basic weapon whenever the Primary slot ends up empty. Done in
-- death_complete (fires AFTER the corpse is built) so the summoned item lands on the cursor and stays.
function event_death_complete(e)
  local client = e.self
  -- GetItemIDAt returns INVALID_ID (-1), NOT 0, for an empty slot -- test `<= 0`, not `== 0`.
  local primary = client:GetItemIDAt(13)       -- slot 13 = Primary
  if not primary or primary <= 0 then
    client:SummonItem(5013)                    -- Rusty Short Sword (dmg 4 / dly 28, no level req, Bard-usable)
  end
end

function event_say(e)
  -- consume "spellpick <N>" (spell window) and "aapick <N>" (AA window) picks
  spell_choice.handle_say(e)
  aa_choice.handle_say(e)
  pok_travel.handle_say(e)        -- "portals" (list) + "portalgo <short>" (travel)
  bazaar_broker.handle_global_say(e)  -- vendor window: "vpset .../vshop/vclose"
  loot_tracker.handle_say(e)      -- ALS: "alspick <eid> loot|leave" + "alslootall"
end

-- ALS: when the Replace model is on, cancel native corpse looting so all loot flows through the
-- loot window. (Return 1 from event_loot cancels the native loot -- corpse.cpp Handle EVENT_LOOT.)
function event_loot(e)
  if loot_tracker.block_native(e) then
    return 1
  end
end

-- Discover a Plane of Knowledge portal by clicking that zone's PoK book (a door to poknowledge).
-- Return 1 to CANCEL the door's normal teleport -- the book only attunes/discovers; actual travel
-- is via the Portal window. (Handle_OP_ClickDoor skips HandleClick when the event returns non-zero.)
function event_click_door(e)
  if e.door and e.door:GetDestinationZoneName() == "poknowledge" then
    pok_travel.discover(e.self, eq.get_zone_short_name())  -- attune (first click)
    pok_travel.open(e.self)                                -- open the Portal window
    return 1
  end
end

test_items = {
    [Class.WARRIOR]			= {38000, 38020}, -- Warrior
    [Class.CLERIC]			= {38168, 38188}, -- Cleric
    [Class.PALADIN]			= {38084, 38104}, -- Paladin
    [Class.RANGER]			= {38105, 38125}, -- Ranger
    [Class.SHADOWKNIGHT]	= {38063, 38083}, -- Shadowknight
    [Class.DRUID]			= {38189, 38209}, -- Druid
    [Class.MONK]			= {38021, 38041}, -- Monk
    [Class.BARD]			= {38147, 38167}, -- Bard
    [Class.ROGUE]			= {38042, 38062}, -- Rogue
    [Class.SHAMAN]			= {38210, 38230}, -- Shaman
    [Class.NECROMANCER]		= {38294, 38314}, -- Necromancer
    [Class.WIZARD]			= {38231, 38251}, -- Wizard
    [Class.MAGICIAN]		= {38252, 38272}, -- Magician
    [Class.ENCHANTER]		= {38273, 38293}, -- Enchanter
    [Class.BEASTLORD]		= {38126, 38146}, -- Beastlord
    [Class.BERSERKER]		= {38315, 38332}, -- Berserker
}
 
function event_test_buff(e)
    if (e.self:GetLevel() < 25) then
        e.self:SetLevel(25)
        eq.scribe_spells(25,1)
        eq.train_discs(25,1)
        for class_id, v in pairs(test_items) do
            if e.self:GetClass() == class_id then
                for item_id = v[1], v[2] do
                    e.self:SummonItem(item_id);
                end
            end
        end
    end
end

function event_task_complete(e)
  don.on_task_complete(e.self, e.task_id)
end
