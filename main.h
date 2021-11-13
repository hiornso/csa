#ifndef CPT_SONAR_ASSIST_MAIN_H
#define CPT_SONAR_ASSIST_MAIN_H

#include <gtk/gtk.h>

#include "csa_alloc.h"

enum tab {
	CAPTAIN,
	FIRST_MATE,
	ENGINEER,
	RADIO_ENGINEER
};

typedef struct gui_elems {
	GApplication *app;
	
	GtkWidget *window;
	
	
	GtkWidget *enemy_health_bar;
	GtkWidget *player_health_bar;
	
	
	GtkWidget *captain_drawing_area;
	GtkWidget *captain_self_tracking;
	GtkWidget *captain_map_label;
	GtkWidget *captain_map_ui_box;
	GtkWidget *captain_map_builtin_button_grid;
	
	GtkWidget *captain_surface_button;
	
	
	GtkWidget *first_mate_special_weapon_dropdown;
	GtkWidget *first_mate_torpedo_use_button;
	GtkWidget *first_mate_torpedo_levelbar;
	GtkWidget *first_mate_mine_use_button;
	GtkWidget *first_mate_mine_levelbar;
	GtkWidget *first_mate_drone_use_button;
	GtkWidget *first_mate_drone_levelbar;
	GtkWidget *first_mate_sonar_use_button;
	GtkWidget *first_mate_sonar_levelbar;
	GtkWidget *first_mate_silence_use_button;
	GtkWidget *first_mate_silence_levelbar;
	GtkWidget *first_mate_special_weapon_use_button;
	GtkWidget *first_mate_special_weapon_levelbar;
	
	
	GtkWidget *engineer_1_weapons_1;
	GtkWidget *engineer_1_weapons_2;
	GtkWidget *engineer_1_silence;
	GtkWidget *engineer_1_recon;
	
	GtkWidget *engineer_2_silence_1;
	GtkWidget *engineer_2_silence_2;
	GtkWidget *engineer_2_weapons;
	GtkWidget *engineer_2_recon;
	
	GtkWidget *engineer_3_recon;
	GtkWidget *engineer_3_silence_1;
	GtkWidget *engineer_3_silence_2;
	GtkWidget *engineer_3_weapons;
	
	GtkWidget *engineer_recon_1;
	GtkWidget *engineer_recon_2;
	GtkWidget *engineer_recon_3;
	
	GtkWidget *engineer_weapons_1;
	GtkWidget *engineer_weapons_2;
	
	GtkWidget *engineer_silence;
	
	GtkWidget *engineer_nuclear_1;
	GtkWidget *engineer_nuclear_2;
	GtkWidget *engineer_nuclear_3;
	GtkWidget *engineer_nuclear_4;
	GtkWidget *engineer_nuclear_5;
	GtkWidget *engineer_nuclear_6;
	
	
	GtkWidget *radio_engineer_drawing_area;
	GtkWidget *radio_engineer_map_label;
	GtkWidget *radio_engineer_map_ui_box;
	GtkWidget *radio_engineer_map_builtin_button_grid;
	
	GtkWidget *radio_engineer_surface_button;
} GUI_Elements;

// gui_elems
#define ALL_GUI_ELEMENTS						(-1LL)

#define ENEMY_HEALTH_BAR						( 1LL <<  0 )
#define PLAYER_HEALTH_BAR						( 1LL <<  1 )

#define CAPTAIN_TAB								( 1LL <<  2 )
#define COURSE_PLOTTER							( 1LL <<  3 )

#define FIRST_MATE_SPECIAL_WEAPON_DROPDOWN		( 1LL <<  4 )
#define FIRST_MATE_TORPEDO_USE_BUTTON			( 1LL <<  5 )
#define FIRST_MATE_TORPEDO_LEVELBAR				( 1LL <<  6 )
#define FIRST_MATE_MINE_USE_BUTTON				( 1LL <<  7 )
#define FIRST_MATE_MINE_LEVELBAR				( 1LL <<  8 )
#define FIRST_MATE_DRONE_USE_BUTTON				( 1LL <<  9 )
#define FIRST_MATE_DRONE_LEVELBAR				( 1LL << 10 )
#define FIRST_MATE_SONAR_USE_BUTTON				( 1LL << 11 )
#define FIRST_MATE_SONAR_LEVELBAR				( 1LL << 12 )
#define FIRST_MATE_SILENCE_USE_BUTTON			( 1LL << 13 )
#define FIRST_MATE_SILENCE_LEVELBAR				( 1LL << 14 )
#define FIRST_MATE_SPECIAL_WEAPON_USE_BUTTON	( 1LL << 15 )
#define FIRST_MATE_SPECIAL_WEAPON_LEVELBAR		( 1LL << 16 )

