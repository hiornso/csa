#include <gtk/gtk.h>

#include "main.h"
#include "engineering.h"

static long long is_in_subsystem(Omni *data, GtkButton *btn, int subsystem)
{
	GtkWidget *button = (GtkWidget*)btn;
	long long r = 0;
	switch(subsystem){
		case 1: // Subsystem 1
			if(button == data->gui_elems.engineer_1_weapons_1) { r |= ENGINEER_1_WEAPONS_1; break; }
			if(button == data->gui_elems.engineer_1_weapons_2) { r |= ENGINEER_1_WEAPONS_2; break; }
			if(button == data->gui_elems.engineer_1_silence  ) { r |= ENGINEER_1_SILENCE;   break; }
			if(button == data->gui_elems.engineer_1_recon    ) { r |= ENGINEER_1_RECON;     break; }
			break;
		case 2: // Subsystem 2
			if(button == data->gui_elems.engineer_2_weapons  ) { r |= ENGINEER_2_WEAPONS;   break; }
			if(button == data->gui_elems.engineer_2_silence_1) { r |= ENGINEER_2_SILENCE_1; break; }
			if(button == data->gui_elems.engineer_2_silence_2) { r |= ENGINEER_2_SILENCE_2; break; }
			if(button == data->gui_elems.engineer_2_recon    ) { r |= ENGINEER_2_RECON;     break; }
			break;
		case 3: // Subsystem 3
			if(button == data->gui_elems.engineer_3_weapons  ) { r |= ENGINEER_3_WEAPONS;   break; }
			if(button == data->gui_elems.engineer_3_silence_1) { r |= ENGINEER_3_SILENCE_1; break; }
			if(button == data->gui_elems.engineer_3_silence_2) { r |= ENGINEER_3_SILENCE_2; break; }
			if(button == data->gui_elems.engineer_3_recon    ) { r |= ENGINEER_3_RECON;     break; }
			break;
	}
	return r;
}

static int pressed_count_subsystem(Omni *data, int subsystem)
{
	switch(subsystem){
		case 1:
			return	(*(char*)data->widget_changes.engineer_1_weapons_1->state) +
					(*(char*)data->widget_changes.engineer_1_weapons_2->state) +
					(*(char*)data->widget_changes.engineer_1_silence->state  ) +
					(*(char*)data->widget_changes.engineer_1_recon->state    ) ;
		case 2:
			return	(*(char*)data->widget_changes.engineer_2_weapons->state  ) +
					(*(char*)data->widget_changes.engineer_2_silence_1->state) +
					(*(char*)data->widget_changes.engineer_2_silence_2->state) +
					(*(char*)data->widget_changes.engineer_2_recon->state    ) ;
		case 3:
			return	(*(char*)data->widget_changes.engineer_3_weapons->state  ) +
					(*(char*)data->widget_changes.engineer_3_silence_1->state) +
					(*(char*)data->widget_changes.engineer_3_silence_2->state) +
					(*(char*)data->widget_changes.engineer_3_recon->state    ) ;
	}
	return 0;
}

