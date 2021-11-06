#include <gtk/gtk.h>
#include <string.h>

#include "main.h"
#include "tracker.h"
#include "csa_error.h"

#include <luajit.h>
#include <lauxlib.h>
#include <lualib.h>

#define SQUARE(x) ((x)*(x))
#define NONE 0

#define UI_INDENT_SIZE 15

void free_tracker(Tracker *t)
{
	if(t == NULL) return;
	
	if(t->surface_binding != 0 && GTK_IS_WIDGET(t->surface_button)){
		g_signal_handler_disconnect(t->surface_button, t->surface_binding);
	}
	
	lua_close(t->L);
	
	if(t->mrc.surface != NULL){
		cairo_surface_finish(t->mrc.surface);
		cairo_surface_destroy(t->mrc.surface);
	}
	
	csa_free(t->mrc.pixels);
	csa_free(t->mrc.vals);
	
	csa_free(t->mrc.map_pixels);
	csa_free(t->mrc.map_subpixels);
	
	csa_free(t);
}

static Tracker* new_tracker(char *lua_map_script, char mode)
{
	Tracker *t = csa_malloc(sizeof(Tracker));
	if(t == NULL){
		csa_error("failed to allocate memory for tracker.\n");
		return NULL;
	}
	
	t->surface_button = NULL;
	t->surface_binding = 0;
	
	t->L = luaL_newstate();
	
	if(t->L == NULL){
		csa_free(t);
		csa_error("failed to create new Lua state.\n");
		return NULL;
	}
	
	CHECK_LUA_STACK_INIT(t->L);
	
	luaL_openlibs(t->L);
	int status;
	if(mode == FROM_FILE){
		if((status = luaL_loadfile(t->L, lua_map_script))){
			csa_error("failed to load script from file '%s' (error code %i, %s)\n%s\n", 
				lua_map_script, status, 
				status == LUA_ERRSYNTAX ? "Syntax Error" :
				status == LUA_ERRMEM ? "Memory Allocation Error" :
				status == LUA_ERRFILE ? "Error reading file" :
				"Unknown error",
				lua_tostring(t->L, -1)
			);
			lua_close(t->L);
			csa_free(t);
			return NULL;
		}
	}else if(mode == FROM_STRING){
		if((status = luaL_loadstring(t->L, lua_map_script))){
			csa_error("failed to load script from string (error code %i, %s)\n%s\n", 
				status, 
				status == LUA_ERRSYNTAX ? "Syntax Error" :
				status == LUA_ERRMEM ? "Memory Allocation Error" :
				"Unknown error",
				lua_tostring(t->L, -1)
			);
			lua_close(t->L);
			csa_free(t);
			return NULL;
		}
	}else{
		csa_error("invalid script loading mode argument '%i'.\n", mode);
		lua_close(t->L);
		csa_free(t);
		return NULL;
	}
	// load base_csa into package.preload so it can be require'd if wanted
	GError *err = NULL;
	GBytes *map_gbytes = g_resources_lookup_data(RESOURCES_MAPS_CSA_BASE, G_RESOURCE_LOOKUP_FLAGS_NONE, &err);
	if(err == NULL){
		gsize s;
		gpointer map = g_bytes_unref_to_data(map_gbytes, &s);
		
		lua_getfield(t->L, LUA_GLOBALSINDEX, "package");
		lua_getfield(t->L, -1, "preload");
		status = luaL_loadstring(t->L, map);
		if(!status){
			lua_setfield(t->L, -2, "csa_base");
		}
		lua_pop(t->L, 2);
		
		g_free(map);
		
		if(status){
			csa_error("failed to load 'csa_base' from string (error code %i, %s)\n%s\n",
				status,
				status == LUA_ERRSYNTAX ? "Syntax Error" :
				status == LUA_ERRMEM ? "Memory Allocation Error" :
				"Unknown error",
				lua_tostring(t->L, -1)
			);
			lua_close(t->L);
			csa_free(t);
			return NULL;
		}
	}else{
		csa_error("failed to get csa_base map resource for new tracker: %s\n", err->message);
		g_error_free(err);
		lua_close(t->L);
		csa_free(t);
		return NULL;
	}
	// end loading csa_base
	if((status = lua_pcall(t->L, 0, 0, 0))){
		csa_error("failed during priming run of lua script (error code %i)\n%s\n", status, lua_tostring(t->L, -1));
		lua_close(t->L);
		csa_free(t);
		return NULL;
	}
	
	t->mrc.old_res = -1;
    
    t->mrc.clen = 0;
	t->mrc.rlen = 0;
	
	t->mrc.map_pixels = NULL;
	t->mrc.map_subpixels = NULL;
	
    t->mrc.pixels = NULL;
	
	t->mrc.vals = NULL;
	
	t->mrc.surface = NULL;
	
	if(refresh_map_cache(t)){
		free_tracker(t);
		return NULL;
	}
	
	t->funcID = 0;
	
	t->update_type = NO_UPDATE;
	
	CHECK_LUA_STACK_EXIT(t->L);

	return t;
}

static inline void refresh_drawing_areas(Omni *data, int is_captain)
{
	if(is_captain){
		gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_drawing_area));
		gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_self_tracking));
	}else{
		gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.radio_engineer_drawing_area));
	}
}