#define ENGINEER_1_WEAPONS_1					( 1LL << 17 )
#define ENGINEER_1_WEAPONS_2					( 1LL << 18 )
#define ENGINEER_1_SILENCE						( 1LL << 19 )
#define ENGINEER_1_RECON						( 1LL << 20 )
#define ENGINEER_2_SILENCE_1					( 1LL << 21 )
#define ENGINEER_2_SILENCE_2					( 1LL << 22 )
#define ENGINEER_2_WEAPONS						( 1LL << 23 )
#define ENGINEER_2_RECON						( 1LL << 24 )
#define ENGINEER_3_RECON						( 1LL << 25 )
#define ENGINEER_3_SILENCE_1					( 1LL << 26 )
#define ENGINEER_3_SILENCE_2					( 1LL << 27 )
#define ENGINEER_3_WEAPONS						( 1LL << 28 )
#define ENGINEER_RECON_1						( 1LL << 29 )
#define ENGINEER_RECON_2						( 1LL << 30 )
#define ENGINEER_RECON_3						( 1LL << 31 )
#define ENGINEER_WEAPONS_1						( 1LL << 32 )
#define ENGINEER_WEAPONS_2						( 1LL << 33 )
#define ENGINEER_SILENCE						( 1LL << 34 )
#define ENGINEER_NUCLEAR_1						( 1LL << 35 )
#define ENGINEER_NUCLEAR_2						( 1LL << 36 )
#define ENGINEER_NUCLEAR_3						( 1LL << 37 )
#define ENGINEER_NUCLEAR_4						( 1LL << 38 )
#define ENGINEER_NUCLEAR_5						( 1LL << 39 )
#define ENGINEER_NUCLEAR_6						( 1LL << 40 )
	
#define RADIO_ENGINEER_TAB						( 1LL << 41 )

#define NUM_GUI_ELEMS 42

#define ENGINEERING_SUBSYSTEM_1 (ENGINEER_1_RECON | \
                                 ENGINEER_1_SILENCE | \
                                 ENGINEER_1_WEAPONS_1 | \
                                 ENGINEER_1_WEAPONS_2 )
#define ENGINEERING_SUBSYSTEM_2 (ENGINEER_2_RECON | \
                                 ENGINEER_2_WEAPONS | \
                                 ENGINEER_2_SILENCE_1 | \
                                 ENGINEER_2_SILENCE_2 )
#define ENGINEERING_SUBSYSTEM_3 (ENGINEER_3_RECON | \
                                 ENGINEER_3_WEAPONS | \
                                 ENGINEER_3_SILENCE_1 | \
                                 ENGINEER_3_SILENCE_2 )

#define ENGINEERING_BOX_W (ENGINEER_1_WEAPONS_1 | \
                           ENGINEER_1_SILENCE | \
                           ENGINEER_1_RECON | \
                           ENGINEER_RECON_1 | \
                           ENGINEER_NUCLEAR_1 | \
                           ENGINEER_NUCLEAR_2 )
#define ENGINEERING_BOX_N (ENGINEER_2_SILENCE_1 | \
                           ENGINEER_2_SILENCE_2 | \
                           ENGINEER_2_WEAPONS | \
                           ENGINEER_RECON_2 | \
                           ENGINEER_WEAPONS_1 | \
                           ENGINEER_NUCLEAR_3 )
#define ENGINEERING_BOX_S (ENGINEER_3_RECON | \
                           ENGINEER_3_SILENCE_1 | \
                           ENGINEER_3_WEAPONS | \
                           ENGINEER_WEAPONS_2 | \
                           ENGINEER_NUCLEAR_4 | \
                           ENGINEER_SILENCE )