static long long is_in_box(Omni *data, GtkButton *btn, int box)
{
	GtkWidget *button = (GtkWidget*)btn;
	long long r = 0;
	switch(box){
		case 0: // West
			if(button == data->gui_elems.engineer_1_weapons_1) { r |= ENGINEER_1_WEAPONS_1; break; }
			if(button == data->gui_elems.engineer_1_silence  ) { r |= ENGINEER_1_SILENCE;   break; }
			if(button == data->gui_elems.engineer_1_recon    ) { r |= ENGINEER_1_RECON;     break; }
			if(button == data->gui_elems.engineer_recon_1    ) { r |= ENGINEER_RECON_1;     break; }
			if(button == data->gui_elems.engineer_nuclear_1  ) { r |= ENGINEER_NUCLEAR_1;   break; }
			if(button == data->gui_elems.engineer_nuclear_2  ) { r |= ENGINEER_NUCLEAR_2;   break; }
			break;
		case 1: // South
			if(button == data->gui_elems.engineer_2_weapons  ) { r |= ENGINEER_2_WEAPONS;   break; }
			if(button == data->gui_elems.engineer_2_silence_1) { r |= ENGINEER_2_SILENCE_1; break; }
			if(button == data->gui_elems.engineer_2_silence_2) { r |= ENGINEER_2_SILENCE_2; break; }
			if(button == data->gui_elems.engineer_recon_2    ) { r |= ENGINEER_RECON_2;     break; }
			if(button == data->gui_elems.engineer_weapons_1  ) { r |= ENGINEER_WEAPONS_1;   break; }
			if(button == data->gui_elems.engineer_nuclear_3  ) { r |= ENGINEER_NUCLEAR_3;   break; }
			break;
		case 2: // North
			if(button == data->gui_elems.engineer_3_weapons  ) { r |= ENGINEER_3_WEAPONS;   break; }
			if(button == data->gui_elems.engineer_3_silence_1) { r |= ENGINEER_3_SILENCE_1; break; }
			if(button == data->gui_elems.engineer_3_recon    ) { r |= ENGINEER_3_RECON;     break; }
			if(button == data->gui_elems.engineer_weapons_2  ) { r |= ENGINEER_WEAPONS_2;   break; }
			if(button == data->gui_elems.engineer_nuclear_4  ) { r |= ENGINEER_NUCLEAR_4;   break; }
			if(button == data->gui_elems.engineer_silence    ) { r |= ENGINEER_SILENCE;     break; }
			break;
		case 3: // East
			if(button == data->gui_elems.engineer_1_weapons_2) { r |= ENGINEER_1_WEAPONS_2; break; }
			if(button == data->gui_elems.engineer_2_recon    ) { r |= ENGINEER_2_RECON;     break; }
			if(button == data->gui_elems.engineer_3_silence_2) { r |= ENGINEER_3_SILENCE_2; break; }
			if(button == data->gui_elems.engineer_nuclear_5  ) { r |= ENGINEER_NUCLEAR_5;   break; }
			if(button == data->gui_elems.engineer_recon_3    ) { r |= ENGINEER_RECON_3;     break; }
			if(button == data->gui_elems.engineer_nuclear_6  ) { r |= ENGINEER_NUCLEAR_6;   break; }
			break;
	}
	return r;
}

static int pressed_count_box(Omni *data, int box){
	switch(box){
		case 0: // West
			return	(*(char*)data->widget_changes.engineer_1_weapons_1->state) +
					(*(char*)data->widget_changes.engineer_1_silence->state  ) +
					(*(char*)data->widget_changes.engineer_1_recon->state    ) +
					(*(char*)data->widget_changes.engineer_recon_1->state    ) +
					(*(char*)data->widget_changes.engineer_nuclear_1->state  ) +
					(*(char*)data->widget_changes.engineer_nuclear_2->state  ) ;
		case 1: // South
			return	(*(char*)data->widget_changes.engineer_2_weapons->state  ) +
					(*(char*)data->widget_changes.engineer_2_silence_1->state) +
					(*(char*)data->widget_changes.engineer_2_silence_2->state) +
					(*(char*)data->widget_changes.engineer_recon_2->state    ) +
					(*(char*)data->widget_changes.engineer_weapons_1->state  ) +
					(*(char*)data->widget_changes.engineer_nuclear_3->state  ) ;
		case 2: // North
			return	(*(char*)data->widget_changes.engineer_3_weapons->state  ) +
					(*(char*)data->widget_changes.engineer_3_silence_1->state) +
					(*(char*)data->widget_changes.engineer_3_recon->state    ) +
					(*(char*)data->widget_changes.engineer_weapons_2->state  ) +
					(*(char*)data->widget_changes.engineer_nuclear_4->state  ) +
					(*(char*)data->widget_changes.engineer_silence->state    ) ;
		case 3: // East
			return	(*(char*)data->widget_changes.engineer_1_weapons_2->state) +
					(*(char*)data->widget_changes.engineer_2_recon->state    ) +
					(*(char*)data->widget_changes.engineer_3_silence_2->state) +
					(*(char*)data->widget_changes.engineer_nuclear_5->state  ) +
					(*(char*)data->widget_changes.engineer_recon_3->state    ) +
					(*(char*)data->widget_changes.engineer_nuclear_6->state  ) ;
	}
	return 0;
}

static int is_nuclear(Omni *data, GtkButton *btn)
{
	GtkWidget *button = (GtkWidget*)btn;
	return	(button == data->gui_elems.engineer_nuclear_1) ||
			(button == data->gui_elems.engineer_nuclear_2) ||
			(button == data->gui_elems.engineer_nuclear_3) ||
			(button == data->gui_elems.engineer_nuclear_4) ||
			(button == data->gui_elems.engineer_nuclear_5) ||
			(button == data->gui_elems.engineer_nuclear_6) ;
}

