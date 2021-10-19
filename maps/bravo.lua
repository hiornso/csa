require("csa_base")
unpack()

mapsize = 10
illegal = {
	COORDS(D,1),
	COORDS(H,2),
	COORDS(C,4),
	COORDS(H,4),
	COORDS(G,7),
	COORDS(D,9),
}

render_table[1][2].res = 40
init_csa()