void reset_ui_action(__attribute__ ((unused)) GtkEventControllerMotion *gesture, gpointer user_data)
{
	Omni *data = (Omni*)user_data;
	if(data->captain_tracker != NULL){
		if(data->captain_tracker->funcID){
			lua_State *L = data->captain_tracker->L;
			CHECK_LUA_STACK_INIT(L);
			lua_getfield(L, LUA_GLOBALSINDEX, "reset_action");
			int status;
			if((status = lua_pcall(L, 0, 0, 0))){
				csa_error("failed in reset_action lua function for captain tracker (error code %i)\n%s\n", status, lua_tostring(L, -1));
				lua_pop(L, 1);
			}
			data->captain_tracker->funcID = 0;
			refresh_drawing_areas(data, TRUE);
			CHECK_LUA_STACK_EXIT(L);
		}
	}
	if(data->radio_engineer_tracker != NULL){
		if(data->radio_engineer_tracker->funcID){
			lua_State *L = data->radio_engineer_tracker->L;
			CHECK_LUA_STACK_INIT(L);
			lua_getfield(L, LUA_GLOBALSINDEX, "reset_action");
			int status;
			if((status = lua_pcall(L, 0, 0, 0))){
				csa_error("failed in reset_action lua function for radio engineer tracker (error code %i)\n%s\n", status, lua_tostring(L, -1));
				lua_pop(L, 1);
			}
			data->radio_engineer_tracker->funcID = 0;
			refresh_drawing_areas(data, FALSE);
			CHECK_LUA_STACK_EXIT(L);
		}
	}
}

void deconstruct_gridtable(Tracker *tracker, char *new_state, Point *moved_to)
{
	lua_State *L = tracker->L;
	CHECK_LUA_STACK_INIT(L);
	
	// BEGIN STEAL: https://stackoverflow.com/questions/6137684/iterate-through-lua-table
	lua_pushnil(L);
	// stack now contains: -1 => nil; -2 => table
	while(lua_next(L, -2)){
		// stack now contains: -1 => value; -2 => key; -3 => table
		// copy the key so that lua_tostring does not modify the original
		lua_pushvalue(L, -2);
		// stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
		int key   = lua_tointeger(L, -1);
		int value = lua_tointeger(L, -2);
		if(0 <= key && key < SQUARE(tracker->mrc.mapsize)){
			new_state[key] += value;
		}
		if(moved_to != NULL){
			*moved_to = (Point){key % tracker->mrc.mapsize, key / tracker->mrc.mapsize};
		}
		// pop value + copy of key, leaving original key
		lua_pop(L, 2);
		// stack now contains: -1 => key; -2 => table
	}
	// stack now contains: -1 => table (when lua_next returns 0 it pops the key
	// but does not push anything.)
	// Stack is now the same as it was on entry to this function
	// END STEAL
	CHECK_LUA_STACK_EXIT(L);
}

StateListNode* new_tracker_state(Tracker *tracker, StateListNode *s)
{
	char *new_state = csa_calloc(SQUARE(tracker->mrc.mapsize), sizeof(char));
	if(new_state == NULL) return NULL;
	StateListNode *new_sln = csa_malloc(sizeof(StateListNode));
	if(new_sln == NULL){
		csa_free(new_state);
		return NULL;
	}
	free_linked_states(s->next);
	s->next = new_sln;
	new_sln->prev = s;
	new_sln->next = NULL;
	new_sln->state = new_state;
	return new_sln;
}


static void action(Omni *data, double x, double y, int is_captain, enum tracker_update_type update_type)
{
	Tracker *tracker = is_captain ? data->captain_tracker : data->radio_engineer_tracker;
	char choosing_captain_starting_pos = is_captain && data->course_plotter.choosing_captain_starting_pos;
	
	if(tracker == NULL || (tracker->funcID == 0 && !choosing_captain_starting_pos)){
		return;
	}
	
	tracker->update_type = update_type;
	tracker->x = x;
	tracker->y = y;
	refresh_drawing_areas(data, is_captain);
}

void captain_action_motion(
	__attribute__ ((unused)) GtkEventControllerMotion *gesture,
	double x,
	double y,
	gpointer user_data
) {
	action((Omni*)user_data, x, y, TRUE, MOTION);
}

void radio_engineer_action_motion(
	__attribute__ ((unused)) GtkEventControllerMotion *gesture,
	double x,
	double y,
	gpointer user_data
) {
	action((Omni*)user_data, x, y, FALSE, MOTION);
}

void confirm_captain_action(
	__attribute__ ((unused)) GtkGestureClick *gesture,
	__attribute__ ((unused)) int n_press,
	double x,
	double y,
	gpointer user_data
) {
	action((Omni*)user_data, x, y, TRUE, CLICK);
}

void confirm_radio_engineer_action(
	__attribute__ ((unused)) GtkGestureClick *gesture,
	__attribute__ ((unused)) int n_press,
	double x,
	double y,
	gpointer user_data
) {
	action((Omni*)user_data, x, y, FALSE, CLICK);
}

