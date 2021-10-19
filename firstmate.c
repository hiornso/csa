#include <gtk/gtk.h>

#include "main.h"
#include "firstmate.h"

static int is_realtime(Omni *data)
{
	return *(char*)data->widget_changes.first_mate_special_weapon_dropdown->state >= KRAKEN_MISSILE_RT;
}

static int get_special_count(Omni *data)
{
	int rapid = (*(char*)data->widget_changes.first_mate_special_weapon_dropdown->state % KRAKEN_MISSILE_RT) == KRAKEN_MISSILE ||
				(*(char*)data->widget_changes.first_mate_special_weapon_dropdown->state % KRAKEN_MISSILE_RT) == ASGARD_TORPEDO ;
	return rapid ? 3 : 5;
}

#define FIRST_MATE_FUNCTION(MACRO_NAME, NAME, BARLEN) \
int first_mate_##NAME (GtkButton *button, gpointer user_data) \
{ \
	Omni *data = (Omni*)user_data; \
	if((GtkWidget*)button == data->gui_elems.first_mate_##NAME##_use_button){ \
		gtk_widget_set_sensitive(GTK_WIDGET(button), 0); \
		add_state(data, FIRST_MATE_##MACRO_NAME##_USE_BUTTON | FIRST_MATE_##MACRO_NAME##_LEVELBAR); \
		*(char*)data->widget_changes.first_mate_##NAME##_use_button->state = 0; \
		*(char*)data->widget_changes.first_mate_##NAME##_levelbar->state = 0; \
		gtk_level_bar_set_value((GtkLevelBar*)data->gui_elems.first_mate_##NAME##_levelbar, 0); \
	}else{ /* charge button */\
		if(*(char*)data->widget_changes.first_mate_##NAME##_use_button->state == 0){ \
			long long mask = 0; \
			if(*(char*)data->widget_changes.first_mate_##NAME##_levelbar->state >= (BARLEN) - 1){ \
				mask = FIRST_MATE_##MACRO_NAME##_USE_BUTTON; \
			} \
			add_state(data, FIRST_MATE_##MACRO_NAME##_LEVELBAR | mask); \
			if(mask){ \
				gtk_widget_set_sensitive(GTK_WIDGET(data->gui_elems.first_mate_##NAME##_use_button), 1); \
				*(char*)data->widget_changes.first_mate_##NAME##_use_button->state = 1; \
			} \
			gtk_level_bar_set_value((GtkLevelBar*)data->gui_elems.first_mate_##NAME##_levelbar, \
									++(*(char*)data->widget_changes.first_mate_##NAME##_levelbar->state)); \
		} \
	} \
	return 1; \
}

FIRST_MATE_FUNCTION(TORPEDO, torpedo, 2 + is_realtime(data))
FIRST_MATE_FUNCTION(MINE   , mine   , 2 + is_realtime(data))
FIRST_MATE_FUNCTION(DRONE  , drone  , 3 + is_realtime(data))
FIRST_MATE_FUNCTION(SONAR  , sonar  , 2 + is_realtime(data))
FIRST_MATE_FUNCTION(SILENCE, silence, 5 + is_realtime(data))
FIRST_MATE_FUNCTION(SPECIAL_WEAPON, special_weapon, get_special_count(data) + is_realtime(data))

void first_mate_dropdown_set(Omni *data)
{
	GtkComboBox *combobox = (GtkComboBox*)data->gui_elems.first_mate_special_weapon_dropdown;
	char value = *(char*)data->widget_changes.first_mate_special_weapon_dropdown->state;
	
	if(gtk_combo_box_get_active(combobox) != value){ // if no, we don't need to change anything anyway
		
		g_signal_handlers_block_by_func(combobox, (void*)first_mate_update_special, data);
		gtk_combo_box_set_active(combobox, value);
		g_signal_handlers_unblock_by_func(combobox, (void*)first_mate_update_special, data);
		
		gtk_level_bar_set_max_value(
			(GtkLevelBar*)data->gui_elems.first_mate_special_weapon_levelbar,
			get_special_count(data) + is_realtime(data)
		);
		
		gtk_level_bar_set_max_value((GtkLevelBar*)data->gui_elems.first_mate_torpedo_levelbar, 2 + is_realtime(data));
		gtk_level_bar_set_max_value((GtkLevelBar*)data->gui_elems.first_mate_mine_levelbar   , 2 + is_realtime(data));
		gtk_level_bar_set_max_value((GtkLevelBar*)data->gui_elems.first_mate_drone_levelbar  , 3 + is_realtime(data));
		gtk_level_bar_set_max_value((GtkLevelBar*)data->gui_elems.first_mate_sonar_levelbar  , 2 + is_realtime(data));
		gtk_level_bar_set_max_value((GtkLevelBar*)data->gui_elems.first_mate_silence_levelbar, 5 + is_realtime(data));
	}
}

int first_mate_update_special(GtkComboBoxText *combobox, gpointer user_data)
{
	Omni *data = (Omni*)user_data;
		
	int new = gtk_combo_box_get_active((GtkComboBox*)combobox);
	
	int old_rt = is_realtime(data);
	int new_rt = new >= KRAKEN_MISSILE_RT;
	
	long long mask = FIRST_MATE_SPECIAL_WEAPON_DROPDOWN | FIRST_MATE_SPECIAL_WEAPON_LEVELBAR | 
					( *(char*)data->widget_changes.first_mate_special_weapon_use_button->state ? FIRST_MATE_SPECIAL_WEAPON_USE_BUTTON : 0 );
	if(old_rt != new_rt){
		mask |= FIRST_MATE_TORPEDO_LEVELBAR | ( *(char*)data->widget_changes.first_mate_torpedo_use_button->state ? FIRST_MATE_TORPEDO_USE_BUTTON : 0 ) | 
				FIRST_MATE_MINE_LEVELBAR    | ( *(char*)data->widget_changes.first_mate_mine_use_button->state    ? FIRST_MATE_MINE_USE_BUTTON    : 0 ) |
				FIRST_MATE_DRONE_LEVELBAR   | ( *(char*)data->widget_changes.first_mate_drone_use_button->state   ? FIRST_MATE_DRONE_USE_BUTTON   : 0 ) |
				FIRST_MATE_SONAR_LEVELBAR   | ( *(char*)data->widget_changes.first_mate_sonar_use_button->state   ? FIRST_MATE_SONAR_USE_BUTTON   : 0 ) |
				FIRST_MATE_SILENCE_LEVELBAR | ( *(char*)data->widget_changes.first_mate_silence_use_button->state ? FIRST_MATE_SILENCE_USE_BUTTON : 0 ) ;
	}
	
	if(mask){
		add_state(data, mask);
		*(char*)data->widget_changes.first_mate_special_weapon_dropdown->state = new;

#define LEVELBAR_NEW_MAX(NAME, NEW_MAX) \
		gtk_level_bar_set_value((GtkLevelBar*)data->gui_elems.first_mate_##NAME##_levelbar, 0); \
		gtk_level_bar_set_max_value((GtkLevelBar*)data->gui_elems.first_mate_##NAME##_levelbar, (NEW_MAX)); \
		*(char*)data->widget_changes.first_mate_##NAME##_levelbar->state = 0; \
		if(*(char*)data->widget_changes.first_mate_##NAME##_use_button->state){ \
			*(char*)data->widget_changes.first_mate_##NAME##_use_button->state = 0; \
			gtk_widget_set_sensitive(GTK_WIDGET(data->gui_elems.first_mate_##NAME##_use_button), 0); \
		}
		
		LEVELBAR_NEW_MAX(special_weapon, get_special_count(data) + is_realtime(data));
		
		if(old_rt != new_rt){
			LEVELBAR_NEW_MAX(torpedo, 2 + is_realtime(data));
			LEVELBAR_NEW_MAX(mine   , 2 + is_realtime(data));
			LEVELBAR_NEW_MAX(drone  , 3 + is_realtime(data));
			LEVELBAR_NEW_MAX(sonar  , 2 + is_realtime(data));
			LEVELBAR_NEW_MAX(silence, 5 + is_realtime(data));
		}
	}
	
	return 1;
}
