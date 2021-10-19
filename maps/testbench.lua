require("csa_base")
unpack()

illegal = {
	COORDS(C,2),
	COORDS(G,2),
	COORDS(M,2),
	COORDS(N,2),
	COORDS(C,3),
	COORDS(I,3),
	COORDS(M,3),
	COORDS(I,4),
	COORDS(B,7),
	COORDS(D,7),
	COORDS(G,7),
	COORDS(I,7),
	COORDS(B,8),
	COORDS(D,8),
	COORDS(G,8),
	COORDS(D,9),
	COORDS(H,9),
	COORDS(L,9),
	COORDS(M,9),
	COORDS(N,9),
	COORDS(D,11),
	COORDS(C,12),
	COORDS(H,12),
	COORDS(L,12),
	COORDS(A,13),
	COORDS(M,13),
	COORDS(C,14),
	COORDS(G,14),
	COORDS(I,14),
	COORDS(N,14),
	COORDS(D,15),
}

render_table = { -- table of layers
	layers = 3,
	{
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
			grid = {COORDS(A, 1)},
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
	},
	{ -- table of objects in the island layer
		colours = { -- can also be called colors (yuck)
			type = STEPPED,
			transitions = {
				{at = 350.0, r =  50.0, g = 166.0, b = 0.0},
				{at = 330.0, r =  50.0, g = 150.0, b = 0.0},
				{at = 285.0, r =  50.0, g = 133.0, b = 0.0},
				{at = 230.0, r =  50.0, g = 116.0, b = 0.0},
				{at = 215.0, r = 200.0, g = 200.0, b = 0.0},
			},
			wraparound = 370.0,
		},
		len = 3,
		{ -- islands
			grid = illegal,
			res = mapsize,
			middle = true,
			-- multiplier not used when grid is initialised from table
			-- offset not used when grid is initialised from table
		},
		{
			grid = {15,47},
			res = 2 * mapsize + 1,
			middle = false,
		},
		{ -- island noise
			grid = 0, -- table for init, an integer for seeded random noise
			res = 60,
			middle = true,
			multiplier = 0.33, -- defaults to 0.0
			offset = 0.33, -- defaults to 0.0
		},
	},
	{ -- table of objects in the ocean layer
		colours = {
			type = CONTINUOUS,
			mr = 0.0,
			cr = 0.0,
			mg = 0.0,
			cg = 0.0,
			mb = 1.0,
			cb = 0.0,
		},
		len = 1,
		{ -- sea noise
			grid = nil,
			res = 5,
			middle = true,
			multiplier = 0.3,
			offset = 0.6,
		},
	},
}

action_funcs = {}

init_csa(2)

function getgui(is_captain)
	return {}
end
