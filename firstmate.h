#ifndef CPT_SONAR_ASSIST_FIRST_MATE_H
#define CPT_SONAR_ASSIST_FIRST_MATE_H

#include <gtk/gtk.h>

enum special_weapon {
	KRAKEN_MISSILE = 0,
	KAOS_MINE = 1,
	ASGARD_TORPEDO = 2,
	HACKING_SYSTEM = 3,
	MINOS_COUNTERMEASURES_SYSTEM = 4,
	ULYSSES_REPAIR_SYSTEM = 5,
	
	KRAKEN_MISSILE_RT = 6,
	KAOS_MINE_RT = 7,
	ASGARD_TORPEDO_RT = 8,
	HACKING_SYSTEM_RT = 9,
	MINOS_COUNTERMEASURES_SYSTEM_RT = 10,
	ULYSSES_REPAIR_SYSTEM_RT = 11
};

int  first_mate_torpedo       (GtkButton *button, gpointer user_data);
int  first_mate_mine          (GtkButton *button, gpointer user_data);
int  first_mate_drone         (GtkButton *button, gpointer user_data);
int  first_mate_sonar         (GtkButton *button, gpointer user_data);
int  first_mate_silence       (GtkButton *button, gpointer user_data);
int  first_mate_special_weapon(GtkButton *button, gpointer user_data);
void first_mate_dropdown_set(Omni *data);
int  first_mate_update_special(GtkComboBoxText *combobox, gpointer user_data);

#endif
