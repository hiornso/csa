function unpack()
	mapsize = 15
	A,B,C,D,E, F,G,H,I,J, K,L,M,N,O = 0,1,2,3,4, 5,6,7,8,9, 10,11,12,13,14
	NORTH,EAST,SOUTH,WEST = 0,1,2,3
	CONTINUOUS,STEPPED = 0,1
	SECTOR_SIZE = 5

	user_input = {}

	illegal_as_keys = {}

	distlim = 4
	distcache = {}

	RECEIVE, LAUNCH = true, false

	surface_action_index = 17

	action_funcs = {
		[ 1] = launch_torpedo_captain,
		[ 2] = receive_torpedo_captain,
		[ 6] = drone,
		[ 7] = sonar,
		
		[ 9] = asgard_launch_captain,
		[10] = asgard_receive_captain,
		
		[14] = kraken_captain,
		
		[16] = enemy_silence,
		[17] = surface,
		[18] = launch_torpedo_radio_engineer,
		[19] = receive_torpedo_radio_engineer,
		[20] = kraken_radio_engineer,
		[21] = asgard_launch_radio_engineer,
		[22] = asgard_receive_radio_engineer,
	}
	
	render_table = { -- table of layers
		layers = 2,
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
			len = 2,
			{ -- islands
				grid = illegal,
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
end
unpack()

function INDEX(col, row)
	if col < 0 or col >= mapsize or row < 0 or row >= mapsize then
		return -1
	end
	return row * mapsize + col
end
function COORDS(col, row)
	return INDEX(col, row - 1)
end
function SECTOR(col, row)
	return SECTOR_SIZE * math.floor(row / SECTOR_SIZE) + math.floor(col / SECTOR_SIZE)
end
function MAX(a, b)
	if (a > b) then
		return a
	end
	return b
end
function MIN(a, b)
	if (a < b) then
		return a
	end
	return b
end
function CLAMP(n, a, b)
	return MAX(a, MIN(n, b))
end
function ABS(x)
	if (x < 0) then
		return -x
	end
	return x
end

function init_csa(land_layer, island_coords_object)
	if (land_layer == nil) then land_layer = 1 end
	if (island_coords_object == nil) then island_coords_object = 1 end
	
	for key, pos in ipairs(illegal) do
		illegal_as_keys[pos] = 1
	end
	render_table[land_layer][island_coords_object].grid = illegal
	render_table[land_layer][island_coords_object].res = mapsize
end

function need_refresh()
	return false
end

function move(col, row, direction, number)
	if (direction == NORTH) then
		row = row - 1
	elseif (direction == EAST) then
		col = col + 1
	elseif (direction == SOUTH) then
		row = row + 1
	elseif (direction == WEST) then
		col = col - 1
	else
		return {}
	end
	if (illegal_as_keys[INDEX(col, row)]) then
		return {}
	end
	return {[INDEX(col, row)] = number}
end

function expand_distcache(col, row, dist)
	if (dist > distlim) then
		return
	end
	for k,v in pairs({{0,1},{0,-1},{-1,0},{1,0}}) do -- for each of surrounding 4
		x = v[1]
		y = v[2]
		if (not ((col + x < 0) or (col + x > mapsize) or (row + y < 0) or (row + y > mapsize))) then -- not outside the map
			local ind = INDEX(col + x, row + y)
			if (not illegal_as_keys[ind]) then -- not illegal
				if ((distcache[ind] == nil) or (distcache[ind] > dist + 1)) then -- not already found a shorter path to it
					distcache[ind] = dist + 1
					expand_distcache(col + x, row + y, dist + 1)
				end
			end
		end
	end
end
function regenerate_distcache(col, row)
	distcache = {}
	distcache.ind = INDEX(col, row)
	distcache[distcache.ind] = 0
	expand_distcache(col, row, 0)
end
function distance(mouseCol, mouseRow, col, row)
	if (not (distcache.ind == INDEX(mouseCol, mouseRow))) then
		regenerate_distcache(mouseCol, mouseRow)
	end
	local ind = INDEX(col, row)
	if (distcache[ind]) then
		return distcache[ind]
	else
		return distlim + 1
	end
end

function launch_torpedo(col, row, mouseCol, mouseRow, number, key)
	mouseCol = CLAMP(mouseCol, 0, mapsize - 1)
	mouseRow = CLAMP(mouseRow, 0, mapsize - 1)
	local damage = user_input[key]
	if (damage == 2) then
		if ((col == mouseCol) and (row == mouseRow)) then
			return {[INDEX(col, row)] = number}
		end
	elseif (damage == 1) then
		if (((col - 1 <= mouseCol) and (mouseCol <= col + 1)) and
		    ((row - 1 <= mouseRow) and (mouseRow <= row + 1)) and
			(not ((col == mouseCol) and (row == mouseRow))))
		then
			return {[INDEX(col, row)] = number}
		end
	else -- damage == 0
		if ((distance(mouseCol, mouseRow, col, row) <= 4) and not
		    (((col - 1 <= mouseCol) and (mouseCol <= col + 1)) and
		    ((row - 1 <= mouseRow) and (mouseRow <= row + 1))))
		then
			return {[INDEX(col, row)] = number}
		end
	end
	return {}
end
function launch_torpedo_captain(col, row, mouseCol, mouseRow, number)
	return launch_torpedo(col, row, mouseCol, mouseRow, number, "Damage taken (Player)")
end
function launch_torpedo_radio_engineer(col, row, mouseCol, mouseRow, number)
	return launch_torpedo(col, row, mouseCol, mouseRow, number, "Damage taken (Enemy)")
end

function receive_torpedo(col, row, mouseCol, mouseRow, number, key)
	mouseCol = CLAMP(mouseCol, 0, mapsize - 1)
	mouseRow = CLAMP(mouseRow, 0, mapsize - 1)
	local damage = user_input[key]
	if (damage == 2) then
		if ((col == mouseCol) and (row == mouseRow)) then
			return {[INDEX(col, row)] = number}
		end
	elseif (damage == 1) then
		if (((col - 1 <= mouseCol) and (mouseCol <= col + 1)) and
		    ((row - 1 <= mouseRow) and (mouseRow <= row + 1)) and
			(not ((col == mouseCol) and (row == mouseRow))))
		then
			return {[INDEX(col, row)] = number}
		end
	else
		if (not (((col - 1 <= mouseCol) and (mouseCol <= col + 1)) and
		    ((row - 1 <= mouseRow) and (mouseRow <= row + 1))))
		then
			return {[INDEX(col, row)] = number}
		end
	end
	return {}
end
function receive_torpedo_captain(col, row, mouseCol, mouseRow, number)
	return receive_torpedo(col, row, mouseCol, mouseRow, number, "Damage taken (Player)")
end
function receive_torpedo_radio_engineer(col, row, mouseCol, mouseRow, number)
	return receive_torpedo(col, row, mouseCol, mouseRow, number, "Damage taken (Enemy)")
end

function kraken(col, row, mouseCol, mouseRow, number, key)
	local damage = user_input[key]
	if (damage == 2) then
		return {}
	elseif (damage == 1) then
		if ((col == mouseCol) and (row == mouseRow)) then
			return {[INDEX(col, row)] = number}
		end
		return {}
	else
		if ((col == mouseCol) and (row == mouseRow)) then
			return {}
		end
		return {[INDEX(col, row)] = number}
	end
end
function kraken_captain(col, row, mouseCol, mouseRow, number)
	return kraken(col, row, mouseCol, mouseRow, number, "Damage taken (Player)")
end
function kraken_radio_engineer(col, row, mouseCol, mouseRow, number)
	return kraken(col, row, mouseCol, mouseRow, number, "Damage taken (Enemy)")
end

function surface(col, row, mouseCol, mouseRow, number)
	if (SECTOR(col, row) == SECTOR(mouseCol, mouseRow)) then
		return {[INDEX(col, row)] = number}
	end
	return {}
end

function enemy_silence(col, row, number)
	local t = {[INDEX(col, row)] = number}
	for k,v in pairs({{0,1},{0,-1},{-1,0},{1,0}}) do
		local x = v[1]
		local y = v[2]
		for i=1,4 do
			if (not illegal_as_keys[INDEX(col + x * i, row + y * i)]) then
				t[INDEX(col + x * i, row + y * i)] = number
			else
				break
			end
		end
	end
	return t
end

function drone(col, row, mouseCol, mouseRow, number)
	local drone_correct = user_input["Drone guess correct"] == "Yes"
	if (drone_correct) then
		if (SECTOR(col, row) == SECTOR(mouseCol, mouseRow)) then
			return {[INDEX(col, row)] = number}
		end
	else
		if (SECTOR(col, row) ~= SECTOR(mouseCol, mouseRow)) then
			return {[INDEX(col, row)] = number}
		end
	end
	return {}
end

function sel_func(c)
	if (c[1] ~= CLAMP(c[1], 0, mapsize - 1)) then
		c[2] = CLAMP(c[2], 0, mapsize - 1)
		return function (col, row)
			return row == c[2]
		end
	elseif (c[2] ~= CLAMP(c[2], 0, mapsize - 1)) then
		return function (col, row)
			return col == c[1]
		end
	else
		return function (col, row)
			return SECTOR(c[1], c[2]) == SECTOR(col, row) 
		end
	end
end
function sonar(col, row, mouseCol, mouseRow, number)
	local f1, f2
	if (intermediate_data == nil) then
		f1 = function (c, r) return false end
	else
		local c = intermediate_data[1]
		f1 = sel_func(c)
	end
	f2 = sel_func({mouseCol, mouseRow})
	local r1 = f1(col, row)
	local r2 = f2(col, row)
	if ((r1 or r2) and not (r1 and r2)) then
		return {[INDEX(col, row)] = number}
	end
	return {}
end

function asgard(col, row, mouseCol, mouseRow, number, key, receiving)
	local damage = user_input[key]
	local hit = (
		((col - 1 <= mouseCol) and (mouseCol <= col + 1)) and
		((row - 1 <= mouseRow) and (mouseRow <= row + 1))
	)
	if (damage == 2) then
		if (hit) then
			if (receiving or (
				((col == mouseCol) or (row == mouseRow))
			)) then
				return {[INDEX(col, row)] = number}
			end
		end
	elseif (damage == 0) then
		if (not hit) then
			--[[
			cond = (
				((ABS(col - mouseCol) + ABS(row - mouseRow)) <= 6) and
				((col == mouseCol) or (row == mouseRow))
			)
			--]]
			
			cond = false
			for k,v in pairs({{0,1},{0,-1},{-1,0},{1,0}}) do
				local x = v[1]
				local y = v[2]
				for i=1,6 do
					if (not illegal_as_keys[INDEX(mouseCol + x * i, mouseRow + y * i)]) then
						if ((mouseCol + x * i == col) and (mouseRow + y * i == row)) then
							cond = true
							break
						end
					else
						break
					end
				end
				if (cond) then
					break
				end
			end
			
			if (receiving or cond) then
				return {[INDEX(col, row)] = number}
			end
		end
	end
	return {}
end
function asgard_launch_captain(col, row, mouseCol, mouseRow, number)
	return asgard(col, row, mouseCol, mouseRow, number, "Damage taken (Player)", LAUNCH)
end
function asgard_launch_radio_engineer(col, row, mouseCol, mouseRow, number)
	return asgard(col, row, mouseCol, mouseRow, number, "Damage taken (Enemy)", LAUNCH)
end
function asgard_receive_captain(col, row, mouseCol, mouseRow, number)
	return asgard(col, row, mouseCol, mouseRow, number, "Damage taken (Player)", RECEIVE)
end
function asgard_receive_radio_engineer(col, row, mouseCol, mouseRow, number)
	return asgard(col, row, mouseCol, mouseRow, number, "Damage taken (Enemy)", RECEIVE)
end

function should_terminate(ind)
	if (ind ~= 7) then
		return true
	end
	return intermediate_data ~= nil
end

function need_input(ind)
	return ind ~= 16 -- don't need input for Radio Engineer silence
end

function add_intermediate(x, y)
	if (intermediate_data == nil) then
		intermediate_data = {}
	end
	table.insert(intermediate_data, {x,y})
end

function reset_action()
	intermediate_data = nil
end

function getgui(is_captain)
	local interactive, ui
	if (is_captain) then
		ui = {
			{"Damage taken (Player)","GtkSpinButton",0,0,2,1,2}, -- start, min, max, step, page
			{"Drone guess correct" ,"GtkComboBoxText",{"No","Yes"},0}, -- {options}, starting value index (0-indexed from beginning of table, ignores table keys)
		}
		interactive = {
			"Torpedo",{
				"Torpedo launched (Player)",1,
				"Torpedo launched (Enemy)",2,
			},
			"Mine",{
				"Mine placed (Player)",3,
				"Mine detonated (Player)",4,
				"Mine detonated (Enemy)",5,
			},
			"Drone",{
				"Drone launched (Enemy)",6,
			},
			"Sonar",{
				"Sonar activated (Enemy)",7,
			},
			"Silence",{
				"Silence activated (Player)",8,
			},
			"Special",{
				"ASGARD Super-Cavitation Torpedo",{
					"Torpedo launched (Player)",9,
					"Torpedo launched (Enemy)",10,
				},
				"KAOS Polarising Mine",{
					"Mine placed (Player)",11,
					"Mine detonated (Player)",12,
					"Mine detonated (Enemy)",13,
				},
				"Kraken Missile",{
					"Missile launched (Enemy)",14,
				},
			},
		}
	else -- radio engineer
		ui = {
			{"Damage taken (Enemy)","GtkSpinButton",0,0,2,1,2}, -- start, min, max, step, page
			{"Drone guess correct" ,"GtkComboBoxText",{"No","Yes"},0}, -- {options}, starting value index (0-indexed from beginning of table, ignores table keys)
		}
		interactive = {
			"Torpedo",{
				"Torpedo launched (Enemy)",18,
				"Torpedo launched (Player)",19,
			},
			"Mine",{
				"Mine placed (Enemy)",3,
				"Mine detonated (Enemy)",4,
				"Mine detonated (Player)",5,
			},
			"Drone",{
				"Drone launched (Player)",6,
			},
			"Sonar",{
				"Sonar activated (Player)",7,
			},
			"Silence",{
				"Silence activated (Enemy)",16,
			},
			"Special",{
				"ASGARD Super-Cavitation Torpedo",{
					"Torpedo launched (Enemy)",21,
					"Torpedo launched (Player)",22,
				},
				"KAOS Polarising Mine",{
					"Mine placed (Enemy)",11,
					"Mine detonated (Enemy)",12,
					"Mine detonated (Player)",13,
				},
				"Kraken Missile",{
					"Missile launched (Player)",20,
				},
			},
		}
	end
	return {
		ui,
		interactive,
	}
end