static int pressed_count_nuclear(Omni *data)
{
	return	(*(char*)data->widget_changes.engineer_nuclear_1->state) +
			(*(char*)data->widget_changes.engineer_nuclear_2->state) +
			(*(char*)data->widget_changes.engineer_nuclear_3->state) +
			(*(char*)data->widget_changes.engineer_nuclear_4->state) +
			(*(char*)data->widget_changes.engineer_nuclear_5->state) +
			(*(char*)data->widget_changes.engineer_nuclear_6->state) ;
}

static void set_states(Omni *data, long long elems, void *value, size_t len)
{
	for(int i = 0; i < NUM_GUI_ELEMS; ++i){
		if(elems & (1LL << i)){
			switch (1LL << i) {
				case ENGINEER_1_WEAPONS_1: memcpy(data->widget_changes.engineer_1_weapons_1->state, value, len); break;
				case ENGINEER_1_WEAPONS_2: memcpy(data->widget_changes.engineer_1_weapons_2->state, value, len); break;
				case ENGINEER_1_SILENCE  : memcpy(data->widget_changes.engineer_1_silence  ->state, value, len); break;
				case ENGINEER_1_RECON    : memcpy(data->widget_changes.engineer_1_recon    ->state, value, len); break;
				case ENGINEER_2_SILENCE_1: memcpy(data->widget_changes.engineer_2_silence_1->state, value, len); break;
				case ENGINEER_2_SILENCE_2: memcpy(data->widget_changes.engineer_2_silence_2->state, value, len); break;
				case ENGINEER_2_WEAPONS  : memcpy(data->widget_changes.engineer_2_weapons  ->state, value, len); break;
				case ENGINEER_2_RECON    : memcpy(data->widget_changes.engineer_2_recon    ->state, value, len); break;
				case ENGINEER_3_RECON    : memcpy(data->widget_changes.engineer_3_recon    ->state, value, len); break;
				case ENGINEER_3_SILENCE_1: memcpy(data->widget_changes.engineer_3_silence_1->state, value, len); break;
				case ENGINEER_3_SILENCE_2: memcpy(data->widget_changes.engineer_3_silence_2->state, value, len); break;
				case ENGINEER_3_WEAPONS  : memcpy(data->widget_changes.engineer_3_weapons  ->state, value, len); break;
				case ENGINEER_RECON_1    : memcpy(data->widget_changes.engineer_recon_1    ->state, value, len); break;
				case ENGINEER_RECON_2    : memcpy(data->widget_changes.engineer_recon_2    ->state, value, len); break;
				case ENGINEER_RECON_3    : memcpy(data->widget_changes.engineer_recon_3    ->state, value, len); break;
				case ENGINEER_WEAPONS_1  : memcpy(data->widget_changes.engineer_weapons_1  ->state, value, len); break;
				case ENGINEER_WEAPONS_2  : memcpy(data->widget_changes.engineer_weapons_2  ->state, value, len); break;
				case ENGINEER_SILENCE    : memcpy(data->widget_changes.engineer_silence    ->state, value, len); break;
				case ENGINEER_NUCLEAR_1  : memcpy(data->widget_changes.engineer_nuclear_1  ->state, value, len); break;
				case ENGINEER_NUCLEAR_2  : memcpy(data->widget_changes.engineer_nuclear_2  ->state, value, len); break;
				case ENGINEER_NUCLEAR_3  : memcpy(data->widget_changes.engineer_nuclear_3  ->state, value, len); break;
				case ENGINEER_NUCLEAR_4  : memcpy(data->widget_changes.engineer_nuclear_4  ->state, value, len); break;
				case ENGINEER_NUCLEAR_5  : memcpy(data->widget_changes.engineer_nuclear_5  ->state, value, len); break;
				case ENGINEER_NUCLEAR_6  : memcpy(data->widget_changes.engineer_nuclear_6  ->state, value, len); break;
			}
		}
	}
}