static int execute_ui_action(gpointer user_data)
{
	OmniWithID *omni_with_ID = (OmniWithID*)user_data;
	Omni *data = omni_with_ID->data;
	lua_State *L = omni_with_ID->L;
	
	CHECK_LUA_STACK_INIT(L);
	
	int ID = omni_with_ID->ID;
	int is_captain;
	if(data->captain_tracker == NULL){
		is_captain = 0;
	}else{
		is_captain = L == data->captain_tracker->L;
	}
	lua_getfield(L, LUA_GLOBALSINDEX, "need_input");
	lua_pushinteger(L, ID);
	int status;
	if((status = lua_pcall(L, 1, 1, 0))){
		error_popup(data, "Error in the need_input lua function, aborting action (error code %i)",status);
		csa_error("failed in the need_input lua function, aborting action (error code %i)\n%s\n",status, lua_tostring(L, -1));
		lua_pop(L, 1);
		CHECK_LUA_STACK_EXIT(L);
		return 1;
	}
	if(lua_toboolean(L, -1) && !(ID == data->course_plotter.surface_action_id && is_captain)){ // if it needs input and it's not surfacing in the captain tracker (since in that case the input is inferred from position)
		if(is_captain){
			data->captain_tracker->funcID = ID;
		}else{
			data->radio_engineer_tracker->funcID = ID;
		}
	}else{ // if it needs no input, it should be triggered immediately rather than on click
		Tracker *tracker = is_captain ? data->captain_tracker : data->radio_engineer_tracker;
		ChangeListNode *cln = csa_malloc(sizeof(ChangeListNode));
		StateListNode *s;
		char *old_state;
		StateListNode *new_sln;
		if(cln != NULL){
			s = is_captain ? data->widget_changes.captain_tracker : data->widget_changes.radio_engineer_tracker;
			old_state = (char*)s->state;
			new_sln = new_tracker_state(tracker, s);
		}
		if(cln == NULL || new_sln == NULL){
			error_popup(data, "Error: failed to allocate memory for new tracker state for %s action\n", is_captain ? "captain" : "radio engineer");
			csa_error("failed to allocate memory for new tracker state for %s tracker %p\n", is_captain ? "captain" : "radio engineer", (void*)tracker);
			lua_pop(L, 1);
			csa_free(cln);
			CHECK_LUA_STACK_EXIT(L);
			return 1;
		}
		char *new_state = (char*)new_sln->state;
		if(is_captain){
			data->widget_changes.captain_tracker = s->next;
		}else{
			data->widget_changes.radio_engineer_tracker = s->next; 
		}
		*cln = (ChangeListNode){NULL, data->changelist, is_captain ? CAPTAIN_TAB : RADIO_ENGINEER_TAB}; 
		if(data->changelist != NULL){ 
			free_linked_changes(data->changelist->next); 
			data->changelist->next = cln; 
		} 
		data->changelist = cln;
		for(int i = 0; i < SQUARE(tracker->mrc.mapsize); ++i){
			Point p = (Point){i % tracker->mrc.mapsize, i / tracker->mrc.mapsize};
			
			lua_getfield(L, LUA_GLOBALSINDEX, "action_funcs");
			lua_pushinteger(L, ID);
			lua_gettable(L, -2);
			lua_pushinteger(L, p.col);
			lua_pushinteger(L, p.row);
			if(ID == data->course_plotter.surface_action_id && is_captain){ // if we are surfacing as captain, infer position to surface at from position history
				int32_t *currp = ((int32_t*)data->course_plotter.position_history->state) + 1 + 2*(*(int32_t*)data->course_plotter.position_history->state - 1);
				lua_pushinteger(L, currp[0]);
				lua_pushinteger(L, currp[1]);
			}
			lua_pushinteger(L, old_state[i]);
			
			int status;
			const int num_args = (ID == data->course_plotter.surface_action_id && is_captain) ? 5 : 3;
			if((status = lua_pcall(L, num_args, 1, 0))){
				csa_error("failed in action_funcs[%i] lua function for point (%i,%i) while running action for %s tracker (error code %i)\n%s\n", ID, p.col, p.row, is_captain ? "captain" : "radio engineer", status, lua_tostring(L, -1));
			}else if(!lua_istable(L, -1)){
				csa_warning("map code did not return a table for point (%i,%i) while running action for %s tracker, so this point is being ignored.\n", p.col, p.row, is_captain ? "captain" : "radio engineer");
			}else{
				deconstruct_gridtable(tracker, new_state, NULL);
			}
			lua_pop(L, 2);
		}
		
		refresh_drawing_areas(data, is_captain);
	}
	lua_pop(L, 1);
	CHECK_LUA_STACK_EXIT(L);
	return 1;
}

static void free_user_data(gpointer data, __attribute__ ((unused)) GClosure *closure)
{
	csa_free(data);
}

static void free_user_data_swf(gpointer data, __attribute__ ((unused)) GClosure *closure)
{
	StateWithField *swf = data;
	csa_free(swf->name);
	csa_free(data);
}

