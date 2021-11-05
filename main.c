#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "main.h"
#include "healthbars.h"
#include "engineering.h"
#include "firstmate.h"
#include "tracker.h"
#include "csa_error.h"

#define SQUARE(x) ((x)*(x))
#define PADDING {0}
#define GTK_GLIB_VERSIONS \
	gtk_get_major_version(), gtk_get_minor_version(), gtk_get_micro_version(), \
	glib_major_version, glib_minor_version, glib_micro_version

#define VERSION_STRING "0.5.0"
#define CSA_APPLICATION_ID "com.csa"
#define ABOUT_WINDOW_SHOW_SYSTEM_INFORMATION TRUE
#define FORCE_APPLICATION_DESTROY_WINDOWS_ON_QUIT TRUE

// build info
#if defined(__clang__)
#define COMPILER "Clang"
#elif defined(__INTEL_COMPILER)
#define COMPILER "Intel C Compiler"
#elif defined(__GNUC__) || defined(__GNUG__)
#define COMPILER "GCC"
#elif defined(_MSC_VER)
#define COMPILER "MSVC"
#else
#define COMPILER "Unknown Compiler"
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define PLATFORM "Windows"
#elif __APPLE__
#define PLATFORM "MacOS"
#elif __linux__
#define PLATFORM "Linux"
#elif __unix__
#define PLATFORM "Unix"
#else
#define PLATFORM "Unknown"
#endif

// be more native to MacOS
#if __APPLE__

#define PRIMARY_MASK GDK_META_MASK
#define PRIMARY_STRING "<Meta>"
#define REDO_USES_Z 1
#define SYSTEM_INFO_STRING "(MacOS Build)"

#else

#define PRIMARY_MASK GDK_CONTROL_MASK 
#define PRIMARY_STRING "<Control>"
#define REDO_USES_Z 0
#define SYSTEM_INFO_STRING "(Standard Build)"

#endif


#define HANDLE_UNDO(BITMASK, NAME, FUNC, ELEM_TYPE, STATE_TYPE) \
	if(elems & BITMASK){ \
		data->widget_changes.NAME = data->widget_changes.NAME->prev; \
		FUNC((ELEM_TYPE*)data->gui_elems.NAME, \
			 *(STATE_TYPE*)data->widget_changes.NAME->state); \
	}

