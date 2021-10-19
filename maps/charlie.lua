require("csa_base")
unpack()

mapsize = 10
illegal = {
	COORDS(C,4),
	COORDS(D,4),
	COORDS(E,4),
	COORDS(G,6),
	COORDS(G,7),
	COORDS(H,7),
}

render_table[1][2].res = 40
init_csa()