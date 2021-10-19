require("csa_base")
unpack()

illegal = {
	COORDS(A, 2),
}

ice = {
	COORDS(A, 1),
}

ice_layer = { -- ice layer
	colours = {
		type = STEPPED,
		transitions = {
			{at = 340.0, r = 210.0, g = 224.0, b = 247.0},
			{at = 330.0, r = 174.0, g = 201.0, b = 245.0},
			{at = 285.0, r = 158.0, g = 195.0, b = 255.0},
			{at = 230.0, r = 137.0, g = 176.0, b = 240.0},
			{at = 215.0, r = 111.0, g = 156.0, b = 227.0},
		},
		wraparound = 999.0,
	},
	len = 2,
	{ -- islands
		grid = ice,
		res = mapsize,
		middle = true,
		-- multiplier not used when grid is initialised from table
		-- offset not used when grid is initialised from table
	},
	{ -- island noise
		grid = 0, -- table for init, an integer for seeded random noise
		res = 60,
		middle = true,
		multiplier = 0.33, -- defaults to 0.0
		offset = 0.33, -- defaults to 0.0
	},
}
render_table.layers = render_table.layers + 1
table.insert(render_table, 1, ice_layer)

init_csa(2) -- since we shoved the land layer down one by putting ice on top of it, it's now layer 2