static void undo(Omni *data)
{
	if(data->changelist->prev != NULL){
		long long elems = data->changelist->update_widget;
		data->changelist = data->changelist->prev;
		
		HANDLE_UNDO( ENEMY_HEALTH_BAR ,    enemy_health_bar ,    gtk_level_bar_set_value, GtkLevelBar, int);
		HANDLE_UNDO( PLAYER_HEALTH_BAR,    player_health_bar,    gtk_level_bar_set_value, GtkLevelBar, int);
		
		if(elems & CAPTAIN_TAB){
			data->widget_changes.captain_tracker = data->widget_changes.captain_tracker->prev;
			gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_drawing_area));
			gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_self_tracking));
		}
		if(elems & COURSE_PLOTTER){
			data->course_plotter.position_history = data->course_plotter.position_history->prev;
			gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_self_tracking));
		}
		
		if(elems & FIRST_MATE_SPECIAL_WEAPON_DROPDOWN){
			data->widget_changes.first_mate_special_weapon_dropdown = data->widget_changes.first_mate_special_weapon_dropdown->prev;
			first_mate_dropdown_set(data);
		}
		HANDLE_UNDO( FIRST_MATE_TORPEDO_USE_BUTTON       , first_mate_torpedo_use_button       , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_UNDO( FIRST_MATE_TORPEDO_LEVELBAR         , first_mate_torpedo_levelbar         , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_UNDO( FIRST_MATE_MINE_USE_BUTTON          , first_mate_mine_use_button          , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_UNDO( FIRST_MATE_MINE_LEVELBAR            , first_mate_mine_levelbar            , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_UNDO( FIRST_MATE_DRONE_USE_BUTTON         , first_mate_drone_use_button         , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_UNDO( FIRST_MATE_DRONE_LEVELBAR           , first_mate_drone_levelbar           , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_UNDO( FIRST_MATE_SONAR_USE_BUTTON         , first_mate_sonar_use_button         , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_UNDO( FIRST_MATE_SONAR_LEVELBAR           , first_mate_sonar_levelbar           , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_UNDO( FIRST_MATE_SILENCE_USE_BUTTON       , first_mate_silence_use_button       , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_UNDO( FIRST_MATE_SILENCE_LEVELBAR         , first_mate_silence_levelbar         , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_UNDO( FIRST_MATE_SPECIAL_WEAPON_USE_BUTTON, first_mate_special_weapon_use_button, gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_UNDO( FIRST_MATE_SPECIAL_WEAPON_LEVELBAR  , first_mate_special_weapon_levelbar  , gtk_level_bar_set_value , GtkLevelBar, char);
		
		HANDLE_UNDO( ENGINEER_1_WEAPONS_1, engineer_1_weapons_1, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_1_WEAPONS_2, engineer_1_weapons_2, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_1_SILENCE  , engineer_1_silence  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_1_RECON    , engineer_1_recon    , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_UNDO( ENGINEER_2_SILENCE_1, engineer_2_silence_1, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_2_SILENCE_2, engineer_2_silence_2, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_2_WEAPONS  , engineer_2_weapons  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_2_RECON    , engineer_2_recon    , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_UNDO( ENGINEER_3_RECON    , engineer_3_recon    , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_3_SILENCE_1, engineer_3_silence_1, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_3_SILENCE_2, engineer_3_silence_2, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_3_WEAPONS  , engineer_3_weapons  , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_UNDO( ENGINEER_RECON_1    , engineer_recon_1    , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_RECON_2    , engineer_recon_2    , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_RECON_3    , engineer_recon_3    , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_UNDO( ENGINEER_WEAPONS_1  , engineer_weapons_1  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_WEAPONS_2  , engineer_weapons_2  , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_UNDO( ENGINEER_SILENCE    , engineer_silence,     gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_UNDO( ENGINEER_NUCLEAR_1  , engineer_nuclear_1  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_NUCLEAR_2  , engineer_nuclear_2  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_NUCLEAR_3  , engineer_nuclear_3  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_NUCLEAR_4  , engineer_nuclear_4  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_NUCLEAR_5  , engineer_nuclear_5  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_UNDO( ENGINEER_NUCLEAR_6  , engineer_nuclear_6  , gtk_widget_set_sensitive, GtkWidget, char);
		
		if(elems & RADIO_ENGINEER_TAB){
			data->widget_changes.radio_engineer_tracker = data->widget_changes.radio_engineer_tracker->prev;
			gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.radio_engineer_drawing_area));
		}
	}
}

#define HANDLE_REDO(BITMASK, NAME, FUNC, ELEM_TYPE, STATE_TYPE) \
	if(elems & BITMASK){ \
		data->widget_changes.NAME = data->widget_changes.NAME->next; \
		FUNC((ELEM_TYPE*)data->gui_elems.NAME, \
			 *(STATE_TYPE*)data->widget_changes.NAME->state); \
	}
		
static void redo(Omni *data)
{
	if(data->changelist->next != NULL){
		data->changelist = data->changelist->next;
		long long elems = data->changelist->update_widget;
		
		HANDLE_REDO( ENEMY_HEALTH_BAR ,    enemy_health_bar ,    gtk_level_bar_set_value, GtkLevelBar, int);
		HANDLE_REDO( PLAYER_HEALTH_BAR,    player_health_bar,    gtk_level_bar_set_value, GtkLevelBar, int);
		
		if(elems & CAPTAIN_TAB){
			data->widget_changes.captain_tracker = data->widget_changes.captain_tracker->next;
			gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_drawing_area));
			gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_self_tracking));
		}
		if(elems & COURSE_PLOTTER){
			data->course_plotter.position_history = data->course_plotter.position_history->next;
			gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_self_tracking));
		}
		
		if(elems & FIRST_MATE_SPECIAL_WEAPON_DROPDOWN){
			data->widget_changes.first_mate_special_weapon_dropdown = data->widget_changes.first_mate_special_weapon_dropdown->next;
			first_mate_dropdown_set(data);
		}
		HANDLE_REDO( FIRST_MATE_TORPEDO_USE_BUTTON       , first_mate_torpedo_use_button       , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_REDO( FIRST_MATE_TORPEDO_LEVELBAR         , first_mate_torpedo_levelbar         , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_REDO( FIRST_MATE_MINE_USE_BUTTON          , first_mate_mine_use_button          , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_REDO( FIRST_MATE_MINE_LEVELBAR            , first_mate_mine_levelbar            , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_REDO( FIRST_MATE_DRONE_USE_BUTTON         , first_mate_drone_use_button         , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_REDO( FIRST_MATE_DRONE_LEVELBAR           , first_mate_drone_levelbar           , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_REDO( FIRST_MATE_SONAR_USE_BUTTON         , first_mate_sonar_use_button         , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_REDO( FIRST_MATE_SONAR_LEVELBAR           , first_mate_sonar_levelbar           , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_REDO( FIRST_MATE_SILENCE_USE_BUTTON       , first_mate_silence_use_button       , gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_REDO( FIRST_MATE_SILENCE_LEVELBAR         , first_mate_silence_levelbar         , gtk_level_bar_set_value , GtkLevelBar, char);
		HANDLE_REDO( FIRST_MATE_SPECIAL_WEAPON_USE_BUTTON, first_mate_special_weapon_use_button, gtk_widget_set_sensitive, GtkWidget  , char);
		HANDLE_REDO( FIRST_MATE_SPECIAL_WEAPON_LEVELBAR  , first_mate_special_weapon_levelbar  , gtk_level_bar_set_value , GtkLevelBar, char);
		
		HANDLE_REDO( ENGINEER_1_WEAPONS_1, engineer_1_weapons_1, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_1_WEAPONS_2, engineer_1_weapons_2, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_1_SILENCE  , engineer_1_silence  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_1_RECON    , engineer_1_recon    , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_REDO( ENGINEER_2_SILENCE_1, engineer_2_silence_1, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_2_SILENCE_2, engineer_2_silence_2, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_2_WEAPONS  , engineer_2_weapons  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_2_RECON    , engineer_2_recon    , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_REDO( ENGINEER_3_RECON    , engineer_3_recon    , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_3_SILENCE_1, engineer_3_silence_1, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_3_SILENCE_2, engineer_3_silence_2, gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_3_WEAPONS  , engineer_3_weapons  , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_REDO( ENGINEER_RECON_1    , engineer_recon_1    , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_RECON_2    , engineer_recon_2    , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_RECON_3    , engineer_recon_3    , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_REDO( ENGINEER_WEAPONS_1  , engineer_weapons_1  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_WEAPONS_2  , engineer_weapons_2  , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_REDO( ENGINEER_SILENCE    , engineer_silence    , gtk_widget_set_sensitive, GtkWidget, char);
		
		HANDLE_REDO( ENGINEER_NUCLEAR_1  , engineer_nuclear_1  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_NUCLEAR_2  , engineer_nuclear_2  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_NUCLEAR_3  , engineer_nuclear_3  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_NUCLEAR_4  , engineer_nuclear_4  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_NUCLEAR_5  , engineer_nuclear_5  , gtk_widget_set_sensitive, GtkWidget, char);
		HANDLE_REDO( ENGINEER_NUCLEAR_6  , engineer_nuclear_6  , gtk_widget_set_sensitive, GtkWidget, char);
		
		if(elems & RADIO_ENGINEER_TAB){
			data->widget_changes.radio_engineer_tracker = data->widget_changes.radio_engineer_tracker->next;
			gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.radio_engineer_drawing_area));
		}
	}
}

void error_popup(Omni *data, char *fmt, ...)
{
	va_list args1;
	va_start(args1, fmt);
	va_list args2;
    va_copy(args2, args1);
	int s = vsnprintf(NULL, 0, fmt, args1) + 1;
	va_end(args1);
	char *formatted = csa_malloc(s);
	char *message;
	if(formatted == NULL){
		message = "An error occurred, but the message could not be displayed due to a memory allocation error in the error popup function.\n";
	}else{
		vsnprintf(formatted, s, fmt, args2);
		message = formatted;
	}
	va_end(args1);

	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget *dialog = gtk_message_dialog_new(
		(GtkWindow*)data->gui_elems.window,
		flags,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,
		"%s",
		message
	);
	
	csa_free(formatted);
	
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	g_signal_connect_swapped(dialog, "response", G_CALLBACK (gtk_window_destroy), dialog);
	gtk_widget_show(dialog);
}

void free_linked_changes(ChangeListNode *node)
{
	ChangeListNode *next;
	while(node != NULL){
		next = node->next;
		csa_free(node);
		node = next;
	}
}

void free_linked_states(StateListNode *node)
{
	StateListNode *next;
	while(node != NULL){
		next = node->next;
		csa_free(node->state);
		csa_free(node);
		node = next;
	}
}

static void free_window_data(Omni *data)
{
	ChangeListNode *cln = data->changelist;
	while(cln->next != NULL) cln = cln->next; // move to the end of the changelist
	while(cln != NULL){
		ChangeListNode *cln_freeing = cln;
		cln = cln->prev;
		csa_free(cln_freeing);
	}
	
	StateListNode *sln;
	
#define FREE_STATELIST(STATELISTNODE) FREE_STATELIST_GENERIC(widget_changes, STATELISTNODE)
#define FREE_STATELIST_GENERIC(FIELD, STATELISTNODE) \
do { \
	sln = data->FIELD.STATELISTNODE; \
	if(sln == NULL) break; /* nothing to free */ \
	while(sln->next != NULL) sln = sln->next; /* move to the end of the statelist */ \
	while(sln != NULL){ \
		StateListNode *sln_freeing = sln; \
		sln = sln->prev; \
		csa_free(sln_freeing->state); \
		csa_free(sln_freeing); \
	} \
} while(0)
	
	FREE_STATELIST(enemy_health_bar);
	FREE_STATELIST(player_health_bar);
				
	FREE_STATELIST(first_mate_special_weapon_dropdown);
	FREE_STATELIST(first_mate_torpedo_use_button);
	FREE_STATELIST(first_mate_torpedo_levelbar);
	FREE_STATELIST(first_mate_mine_use_button);
	FREE_STATELIST(first_mate_mine_levelbar);
	FREE_STATELIST(first_mate_drone_use_button);
	FREE_STATELIST(first_mate_drone_levelbar);
	FREE_STATELIST(first_mate_sonar_use_button);
	FREE_STATELIST(first_mate_sonar_levelbar);
	FREE_STATELIST(first_mate_silence_use_button);
	FREE_STATELIST(first_mate_silence_levelbar);
	FREE_STATELIST(first_mate_special_weapon_use_button);
	FREE_STATELIST(first_mate_special_weapon_levelbar);
				
	FREE_STATELIST(engineer_1_weapons_1);
	FREE_STATELIST(engineer_1_weapons_2);
	FREE_STATELIST(engineer_1_silence);
	FREE_STATELIST(engineer_1_recon);
				
	FREE_STATELIST(engineer_2_silence_1);
	FREE_STATELIST(engineer_2_silence_2);
	FREE_STATELIST(engineer_2_weapons);
	FREE_STATELIST(engineer_2_recon);
				
	FREE_STATELIST(engineer_3_weapons);
	FREE_STATELIST(engineer_3_recon);
	FREE_STATELIST(engineer_3_silence_1);
	FREE_STATELIST(engineer_3_silence_2);
				
	FREE_STATELIST(engineer_recon_1);
	FREE_STATELIST(engineer_recon_2);
	FREE_STATELIST(engineer_recon_3);
				
	FREE_STATELIST(engineer_weapons_1);
	FREE_STATELIST(engineer_weapons_2);
				
	FREE_STATELIST(engineer_silence);
				
	FREE_STATELIST(engineer_nuclear_1);
	FREE_STATELIST(engineer_nuclear_2);
	FREE_STATELIST(engineer_nuclear_3);
	FREE_STATELIST(engineer_nuclear_4);
	FREE_STATELIST(engineer_nuclear_5);
	FREE_STATELIST(engineer_nuclear_6);
	
	free_tracker(data->captain_tracker);
	free_tracker(data->radio_engineer_tracker);
	
	FREE_STATELIST(captain_tracker);
	FREE_STATELIST(radio_engineer_tracker);
	
	FREE_STATELIST_GENERIC(course_plotter, position_history);
	
	csa_free(data);
}

int add_state(Omni *data, long long elem)
{
	char errmsg[] = "Error: failed to allocate memory for undo record while backing up states. This action has not been applied. (add_state bitmask = %llx)";
	
	ChangeListNode *cln = csa_malloc(sizeof(ChangeListNode));
	if(cln == NULL){
		error_popup(data, errmsg, elem);
		return -1;
	}
	StateListNode *sln[NUM_GUI_ELEMS];
	StateListNode **slnr[NUM_GUI_ELEMS];
	int size[NUM_GUI_ELEMS];
	memset(size, -1, sizeof(int) * NUM_GUI_ELEMS); // set all elems of size to -1
	
	for(int i = 0; i < NUM_GUI_ELEMS; ++i){
		if(elem & (1LL << i)){
			switch(1LL << i){

#define GEN_CASE(BITMASK, NAME, TYPE) \
	case BITMASK: \
		size[i] = sizeof(TYPE); \
		slnr[i] = &(data->widget_changes.NAME); \
		break;

				GEN_CASE( ENEMY_HEALTH_BAR,     enemy_health_bar,     int);
				GEN_CASE( PLAYER_HEALTH_BAR,    player_health_bar,    int);
				
				case CAPTAIN_TAB:
					csa_warning("program attempted illegal action: tried to use add_state to add state to captain tab StateListNode.\n");
					elem ^= CAPTAIN_TAB;
					continue;
				
				GEN_CASE( FIRST_MATE_SPECIAL_WEAPON_DROPDOWN  , first_mate_special_weapon_dropdown  , char);
				GEN_CASE( FIRST_MATE_TORPEDO_USE_BUTTON       , first_mate_torpedo_use_button       , char);
				GEN_CASE( FIRST_MATE_TORPEDO_LEVELBAR         , first_mate_torpedo_levelbar         , char);
				GEN_CASE( FIRST_MATE_MINE_USE_BUTTON          , first_mate_mine_use_button          , char);
				GEN_CASE( FIRST_MATE_MINE_LEVELBAR            , first_mate_mine_levelbar            , char);
				GEN_CASE( FIRST_MATE_DRONE_USE_BUTTON         , first_mate_drone_use_button         , char);
				GEN_CASE( FIRST_MATE_DRONE_LEVELBAR           , first_mate_drone_levelbar           , char);
				GEN_CASE( FIRST_MATE_SONAR_USE_BUTTON         , first_mate_sonar_use_button         , char);
				GEN_CASE( FIRST_MATE_SONAR_LEVELBAR           , first_mate_sonar_levelbar           , char);
				GEN_CASE( FIRST_MATE_SILENCE_USE_BUTTON       , first_mate_silence_use_button       , char);
				GEN_CASE( FIRST_MATE_SILENCE_LEVELBAR         , first_mate_silence_levelbar         , char);
				GEN_CASE( FIRST_MATE_SPECIAL_WEAPON_USE_BUTTON, first_mate_special_weapon_use_button, char);
				GEN_CASE( FIRST_MATE_SPECIAL_WEAPON_LEVELBAR  , first_mate_special_weapon_levelbar  , char);
				
				GEN_CASE( ENGINEER_1_WEAPONS_1, engineer_1_weapons_1, char);
				GEN_CASE( ENGINEER_1_WEAPONS_2, engineer_1_weapons_2, char);
				GEN_CASE( ENGINEER_1_SILENCE,   engineer_1_silence,   char);
				GEN_CASE( ENGINEER_1_RECON,     engineer_1_recon,     char);
				
				GEN_CASE( ENGINEER_2_SILENCE_1, engineer_2_silence_1, char);
				GEN_CASE( ENGINEER_2_SILENCE_2, engineer_2_silence_2, char);
				GEN_CASE( ENGINEER_2_WEAPONS,   engineer_2_weapons,   char);
				GEN_CASE( ENGINEER_2_RECON,     engineer_2_recon,     char);
				
				GEN_CASE( ENGINEER_3_RECON,     engineer_3_recon,     char);
				GEN_CASE( ENGINEER_3_SILENCE_1, engineer_3_silence_1, char);
				GEN_CASE( ENGINEER_3_SILENCE_2, engineer_3_silence_2, char);
				GEN_CASE( ENGINEER_3_WEAPONS,   engineer_3_weapons,   char);
				
				GEN_CASE( ENGINEER_RECON_1,     engineer_recon_1,     char);
				GEN_CASE( ENGINEER_RECON_2,     engineer_recon_2,     char);
				GEN_CASE( ENGINEER_RECON_3,     engineer_recon_3,     char);
				
				GEN_CASE( ENGINEER_WEAPONS_1,   engineer_weapons_1,   char);
				GEN_CASE( ENGINEER_WEAPONS_2,   engineer_weapons_2,   char);
				
				GEN_CASE( ENGINEER_SILENCE,     engineer_silence,     char);
				
				GEN_CASE( ENGINEER_NUCLEAR_1,   engineer_nuclear_1,   char);
				GEN_CASE( ENGINEER_NUCLEAR_2,   engineer_nuclear_2,   char);
				GEN_CASE( ENGINEER_NUCLEAR_3,   engineer_nuclear_3,   char);
				GEN_CASE( ENGINEER_NUCLEAR_4,   engineer_nuclear_4,   char);
				GEN_CASE( ENGINEER_NUCLEAR_5,   engineer_nuclear_5,   char);
				GEN_CASE( ENGINEER_NUCLEAR_6,   engineer_nuclear_6,   char);
				
				case RADIO_ENGINEER_TAB:
					csa_warning("program attempted illegal action: tried to use add_state to add state to radio engineer tab StateListNode.\n");
					elem ^= RADIO_ENGINEER_TAB;
					continue;
				
				default:
					continue; // yes this refers to the outside loop - continue does nothing to switches
			}
			sln[i] = csa_malloc(sizeof(StateListNode));
			if(sln[i] != NULL){
				*(sln[i]) = (StateListNode){NULL, *(slnr[i]), csa_malloc(size[i])};
			}
			if(sln[i] == NULL || sln[i]->state == NULL){
				for(int j = 0; j < i; ++j){
					if(elem & (1LL << j)){
						csa_free(sln[j]->state);
						csa_free(sln[j]);
					}
				}
				csa_free(cln);
				if(sln[i] != NULL){
					csa_free(sln[i]);
				}
				error_popup(data, errmsg, elem);
				return -1;
			}
		}
	}
	*cln = (ChangeListNode){NULL, data->changelist, elem};
	if(data->changelist != NULL){
		free_linked_changes(data->changelist->next);
		data->changelist->next = cln;
	}
	data->changelist = cln;
	for(int i = 0; i < NUM_GUI_ELEMS; ++i){
		if((elem & (1LL << i)) && size[i] >= 0){
			free_linked_states((*(slnr[i]))->next);
			(*(slnr[i]))->next = sln[i];
			memcpy(sln[i]->state, (*(slnr[i]))->state, size[i]);
			*(slnr[i]) = sln[i];
		}
	}
	
	return 0;
}

static int sort_map_names(const void *ptr1, const void *ptr2)
{
	const char *s1 = *(const char**)ptr1;
	const char *s2 = *(const char**)ptr2;
	
	int len1 = strlen(s1);
	int len2 = strlen(s2);
	
	// are the final 3 chars " RT"?
	int is_RT_1 = strcmp(s1 + len1 - 3, " RT") == 0;
	int is_RT_2 = strcmp(s2 + len2 - 3, " RT") == 0;
	
	// use info to make sure RT is always after non-RT (turn-based/TBT)
	int diff = is_RT_1 - is_RT_2;
	if(diff){
		return -diff;
	}
	
	return -strcmp(s1, s2);
}

static void not_implemented (
	__attribute__ ((unused)) GSimpleAction *action,
	__attribute__ ((unused)) GVariant *parameter,
	gpointer user_data
) {
	Omni omni; // fake
	Omni *data;
	if(G_IS_APPLICATION(user_data)){
		omni.gui_elems.window = NULL; // spoof the Omni struct - just the bit we need to create the error popup.
		data = &omni;
	}else{
		data = (Omni*)user_data;
	}
	error_popup(data, "Not yet implemented. :3");
}

static void close_activate (
	__attribute__ ((unused)) GSimpleAction *action,
	__attribute__ ((unused)) GVariant *parameter,
	gpointer user_data
) {
	Omni *data = (Omni*)user_data;
	gtk_window_destroy(GTK_WINDOW(data->gui_elems.window));
}

static void undo_activate (
	__attribute__ ((unused)) GSimpleAction *action,
	__attribute__ ((unused)) GVariant *parameter,
	gpointer user_data
) {
	undo((Omni*)user_data);
}

static void redo_activate (
	__attribute__ ((unused)) GSimpleAction *action,
	__attribute__ ((unused)) GVariant *parameter,
	gpointer user_data
) {
	redo((Omni*)user_data);
}

static void mines_activate (
	GSimpleAction *action,
	GVariant *value,
	gpointer user_data
) {
	g_simple_action_set_state(action, value);
	
	__attribute__ ((unused)) Omni *data = (Omni*)user_data; // unused... for now
	
	// TODO: toggle setting in data to reflect whether we should render mines, then trigger map refresh
}

static void escape_activate (
	__attribute__ ((unused)) GSimpleAction *action,
	__attribute__ ((unused)) GVariant *parameter,
	gpointer user_data
) {
	reset_ui_action(NULL, user_data);
}

static void new_window(GApplication *app, FilesToOpen f)
{
	// create program data storage
	Omni *data = (Omni*)csa_malloc(sizeof(Omni));
	if(data == NULL){
		csa_error("failed to allocate memory for Omni struct while initialising window state - this window is not being opened.\n");
		return;
	}
	
	data->changelist = csa_malloc(sizeof(ChangeListNode));
	
	if(data->changelist == NULL){
		csa_error("failed to allocate memory for ChangeListNode while initialising window state - this window is not being opened.\n");
		csa_free(data);
		return;
	}
	
	*data->changelist = (ChangeListNode){NULL, NULL, ALL_GUI_ELEMENTS};
	
#define NUM_STATELISTS (2 + 13 + 4*3 + 3+2+1 + 6)
#define INIT_STATELIST(WIDGET,TYPE,INIT) \
	(((refs[n_statelist++] = data->widget_changes.WIDGET = csa_malloc(sizeof(StateListNode))) == NULL) ? TRUE : \
	((((*data->widget_changes.WIDGET = (StateListNode){NULL, NULL, refs[n_statelist++] = csa_malloc(sizeof(TYPE))}).state) == NULL) ? TRUE : \
	((*(TYPE*)data->widget_changes.WIDGET->state = (INIT)) ? FALSE : FALSE)))

	int n_statelist = 0;
	StateListNode *refs[NUM_STATELISTS * 2];
	for(int i = 0;  i < NUM_STATELISTS * 2; ++i){
		refs[i] = NULL;
	}
	
	if(	INIT_STATELIST(enemy_health_bar , int, 4) ||
		INIT_STATELIST(player_health_bar, int, 4) ||
		
		INIT_STATELIST(first_mate_special_weapon_dropdown  , char, 0) ||
		INIT_STATELIST(first_mate_torpedo_use_button       , char, 0) ||
		INIT_STATELIST(first_mate_torpedo_levelbar         , char, 0) ||
		INIT_STATELIST(first_mate_mine_use_button          , char, 0) ||
		INIT_STATELIST(first_mate_mine_levelbar            , char, 0) ||
		INIT_STATELIST(first_mate_drone_use_button         , char, 0) ||
		INIT_STATELIST(first_mate_drone_levelbar           , char, 0) ||
		INIT_STATELIST(first_mate_sonar_use_button         , char, 0) ||
		INIT_STATELIST(first_mate_sonar_levelbar           , char, 0) ||
		INIT_STATELIST(first_mate_silence_use_button       , char, 0) ||
		INIT_STATELIST(first_mate_silence_levelbar         , char, 0) ||
		INIT_STATELIST(first_mate_special_weapon_use_button, char, 0) ||
		INIT_STATELIST(first_mate_special_weapon_levelbar  , char, 0) ||
		
		INIT_STATELIST(engineer_1_weapons_1, char, 1) ||
		INIT_STATELIST(engineer_1_weapons_2, char, 1) ||
		INIT_STATELIST(engineer_1_silence  , char, 1) ||
		INIT_STATELIST(engineer_1_recon    , char, 1) ||
		
		INIT_STATELIST(engineer_2_silence_1, char, 1) ||
		INIT_STATELIST(engineer_2_silence_2, char, 1) ||
		INIT_STATELIST(engineer_2_weapons  , char, 1) ||
		INIT_STATELIST(engineer_2_recon    , char, 1) ||
		
		INIT_STATELIST(engineer_3_weapons  , char, 1) ||
		INIT_STATELIST(engineer_3_recon    , char, 1) ||
		INIT_STATELIST(engineer_3_silence_1, char, 1) ||
		INIT_STATELIST(engineer_3_silence_2, char, 1) ||
		
		INIT_STATELIST(engineer_recon_1    , char, 1) ||
		INIT_STATELIST(engineer_recon_2    , char, 1) ||
		INIT_STATELIST(engineer_recon_3    , char, 1) ||
		
		INIT_STATELIST(engineer_weapons_1  , char, 1) ||
		INIT_STATELIST(engineer_weapons_2  , char, 1) ||
		
		INIT_STATELIST(engineer_silence    , char, 1) ||
		
		INIT_STATELIST(engineer_nuclear_1  , char, 1) ||
		INIT_STATELIST(engineer_nuclear_2  , char, 1) ||
		INIT_STATELIST(engineer_nuclear_3  , char, 1) ||
		INIT_STATELIST(engineer_nuclear_4  , char, 1) ||
		INIT_STATELIST(engineer_nuclear_5  , char, 1) ||
		INIT_STATELIST(engineer_nuclear_6  , char, 1)
		
		)
	{
		csa_error("failed to allocate memory for one of the StateListNodes while initialising window state - this window is not being opened.\n");
		
		csa_free(data->changelist);
		csa_free(data);
		for(int i = 0; i < NUM_STATELISTS * 2; ++i){
			csa_free(refs[i]);
		}
		
		return;
	}
	
	data->gui_elems.app = app;
	
	// create builder from ui file - aborts upon error, but should be fine since it's statically compiled right in
	GtkBuilder *builder = gtk_builder_new_from_resource("/builder/csa.ui");
	
	// pull out toplevel window and set up bindings
	GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
	gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(gtk_builder_get_object(builder,"rootbox")));
	
	gtk_window_set_title(GTK_WINDOW(window),"Captain Sonar Assist");
	gtk_window_set_icon_name(GTK_WINDOW(window), "captain_sonar_assist");
	data->gui_elems.window = window;

	g_signal_connect_swapped(window, "destroy", G_CALLBACK(free_window_data), data);
	
	// menu action-callback linking stuff, blatantly stolen from an example gtk4 program.
	const GActionEntry window_entries[] = {
		{ "open",              not_implemented, NULL, NULL, NULL, PADDING },
		{ "save",              not_implemented, NULL, NULL, NULL, PADDING },
		{ "save-as",           not_implemented, NULL, NULL, NULL, PADDING },
		{ "close-window",      close_activate,  NULL, NULL, NULL, PADDING },
		
		{ "undo",              undo_activate,   NULL, NULL, NULL, PADDING },
		{ "redo",              redo_activate,   NULL, NULL, NULL, PADDING },
		
		{ "escape",            escape_activate, NULL, NULL, NULL, PADDING }
	};
	
	g_action_map_add_action_entries(G_ACTION_MAP(window),
									window_entries, G_N_ELEMENTS(window_entries),
									data);
	// TODO: figure out why this ^^^ function call is responsible for a bunch of GLib warnings on MacOS at start - seems to be linked to the menu stuff since they go away if the custom menu is not set.
	
	GVariant *toggle_mine_layer_value = g_variant_new("b", FALSE);
	GSimpleAction *toggle_mine_layer_action = g_simple_action_new_stateful("toggle-mine-layer", NULL, toggle_mine_layer_value);
	g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(toggle_mine_layer_action));
	g_object_unref(toggle_mine_layer_action);
	g_signal_connect(toggle_mine_layer_action, "change-state", G_CALLBACK(mines_activate), data);
	
	gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), TRUE);
	
	// pull out enemy and player health bar stuff
	GtkWidget *enemy_health_minus = GTK_WIDGET(gtk_builder_get_object(builder,"enemy_health_minus"));
	GtkWidget *enemy_health_bar   = GTK_WIDGET(gtk_builder_get_object(builder,"enemy_health_bar"  ));
	GtkWidget *enemy_health_plus  = GTK_WIDGET(gtk_builder_get_object(builder,"enemy_health_plus" ));
	
	GtkWidget *player_health_minus = GTK_WIDGET(gtk_builder_get_object(builder,"player_health_minus"));
	GtkWidget *player_health_bar   = GTK_WIDGET(gtk_builder_get_object(builder,"player_health_bar"  ));
	GtkWidget *player_health_plus  = GTK_WIDGET(gtk_builder_get_object(builder,"player_health_plus" ));
	
	data->gui_elems.enemy_health_bar  = enemy_health_bar;
	data->gui_elems.player_health_bar = player_health_bar;
	
	// and set up the bindings for it
	g_signal_connect(enemy_health_minus , "clicked", G_CALLBACK(dec_enemy_health ), data);
	g_signal_connect(enemy_health_plus  , "clicked", G_CALLBACK(inc_enemy_health ), data);
	
	g_signal_connect(player_health_minus, "clicked", G_CALLBACK(dec_player_health), data);
	g_signal_connect(player_health_plus , "clicked", G_CALLBACK(inc_player_health), data);
	
	// pull out the first mate dropdown, buttons and bars
	data->gui_elems.first_mate_special_weapon_dropdown   = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_special_dropdown"     ));
	data->gui_elems.first_mate_torpedo_use_button        = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_torpedo_use_button"   ));
	data->gui_elems.first_mate_torpedo_levelbar          = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_torpedo_levelbar"     ));
	data->gui_elems.first_mate_mine_use_button           = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_mine_use_button"      ));
	data->gui_elems.first_mate_mine_levelbar             = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_mine_levelbar"        ));
	data->gui_elems.first_mate_drone_use_button          = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_drone_use_button"     ));
	data->gui_elems.first_mate_drone_levelbar            = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_drone_levelbar"       ));
	data->gui_elems.first_mate_sonar_use_button          = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_sonar_use_button"     ));
	data->gui_elems.first_mate_sonar_levelbar            = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_sonar_levelbar"       ));
	data->gui_elems.first_mate_silence_use_button        = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_silence_use_button"   ));
	data->gui_elems.first_mate_silence_levelbar          = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_silence_levelbar"     ));
	data->gui_elems.first_mate_special_weapon_use_button = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_special_use_button"   ));
	data->gui_elems.first_mate_special_weapon_levelbar   = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_special_levelbar"     ));
	
	GtkWidget *first_mate_torpedo_charge_button          = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_torpedo_charge_button"));
	GtkWidget *first_mate_mine_charge_button             = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_mine_charge_button"   ));
	GtkWidget *first_mate_drone_charge_button            = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_drone_charge_button"  ));
	GtkWidget *first_mate_sonar_charge_button            = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_sonar_charge_button"  ));
	GtkWidget *first_mate_silence_charge_button          = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_silence_charge_button"));
	GtkWidget *first_mate_special_weapon_charge_button   = GTK_WIDGET(gtk_builder_get_object(builder,"first_mate_special_charge_button"));
	
	// set up the bindings for them
	g_signal_connect(data->gui_elems.first_mate_special_weapon_dropdown  , "changed", G_CALLBACK(first_mate_update_special), data);
	g_signal_connect(data->gui_elems.first_mate_torpedo_use_button       , "clicked", G_CALLBACK(first_mate_torpedo       ), data);
	g_signal_connect(first_mate_torpedo_charge_button                    , "clicked", G_CALLBACK(first_mate_torpedo       ), data);
	g_signal_connect(data->gui_elems.first_mate_mine_use_button          , "clicked", G_CALLBACK(first_mate_mine          ), data);
	g_signal_connect(first_mate_mine_charge_button                       , "clicked", G_CALLBACK(first_mate_mine          ), data);
	g_signal_connect(data->gui_elems.first_mate_drone_use_button         , "clicked", G_CALLBACK(first_mate_drone         ), data);
	g_signal_connect(first_mate_drone_charge_button                      , "clicked", G_CALLBACK(first_mate_drone         ), data);
	g_signal_connect(data->gui_elems.first_mate_sonar_use_button         , "clicked", G_CALLBACK(first_mate_sonar         ), data);
	g_signal_connect(first_mate_sonar_charge_button                      , "clicked", G_CALLBACK(first_mate_sonar         ), data);
	g_signal_connect(data->gui_elems.first_mate_silence_use_button       , "clicked", G_CALLBACK(first_mate_silence       ), data);
	g_signal_connect(first_mate_silence_charge_button                    , "clicked", G_CALLBACK(first_mate_silence       ), data);
	g_signal_connect(data->gui_elems.first_mate_special_weapon_use_button, "clicked", G_CALLBACK(first_mate_special_weapon), data);
	g_signal_connect(first_mate_special_weapon_charge_button             , "clicked", G_CALLBACK(first_mate_special_weapon), data);
	
	// pull out the buttons for engineering
#define GET_OBJECT(OBJ)\
	data->gui_elems.OBJ = GTK_WIDGET(gtk_builder_get_object(builder,#OBJ));\
	g_signal_connect(G_OBJECT(data->gui_elems.OBJ), "clicked", G_CALLBACK(handle_engineering), data);
	
	GET_OBJECT(engineer_1_weapons_1);
	GET_OBJECT(engineer_1_weapons_2);
	GET_OBJECT(engineer_1_silence);
	GET_OBJECT(engineer_1_recon);
	
	GET_OBJECT(engineer_2_silence_1);
	GET_OBJECT(engineer_2_silence_2);
	GET_OBJECT(engineer_2_weapons);
	GET_OBJECT(engineer_2_recon);
	
	GET_OBJECT(engineer_3_recon);
	GET_OBJECT(engineer_3_silence_1);
	GET_OBJECT(engineer_3_silence_2);
	GET_OBJECT(engineer_3_weapons);
	
	GET_OBJECT(engineer_recon_1);
	GET_OBJECT(engineer_recon_2);
	GET_OBJECT(engineer_recon_3);
	
	GET_OBJECT(engineer_weapons_1);
	GET_OBJECT(engineer_weapons_2);
	
	GET_OBJECT(engineer_silence);
	
	GET_OBJECT(engineer_nuclear_1);
	GET_OBJECT(engineer_nuclear_2);
	GET_OBJECT(engineer_nuclear_3);
	GET_OBJECT(engineer_nuclear_4);
	GET_OBJECT(engineer_nuclear_5);
	GET_OBJECT(engineer_nuclear_6);
	
	g_signal_connect(
		gtk_builder_get_object(builder,"engineer_surface_button"),
		"clicked", G_CALLBACK(handle_engineer_surface), data
	);
	
	data->gui_elems.captain_map_dropdown = GTK_WIDGET(gtk_builder_get_object(builder,"captain_map_dropdown"));
	g_signal_connect(data->gui_elems.captain_map_dropdown, "changed", G_CALLBACK(captain_dropdown_fix), data);
	data->gui_elems.radio_engineer_map_dropdown = GTK_WIDGET(gtk_builder_get_object(builder,"radio_engineer_map_dropdown"));
	g_signal_connect(data->gui_elems.radio_engineer_map_dropdown, "changed", G_CALLBACK(radio_engineer_dropdown_fix), data);
	
	GError *err = NULL;
	char **names = g_resources_enumerate_children(RESOURCES_MAPS_BUILTINS, G_RESOURCE_LOOKUP_FLAGS_NONE, &err);
	if(err == NULL){
		int len = 0;
		for(;names[len] != NULL; ++len);
		char **sorted_names = csa_malloc(sizeof(char*) * len);
		if(sorted_names == NULL){
			csa_warning("failed to allocate memory to sort static map names, so they are being left unsorted.\n");
			for(int i = 0; names[i] != NULL; ++i){
				gtk_combo_box_text_prepend_text((GtkComboBoxText*)data->gui_elems.captain_map_dropdown, names[i]);
				gtk_combo_box_text_prepend_text((GtkComboBoxText*)data->gui_elems.radio_engineer_map_dropdown, names[i]);
			}
		}else{
			memcpy(sorted_names, names, sizeof(char*) * len);
			qsort(sorted_names, len, sizeof(char*), sort_map_names);
			for(int i = 0; i < len; ++i){
				gtk_combo_box_text_prepend_text((GtkComboBoxText*)data->gui_elems.captain_map_dropdown, sorted_names[i]);
				gtk_combo_box_text_prepend_text((GtkComboBoxText*)data->gui_elems.radio_engineer_map_dropdown, sorted_names[i]);
			}
			csa_free(sorted_names);
		}
	}else{
		csa_error("failed to load static maps: %s\n",err->message);
		g_error_free(err);
	}
	g_strfreev(names);
	
	g_signal_connect(gtk_builder_get_object(builder,"captain_north"), "clicked", G_CALLBACK(handle_move_captain_NORTH), data);
	g_signal_connect(gtk_builder_get_object(builder,"captain_east" ), "clicked", G_CALLBACK(handle_move_captain_EAST ), data);
	g_signal_connect(gtk_builder_get_object(builder,"captain_south"), "clicked", G_CALLBACK(handle_move_captain_SOUTH), data);
	g_signal_connect(gtk_builder_get_object(builder,"captain_west" ), "clicked", G_CALLBACK(handle_move_captain_WEST ), data);
	
	g_signal_connect(gtk_builder_get_object(builder,"radio_engineer_north"), "clicked", G_CALLBACK(handle_move_radio_engineer_NORTH), data);
	g_signal_connect(gtk_builder_get_object(builder,"radio_engineer_east" ), "clicked", G_CALLBACK(handle_move_radio_engineer_EAST ), data);
	g_signal_connect(gtk_builder_get_object(builder,"radio_engineer_south"), "clicked", G_CALLBACK(handle_move_radio_engineer_SOUTH), data);
	g_signal_connect(gtk_builder_get_object(builder,"radio_engineer_west" ), "clicked", G_CALLBACK(handle_move_radio_engineer_WEST ), data);
	
	data->gui_elems.captain_surface_button = GTK_WIDGET(gtk_builder_get_object(builder,"captain_surface"));
	data->gui_elems.radio_engineer_surface_button = GTK_WIDGET(gtk_builder_get_object(builder,"radio_engineer_surface"));
	
	
	data->widget_changes.captain_tracker = NULL;
	data->captain_tracker = NULL;
	
	data->widget_changes.radio_engineer_tracker = NULL;
	data->radio_engineer_tracker = NULL;
	
	GtkGesture *captain_drawing_area_gesture_click        = gtk_gesture_click_new();
	GtkGesture *captain_self_tracking_gesture_click       = gtk_gesture_click_new();
	GtkGesture *radio_engineer_drawing_area_gesture_click = gtk_gesture_click_new();
	
	g_signal_connect(captain_drawing_area_gesture_click       , "pressed", G_CALLBACK(confirm_captain_action       ), data);
	g_signal_connect(captain_self_tracking_gesture_click      , "pressed", G_CALLBACK(confirm_captain_action       ), data);
	g_signal_connect(radio_engineer_drawing_area_gesture_click, "pressed", G_CALLBACK(confirm_radio_engineer_action), data);
	
	GtkEventController *captain_drawing_area_gesture_motion        = gtk_event_controller_motion_new();
	GtkEventController *captain_self_tracking_gesture_motion       = gtk_event_controller_motion_new();
	GtkEventController *radio_engineer_drawing_area_gesture_motion = gtk_event_controller_motion_new();
	
	g_signal_connect(captain_drawing_area_gesture_motion       , "leave", G_CALLBACK(reset_ui_action), data);
	g_signal_connect(captain_self_tracking_gesture_motion      , "leave", G_CALLBACK(reset_ui_action), data);
	g_signal_connect(radio_engineer_drawing_area_gesture_motion, "leave", G_CALLBACK(reset_ui_action), data);
	
	g_signal_connect(captain_drawing_area_gesture_motion       , "motion", G_CALLBACK(captain_action_motion       ), data);
	g_signal_connect(captain_self_tracking_gesture_motion      , "motion", G_CALLBACK(captain_action_motion       ), data);
	g_signal_connect(radio_engineer_drawing_area_gesture_motion, "motion", G_CALLBACK(radio_engineer_action_motion), data);
	
	GtkWidget *captain_drawing_area = GTK_WIDGET(gtk_builder_get_object(builder,"captain_drawing_area"));
	data->gui_elems.captain_drawing_area = captain_drawing_area;
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA(captain_drawing_area), draw, data, NULL);
	gtk_widget_add_controller(captain_drawing_area, GTK_EVENT_CONTROLLER(captain_drawing_area_gesture_click));
	gtk_widget_add_controller(captain_drawing_area, GTK_EVENT_CONTROLLER(captain_drawing_area_gesture_motion));
	
	GtkWidget *captain_self_tracking = GTK_WIDGET(gtk_builder_get_object(builder,"captain_self_tracking"));
	data->gui_elems.captain_self_tracking = captain_self_tracking;
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA(captain_self_tracking), draw, data, NULL);
	gtk_widget_add_controller(captain_self_tracking, GTK_EVENT_CONTROLLER(captain_self_tracking_gesture_click));
	gtk_widget_add_controller(captain_self_tracking, GTK_EVENT_CONTROLLER(captain_self_tracking_gesture_motion));
	
	GtkWidget *radio_engineer_drawing_area = GTK_WIDGET(gtk_builder_get_object(builder,"radio_engineer_drawing_area"));
	data->gui_elems.radio_engineer_drawing_area = radio_engineer_drawing_area;
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA(radio_engineer_drawing_area), draw, data, NULL);
	gtk_widget_add_controller(radio_engineer_drawing_area, GTK_EVENT_CONTROLLER(radio_engineer_drawing_area_gesture_click));
	gtk_widget_add_controller(radio_engineer_drawing_area, GTK_EVENT_CONTROLLER(radio_engineer_drawing_area_gesture_motion));
	
	data->gui_elems.captain_map_ui_box        = GTK_WIDGET(gtk_builder_get_object(builder,"captain_map_ui_box"       ));
	data->gui_elems.radio_engineer_map_ui_box = GTK_WIDGET(gtk_builder_get_object(builder,"radio_engineer_map_ui_box"));
	
	data->gui_elems.captain_map_builtin_button_grid        = GTK_WIDGET(gtk_builder_get_object(builder,"captain_map_builtin_button_grid"       ));
	data->gui_elems.radio_engineer_map_builtin_button_grid = GTK_WIDGET(gtk_builder_get_object(builder,"radio_engineer_map_builtin_button_grid"));
	
	data->course_plotter.position_history = NULL;
	data->course_plotter.surface_action_id = 0;
	data->course_plotter.choosing_captain_starting_pos = TRUE;
	
	g_object_unref(builder);

	if(f.type == SAVE_FILE){
		printf("Temporary error: loading save files is not yet implemented!\n");
		// when a save file is loaded it should check for a null char at the beginning
		// since text files such as maps (which are lua scripts) cannot begin with a null
		// char, thus allowing an error to be raised if the file is not a save file.
		// (and obviously to support this the save file writer will need to add the null
		// char at the beginning when writing save files)
	}else{
		// if a save file is passed in here, then the lua loader will fail at the beginning null char,
		// so there is no chance a save could be interpreted as a script. Good luck getting a binary
		// file past the syntax checker anyway though.
		if(f.type & LOAD_FIRST){
			if(!init_tracker(data, f.file1, f.type & STATIC_FIRST ? FROM_STRING : FROM_FILE, TRUE)){
				gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(data->gui_elems.captain_map_dropdown), GTK_SENSITIVITY_OFF);
				gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_drawing_area));
				gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.captain_self_tracking));
			}else{
				error_popup(data, "Failed to load map.");
			}
		}
		if(f.type & LOAD_SECOND){
			if(!init_tracker(data, f.file2, f.type & STATIC_SECOND ? FROM_STRING : FROM_FILE, FALSE)){
				gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(data->gui_elems.radio_engineer_map_dropdown), GTK_SENSITIVITY_OFF);
				gtk_widget_queue_draw(GTK_WIDGET(data->gui_elems.radio_engineer_drawing_area));
			}else{
				error_popup(data, "Failed to load map.");
			}
		}
	}
	
	gtk_widget_show(window);
}

static void new_window_activate (
	__attribute__ ((unused)) GSimpleAction *action,
	__attribute__ ((unused)) GVariant *parameter,
	gpointer user_data
) {
	new_window(G_APPLICATION(user_data), (FilesToOpen){LOAD_NOTHING, NULL, NULL});
}

static void quit_activate (
	__attribute__ ((unused)) GSimpleAction *action,
	__attribute__ ((unused)) GVariant *parameter,
	gpointer user_data
) {
	g_application_quit(G_APPLICATION(user_data));
}

static void about_activate (
	__attribute__ ((unused)) GSimpleAction *action,
	__attribute__ ((unused)) GVariant *parameter,
	gpointer user_data
) {
	GApplication *app = G_APPLICATION(user_data);
	GtkAboutDialog *dialog = (GtkAboutDialog*)gtk_about_dialog_new();
	
	gtk_about_dialog_set_program_name(dialog, "Captain Sonar Assist");
	gtk_about_dialog_set_version(dialog, "v" VERSION_STRING " " SYSTEM_INFO_STRING);
	//gtk_about_dialog_set_comments(dialog, "A program to make playing Captain Sonar on a computer more convenient, and enable more rigorous tracking of enemy submarines.");
	gtk_about_dialog_set_comments(dialog, "A program about tracking a submarine.");
	
#if ABOUT_WINDOW_SHOW_SYSTEM_INFORMATION
	const char format[] = "CSA built using " COMPILER " (" __VERSION__ ")\nCSA built at " __DATE__ " " __TIME__ "\nCSA built for platform " PLATFORM "\nGTK %i.%i.%i\nGLib %i.%i.%i\n";
	int libvers_len = snprintf(NULL, 0, format,
		GTK_GLIB_VERSIONS
	);
	char *libvers = csa_malloc(libvers_len + 1); // +1 for NULL termination
	if(libvers == NULL){
		gtk_about_dialog_set_system_information(dialog, "Failed to allocate memory to format string to display library versions.\nTry reopening the dialog.");
	}else{
		snprintf(libvers, libvers_len + 1, format,
			GTK_GLIB_VERSIONS
		);
		gtk_about_dialog_set_system_information(dialog, libvers);
		csa_free(libvers);
	}
	
#endif
	
	const char *authors[] = {"Oliver Hiorns", NULL};
	gtk_about_dialog_set_authors(dialog, authors);
	const char *artists[] = {"Eliyahu Gluschove-Koppel", NULL};
	//gtk_about_dialog_set_artists(dialog, artists);
	gtk_about_dialog_add_credit_section(dialog, "Art by", artists);
	
	gtk_about_dialog_set_logo(dialog, (GdkPaintable*)gdk_texture_new_from_resource("/images/logo/logo.png"));
	
	GtkWidget *w = gtk_window_get_titlebar(GTK_WINDOW(dialog));
	w = gtk_widget_get_first_child(w);
	w = gtk_widget_get_first_child(w);
	w = gtk_widget_get_first_child(w);
	w = gtk_widget_get_next_sibling(w);
	GtkWidget *x = gtk_widget_get_first_child(GTK_WIDGET(dialog));
	x = gtk_widget_get_first_child(x);
	gtk_window_set_titlebar(GTK_WINDOW(dialog), NULL);
	gtk_box_prepend(GTK_BOX(x), w);
	gtk_widget_set_halign(w, GTK_ALIGN_CENTER);
	gtk_window_set_application(GTK_WINDOW(dialog), GTK_APPLICATION(app));
	gtk_widget_show(GTK_WIDGET(dialog));
}

#if FORCE_APPLICATION_DESTROY_WINDOWS_ON_QUIT
static void window_destroy_gfunc(void *window, __attribute__ ((unused)) void *user_data)
{
	gtk_window_destroy(GTK_WINDOW(window));
}

static void force_destroy_windows(GApplication *app)
{
	GList *list = gtk_application_get_windows(GTK_APPLICATION(app));
	g_list_foreach(list, window_destroy_gfunc, NULL); 
	// about the above: could just pass (GFunc)gtk_window_destroy but this is better practice (calling-convention independent)...
	// don't mention all the other places this program assumes it's fine to drop excess arguments
}
#endif

static void initialise_application(GApplication *app, __attribute__ ((unused)) gpointer user_data)
{
#if FORCE_APPLICATION_DESTROY_WINDOWS_ON_QUIT
	g_signal_connect(app, "shutdown", G_CALLBACK(force_destroy_windows), NULL);
#endif
	
	// menu action-callback linking stuff, blatantly stolen from an example gtk4 program.
	const GActionEntry app_entries[] = {
		{ "new",               new_window_activate,  NULL, NULL, NULL, PADDING },
		{ "preferences",       not_implemented,      NULL, NULL, NULL, PADDING },
		{ "quit",              quit_activate,        NULL, NULL, NULL, PADDING },
		
		{ "about",             about_activate,       NULL, NULL, NULL, PADDING },
		{ "help",              not_implemented,      NULL, NULL, NULL, PADDING }
	};
	
	g_action_map_add_action_entries(G_ACTION_MAP (app),
									app_entries, G_N_ELEMENTS (app_entries),
									app);
	
	char *accels[3] = {NULL, NULL, NULL};
	
	accels[0] = PRIMARY_STRING "n";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.new", (const char * const *)accels);
	accels[0] = PRIMARY_STRING "o";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.open", (const char * const *)accels);
	accels[0] = PRIMARY_STRING "s";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.save", (const char * const *)accels);
	accels[0] = PRIMARY_STRING "<Shift>s";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.save-as", (const char * const *)accels);
	accels[0] = PRIMARY_STRING "comma";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.preferences", (const char * const *)accels);
	accels[0] = PRIMARY_STRING "w";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.close-window", (const char * const *)accels);
	accels[0] = PRIMARY_STRING "q";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit", (const char * const *)accels);
	
	accels[0] = PRIMARY_STRING "z";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.undo", (const char * const *)accels);
#if REDO_USES_Z
	accels[0] = PRIMARY_STRING "<Shift>z";
	accels[1] = PRIMARY_STRING "y";
#else
	accels[1] = PRIMARY_STRING "<Shift>z";
	accels[0] = PRIMARY_STRING "y";
#endif
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.redo", (const char * const *)accels);
	
	accels[1] = NULL;
	accels[0] = "m";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.toggle-mine-layer", (const char * const *)accels);
	accels[0] = "Escape";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.escape", (const char * const *)accels);
	
	accels[0] = "F1";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.help", (const char * const *)accels);
	accels[0] = "F7";
	gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.about", (const char * const *)accels);
	
	// this could be done automatically by placing the menu resource at /com/csa/gtk/menus.ui but this way seems clearer
	GtkBuilder *builder = gtk_builder_new_from_resource("/builder/menu.ui");
	GMenuModel *model = G_MENU_MODEL(gtk_builder_get_object(builder, "menubar"));
	gtk_application_set_menubar(GTK_APPLICATION(app), model);
	g_object_unref(builder);
}

static void line_wrap(char *str)
{
#define LINE_LEN 150
	for(int i = 0, last = 0; str[i] != '\0'; ++i) {
		if(i - last > LINE_LEN && str[i] == ' ') {
			str[i] = '\n';
			last = i;
		}else if(str[i] == '\n') {
			last = i;
		}
	}
}

static void open_map(GApplication *application, GFile **files, int n_files, __attribute__ ((unused)) char *hint, gpointer user_data)
{
	CmdLineOptions *options = (CmdLineOptions*)user_data;

	char *fmt = options->format;
	
	if(fmt != NULL && strcmp(fmt, "help") == 0){
		char formatters_description[] = 
			"\033[1mFormat strings\033[0m (for use with -f):\n"
			"Format strings are parsed on a per-window basis, with windows separated by "
			"spaces. Each format unit reads the next file path or map name in from the  "
			"list of files passed as arguments. Valid format units are:\n"
			"\033[1;34m\tn\033[0m: \033[1;33mnew window\033[0m\n"
			"\033[1;34m\ts\033[0m: \033[1;33mload save file\033[0m\n"
			"\033[1;34m\tc\033[0m: \033[1;33mload file into captain tab\033[0m\n"
			"\033[1;34m\tr\033[0m: \033[1;33mload file into radio engineer tab\033[0m\n"
			"\033[1;34m\tb\033[0m: \033[1;33mload file into both captain and radio engineer tabs\033[0m\n"
			"The `\033[1;34mc\033[0m`, `\033[1;34mr\033[0m` and `\033[1;34mb\033[0m` format units can be "
			"prefixed with an `\033[1;34ms\033[0m` to load from the maps built into the program rather "
			"than an external file (this treats the corresponding argument as a map name rather than "
			"file path).\n"
			"For example:\n"
			"\t\033[32mcsa \033[33m-f \033[1m\"n scr b s n\" \033[34mAlpha ~/map.lua ~/map2.lua ~/mysave.csa\033[0m\n"
			"creates an empty window, a window with the inbuilt \033[34mAlpha\033[0m map loaded into the "
			"Captain tracker and \033[34m~/map.lua\033[0m loaded into the Radio Engineer tracker, a window "
			"with \033[34m~/map2.lua\033[0m loaded into both the Captain and Radio Engineer trackers, a window "
			"into which \033[34m~/mysave.csa\033[0m is loaded and another empty window.\n";
		line_wrap(formatters_description);
		printf("%s", formatters_description);
		return;
	}
	
	int i = 0; // file index in file array
	int j = 0; // char index in format string
	if(fmt == NULL){
		if(n_files == 0){
			new_window(application, (FilesToOpen){LOAD_NOTHING, NULL, NULL});
		}else{
			for(; i < n_files; ++i){
				char *f = g_file_get_path(files[i]);
				int action;
				FILE *fp = fopen(f, "rb");
				if(fp == NULL){
					csa_warning("file '%s' could not be opened to check type (save file or map file) so the created window will be left empty.\n", f);
					g_free(f);
					f = NULL;
					action = LOAD_NOTHING;
				}else{
					int c = fgetc(fp);
					if(c == EOF){
						csa_warning("file '%s' is empty, so cannot be a valid save file or map file, so the created window will be left empty.\n", f);
						g_free(f);
						f = NULL;
						action = LOAD_NOTHING;
					}else if(c == '\0'){ // all save files begin with a null char to allow this to work, since no map file could begin with a null char
						action = SAVE_FILE;
					}else{ // assume it's a map file, let the lua loader handle it if it's not
						action = LOAD_FIRST | LOAD_SECOND; // load into both captain and radio engineer tabs by default (can use format string to gain more control over this behaviour)
					}
					fclose(fp);
				}
				new_window(application, (FilesToOpen){action, f, f});
				g_free(f);
			}
		}
	}else{
		while(1){
			while(fmt[j] == ' ') ++j; // skip any whitespace
			if(fmt[j] == '\0') break;
			int ind = j;
			while(fmt[j] != ' ' && fmt[j] != '\0') ++j; // increment until we either reach whitespace or NULL. format word now has indices current_ind <= ind < j
			FilesToOpen f = (FilesToOpen){LOAD_NOTHING, NULL, NULL};
			if(fmt[ind] == 'n'){
				new_window(application, f);
				++ind;
				if(ind != j){
					csa_warning("format specifier 'n' does not take further modifiers but further modifiers were given.\n");
				}
			}else if((fmt[ind] == 's') && (ind + 1 == j)){
				++ind;
				f.type = SAVE_FILE;
				if(i < n_files){
					f.file1 = g_file_get_path(files[i]);
					new_window(application, f);
					g_free(f.file1);
					++i;
				}else{
					csa_warning("format specifier 's' requires a file but too few files were provided. An empty window is being created since a save file was not specified.\n");
				}
			}else{
				int is_static = FALSE;
				for(; ind < j; ++ind){
					if(fmt[ind] == 's'){
						is_static = TRUE;
					}else{
						for(int k = 0; k < 2; ++k){
							char ch = k == 0 ? 'c' : 'r'; // captain then radio engineer
							char *tabname = k == 0 ? "captain" : "radio engineer";
							int load_n = k == 0 ? LOAD_FIRST : LOAD_SECOND;
							int static_n = k == 0 ? STATIC_FIRST : STATIC_SECOND;
							
							if(fmt[ind] == ch || fmt[ind] == 'b'){
								if(f.type & load_n){
									csa_warning("file to load for %s tab respecified. Ignoring respecification.%s\n", tabname, fmt[ind] == 'b' ? " (The specified map will still be set for the other tab if it has not been defined yet.)" : "");
								}else{
									f.type |= load_n | (is_static ? static_n : 0);
									if(i < n_files){
										if(is_static){
											char *mapname = g_file_get_basename(files[i]);
											char prefix[] = RESOURCES_MAPS_BUILTINS;
											char *path = csa_malloc(strlen(prefix) + strlen(mapname) + 1);
											if(path == NULL){
												csa_error("failed to allocate memory to construct path to map resource for map '%s' to load into %s tab. Abandoning loading this resource (the tab will be left empty).\n", mapname, tabname);
												f.type &= ~load_n & ~static_n;
											}else{
												strcpy(path, prefix);
												strcat(path, mapname);
												GError *err = NULL;
												GBytes *map_gbytes = g_resources_lookup_data(path, G_RESOURCE_LOOKUP_FLAGS_NONE, &err);
												if(err == NULL){
													gsize s;
													gpointer map = g_bytes_unref_to_data(map_gbytes, &s);
													if(k == 0){
														f.file1 = (char*)map;
													}else{
														f.file2 = (char*)map;
													}
												}else{
													csa_error("failed to get map resource %s: %s\n", mapname, err->message);
													g_error_free(err);
													f.type &= ~load_n & ~static_n;
												}
												csa_free(path);
											}
											g_free(mapname);
										}else{
											if(k == 0){
												f.file1 = g_file_get_path(files[i]);
											}else{
												f.file2 = g_file_get_path(files[i]);
											}
										}
										if(k == 1 || fmt[ind] != 'b'){ // only increment file counter on second use for formatter 'b' (both) since we're reusing the same file to load into both Captain and Radio Engineer
											++i;
										}
									}else{
										csa_warning("format specifier '%c' requires a file but too few files were provided. As a result the %s tab will not have anything loaded into it.\n", fmt[ind], tabname);
										f.type &= ~load_n & ~static_n;
									}
									if(fmt[ind] != 'b'){
										is_static = FALSE;
									}
								}
							}else{
								csa_warning("invalid format specifier unit '%c' is being ignored.\n", fmt[ind]);
								break;
							}
						}
					}
				}
				new_window(application, f);
				
				if(f.file1 != NULL && f.file1 != f.file2){
					g_free(f.file1);
				}
				if(f.file2 != NULL){
					g_free(f.file2);
				}
			}
		}
	}
	if(i < n_files){
		char *filename = g_file_get_path(files[i]);
		csa_warning("too many files passed, discarding file '%s' and those after it due to not being specified in the format string.\n", filename);
		g_free(filename);
	}
	
}

static void activate(GApplication *application, gpointer user_data)
{
	open_map(application, NULL, 0, NULL, user_data);
}

static int handle_cmdline_args(__attribute__ ((unused)) GApplication *application, __attribute__ ((unused)) GVariantDict *options_dict, gpointer user_data)
{
	CmdLineOptions *options = (CmdLineOptions*)user_data;
	
	if(options->print_version){
		fprintf(stdout,
			"CSA " VERSION_STRING " " SYSTEM_INFO_STRING "\nCSA built using " COMPILER " (" __VERSION__ ") at " __DATE__ " " __TIME__ " for platform " PLATFORM "\nGTK %i.%i.%i\nGLib %i.%i.%i\n",
			GTK_GLIB_VERSIONS
		);
		
		return 0;
	}
	
	return -1;
}

int main(int argc, char *argv[])
{
	GtkApplication *app = gtk_application_new(CSA_APPLICATION_ID ,G_APPLICATION_HANDLES_OPEN);
	
	CmdLineOptions cmd_line_options = (CmdLineOptions){FALSE, NULL};
	const GOptionEntry options[] = {
		{
			"version",
			'v',
			G_OPTION_FLAG_NONE,
			
			G_OPTION_ARG_NONE,
			&cmd_line_options.print_version,
			
			"Print the version and exit.",
			NULL
		},
		{
			"format",
			'f',
			G_OPTION_FLAG_NONE,
			
			G_OPTION_ARG_STRING,
			&cmd_line_options.format,
			
			"Supply a format string to control how the file arguments passed are used to initialise new windows. Pass \"help\" for a more in-depth explanation.",
			"FORMAT"
		},
		{ NULL, 0, 0, 0, NULL, NULL, NULL }
	};
	g_application_add_main_option_entries(G_APPLICATION(app), options);
	g_application_set_option_context_summary(G_APPLICATION(app), "A program about tracking a submarine.");
	char csa_description[] =
		"CSA (Captain Sonar Assist) is a tool to help players win games of Captain Sonar, "
		"especially by tracking their opponents' positions on maps where this is normally "
		"infeasible for humans. It also offers ways to keep track of engineering and first "
		"mate game state for convenience, and generally allows games to be played completely "
		"within the application, without the need for pen or paper. It aims to be modular, "
		"stable, accessible and performant. The maps that normally come with Captain Sonar "
		"are built into the application, but new maps can be easily written with highly "
		"customisable behaviour using Lua.\n"
		"\n"
		"To report any bugs, make a feature request or contribute to the project, see "
		"https://github.com/hiornso/csa";
	line_wrap(csa_description);
	g_application_set_option_context_description(G_APPLICATION(app), csa_description);
	g_application_set_option_context_parameter_string(G_APPLICATION(app), "[FILES…]");
	
	g_signal_connect(app, "startup", G_CALLBACK(initialise_application), NULL);
	g_signal_connect(app, "activate", G_CALLBACK(activate), &cmd_line_options);
	g_signal_connect(app, "open", G_CALLBACK(open_map), &cmd_line_options);
	g_signal_connect(app, "handle-local-options", G_CALLBACK(handle_cmdline_args), &cmd_line_options);
	
	int status = g_application_run(G_APPLICATION (app), argc, argv);
	
	g_object_unref(app);
	
	csa_alloc_print_report();
	
	return status;
}