static int construct_action_ui_depthcount(Omni *data, lua_State *L, GtkWidget *box, int depth, int target_depth, int construct, int *num_btns, OmniWithID **preallocated)
{
	CHECK_LUA_STACK_INIT(L);
	int nd = 0;
	int i = 0;
	while(1){
		lua_pushinteger(L, ++i);
		lua_gettable(L, -2);
		lua_pushinteger(L, ++i);
		lua_gettable(L, -3);
		if(lua_type(L, -2) == LUA_TSTRING){
			if(lua_type(L, -1) == LUA_TNUMBER){
				if(construct){
					GtkWidget *button = gtk_button_new_with_label(lua_tostring(L, -2));
					OmniWithID *data_with_id = preallocated[*num_btns];
					data_with_id->ID = lua_tointeger(L, -1);
					if(data_with_id->ID == 0){
						csa_warning("refusing to connect callback to button '%s' with action ID 0\n", lua_tostring(L, -2));
						csa_free(data_with_id);
					}else{
						lua_getfield(L, LUA_GLOBALSINDEX, "action_funcs");
						lua_pushinteger(L, data_with_id->ID);
						lua_gettable(L, -2);
						int valid_func = !lua_isnil(L, -1);
						lua_pop(L, 2);
						if(valid_func){
							// g_signal_connect_swapped(button, "clicked", G_CALLBACK(execute_ui_action), data_with_id);
							// g_signal_connect_swapped(data->gui_elems.window, "destroy", G_CALLBACK(free_user_data), data_with_id);
							// DONE: the above two lines can be replaced by the single line below after this bug has been fixed: https://discourse.gnome.org/t/bug-in-gtk4-widgets-inside-gtkexpanders-do-not-receive-destroy-signals/7371/2
							// if you are getting memory leaks on your system from this, you can comment the below line and uncomment the first 2 g_signal_connects to make it stop leaking, or you can just build a fixed version of gtk4
							g_signal_connect_data(button, "clicked", G_CALLBACK(execute_ui_action), data_with_id, free_user_data, G_CONNECT_SWAPPED);
						}else{
							csa_warning("refusing to connect callback to button '%s' since the corresponding entry in the lua action_funcs table is nil.\n", lua_tostring(L, -2));
							csa_free(data_with_id);
						}
					}
					gtk_widget_set_margin_start(button, UI_INDENT_SIZE * (target_depth - depth));
					gtk_widget_set_margin_end  (button, UI_INDENT_SIZE * (target_depth - depth));
					gtk_box_append(GTK_BOX(box), button);
				}
				++(*num_btns);
			}else if(lua_type(L, -1) == LUA_TTABLE){
				GtkWidget *expander = NULL;
				GtkWidget *newbox = NULL;
				if(construct){
					expander = gtk_expander_new(lua_tostring(L, -2));
					newbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); // TODO: fix spacing?
					gtk_widget_set_margin_start(newbox, UI_INDENT_SIZE);
					gtk_widget_set_margin_end(newbox, UI_INDENT_SIZE);
					gtk_expander_set_child(GTK_EXPANDER(expander), newbox);
				}
				int d = construct_action_ui_depthcount(data, L, newbox, depth + 1, target_depth, construct, num_btns, preallocated);
				nd = MAX(nd,d);
				if(construct){
					gtk_box_append(GTK_BOX(box), expander);
				}
			}else{
				break;
			}
			lua_pop(L, 2);
		}else{
			break;
		}
	}
	lua_pop(L, 2);
	CHECK_LUA_STACK_EXIT(L);
	return nd + 1;
}

static int construct_action_ui(Omni *data, lua_State *L, GtkWidget *box)
{
	int num_btns = 0;
	int depth = construct_action_ui_depthcount(data, L, box, 0, 0, FALSE, &num_btns, NULL) - 1;
	OmniWithID **preallocated = csa_malloc(sizeof(OmniWithID*) * num_btns);
	if(preallocated == NULL) return -1;
	for(int i = 0; i < num_btns; ++i){
		preallocated[i] = csa_malloc(sizeof(OmniWithID)); // will be freed via signal later
		if(preallocated[i] == NULL){
			for(int j = 0; j < i; ++j){
				csa_free(preallocated[j]);
			}
			csa_free(preallocated);
			return -1;
		}
		*(preallocated[i]) = (OmniWithID){data, L, 0};
	}
	num_btns = 0;
	construct_action_ui_depthcount(data, L, box, 0, depth, TRUE, &num_btns, preallocated);
	csa_free(preallocated);
	return 0;
}

static int update_ui_input_value_spin_button(GtkSpinButton *spin_button, gpointer user_data)
{
	StateWithField *swf = user_data;
	
	lua_State *L = swf->L;
	CHECK_LUA_STACK_INIT(L);
	char *name = swf->name;
	lua_getfield(L, LUA_GLOBALSINDEX, "user_input");
	lua_pushnumber(L, gtk_spin_button_get_value(spin_button));
	lua_setfield(L, -2, name);
	lua_pop(L, 1);
	
	CHECK_LUA_STACK_EXIT(L);
	return 1;
}

static int update_ui_input_value_combo_box_text(GtkComboBox *combobox, gpointer user_data)
{
	StateWithField *swf = user_data;
	
	lua_State *L = swf->L;
	CHECK_LUA_STACK_INIT(L);
	char *name = swf->name;
	lua_getfield(L, LUA_GLOBALSINDEX, "user_input");
	char *value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox));
	lua_pushstring(L, value);
	g_free(value);
	lua_setfield(L, -2, name);
	lua_pop(L, 1);
	
	CHECK_LUA_STACK_EXIT(L);
	return 1;
}

/*
static void empty_box(GtkWidget *widget)
{
	GtkWidget *w = NULL;
	while((w = gtk_widget_get_first_child(widget)) != NULL){
		gtk_box_remove(GTK_BOX(widget), w);
	}
}
//*/

