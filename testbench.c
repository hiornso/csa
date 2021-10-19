#include <stdio.h>
#include <sys/time.h>
#include <gtk/gtk.h>

#include "main.h"
#include "tracker.h"
#include "accelerated.h"
#include "csa_error.h"

#define OUTPUT_PNG 0

#define ITERS  50
#define WIDTH  1000
#define HEIGHT 1000

void error_popup(__attribute__ ((unused)) Omni *data, __attribute__ ((unused)) char *fmt, ...){}
void free_linked_changes(__attribute__ ((unused)) ChangeListNode *cln){}
void free_linked_states(__attribute__ ((unused)) StateListNode *sln){}

int main()
{
	gtk_init();
	GtkWidget *dummy = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	
	Omni data;
	data.gui_elems.radio_engineer_drawing_area = (void*)((long long)NULL + 1LL); // to make draw select captain tracker
	data.gui_elems.captain_drawing_area = dummy; // make it do course plotter, we don't care about the rendering of the dots, just the map
	
	if(init_tracker(&data, "maps/testbench.lua", FROM_FILE, TRUE)){ // make a "captain tracker"
		csa_error("an error occurred in init_tracker.\n");
		return -1;
	}
	
	int width = WIDTH;
	int height = HEIGHT;
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *cr = cairo_create(surface);
	cairo_save(cr);
	
	struct timeval start, end;
	gettimeofday(&start, NULL);
	for(int i = 0; i < ITERS; ++i){
		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);
		cairo_restore(cr);
		cairo_save(cr);
		
		draw((GtkDrawingArea*)dummy, cr, width, height, &data);
		data.captain_tracker->mrc.old_res = -1;
	}
	gettimeofday(&end, NULL);
	long long usec = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
	double sec = (double)usec / 1.0e6;
	printf("Generated %i frames in %f seconds (average of %f fps)\n",ITERS,sec,ITERS/sec);
	
#if OUTPUT_PNG
	if(cairo_surface_write_to_png(surface, "testbench.png") != CAIRO_STATUS_SUCCESS){
		csa_error("an error occurred while writing the PNG.\n");
	}
#endif
	
	cairo_surface_destroy(surface);
	cairo_destroy(cr);
	
	return 0;
}
