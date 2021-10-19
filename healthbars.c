#include <gtk/gtk.h>

#include "main.h"
#include "healthbars.h"

int dec_enemy_health(__attribute__ ((unused)) GtkButton *button, gpointer user_data)
{
	Omni *data = user_data;
	
	if(*(int*)data->widget_changes.enemy_health_bar->state > 0){
		
		if(add_state(data, ENEMY_HEALTH_BAR)) return 1;
		
		(*(int*)data->widget_changes.enemy_health_bar->state)--;
		
		gtk_level_bar_set_value((GtkLevelBar*)data->gui_elems.enemy_health_bar,
								*(int*)data->widget_changes.enemy_health_bar->state);
	}
	
	return 1;
}

int inc_enemy_health(__attribute__ ((unused)) GtkButton *button, gpointer user_data)
{
	Omni *data = user_data;
	
	if(*(int*)data->widget_changes.enemy_health_bar->state < 4){
		
		if(add_state(data, ENEMY_HEALTH_BAR)) return 1;
		
		(*(int*)data->widget_changes.enemy_health_bar->state)++;
		
		gtk_level_bar_set_value((GtkLevelBar*)data->gui_elems.enemy_health_bar,
								*(int*)data->widget_changes.enemy_health_bar->state);
	}
	
	return 1;
}

int dec_player_health(__attribute__ ((unused)) GtkButton *button, gpointer user_data)
{
	Omni *data = user_data;
	
	if(*(int*)data->widget_changes.player_health_bar->state > 0){
		
		if(add_state(data, PLAYER_HEALTH_BAR)) return 1;
		
		(*(int*)data->widget_changes.player_health_bar->state)--;
		
		gtk_level_bar_set_value((GtkLevelBar*)data->gui_elems.player_health_bar,
								*(int*)data->widget_changes.player_health_bar->state);
	}
	
	return 1;
}

int inc_player_health(__attribute__ ((unused)) GtkButton *button, gpointer user_data)
{
	Omni *data = user_data;
	
	if(*(int*)data->widget_changes.player_health_bar->state < 4){
		
		if(add_state(data, PLAYER_HEALTH_BAR)) return 1;
		
		(*(int*)data->widget_changes.player_health_bar->state)++;
		
		gtk_level_bar_set_value((GtkLevelBar*)data->gui_elems.player_health_bar,
								*(int*)data->widget_changes.player_health_bar->state);
	}
	
	return 1;
}
