#ifndef CPT_SONAR_ASSIST_HEALTHBARS_H
#define CPT_SONAR_ASSIST_HEALTHBARS_H

#include <gtk/gtk.h>

int dec_enemy_health(GtkButton *button, gpointer user_data);
int inc_enemy_health(GtkButton *button, gpointer user_data);
int dec_player_health(GtkButton *button, gpointer user_data);
int inc_player_health(GtkButton *button, gpointer user_data);

#endif
