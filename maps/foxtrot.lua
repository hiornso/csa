require("csa_base")
unpack()

mapsize = 10
illegal = {
	COORDS(B,3),
	COORDS(B,4),
	COORDS(B,5),
	COORDS(H,2),
	COORDS(I,2),
	COORDS(D,2),
	COORDS(D,3),
	COORDS(D,4),
	COORDS(E,4),
	COORDS(F,4),
	COORDS(G,4),
	COORDS(H,4),
	COORDS(H,5),
	COORDS(G,6),
	COORDS(H,6),
	COORDS(I,6),
	COORDS(G,8),
	COORDS(H,8),
	COORDS(I,8),
	COORDS(B,7),
	COORDS(B,8),
	COORDS(C,9),
	COORDS(D,8),
	COORDS(E,8),
	COORDS(E,9),

}

render_table[1][2].res = 40
init_csa()