#ifndef CPT_SONAR_ASSIST_TRACKING_H
#define CPT_SONAR_ASSIST_TRACKING_H

#include <inttypes.h>

#include "main.h"

#include <gtk/gtk.h>
#include <luajit.h>

#ifndef CHECK_LUA_STACK
#define CHECK_LUA_STACK 1
#endif

#define COORDS(x,y) ((Point){(x), (y) - 1})

#define RESOURCES_MAPS_CSA_BASE "/maps/csa_base"
#define RESOURCES_MAPS_BUILTINS "/maps/builtins/"

enum column {A,B,C,D,E, F,G,H,I,J, K,L,M,N,O};
enum direction { NORTH, EAST, SOUTH, WEST };

typedef struct point {
	int32_t col, row;
} Point; // cannot change due to representation of data->course_plotter.position_history->state

typedef struct fpoint {
	double col, row;
} FPoint;

typedef struct colour {
	float r,g,b;
} Colour;

typedef struct colour_transition {
	float threshold;
	Colour c;
} ColourTransition;

enum map_layer_colour_mapping_type {
	CONTINUOUS,
	STEPPED,
};

typedef struct map_layer_colour_mapping {
	char mapping_type;
	union {
		struct {
			float mr, cr, mg, cg, mb, cb;
		} continuous;
		struct {
			float wraparound;
			int len;
			ColourTransition *transitions;
		} stepped;
	} mappings;
} MapLayerColourMapping;

typedef struct map_render_cache {
    int old_res;
    
    int mapsize;
	int clen;
	int rlen;
	char colfmt[4];
	
	float *map_pixels;
	float *map_subpixels;
	
    unsigned char *pixels;
	
	float *vals;
	
	cairo_surface_t *surface;
    
} MapRenderCache;

enum tracker_update_type {
	NO_UPDATE,
	MOTION,
	CLICK,
};

typedef struct tracker {
	MapRenderCache mrc;
	lua_State *L;
	int funcID;
	enum tracker_update_type update_type;
	double x,y;
	gpointer surface_button;
	gulong surface_binding;
} Tracker;

enum load_mode {
	FROM_FILE,
	FROM_STRING
};

typedef struct omni_with_ID {
	Omni *data;
	lua_State *L;
	int ID;
} OmniWithID;

typedef struct state_with_field {
	lua_State *L;
	char *name;
} StateWithField;

#if CHECK_LUA_STACK
#define CHECK_LUA_STACK_INIT(L) const int INITIAL_STACK_SIZE = lua_gettop((L));
#define CHECK_LUA_STACK_EXIT(L) {const int FINAL_STACK_SIZE = lua_gettop((L)); if(FINAL_STACK_SIZE != INITIAL_STACK_SIZE){csa_error("lua stack size different at function exit (%i) from at function entry (%i).\n", FINAL_STACK_SIZE, INITIAL_STACK_SIZE);}}
#else
#define CHECK_LUA_STACK_INIT(L) do{}while(0);
#define CHECK_LUA_STACK_EXIT(L) if(0){}
#endif

// maprender.c
int refresh_map_cache(Tracker *tracker);
void draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);

//tracker.c
void free_tracker(Tracker *t);

void reset_ui_action(GtkEventControllerMotion *gesture, gpointer user_data);
void deconstruct_gridtable(Tracker *tracker, char *new_state, Point *moved_to);
StateListNode* new_tracker_state(Tracker *tracker, StateListNode *s);

void captain_action_motion(GtkEventControllerMotion *gesture, double x, double y, gpointer user_data);
void radio_engineer_action_motion(GtkEventControllerMotion *gesture, double x, double y, gpointer user_data);
void confirm_captain_action(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);
void confirm_radio_engineer_action(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

int init_tracker(Omni *data, const char *lua_map_script_filename, const char mode, const char is_captain);

int handle_move_captain_NORTH(GtkButton *button, gpointer user_data);
int handle_move_captain_EAST (GtkButton *button, gpointer user_data);
int handle_move_captain_SOUTH(GtkButton *button, gpointer user_data);
int handle_move_captain_WEST (GtkButton *button, gpointer user_data);

int handle_move_radio_engineer_NORTH(GtkButton *button, gpointer user_data);
int handle_move_radio_engineer_EAST (GtkButton *button, gpointer user_data);
int handle_move_radio_engineer_SOUTH(GtkButton *button, gpointer user_data);
int handle_move_radio_engineer_WEST (GtkButton *button, gpointer user_data);

#endif
