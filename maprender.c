#include <gtk/gtk.h>
#include <string.h>

#include "main.h"
#include "tracker.h"
#include "accelerated.h"
#include "csa_error.h"

#define SQUARE(x) ((x)*(x))

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NO_CACHE_MAP (0)
#define TEXTSIZE (15)
#define DOT_SIZE (0.04) // multiple of <res/mapsize>
#define CROSS_SIZE (0.18) // multiple of <res/mapsize>
#define COURSE_PLOTTER_LINE_THICKNESS (0.05) // multiple of <res/mapsize>

int refresh_map_cache(Tracker *tracker)
{
	CHECK_LUA_STACK_INIT(tracker->L);
	
	lua_getfield(tracker->L, LUA_GLOBALSINDEX, "mapsize");
	tracker->mrc.mapsize = lua_tointeger(tracker->L, -1);
	lua_pop(tracker->L, 1);
	if(tracker->mrc.mapsize == 0){
		csa_error("mapsize has invalid value (cannot refresh map cache)\n");
		CHECK_LUA_STACK_EXIT(tracker->L);
		return -1;
	}
	
	tracker->mrc.rlen = 0; // row string len
	tracker->mrc.clen = 0; // col string len
	int copy_s;
	
	copy_s = tracker->mrc.mapsize;
	do {
		++(tracker->mrc.rlen);
		copy_s /= 10;
	} while(copy_s);
	copy_s = tracker->mrc.mapsize;
	do {
		++(tracker->mrc.clen);
		copy_s /= 26;
	} while(copy_s);
	
	snprintf(tracker->mrc.colfmt, 4, "%%%ii", tracker->mrc.rlen);
	
	csa_free(tracker->mrc.map_pixels);
	tracker->mrc.map_pixels = csa_calloc(SQUARE(tracker->mrc.mapsize), sizeof(float));
	if(tracker->mrc.map_pixels == NULL){
		csa_error("failed to allocate memory for map_pixels (cannot refresh map cache)\n");
		CHECK_LUA_STACK_EXIT(tracker->L);
		return -1;
	}
	
	lua_getfield(tracker->L, LUA_GLOBALSINDEX, "illegal");
	if(lua_istable(tracker->L, -1)){
		lua_pushnil(tracker->L);
		while(lua_next(tracker->L, -2)){
			lua_pushvalue(tracker->L, -2);
			int value = lua_tointeger(tracker->L, -2);
			if(0 <= value && value < SQUARE(tracker->mrc.mapsize)){
				tracker->mrc.map_pixels[value] = 1.f;
			}else{
				csa_warning("lua global 'illegal' has coords outside map\n");
			}
			lua_pop(tracker->L, 2);
		}
	}else{
		csa_error("lua global 'illegal' is not a table (cannot refresh map cache)\n");
		lua_pop(tracker->L, 1);
		CHECK_LUA_STACK_EXIT(tracker->L);
		return -1;
	}
	lua_pop(tracker->L, 1);
	
	
	csa_free(tracker->mrc.map_subpixels);
	tracker->mrc.map_subpixels = csa_calloc(SQUARE(2 * tracker->mrc.mapsize + 1), sizeof(float));
	if(tracker->mrc.map_subpixels == NULL){
		csa_error("failed to allocate memory for map_subpixels (cannot refresh map cache)\n");
		CHECK_LUA_STACK_EXIT(tracker->L);
		return -1;
	}
	
	CHECK_LUA_STACK_EXIT(tracker->L);
	return 0;
}

static void draw_cross(cairo_t *cr, double x, double y, double res, double mapsize)
{
	cairo_move_to(cr, x, y);
	cairo_rel_move_to(cr,       -CROSS_SIZE * res / mapsize,       -CROSS_SIZE * res / mapsize);
	cairo_rel_line_to(cr,  2.0 * CROSS_SIZE * res / mapsize,  2.0 * CROSS_SIZE * res / mapsize);
	cairo_rel_move_to(cr, -2.0 * CROSS_SIZE * res / mapsize, 0.0                              );
	cairo_rel_line_to(cr,  2.0 * CROSS_SIZE * res / mapsize, -2.0 * CROSS_SIZE * res / mapsize);
	cairo_stroke(cr);
}

