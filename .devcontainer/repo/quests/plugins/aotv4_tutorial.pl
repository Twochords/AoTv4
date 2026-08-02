#::: AoTv4: Tutorial exit routing.
#::: The stock Tutorial dumps players in the Plane of Knowledge (expansion 4), which strands brand-new
#::: characters on this Classic-locked server. plugin::AoTTutorialExit($client) sends them to a valid,
#::: faction-safe home city instead -- called from both the exit door (tutorialb/player.pl) and Arias
#::: ("leave"/"escape"). Routing is by the character's assigned START ZONE (which already encodes the
#::: race+deity good/evil split); if that's unset or points at a non-Classic zone we fall back to the
#::: race's home city, and finally to West Freeport as a universal Classic-safe backstop.
#:::
#::: dest = [zone_id, x, y, z]. Non-Classic race homes are remapped: Iksar (Cabilis/Kunark) -> Grobb,
#::: Vah Shir (Shar Vahl/Luclin) -> Halas, Froglok (Gukta/LoY) -> Rivervale.

sub AoTTutorialExit {
	my $client = shift;
	return 0 unless $client;

	# Primary: by assigned start zone (race + deity precise). surefall->qeynos, cabeast->grobb,
	# sharvahl->halas, and the paired sub-cities (kaladimb->a, felwitheb->a, erudnint->erudnext).
	my %by_startzone = (
		1  => [1,  0,     10,    5],      3   => [1,  0,     10,    5],      # qeynos (surefall 3 -> qeynos)
		2  => [2,  -74,   428,   3],                                         # qeynos2
		19 => [19, -98.4, 11.5,  3.1],                                       # rivervale
		23 => [24, -309.8,109.6, 23.1],   24  => [24, -309.8,109.6, 23.1],   # erudnext (erudnint 23 -> erudnext)
		25 => [25, -965.3,2434.5,5.6],                                       # nektulos
		29 => [29, 12.2,  -32.9, 3.1],    155 => [29, 12.2,  -32.9, 3.1],    # halas (sharvahl 155 -> halas)
		40 => [40, 156.9, -2.9,  31.1],                                      # neriaka
		41 => [41, -499,  2.9,   -10.9],                                     # neriakb
		42 => [42, -968.9,891.9, -52.8],                                     # neriakc
		45 => [45, -343,  189,   -38.22],                                    # qcat
		49 => [49, 520.1, 235.4, 59.1],                                      # oggok
		50 => [50, 560,   -2234, 3],                                         # rathemtn
		52 => [52, 1.1,   14.5,  3.1],    106 => [52, 1.1,   14.5,  3.1],    # grobb (cabeast 106 -> grobb)
		54 => [54, -197,  27,    -0.7],                                      # gfaydark
		55 => [55, 7.6,   489.0, -24.9],                                     # akanon
		60 => [60, -2,    -18,   3],      67  => [60, -2,    -18,   3],      # kaladima (kaladimb 67 -> a)
		61 => [61, 26.3,  14.9,  3.1],    62  => [61, 26.3,  14.9,  3.1],    # felwithea (felwitheb 62 -> a)
		68 => [68, -214.5,2940.1,0.1],                                       # butcher
		75 => [75, 200,   800,   3.39],                                      # paineel
	);

	# Fallback: by RACE, for when start zone is unset/invalid (0, Freeport 8/10/382/383, Crescent 394...).
	my %by_race = (
		1   => [1,  0,     10,    5],      # Human     -> Qeynos
		2   => [29, 12.2,  -32.9, 3.1],    # Barbarian -> Halas
		3   => [24, -309.8,109.6, 23.1],   # Erudite   -> Erudin (erudnext)
		4   => [54, -197,  27,    -0.7],   # Wood Elf  -> Greater Faydark (Kelethin)
		5   => [61, 26.3,  14.9,  3.1],    # High Elf  -> Felwithe
		6   => [40, 156.9, -2.9,  31.1],   # Dark Elf  -> Neriak
		7   => [1,  0,     10,    5],      # Half Elf  -> Qeynos
		8   => [60, -2,    -18,   3],      # Dwarf     -> Kaladim
		9   => [52, 1.1,   14.5,  3.1],    # Troll     -> Grobb
		10  => [49, 520.1, 235.4, 59.1],   # Ogre      -> Oggok
		11  => [19, -98.4, 11.5,  3.1],    # Halfling  -> Rivervale
		12  => [55, 7.6,   489.0, -24.9],  # Gnome     -> Ak'Anon
		128 => [52, 1.1,   14.5,  3.1],    # Iksar     -> Grobb    (Cabilis is Kunark)
		130 => [29, 12.2,  -32.9, 3.1],    # Vah Shir  -> Halas    (Shar Vahl is Luclin)
		330 => [19, -98.4, 11.5,  3.1],    # Froglok   -> Rivervale (Gukta is post-Classic)
	);

	my $dest = $by_startzone{$client->GetStartZone()}
		|| $by_race{$client->GetBaseRace()}
		|| [9, -60.9, -61.5, -24.9];       # West Freeport -- universal Classic-safe backstop

	quest::movepc($dest->[0], $dest->[1], $dest->[2], $dest->[3]);
	return 1;
}

return 1;