#define ENGINEERING_BOX_E (ENGINEER_1_WEAPONS_2 | \
                           ENGINEER_2_RECON | \
                           ENGINEER_3_SILENCE_2 | \
                           ENGINEER_NUCLEAR_5 | \
                           ENGINEER_RECON_3 | \
                           ENGINEER_NUCLEAR_6 )
#define ENGINEERING_NUCLEARS (ENGINEER_NUCLEAR_1 | \
                              ENGINEER_NUCLEAR_2 | \
                              ENGINEER_NUCLEAR_3 | \
                              ENGINEER_NUCLEAR_4 | \
                              ENGINEER_NUCLEAR_5 | \
                              ENGINEER_NUCLEAR_6 )

typedef struct state_list_node {
	struct state_list_node *next;
	struct state_list_node *prev;
	void *state;
} StateListNode;

typedef struct change_list_node {
	struct change_list_node *next;
	struct change_list_node *prev;
	long long update_widget;
} ChangeListNode;

typedef struct widget_changes_struct {
	StateListNode *enemy_health_bar;
	StateListNode *player_health_bar;
	
	
	StateListNode *captain_tracker;
	StateListNode *captain_map_dropdown;
	
	
	StateListNode *first_mate_special_weapon_dropdown;
	StateListNode *first_mate_torpedo_use_button;
	StateListNode *first_mate_torpedo_levelbar;
	StateListNode *first_mate_mine_use_button;
	StateListNode *first_mate_mine_levelbar;
	StateListNode *first_mate_drone_use_button;
	StateListNode *first_mate_drone_levelbar;
	StateListNode *first_mate_sonar_use_button;
	StateListNode *first_mate_sonar_levelbar;
	StateListNode *first_mate_silence_use_button;
	StateListNode *first_mate_silence_levelbar;
	StateListNode *first_mate_special_weapon_use_button;
	StateListNode *first_mate_special_weapon_levelbar;
	
	
	StateListNode *engineer_1_weapons_1;
	StateListNode *engineer_1_weapons_2;
	StateListNode *engineer_1_silence;
	StateListNode *engineer_1_recon;
	
	StateListNode *engineer_2_silence_1;
	StateListNode *engineer_2_silence_2;
	StateListNode *engineer_2_weapons;
	StateListNode *engineer_2_recon;
	
	StateListNode *engineer_3_recon;
	StateListNode *engineer_3_silence_1;
	StateListNode *engineer_3_silence_2;
	StateListNode *engineer_3_weapons;
	
	StateListNode *engineer_recon_1;
	StateListNode *engineer_recon_2;
	StateListNode *engineer_recon_3;
	
	StateListNode *engineer_weapons_1;
	StateListNode *engineer_weapons_2;
	
	StateListNode *engineer_silence;
	
	StateListNode *engineer_nuclear_1;
	StateListNode *engineer_nuclear_2;
	StateListNode *engineer_nuclear_3;
	StateListNode *engineer_nuclear_4;
	StateListNode *engineer_nuclear_5;
	StateListNode *engineer_nuclear_6;
	
	
	StateListNode *radio_engineer_tracker;
	StateListNode *radio_engineer_map_dropdown;
	
} WidgetChanges;

typedef struct cliopts {
	gboolean print_version;
	gchar *format;
} CmdLineOptions;

typedef struct course_plotter {
	StateListNode *position_history;
	/*
	position history data format:
	{
		int32_t num_points; // if <=0, this move was a surface
		int32_t points[num_points * 2]; // array of points added by this move, but unpacked from Point into int32_t[2] for x,y
	}
	*/
	int surface_action_id;
	char choosing_captain_starting_pos;
} CoursePlotter;

typedef struct omni {
	GUI_Elements gui_elems;
	ChangeListNode *changelist;
	WidgetChanges widget_changes;
	CoursePlotter course_plotter;
	struct tracker *captain_tracker;
	struct tracker *radio_engineer_tracker;
} Omni;

// prototypes

void error_popup(Omni *data, char *fmt, ...);
void free_linked_changes(ChangeListNode *node);
void free_linked_states(StateListNode *node);
int add_state(Omni *data, long long elem);
const char* crop_to_filename(const char *path);
void update_window_title(Omni *data);
void load_map_file(Omni *data, char *f);

#endif