static void apply_to_widgets(Omni *data, long long elems, void (*fp)(GtkWidget*, gboolean), gboolean value)
{
	for(int i = 0; i < NUM_GUI_ELEMS; ++i){
		if(elems & (1LL << i)){
			switch (1LL << i) {
				case ENGINEER_1_WEAPONS_1: fp(data->gui_elems.engineer_1_weapons_1, value); break;
				case ENGINEER_1_WEAPONS_2: fp(data->gui_elems.engineer_1_weapons_2, value); break;
				case ENGINEER_1_SILENCE  : fp(data->gui_elems.engineer_1_silence  , value); break;
				case ENGINEER_1_RECON    : fp(data->gui_elems.engineer_1_recon    , value); break;
				case ENGINEER_2_SILENCE_1: fp(data->gui_elems.engineer_2_silence_1, value); break;
				case ENGINEER_2_SILENCE_2: fp(data->gui_elems.engineer_2_silence_2, value); break;
				case ENGINEER_2_WEAPONS  : fp(data->gui_elems.engineer_2_weapons  , value); break;
				case ENGINEER_2_RECON    : fp(data->gui_elems.engineer_2_recon    , value); break;
				case ENGINEER_3_RECON    : fp(data->gui_elems.engineer_3_recon    , value); break;
				case ENGINEER_3_SILENCE_1: fp(data->gui_elems.engineer_3_silence_1, value); break;
				case ENGINEER_3_SILENCE_2: fp(data->gui_elems.engineer_3_silence_2, value); break;
				case ENGINEER_3_WEAPONS  : fp(data->gui_elems.engineer_3_weapons  , value); break;
				case ENGINEER_RECON_1    : fp(data->gui_elems.engineer_recon_1    , value); break;
				case ENGINEER_RECON_2    : fp(data->gui_elems.engineer_recon_2    , value); break;
				case ENGINEER_RECON_3    : fp(data->gui_elems.engineer_recon_3    , value); break;
				case ENGINEER_WEAPONS_1  : fp(data->gui_elems.engineer_weapons_1  , value); break;
				case ENGINEER_WEAPONS_2  : fp(data->gui_elems.engineer_weapons_2  , value); break;
				case ENGINEER_SILENCE    : fp(data->gui_elems.engineer_silence    , value); break;
				case ENGINEER_NUCLEAR_1  : fp(data->gui_elems.engineer_nuclear_1  , value); break;
				case ENGINEER_NUCLEAR_2  : fp(data->gui_elems.engineer_nuclear_2  , value); break;
				case ENGINEER_NUCLEAR_3  : fp(data->gui_elems.engineer_nuclear_3  , value); break;
				case ENGINEER_NUCLEAR_4  : fp(data->gui_elems.engineer_nuclear_4  , value); break;
				case ENGINEER_NUCLEAR_5  : fp(data->gui_elems.engineer_nuclear_5  , value); break;
				case ENGINEER_NUCLEAR_6  : fp(data->gui_elems.engineer_nuclear_6  , value); break;
			}
		}
	}
}

int handle_engineering(GtkButton *button, gpointer user_data)
{
	Omni *data = user_data;
	
	/*
	for each subsystem:
		test if button is part of subsystem
		if yes:
			test if it's the last one to be pressed
			if yes:
				add state and reset all others in susbsystem
				return;
			else:
				break; // since it's not going to be part of another subsytem
	for each box (direction):
		test if button is part of box
		if yes:
			test if it's the last one to be pressed
			if yes:
				add state and reset all the others in box
				return;
			if no:
				test if it's a nuclear
				if yes:
					test if it's the last nuclear to be pressed
					if yes:
						add state and reset all the other nuclears [damage]
						return;
				add state and set just that specific button
				return;
	error since this means a button was pressed that was not in any box
	return;
	*/
	
	long long widget;
	for(int i = 1; i <= 3; ++i){
		if((widget = is_in_subsystem(data, button, i))){
			if(pressed_count_subsystem(data, i) <= 1){
				// add state for and reset all others
				long long widgets = (i == 1 ? ENGINEERING_SUBSYSTEM_1 :
									(i == 2 ? ENGINEERING_SUBSYSTEM_2 :
											  ENGINEERING_SUBSYSTEM_3 )) ^ widget;
				add_state(data, widgets);
				char c = 1;
				set_states(data, widgets, &c, sizeof(char));
				apply_to_widgets(data, widgets, gtk_widget_set_sensitive, TRUE);
				return 1;
			}else{
				break;
			}
		}
	}
	for(int i = 0; i < 4; ++i){
		if((widget = is_in_box(data, button, i))){
			if(pressed_count_box(data, i) <= 1){
				// add state for and reset all others
				long long widgets = (i == 0 ? ENGINEERING_BOX_W :
									(i == 1 ? ENGINEERING_BOX_N :
									(i == 2 ? ENGINEERING_BOX_S :
											  ENGINEERING_BOX_E ))) ^ widget;
				add_state(data, widgets);
				char c = 1;
				set_states(data, widgets, &c, sizeof(char));
				apply_to_widgets(data, widgets, gtk_widget_set_sensitive, TRUE);
				return 1;
			}else{
				if(is_nuclear(data, button)){
					if(pressed_count_nuclear(data) <= 1){
						// add state for and reset all other nuclears
						long long widgets = ENGINEERING_NUCLEARS ^ widget;
						add_state(data, widgets);
						char c = 1;
						set_states(data, widgets, &c, sizeof(char));
						apply_to_widgets(data, widgets, gtk_widget_set_sensitive, TRUE);
						return 1;
					}
				}
				// add state for and set insensitive just that specific button
				add_state(data, widget);
				char c = 0;
				set_states(data, widget, &c, sizeof(char));
				gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
				return 1;
			}
		}
	}
	// error: function called by unknown button
	error_popup(data, "Error: handle_engineering function called by unknown button. This is a bug! The button was located at %p", button);
	
	return 1;
}

