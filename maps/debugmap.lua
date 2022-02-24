require("csa_base")
unpack()

illegal = {}

render_table = { -- table of layers
	layers = 1,
	{ -- table of objects in the ocean layer
		colours = {
			type = CONTINUOUS,
			mr = 1.0,
			cr = 0.0,
			mg = 1.0,
			cg = 0.0,
			mb = 1.0,
			cb = 0.0,
		},
		len = 1,
		{ -- sea noise
			grid = nil,
			res = 5,
			middle = true,
			multiplier = 0.8,
			offset = 0.1,
		},
	},
}

action_funcs = {}

function getgui(is_captain)
	return {}
end