static void course_plotter_connect_points(cairo_t *cr, Point point, Point prev_point, int res, int mapsize, double radius_factor)
{
	FPoint vec = (FPoint){(point.col - prev_point.col) / 2.0, (point.row - prev_point.row) / 2.0};
	FPoint mid = (FPoint){(point.col + prev_point.col) / 2.0, (point.row + prev_point.row) / 2.0};
	
	vec = (FPoint){
		vec.col * (1.0 - 2.0 * DOT_SIZE * radius_factor),
		vec.row * (1.0 - 2.0 * DOT_SIZE * radius_factor)
	};
	
	cairo_move_to(cr, (mid.col - vec.col + 0.5) * (double)res / mapsize, (mid.row - vec.row + 0.5) * (double)res / mapsize);
	cairo_line_to(cr, (mid.col + vec.col + 0.5) * (double)res / mapsize, (mid.row + vec.row + 0.5) * (double)res / mapsize);
	
	cairo_stroke(cr);
}

static void course_plotter_mark_point(cairo_t *cr, Point point, int res, int mapsize, double radius_factor)
{
	point.col *= res; point.col /= mapsize; point.col += res/mapsize/2;
	point.row *= res; point.row /= mapsize; point.row += res/mapsize/2;
	
	cairo_arc(cr, point.col, point.row, (res * DOT_SIZE) / mapsize * radius_factor, 0.0,  2.0 * M_PI);
	cairo_stroke(cr);
}

