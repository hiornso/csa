require("csa_base")
unpack()

illegal = {
	COORDS(A,1),
}

micro_islands = {
	grid = {15,47},
	res = 2 * mapsize + 1,
	middle = false,
}

render_table[1].len = render_table[1].len + 1
table.insert(render_table[1], 2, micro_islands)

init_csa()