static int construct_ui(Omni *data, lua_State *L, int is_captain)
{
	CHECK_LUA_STACK_INIT(L);
	GtkWidget *ui_box = is_captain ? data->gui_elems.captain_map_ui_box : data->gui_elems.radio_engineer_map_ui_box;
	lua_getfield(L, LUA_GLOBALSINDEX, "user_input");
	if(lua_isnil(L, -1)){
		csa_warning("lua global 'user_input' has not been initialised, initialising to default value of {}\n");
		lua_newtable(L);
		lua_setfield(L, LUA_GLOBALSINDEX, "user_input");
	}
	lua_pop(L, 1);
	lua_getfield(L, LUA_GLOBALSINDEX, "getgui");
	lua_pushboolean(L, is_captain);
	int status;
	if((status = lua_pcall(L, 1, 1, 0))){
		csa_error("the getgui lua function failed to generate the UI of the %s screen/could not be used to construct the UI (error code %i)\n%s\n",
		        (is_captain ? "Captain" : "Radio Engineer"), status, lua_tostring(L, -1));
		CHECK_LUA_STACK_EXIT(L);
		return -1;
	}else{
		if(lua_istable(L, -1)){
			// handle data inputs
			lua_pushinteger(L, 1);
			lua_gettable(L, -2);
			if(lua_istable(L, -1)){
				lua_pushnil(L);
				while(lua_next(L, -2)){
					if(lua_istable(L, -1)){
						lua_pushinteger(L, 2);
						lua_gettable(L, -2);
						const char * const ui_type = lua_tostring(L, -1);
						GtkWidget *widget = NULL;
						if(strcmp(ui_type, "GtkSpinButton") == 0){
							double v[5];
							
#define GET_LUA_NUMBER(INTO, STATE, INDEX, FIELD) \
	lua_pushinteger((STATE), (FIELD)); \
	lua_gettable((STATE), (INDEX) - 1); \
	INTO = lua_tonumber((STATE), -1); \
	lua_pop((STATE), 1);
							
							GET_LUA_NUMBER(v[0], L, -2, 3);
							GET_LUA_NUMBER(v[1], L, -2, 4);
							GET_LUA_NUMBER(v[2], L, -2, 5);
							GET_LUA_NUMBER(v[3], L, -2, 6);
							GET_LUA_NUMBER(v[4], L, -2, 7);
							
							widget = gtk_spin_button_new(
								gtk_adjustment_new(v[0],v[1],v[2],v[3],v[4],0.0),
								0.0,
								0
							);
							
							lua_pushinteger(L, 1);
							lua_gettable(L, -3);
							
							lua_getfield(L, LUA_GLOBALSINDEX, "user_input");
							lua_pushvalue(L, -2);
							lua_pushnumber(L, v[0]);
							lua_settable(L, -3);
							lua_pop(L, 1);
							
							StateWithField *swf = csa_malloc(sizeof(StateWithField));
							char *name = NULL;
							const char *internal_name = lua_tostring(L, -1);
							if(swf != NULL){
								name = csa_malloc(strlen(internal_name) + 1);
							}
							if(name != NULL){
								strcpy(name, internal_name);
							}else{
								csa_free(swf);
							}
							if(swf == NULL || name == NULL){
								csa_error("failed to allocate memory for UI input callback for '%s' input. This input will be non-functional.\n", internal_name);
							}else{
								*swf = (StateWithField){L, name};
								g_signal_connect_data(widget, "value-changed", G_CALLBACK(update_ui_input_value_spin_button), swf, free_user_data_swf, NONE);
							}
							lua_pop(L, 1);
							
						}else if(strcmp(ui_type, "GtkComboBoxText") == 0){
							
							lua_pushinteger(L, 3);
							lua_gettable(L, -3);
							
							if(lua_istable(L, -1)){
								widget = gtk_combo_box_text_new();
								
								lua_pushnil(L);
								while(lua_next(L, -2)){
									gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), lua_tostring(L, -1));
									lua_pop(L, 1);
								}
								
								lua_pushinteger(L, 4);
								lua_gettable(L, -4);
								gtk_combo_box_set_active(GTK_COMBO_BOX(widget), lua_tointeger(L, -1));
								lua_pop(L, 1);
								
								lua_pushinteger(L, 1);
								lua_gettable(L, -4);
								
								lua_getfield(L, LUA_GLOBALSINDEX, "user_input");
								lua_pushvalue(L, -2);
								char *starting_value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
								lua_pushstring(L, starting_value);
								g_free(starting_value);
								lua_settable(L, -3);
								lua_pop(L, 1);
								
								StateWithField *swf = csa_malloc(sizeof(StateWithField));
								char *name = NULL;
								const char *internal_name = lua_tostring(L, -1);
								if(swf != NULL){
									name = csa_malloc(strlen(internal_name) + 1);
								}
								if(name != NULL){
									strcpy(name, internal_name);
								}else{
									csa_free(swf);
								}
								if(swf == NULL || name == NULL){
									csa_error("failed to allocate memory for UI input callback for '%s' input. This input will be non-functional.\n", internal_name);
								}else{
									*swf = (StateWithField){L, name};
									g_signal_connect_data(widget, "changed", G_CALLBACK(update_ui_input_value_combo_box_text), swf, free_user_data_swf, NONE);
								}
								lua_pop(L, 1);
							}else{
								csa_warning("GtkComboBoxText UI entry component does not have table at index 3, skipping UI component.\n");
							}
							lua_pop(L, 1);
						}else{
							csa_warning("ignoring entry with unrecognised type in input UI constructor table.\n");
						}
						
						lua_pop(L, 1);
						
						if(widget != NULL){
							GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
							lua_pushinteger(L, 1);
							lua_gettable(L, -2);
							GtkWidget *label = gtk_label_new(lua_tostring(L, -1));
							gtk_widget_set_hexpand(label, TRUE);
							gtk_widget_set_halign(label, GTK_ALIGN_START);
							gtk_widget_set_margin_start(label, 5);
							gtk_box_append(GTK_BOX(box), label);
							lua_pop(L, 1);
							gtk_box_append(GTK_BOX(box), widget);
							gtk_widget_set_hexpand(box, FALSE);
							gtk_box_append(GTK_BOX(ui_box), box);
						}
					}else{
						csa_warning("an element of the user input UI constructor table is not a table, discarding this element.");
					}
					lua_pop(L, 1);
				}
			}else{
				csa_warning("element at index 1 in returned table from getgui is not a table. User input UI cannot be constructed.\n");
			}
			lua_pop(L, 1);
			// handle action/button inputs
			lua_pushinteger(L, 2);
			lua_gettable(L, -2);
			if(lua_istable(L, -1)){
				if(construct_action_ui(data, L, ui_box)){
					csa_error("failed to allocate memory for button-ID associations, so the action UI could not be constructed.\n");
				}
			}else{
				csa_warning("element at index 2 in returned table from getgui is not a table. User action UI cannot be constructed.\n");
			}
			lua_pop(L, 1);
		}else{
			csa_warning("return value from getgui is not a table (cannot construct input UI).\n");
		}
	}
	lua_pop(L, 1);
	
	CHECK_LUA_STACK_EXIT(L);
	return 0;
}