void draw (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data)
{
	Omni *data = user_data;
	
	int is_captain = (GtkWidget*)area != data->gui_elems.radio_engineer_drawing_area;
	Tracker *tracker = is_captain ? data->captain_tracker : data->radio_engineer_tracker;
	if(tracker == NULL) return;
	lua_State *L = tracker->L;
	
	CHECK_LUA_STACK_INIT(L);
	
	cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, TEXTSIZE);
	
	cairo_text_extents_t text_settings;
	cairo_text_extents(cr, "QWERTYUIOPASDFGHJKLZXCVBNM", &text_settings);
	const double alpha_height = text_settings.height;
	
	char cc[tracker->mrc.rlen + 1];
	snprintf(cc, tracker->mrc.rlen + 1, tracker->mrc.colfmt, tracker->mrc.mapsize);
	cairo_text_extents(cr, cc, &text_settings);
	const int text_width = text_settings.x_advance + 5;
	
	const int res = MIN(width - text_width, height - TEXTSIZE);
	
	int need_refresh = 0; // default to not needing refresh
	lua_getfield(L, LUA_GLOBALSINDEX, "need_refresh");
	int status;
	if((status = lua_pcall(L, 0, 1, 0))){
		csa_error("failed in the need_refresh lua function, defaulting to not refreshing (error code %i)\n%s\n", status, lua_tostring(L, -1));
	}else{
		need_refresh = lua_toboolean(L, -1);
	}
	lua_pop(L, 1);
	
	if(tracker->mrc.old_res != res || need_refresh) { // needs regeneration
	
		tracker->mrc.old_res = res;
		
		if(tracker->mrc.surface != NULL){
			cairo_surface_finish(tracker->mrc.surface);
			cairo_surface_destroy(tracker->mrc.surface);
			tracker->mrc.surface = NULL;
		}
		
		csa_free(tracker->mrc.pixels);
		csa_free(tracker->mrc.vals);
		
		lua_getfield(L, LUA_GLOBALSINDEX, "render_table");
		if(!lua_istable(L, -1)){
			csa_warning("aborting rendering due to lua global 'render_table' being nil.\n");
			lua_pop(L, 1);
			tracker->mrc.old_res = -1;
			CHECK_LUA_STACK_EXIT(L);
			return;
		}
		
		lua_getfield(L, -1, "layers");
		const int layerCount = lua_tointeger(L, -1);
		lua_pop(L, 1);
		const int layerSize = res * res;
		const cairo_format_t mrc_pixels_format = CAIRO_FORMAT_RGB24; // no noticeable perf benefit from using compatible surface type (CAIRO_FORMAT_ARGB32) but does weird things to colour - easier to not worry about setting alpha channel
		const int mrc_pixels_stride = cairo_format_stride_for_width(mrc_pixels_format, res);
		if((tracker->mrc.pixels = csa_calloc(res * mrc_pixels_stride, sizeof(unsigned char))) == NULL){
			csa_error("failed to allocate memory for map framebuffer.\n");
			lua_pop(L, 1);
			tracker->mrc.old_res = -1;
			CHECK_LUA_STACK_EXIT(L);
			return;
		}
		if((tracker->mrc.vals = csa_calloc(layerCount * layerSize, sizeof(float))) == NULL){
			csa_error("failed to allocate memory for heightmap buffer.\n");
			csa_free(tracker->mrc.pixels);
			tracker->mrc.pixels = NULL;
			lua_pop(L, 1);
			tracker->mrc.old_res = -1;
			CHECK_LUA_STACK_EXIT(L);
			return;
		}
		MapLayerColourMapping *mappings;
		if((mappings = csa_calloc(layerCount, sizeof(MapLayerColourMapping))) == NULL){
			csa_error("failed to allocate memory for map layer colour mappings.\n");
			csa_free(tracker->mrc.pixels);
			csa_free(tracker->mrc.vals);
			tracker->mrc.pixels = NULL;
			tracker->mrc.vals = NULL;
			lua_pop(L, 1);
			tracker->mrc.old_res = -1;
			CHECK_LUA_STACK_EXIT(L);
			return;
		}
		
#define GET_LUA_NUMBER(INTO, STATE, INDEX, FIELD) \
	lua_getfield((STATE), (INDEX), (FIELD)); \
	INTO = lua_tonumber((STATE), -1); \
	lua_pop((STATE), 1);
		
		// stack: render() output
		for(int i = 0; i < layerCount; ++i){
			lua_pushinteger(L, i + 1);
			lua_gettable(L, -2); // get the layer
			// stack: render() output, layer
			
			if(lua_istable(L, -1)){
				// try to get the colour mappings
				int has_colours = 1;
				lua_getfield(L, -1, "colours");
				if(!lua_istable(L, -1)){
					lua_pop(L, 1);
					lua_getfield(L, -1, "colors"); // yuck
					if(!lua_istable(L, -1)){
						csa_warning("lua map did not provide a colours/colors entry in the table for layer %i, so this and further layers cannot be considered for rendering.\n", i + 1);
						has_colours = 0;
					}
				}
				// stack: render() output, layer, colours (or null if non-existent)
				if(has_colours){
					lua_getfield(L, -1, "type");
					mappings[i].mapping_type = lua_tointeger(L, -1);
					lua_pop(L, 1);

					switch(mappings[i].mapping_type){
						case CONTINUOUS:
							GET_LUA_NUMBER(mappings[i].mappings.continuous.mr, L, -1, "mr");
							GET_LUA_NUMBER(mappings[i].mappings.continuous.cr, L, -1, "cr");
							GET_LUA_NUMBER(mappings[i].mappings.continuous.mg, L, -1, "mg");
							GET_LUA_NUMBER(mappings[i].mappings.continuous.cg, L, -1, "cg");
							GET_LUA_NUMBER(mappings[i].mappings.continuous.mb, L, -1, "mb");
							GET_LUA_NUMBER(mappings[i].mappings.continuous.cb, L, -1, "cb");
							break;
						case STEPPED:
							GET_LUA_NUMBER(mappings[i].mappings.stepped.wraparound, L, -1, "wraparound");
							lua_getfield(L, -1, "transitions");
							if(lua_istable(L, -1)){
								mappings[i].mappings.stepped.len = lua_objlen(L, -1);
								mappings[i].mappings.stepped.transitions = csa_calloc(mappings[i].mappings.stepped.len, sizeof(ColourTransition));
								if(mappings[i].mappings.stepped.transitions == NULL){
									csa_error("failed to allocate memory for the colour transitions array for layer %i. RENDERING SHALL PROCEED DISCARDING THIS LAYER / USING AN EMPTY LAYER.\n", i + 1);
									mappings[i].mappings.stepped.len = 0;
								}else{
									for(int j = 0; j < mappings[i].mappings.stepped.len; ++j){
										lua_pushinteger(L, j + 1);
										lua_gettable(L, -2);
										
										if(lua_istable(L, -1)){
											GET_LUA_NUMBER(mappings[i].mappings.stepped.transitions[j].threshold, L, -1, "at");
											GET_LUA_NUMBER(mappings[i].mappings.stepped.transitions[j].c.r, L, -1, "r");
											GET_LUA_NUMBER(mappings[i].mappings.stepped.transitions[j].c.g, L, -1, "g");
											GET_LUA_NUMBER(mappings[i].mappings.stepped.transitions[j].c.b, L, -1, "b");
										}else{
											csa_warning("lua map did not provide a table for entry %i of the transitions table for layer %i, so further layers cannot be considered for rendering.\n", j + 1, i + 1);
										}
										
										lua_pop(L, 1);
									}
								}
							}else{
								csa_warning("lua map did not provide a transitions table for the colours table in the table for layer %i. Defaulting to empty table.\n", i + 1);
								mappings[i].mappings.stepped.len = 0;
								mappings[i].mappings.stepped.transitions = NULL;
							}
							lua_pop(L, 1);
							break;
						default:
							csa_warning("lua map did not provide a valid value for the colour mapping type in the colours table in the table for layer %i, so this and further layers cannot be considered for rendering.\n", i + 1);
					}
					
				}
				
				lua_pop(L, 1); // pop colours stuff
				
				// try to get the map layer objects
				lua_getfield(L, -1, "len");
				int len = lua_tointeger(L, -1);
				lua_pop(L, 1);
				
				for(int j = 0; j < len; ++j){ // for each object in layer
					lua_pushinteger(L, j + 1);
					lua_gettable(L, -2); // get the object
					
					lua_getfield(L, -1, "res");
					int dims = lua_tointeger(L, -1);
					lua_pop(L, 1);
					float *layer_pixels = csa_calloc(SQUARE(dims), sizeof(float));
					if(layer_pixels == NULL){
						csa_error("failed to allocate memory for layer pixels for layer %i, object %i. RENDERING SHALL PROCEED DISCARDING THIS OBJECT.\n", i + 1, j + 1);
					}else{
						lua_getfield(L, -1, "grid");
						if(lua_istable(L, -1)){ // init from table
							lua_pushnil(L);
							while(lua_next(L, -2)){
								int k = lua_tointeger(L, -1);
								if(0 <= k && k < SQUARE(dims)){
									layer_pixels[k] = 1.f;
								}else{
									csa_warning("lua table 'grid' in layer %i, object %i contains coordinates outside the valid range. (Ignoring)\n", i + 1, j + 1);
								}
								lua_pop(L, 1);
							}
						}else{ // init from seeded noise
							srand(lua_tointeger(L, -1));
							float multiplier, offset;
							GET_LUA_NUMBER(multiplier, L, -2, "multiplier");
							GET_LUA_NUMBER(offset, L, -2, "offset");
							for(int k = 0; k < SQUARE(dims); ++k){
								layer_pixels[k] = offset + multiplier * ((float)rand()/(float)RAND_MAX);
							}
						}
						
						lua_pop(L, 1);
						
						lua_getfield(L, -1, "middle");
						int middle = lua_toboolean(L, -1);
						lua_pop(L, 1);
						if(bicubic(layer_pixels, dims, tracker->mrc.vals + layerSize * i, res, middle)){
							csa_error("failed to allocate memory for bicubic interpolation of layer %i, object %i. RENDERING SHALL PROCEED DISCARDING THIS OBJECT.\n", i + 1, j + 1);
						}
						csa_free(layer_pixels);
					}
					
					lua_pop(L, 1);
				}
			}else{
				csa_warning("layer %i is not a table, skipping.\n", i + 1);
			}
			
			lua_pop(L, 1); // pop layer
		}
		
		lua_pop(L, 1);
		
		composite(tracker, mappings, res, res * 4, layerCount, layerSize);
		
		for(int i = 0; i < layerCount; ++i){
			if(mappings[i].mapping_type == STEPPED){
				csa_free(mappings[i].mappings.stepped.transitions);
			}
		}
		csa_free(mappings);
		
		cairo_surface_t *pixels_surface = cairo_image_surface_create_for_data( // just needed to copy the MRC pixel data into the MRC surface
			tracker->mrc.pixels,
			mrc_pixels_format,
			res, res,
			mrc_pixels_stride
		);
		tracker->mrc.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, res + text_width, res + TEXTSIZE);
		cairo_t *cr2 = cairo_create(tracker->mrc.surface);
		
		// this error checking is more for informative purposes - if it fails, there's not much we can do, but there's also no point aborting since cairo seems designed not to cause any problems if there is any kind of error
		if(cairo_surface_status(pixels_surface) != CAIRO_STATUS_SUCCESS){
			csa_error("failed to create cairo surface 'pixels_surface' - this will likely lead to the map rendering silently failing, resulting in an empty map.\n");
		}
		if(cairo_surface_status(tracker->mrc.surface) != CAIRO_STATUS_SUCCESS){
			csa_error("failed to create cairo surface 'tracker->mrc.surface' - this will likely lead to the map rendering silently failing, resulting in an empty map.\n");
		}
		if(cairo_status(cr2) != CAIRO_STATUS_SUCCESS){
			csa_error("failed to create cairo context 'cr2' - this will likely lead to the map rendering silently failing, resulting in an empty map.\n");
		}
		
		cairo_translate(cr2, text_width, TEXTSIZE);
		cairo_set_source_surface(cr2, pixels_surface, 0, 0);
    	cairo_paint(cr2);
		
		cairo_surface_finish(pixels_surface);
		cairo_surface_destroy(pixels_surface);
		
		cairo_set_source_rgba(cr2, 0, 0, 0, 0.1);
		for(int i = 0; i <= tracker->mrc.mapsize; ++i){ // vertical lines
			cairo_set_line_width(cr2, i % 5 == 0 ? 4 : 2);
			cairo_move_to(cr2, i * res / tracker->mrc.mapsize, 0);
			cairo_line_to(cr2, i * res / tracker->mrc.mapsize, res);
			cairo_stroke(cr2);
		}
		for(int i = 0; i <= tracker->mrc.mapsize; ++i){ // horizontal lines
			cairo_set_line_width(cr2, i % 5 == 0 ? 4 : 2);
			cairo_move_to(cr2, 0,   i * res / tracker->mrc.mapsize);
			cairo_line_to(cr2, res, i * res / tracker->mrc.mapsize);
			cairo_stroke(cr2);
		}
		cairo_set_source_rgba(cr2, 0, 0, 0, 0.2);
		for(int y = 0; y < tracker->mrc.mapsize; ++y){
			for(int x = 0; x < tracker->mrc.mapsize; ++x){
				cairo_arc(
					cr2, 
					x * res / tracker->mrc.mapsize + res / (2*tracker->mrc.mapsize), 
					y * res / tracker->mrc.mapsize + res / (2*tracker->mrc.mapsize), 
					(DOT_SIZE * res) / tracker->mrc.mapsize, 0.0, 2.0 * M_PI
				);
				cairo_fill(cr2);
			}
		}
		
		GtkStyleContext *style = gtk_widget_get_style_context(GTK_WIDGET(area));
		GdkRGBA col;
		gtk_style_context_get_color(style, &col);
		
		cairo_set_source_rgb(cr2, col.red, col.green, col.blue);
		for(int x = 0; x < tracker->mrc.mapsize; ++x){
			int copy_s = x;
			char c[tracker->mrc.clen + 1];
			c[tracker->mrc.clen] = '\0';
			int ind = tracker->mrc.clen - 1;
			do {
				c[ind] = 'A' + copy_s % 26;
				copy_s /= 26;
				--ind;
			} while(copy_s);
			cairo_text_extents_t text_settings;
			cairo_text_extents(cr2, c, &text_settings);
			cairo_move_to(
				cr2,
				x * (double)res / tracker->mrc.mapsize + res / (2.0 * tracker->mrc.mapsize) - text_settings.width / 2.0,
				-TEXTSIZE + alpha_height
			);
			cairo_show_text(cr2, c);
			cairo_fill(cr2);
		}
		for(int y = 0; y < tracker->mrc.mapsize; ++y){
			char c[tracker->mrc.rlen + 1];
			snprintf(c, tracker->mrc.rlen + 1, tracker->mrc.colfmt, y + 1);
			cairo_text_extents_t text_settings;
			cairo_text_extents(cr2, c, &text_settings);
			cairo_move_to(
				cr2,
				-text_settings.x_advance - 3.0,
				y * (double)res / tracker->mrc.mapsize + res / (2.0 * tracker->mrc.mapsize) + text_settings.height / 2.0
			);
			cairo_show_text(cr2, c);
			cairo_fill(cr2);
		}
		
		cairo_destroy(cr2);
		
		csa_free(tracker->mrc.pixels);
		csa_free(tracker->mrc.vals);
		
		tracker->mrc.pixels = NULL;
		tracker->mrc.vals = NULL;
	}
	
	double x = tracker->x;
	double y = tracker->y;
	int amount = (MAX(width - text_width, height - TEXTSIZE) - res) / 2;
	if(width - text_width > height - TEXTSIZE){
		cairo_translate(cr, amount, 0);
		x -= (double)amount;
	}else{
		cairo_translate(cr, 0, amount);
		y -= (double)amount;
	}
	x -= (double)text_width;
	y -= (double)TEXTSIZE;
	double v = (double)res / (double)tracker->mrc.mapsize;
	if(x >= 0.0){
		x /= v;
	}else{
		x = -1.0;
	}
	if(y >= 0.0){
		y /= v;
	}else{
		y = -1.0;
	}
	Point mousep = (Point){(int)x,(int)y};
	
	cairo_set_source_surface(cr, tracker->mrc.surface, 0, 0);
    cairo_paint(cr);
	