int handle_engineer_surface(__attribute__ ((unused)) GtkButton *button, gpointer user_data)
{
	Omni *data = user_data;
	
	long long widgets = 0;
	
	if(*(char*)data->widget_changes.engineer_1_weapons_1->state == 0) { widgets |= ENGINEER_1_WEAPONS_1; }
	if(*(char*)data->widget_changes.engineer_1_silence  ->state == 0) { widgets |= ENGINEER_1_SILENCE;   }
	if(*(char*)data->widget_changes.engineer_1_recon    ->state == 0) { widgets |= ENGINEER_1_RECON;     }
	if(*(char*)data->widget_changes.engineer_recon_1    ->state == 0) { widgets |= ENGINEER_RECON_1;     }
	if(*(char*)data->widget_changes.engineer_nuclear_1  ->state == 0) { widgets |= ENGINEER_NUCLEAR_1;   }
	if(*(char*)data->widget_changes.engineer_nuclear_2  ->state == 0) { widgets |= ENGINEER_NUCLEAR_2;   }
	if(*(char*)data->widget_changes.engineer_2_weapons  ->state == 0) { widgets |= ENGINEER_2_WEAPONS;   }
	if(*(char*)data->widget_changes.engineer_2_silence_1->state == 0) { widgets |= ENGINEER_2_SILENCE_1; }
	if(*(char*)data->widget_changes.engineer_2_silence_2->state == 0) { widgets |= ENGINEER_2_SILENCE_2; }
	if(*(char*)data->widget_changes.engineer_recon_2    ->state == 0) { widgets |= ENGINEER_RECON_2;     }
	if(*(char*)data->widget_changes.engineer_weapons_1  ->state == 0) { widgets |= ENGINEER_WEAPONS_1;   }
	if(*(char*)data->widget_changes.engineer_nuclear_3  ->state == 0) { widgets |= ENGINEER_NUCLEAR_3;   }
	if(*(char*)data->widget_changes.engineer_3_weapons  ->state == 0) { widgets |= ENGINEER_3_WEAPONS;   }
	if(*(char*)data->widget_changes.engineer_3_silence_1->state == 0) { widgets |= ENGINEER_3_SILENCE_1; }
	if(*(char*)data->widget_changes.engineer_3_recon    ->state == 0) { widgets |= ENGINEER_3_RECON;     }
	if(*(char*)data->widget_changes.engineer_weapons_2  ->state == 0) { widgets |= ENGINEER_WEAPONS_2;   }
	if(*(char*)data->widget_changes.engineer_nuclear_4  ->state == 0) { widgets |= ENGINEER_NUCLEAR_4;   }
	if(*(char*)data->widget_changes.engineer_silence    ->state == 0) { widgets |= ENGINEER_SILENCE;     }
	if(*(char*)data->widget_changes.engineer_1_weapons_2->state == 0) { widgets |= ENGINEER_1_WEAPONS_2; }
	if(*(char*)data->widget_changes.engineer_2_recon    ->state == 0) { widgets |= ENGINEER_2_RECON;     }
	if(*(char*)data->widget_changes.engineer_3_silence_2->state == 0) { widgets |= ENGINEER_3_SILENCE_2; }
	if(*(char*)data->widget_changes.engineer_nuclear_5  ->state == 0) { widgets |= ENGINEER_NUCLEAR_5;   }
	if(*(char*)data->widget_changes.engineer_recon_3    ->state == 0) { widgets |= ENGINEER_RECON_3;     }
	if(*(char*)data->widget_changes.engineer_nuclear_6  ->state == 0) { widgets |= ENGINEER_NUCLEAR_6;   }
	
	add_state(data, widgets);
	char c = 1;
	set_states(data, widgets, &c, sizeof(char));
	apply_to_widgets(data, widgets, gtk_widget_set_sensitive, TRUE);
	
	return 1;
}