int init_tracker(Omni *data, char *lua_map_script_filename, char mode, char is_captain)
{
	Tracker *tracker = new_tracker(lua_map_script_filename, mode);
	if(tracker == NULL){
		return -1;
	}
	
	CHECK_LUA_STACK_INIT(tracker->L);
	
	tracker->surface_button = is_captain ? data->gui_elems.captain_surface_button : data->gui_elems.radio_engineer_surface_button;
	
	lua_State *L = tracker->L;
	OmniWithID *data_with_id = csa_malloc(sizeof(OmniWithID));
	if(data_with_id == NULL){
		csa_error("failed to allocate memory to bind Surface button. Aborting map loading.\n");
		free_tracker(tracker);
		return -1;
	}
	lua_getfield(L, LUA_GLOBALSINDEX, "surface_action_index");
	*data_with_id = (OmniWithID){
		data,
		L,
		lua_tointeger(L, -1)
	};
	lua_pop(L, 1);
	lua_getfield(L, LUA_GLOBALSINDEX, "action_funcs");
	if(lua_isnil(L, -1)){
		csa_error("lua does not have defined action_funcs table. Aborting map loading.\n");
		csa_free(data_with_id);
		free_tracker(tracker);
		return -1;
	}
	if(data_with_id->ID == 0){
		csa_warning("refusing to connect callback to button 'Surface' with action ID 0\n");
		csa_free(data_with_id);
	}else{
		lua_pushinteger(L, data_with_id->ID);
		lua_gettable(L, -2);
		int valid_func = !lua_isnil(L, -1);
		lua_pop(L, 2);
		if(valid_func){
			tracker->surface_binding = g_signal_connect_data(tracker->surface_button, "clicked", G_CALLBACK(execute_ui_action), data_with_id, free_user_data, G_CONNECT_SWAPPED);
		}else{
			csa_warning("refusing to connect callback to button 'Surface' since the corresponding entry in the lua action_funcs table is nil.\n");
			csa_free(data_with_id);
		}
	}
	
	StateListNode *tracker_state = csa_malloc(sizeof(StateListNode));
	if(tracker_state == NULL){
		csa_error("failed to allocate memory for initial %s map StateListNode.\n", is_captain ? "captain" : "radio engineer");
		free_tracker(tracker);
		return -1;
	}
	char *tracker_map;
	*tracker_state = (StateListNode){
		NULL,
		NULL,
		tracker_map = csa_malloc(sizeof(char) * SQUARE(tracker->mrc.mapsize))
	};
	if(tracker_map == NULL){
		csa_error("failed to allocate memory for initial %s map state.\n", is_captain ? "captain" : "radio engineer");
		free_tracker(tracker);
		csa_free(tracker_state);
		return -1;
	}
	for(int i = 0; i < SQUARE(tracker->mrc.mapsize); ++i){
		tracker_map[i] = 1 - (char)tracker->mrc.map_pixels[i];
	}
	
	if(construct_ui(data, tracker->L, is_captain)){
		free_tracker(tracker);
		csa_free(tracker_map);
		csa_free(tracker_state);
		return -1;
	}
	
	// if all went well, assign trackers into Omni struct
	data->course_plotter.surface_action_id = data_with_id->ID;
	if(is_captain){
		data->widget_changes.captain_tracker = tracker_state;
		data->captain_tracker = tracker;
	}else{
		data->widget_changes.radio_engineer_tracker = tracker_state;
		data->radio_engineer_tracker = tracker;
		// if radio engineer, inputs can be made sensitive immediately. For the captain, it will be made sensitive after a starting location has been chosen
		gtk_widget_set_sensitive(data->gui_elems.radio_engineer_map_builtin_button_grid, TRUE);
	}
	
	CHECK_LUA_STACK_EXIT(L); // don't need to check any other exit paths since they all guarantee that the lua object is closed anyway
	
	return 0;
}