#if NO_CACHE_MAP
	cairo_surface_finish(tracker->mrc.surface);
	cairo_surface_destroy(tracker->mrc.surface);
	
	tracker->mrc.surface = NULL;
	
	tracker->mrc.old_res = -1;
#endif
	
	cairo_translate(cr, text_width, TEXTSIZE);
	
	if(is_captain && data->course_plotter.choosing_captain_starting_pos){
		mousep.col = CLAMP(mousep.col, 0, tracker->mrc.mapsize - 1);
		mousep.row = CLAMP(mousep.row, 0, tracker->mrc.mapsize - 1);
		if(tracker->update_type == CLICK){
			if(data->course_plotter.position_history != NULL){
				csa_error("course plotter position history pointer %p being overwritten - this will probably leak memory!\n", data->course_plotter.position_history);
			}
			data->course_plotter.position_history = csa_malloc(sizeof(StateListNode));
			if(data->course_plotter.position_history == NULL){
				csa_error("failed to allocate memory for course plotter position history statelist, and so failed to set starting point to (%i, %i).\n", mousep.col, mousep.row);
			}else{
				*data->course_plotter.position_history = (StateListNode){NULL, NULL, csa_malloc(sizeof(int32_t) + 1 * sizeof(Point))};
				if(data->course_plotter.position_history->state == NULL){
					csa_error("failed to allocate memory for couse plotter position history statelist element, and so failed to set starting point to (%i, %i).\n", mousep.col, mousep.row);
					csa_free(data->course_plotter.position_history);
				}else{
					int32_t *num_points = (int32_t*)data->course_plotter.position_history->state;
					int32_t *points = num_points + 1;
					*num_points = 1;
					points[0] = mousep.col;
					points[1] = mousep.row;
					
					data->course_plotter.choosing_captain_starting_pos = FALSE;
					gtk_widget_set_sensitive(data->gui_elems.captain_map_builtin_button_grid, TRUE);
					gtk_widget_set_sensitive(data->gui_elems.captain_map_ui_box, TRUE);
				}
			}
		}else if(tracker->update_type == MOTION){
			cairo_rectangle(cr,
				mousep.col * res / tracker->mrc.mapsize,
				mousep.row * res / tracker->mrc.mapsize,
				res / tracker->mrc.mapsize,
				res / tracker->mrc.mapsize
			);
			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.4);
			cairo_fill(cr);
		}
		tracker->update_type = NO_UPDATE;
	}
	if(!(is_captain && data->course_plotter.choosing_captain_starting_pos)){
		char *new_state = NULL;
		int should_terminate;
		char *map_state = (GtkWidget*)area == data->gui_elems.radio_engineer_drawing_area ? 
			data->widget_changes.radio_engineer_tracker->state :
			data->widget_changes.captain_tracker->state;
		if(tracker->funcID != 0){
			if(tracker->update_type == CLICK){
				lua_getfield(L, LUA_GLOBALSINDEX, "should_terminate");
				lua_pushinteger(L, tracker->funcID);
				
				should_terminate = 1; // default to termination
				if(lua_pcall(L, 1, 1, 0)){
					error_popup(data, "Error in the should_terminate lua function, assuming termination (error code %i)", status);
					csa_error("failed in the should_terminate lua function, assuming termination (error code %i)\n%s\n", status, lua_tostring(L, -1));
				}else{
					should_terminate = lua_toboolean(L, -1);
				}
				lua_pop(L, 1);
				
				if(should_terminate){
					ChangeListNode *cln = csa_malloc(sizeof(ChangeListNode));
					StateListNode *s;
					StateListNode *new_sln;
					if(cln != NULL){
						s = is_captain ? data->widget_changes.captain_tracker : data->widget_changes.radio_engineer_tracker;
						new_sln = new_tracker_state(tracker, s);
					}
					if(cln == NULL || new_sln == NULL){
						error_popup(data, "Error: failed to allocate memory for new tracker state for %s action\n", is_captain ? "captain" : "radio engineer");
						csa_error("failed to allocate memory for new tracker state for %s tracker %p\n", is_captain ? "captain" : "radio engineer", (void*)tracker);
						csa_free(cln);
					}else{
						new_state = (char*)new_sln->state;
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
					}
				}else{
					lua_getfield(L, LUA_GLOBALSINDEX, "add_intermediate");
					lua_pushnumber(L, mousep.col);
					lua_pushnumber(L, mousep.row);
					if((status = lua_pcall(L, 2, 0, 0))){
						csa_error("failed in the add_intermediate lua function (status code %i)\n%s\n", status, lua_tostring(L, -1));
						lua_pop(L, 1);
					}
					new_state = csa_calloc(SQUARE(tracker->mrc.mapsize), sizeof(char));
					if(new_state == NULL){
						csa_error("failed to allocate memory for constructing new preview state for map ""(ADDED INTERMEDIATE)\n");
					}
				}
			}else if(tracker->update_type == MOTION){
				should_terminate = 0;
				new_state = csa_calloc(SQUARE(tracker->mrc.mapsize), sizeof(char));
				if(new_state == NULL){
					csa_error("failed to allocate memory for constructing new preview state for map ""(MOTION)\n");
				}
			}
			tracker->update_type = NO_UPDATE;
		}
		if(new_state != NULL){
			for(int i = 0; i < SQUARE(tracker->mrc.mapsize); ++i){
				Point p = (Point){i % tracker->mrc.mapsize, i / tracker->mrc.mapsize};
				
				lua_getfield(L, LUA_GLOBALSINDEX, "action_funcs");
				lua_pushinteger(L, tracker->funcID);
				lua_gettable(L, -2);
				lua_pushinteger(L, p.col);
				lua_pushinteger(L, p.row);
				lua_pushinteger(L, mousep.col);
				lua_pushinteger(L, mousep.row);
				lua_pushinteger(L, map_state[i]);
				
				if((status = lua_pcall(L, 5, 1, 0))){
					csa_error("failed in action_funcs[%i] lua function for point (%i,%i) (error code %i)\n%s\n", tracker->funcID, p.col, p.row, status, lua_tostring(L, -1));
				}else if(!lua_istable(L, -1)){
					csa_warning("map code did not return a table for point (%i,%i), so it is being ignored.\n", p.col, p.row);
				}else{
					deconstruct_gridtable(tracker, new_state, NULL);
				}
				lua_pop(L, 2);
			}
			if(should_terminate){
				lua_getfield(L, LUA_GLOBALSINDEX, "reset_action");
				if((status = lua_pcall(L, 0, 0, 0))){
					csa_error("failed in reset_action lua function for %s tracker (error code %i)\n%s\n", is_captain ? "captain" : "radio engineer", status, lua_tostring(L, -1));
					lua_pop(L, 1);
				}
				tracker->funcID = 0;
				new_state = NULL;
			}
		}
		
		if((GtkWidget*)area == data->gui_elems.captain_drawing_area){ // course plotter
			double alpha = 0.8;
			char first = TRUE;
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha);
			cairo_set_line_width(cr, (COURSE_PLOTTER_LINE_THICKNESS * res) / tracker->mrc.mapsize);
			StateListNode *position_history = data->course_plotter.position_history;
			if(position_history != NULL){
				Point *prev_point = NULL;
				for(; position_history != NULL; position_history = position_history->prev){
					int32_t *num_points = position_history->state;
					if(*num_points <= 0){
						alpha = 0.4;
					}else{
						Point *point = ((Point*)(num_points + 1)) + *num_points - 1; // final point in array
						if(prev_point != NULL){
							// if there is a previous point, join it up with the current point (line)
							course_plotter_connect_points(cr, *point, *prev_point, res, tracker->mrc.mapsize, 4.0);
						}
						// render the current point (circle)
						course_plotter_mark_point(cr, *point, res, tracker->mrc.mapsize, 4.0);
						if(first){
							first = FALSE;
							course_plotter_mark_point(cr, *point, res, tracker->mrc.mapsize, 7.0);
						}
						
						prev_point = point;
						--point; // move back to the next point
						
						cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha); /* set alpha - if there was
						a surface between prev_point and the current point, this will cause all further
						lines/circles to be marked as from before the last surface, but we still had to
						draw the one just before the surface before changing mode since we were there
						after surfacing so it has been visited since the last surface */
						
						for(; point >= (Point*)(num_points + 1); --num_points){
							course_plotter_connect_points(cr, *point, *prev_point, res, tracker->mrc.mapsize, 4.0);
							course_plotter_mark_point(cr, *point, res, tracker->mrc.mapsize, 4.0);
							prev_point = point;
							point--;
						}
					}
				}
			}
		}else{ // a tracker
			cairo_set_source_rgba(cr, 1.0, 0.5, 0.0, 0.8);
			map_state = (GtkWidget*)area == data->gui_elems.radio_engineer_drawing_area ? 
				data->widget_changes.radio_engineer_tracker->state :
				data->widget_changes.captain_tracker->state;
			for(int i = 0; i < SQUARE(tracker->mrc.mapsize); ++i){
				Point p = (Point){i % tracker->mrc.mapsize, i / tracker->mrc.mapsize};
				if(map_state[i]){
					cairo_arc(
						cr,
						p.col * res / tracker->mrc.mapsize + res / (2*tracker->mrc.mapsize),
						p.row * res / tracker->mrc.mapsize + res / (2*tracker->mrc.mapsize),
						(DOT_SIZE * res) / tracker->mrc.mapsize, 0.0, 2.0 * M_PI
					);
					cairo_fill(cr);
				}
			}
		}
		
		if(new_state != NULL){
			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.4);
			cairo_set_line_width(cr, CROSS_SIZE * res * 0.5 / tracker->mrc.mapsize);
			for(int i = 0; i < SQUARE(tracker->mrc.mapsize); ++i){
				Point p = (Point){i % tracker->mrc.mapsize, i / tracker->mrc.mapsize};
				if(!new_state[i]){
					/*
					cairo_rectangle(
						cr,
						p.col * res / tracker->mrc.mapsize,
						p.row * res / tracker->mrc.mapsize,
						res / tracker->mrc.mapsize,
						res / tracker->mrc.mapsize
					);
					//*/
					//*
					draw_cross(
						cr,
						p.col * res / tracker->mrc.mapsize + res / (2*tracker->mrc.mapsize),
						p.row * res / tracker->mrc.mapsize + res / (2*tracker->mrc.mapsize),
						res, tracker->mrc.mapsize
					);
					//*/
					cairo_fill(cr);
				}
			}
			csa_free(new_state);
		}
	}
	
	CHECK_LUA_STACK_EXIT(L);
}
