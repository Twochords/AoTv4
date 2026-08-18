-- Titan Hall induction, step 4. All logic lives in the module; this is just the hail.
local tutorial = require("aotv4_tutorial")

function event_say(e)
	if string.find(string.lower(e.message), "hail") then
		tutorial.hail(e, 4)
	end
end