static int move(Omni *data, char is_captain, StateListNode *s, int direction, Point *moved_to)
{
	Tracker *tracker = is_captain ? data->captain_tracker : data->radio_engineer_tracker; 
	
	if(tracker == NULL || s == NULL) return -2;
	StateListNode *new_sln = new_tracker_state(tracker, s);
	if(new_sln == NULL) return -1;
	char *new_state = (char*)new_sln->state;
	char *old_state = (char*)s->state;
	
	lua_State *L = tracker->L;
	CHECK_LUA_STACK_INIT(L);
	
	int32_t *currp;
	if(moved_to != NULL){
		currp = ((int32_t*)data->course_plotter.position_history->state) + 1 + 2*(*(int32_t*)data->course_plotter.position_history->state - 1);
	}
	
	for(int i = 0; i < SQUARE(tracker->mrc.mapsize); ++i){
		Point p = (Point){i % tracker->mrc.mapsize, i / tracker->mrc.mapsize};
		
		lua_getfield(L, LUA_GLOBALSINDEX, "move");
		lua_pushinteger(L, p.col);
		lua_pushinteger(L, p.row);
		lua_pushinteger(L, direction);
		lua_pushinteger(L, old_state[i]);
		
		int status;
		if((status = lua_pcall(L, 4, 1, 0))){
			csa_error("failed in the move lua function for point (%i,%i) (error code %i)\n%s\n", p.col, p.row, status, lua_tostring(L, -1));
		}else if(!lua_istable(L, -1)){
			csa_warning("map code did not return a table for point (%i,%i), so it is being ignored.\n", p.col, p.row);
		}else{
			deconstruct_gridtable(tracker, new_state,
				moved_to == NULL ? NULL : (p.col == currp[0] && p.row == currp[1]) ? moved_to : NULL
			);
		}
		lua_pop(L, 1);
	}
	
	CHECK_LUA_STACK_EXIT(L);
	return 0;
}

static int handle_move(
	__attribute__ ((unused)) GtkButton *button,
	gpointer user_data,
	int is_captain,
	enum direction dir
) {
	Omni *data = user_data; 
	
	StateListNode *new_couse_plotter_sln;
	if(is_captain){
		new_couse_plotter_sln = csa_malloc(sizeof(StateListNode));
		if(new_couse_plotter_sln == NULL){
			error_popup(data, "Error: failed to allocate memory for %s move.", is_captain ? "captain" : "radio engineer"); 
			csa_error("failed to allocate memory for StateListNode for CoursePlotter update for %s move.\n", is_captain ? "captain" : "radio engineer"); 
			return 1;
		}
		*new_couse_plotter_sln = (StateListNode){NULL, data->course_plotter.position_history, csa_malloc(sizeof(int32_t) + sizeof(Point))};
		if(new_couse_plotter_sln->state == NULL){
			error_popup(data, "Error: failed to allocate memory for %s move.", is_captain ? "captain" : "radio engineer"); 
			csa_error("failed to allocate memory for StateListNode state for CoursePlotter update for %s move.\n", is_captain ? "captain" : "radio engineer"); 
			return 1;
		}else{
			*(int32_t*)new_couse_plotter_sln->state = 1;
		}
	}
	StateListNode *s = is_captain ? data->widget_changes.captain_tracker : data->widget_changes.radio_engineer_tracker; 
	ChangeListNode *cln = csa_malloc(sizeof(ChangeListNode)); 
	if(cln == NULL){ 
		error_popup(data, "Error: failed to allocate memory for %s move.", is_captain ? "captain" : "radio engineer"); 
		csa_error("failed to allocate memory for ChangeListNode for %s move.\n", is_captain ? "captain" : "radio engineer"); 
		return 1;
	} 
	
	int status = move(data, is_captain, s, dir, is_captain ? (Point*)(((int32_t*)new_couse_plotter_sln->state) + 1) : NULL); 
	
	if(!status){ 
		if(is_captain){
			free_linked_states(data->course_plotter.position_history->next);
			data->course_plotter.position_history->next = new_couse_plotter_sln;
			data->course_plotter.position_history = new_couse_plotter_sln;
			
			data->widget_changes.captain_tracker = s->next;
		}else{
			data->widget_changes.radio_engineer_tracker = s->next; 
		}
		*cln = (ChangeListNode){NULL, data->changelist, is_captain ? (CAPTAIN_TAB | COURSE_PLOTTER) : RADIO_ENGINEER_TAB}; 
		if(data->changelist != NULL){ 
			free_linked_changes(data->changelist->next); 
			data->changelist->next = cln; 
		} 
		data->changelist = cln; 
	}else{ 
		if(status == -1){
			Tracker *t = is_captain ? data->captain_tracker : data->radio_engineer_tracker; 
			error_popup(data, "Error: failed to allocate memory for %s move in C move function with tracker %p", is_captain ? "captain" : "radio engineer", t); 
			csa_error("failed to allocate memory for StateListNode for %s move with tracker %p\n", is_captain ? "captain" : "radio engineer", (void*)t); 
		}else{
			error_popup(data, "Error: invalid tracker or tracker state.");
			csa_error("tracker or tracker StateListNode was NULL\n");
		}
		csa_free(cln);
		if(is_captain){
			csa_free(new_couse_plotter_sln->state);
			csa_free(new_couse_plotter_sln);
		}
	} 
	
	refresh_drawing_areas(data, is_captain);
	
	return 1;
}

#define HANDLE_MOVE_CAPTAIN(DIR) \
int handle_move_captain_##DIR(GtkButton *button, gpointer user_data) \
{ \
	return handle_move(button, user_data, TRUE, DIR);\
}
HANDLE_MOVE_CAPTAIN(NORTH)
HANDLE_MOVE_CAPTAIN(EAST)
HANDLE_MOVE_CAPTAIN(SOUTH)
HANDLE_MOVE_CAPTAIN(WEST)

#define HANDLE_MOVE_RADIO_ENGINEER(DIR) \
int handle_move_radio_engineer_##DIR(GtkButton *button, gpointer user_data) \
{ \
	return handle_move(button, user_data, FALSE, DIR);\
}
HANDLE_MOVE_RADIO_ENGINEER(NORTH)
HANDLE_MOVE_RADIO_ENGINEER(EAST)
HANDLE_MOVE_RADIO_ENGINEER(SOUTH)
HANDLE_MOVE_RADIO_ENGINEER(WEST)

static void file_select_callback(GtkNativeDialog *native, int response, gpointer user_data, int is_captain)
{
	Omni *data = user_data;
	
	GtkWidget *dropdown = is_captain ? data->gui_elems.captain_map_dropdown : data->gui_elems.radio_engineer_map_dropdown;
	
	g_signal_handlers_block_by_func(GTK_COMBO_BOX(dropdown), is_captain ? (void*)captain_dropdown_fix : (void*)radio_engineer_dropdown_fix, user_data);
	
	if(response == GTK_RESPONSE_ACCEPT){
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
		GFile *file = gtk_file_chooser_get_file(chooser);

		char *filename = g_file_get_path(file);
		
		g_object_unref(file);
		
		if(!init_tracker(data, filename, FROM_FILE, is_captain)){
			gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(dropdown), GTK_SENSITIVITY_OFF);
			gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(dropdown), crop_to_filename(filename));
			gtk_combo_box_set_active(GTK_COMBO_BOX(dropdown), 0);
			refresh_drawing_areas(data, is_captain);
		}else{
			gtk_combo_box_set_active(GTK_COMBO_BOX(dropdown), -1);
			error_popup(data, "Failed to load map.");
		}
		
		g_free(filename);
	}else{
		gtk_combo_box_set_active(GTK_COMBO_BOX(dropdown), -1);
	}
	
	g_signal_handlers_unblock_by_func(GTK_COMBO_BOX(dropdown), is_captain ? (void*)captain_dropdown_fix : (void*)radio_engineer_dropdown_fix, user_data);

	g_object_unref(native);
}
static void file_select_callback_captain(GtkNativeDialog *native, int response, gpointer user_data)
{
	file_select_callback(native, response, user_data, TRUE);
}
static void file_select_callback_radio_engineer(GtkNativeDialog *native, int response, gpointer user_data)
{
	file_select_callback(native, response, user_data, FALSE);
}

static int dropdown_fix(GtkComboBoxText *box, gpointer user_data, int is_captain)
{
	Omni *data = user_data;
	
	char *mapname = gtk_combo_box_text_get_active_text(box);
	if(mapname != NULL){
		if(strcmp(mapname, "Open From File") == 0){
			GtkFileChooserNative *native = gtk_file_chooser_native_new (
				"Open File",
				(GtkWindow*)data->gui_elems.window,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				"_Open",
				"_Cancel"
			);
			g_signal_connect(native, "response", G_CALLBACK(is_captain ? file_select_callback_captain : file_select_callback_radio_engineer), data);
			gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(native), TRUE);
			gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
		}else{
			char prefix[] = RESOURCES_MAPS_BUILTINS;
			char *path = csa_malloc(strlen(prefix) + strlen(mapname) + 1);
			if(path == NULL){
				csa_error("failed to allocate memory to construct path to map resource for map '%s' for %s tracker.\n", mapname, is_captain ? "captain" : "radio engineer");
				error_popup(data, "Failed to allocate memory to construct path to map resource.");
			}else{
				strcpy(path, prefix);
				strcat(path, mapname);
				GError *err = NULL;
				GBytes *map_gbytes = g_resources_lookup_data(path, G_RESOURCE_LOOKUP_FLAGS_NONE, &err);
				if(err == NULL){
					gsize s;
					gpointer map = g_bytes_unref_to_data(map_gbytes, &s);
					if(!init_tracker(data, (char*)map, FROM_STRING, is_captain)){
						gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(box), GTK_SENSITIVITY_OFF);
						refresh_drawing_areas(data, is_captain);
					}else{
						g_signal_handlers_block_by_func(box, is_captain ? (void*)captain_dropdown_fix : (void*)radio_engineer_dropdown_fix, user_data);
						gtk_combo_box_set_active(GTK_COMBO_BOX(box), -1);
						g_signal_handlers_unblock_by_func(box, is_captain ? (void*)captain_dropdown_fix : (void*)radio_engineer_dropdown_fix, user_data);
						error_popup(data, "Failed to load static map.");
					}
					g_free(map);
				}else{
					csa_error("failed to get map resource for %s tracker: %s\n", is_captain ? "captain" : "radio engineer", err->message);
					g_error_free(err);
					error_popup(data, "Failed to load map.");
				}
			}
			csa_free(path);
		}
	}
	g_free(mapname);
	
	return 1;
}
int captain_dropdown_fix(GtkComboBoxText *box, gpointer user_data)
{
	return dropdown_fix(box, user_data, TRUE);
}
int radio_engineer_dropdown_fix(GtkComboBoxText *box, gpointer user_data)
{
	return dropdown_fix(box, user_data, FALSE);
}
