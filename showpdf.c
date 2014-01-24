#include <stdlib.h>
#include <poppler.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#define MODE_HEIGHT 1
#define MODE_WIDTH  2
#define MODE_FIT    3

PopplerDocument *doc;
PopplerPage *page;
int pages, current = 0;
char buffer[1024];
double page_width, page_height;
int mode = MODE_HEIGHT;

void on_destroy(GtkWidget *w, gpointer data) {
    gtk_main_quit();
}

gboolean on_expose(GtkWidget *w, GdkEventExpose *e, gpointer data) {
    gtk_widget_queue_draw(w);
    gint win_width, win_height;
    gtk_window_get_size(GTK_WINDOW(w), &win_width, &win_height);
    double scalex = 1, scaley = 1;
    switch (mode) {
        case MODE_HEIGHT: scalex = scaley = win_height / page_height; break;
        case MODE_WIDTH:  scalex = scaley  = win_width  / page_width;  break;
        case MODE_FIT:
            scalex = win_width / page_width;
            scaley = win_height / page_height;
            break;
    }
    cairo_t *cr = gdk_cairo_create(w->window);
    cairo_scale(cr, scalex, scaley);
    poppler_page_render(page, cr);
    cairo_destroy(cr);
    return FALSE;
}

gboolean on_keypress(GtkWidget *w, GdkEvent *e, gpointer data) {
    switch (e->key.keyval) {
        case GDK_Page_Down: current += (current + 1 < pages) ? 1 : 0; break;
        case GDK_Page_Up: current -= (current > 0) ? 1 : 0; break;
        case GDK_Down: current += (current + 1 < pages) ? 1 : 0; break;
        case GDK_Up: current -= (current > 0) ? 1 : 0; break;
        case GDK_Left: current -= (current > 0) ? 1 : 0; break;
        case GDK_Right: current += (current + 1 < pages) ? 1 : 0; break;
        case GDK_Home: current = 0; break;
        case GDK_End: current = pages - 1; break;
        case GDK_space: current += (current + 1 < pages) ? 1 : 0; break;
        case GDK_w: mode = MODE_WIDTH; break;
        case GDK_h: mode = MODE_HEIGHT; break;
        case GDK_f: mode = MODE_FIT; break;
    }
    g_object_unref(page);
    page = poppler_document_get_page(doc, current);
    if (!page) {
        puts("Could not open first page of document");
        g_object_unref(page);
        exit(EXIT_FAILURE);
    }
    poppler_page_get_size(page, &page_width, &page_height);
    on_expose(w, NULL, NULL);
    return FALSE;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s FILE\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    gtk_init(&argc, &argv);
    GError *err = NULL;
    gchar *filename = argv[1], *absolute;
    if (g_path_is_absolute(filename)) {
        absolute = g_strdup (filename);
    } else {
        gchar *dir = g_get_current_dir ();
        absolute = g_build_filename (dir, filename, (gchar *) 0);
        free (dir);
    }
    gchar *uri = g_filename_to_uri (absolute, NULL, &err);
    free (absolute);
    if (uri == NULL) {
        printf("poppler fail: %s\n", err->message);
        exit(EXIT_FAILURE);
    }
    doc = poppler_document_new_from_file(uri, NULL, &err);
    if (!doc) {
        puts(err->message);
        g_object_unref(doc);
        exit(EXIT_FAILURE);
    }
    page = poppler_document_get_page(doc, current);
    if (!page) {
        puts("Could not open first page of document");
        g_object_unref(page);
        exit(EXIT_FAILURE);
    }
    poppler_page_get_size(page, &page_width, &page_height);
    pages = poppler_document_get_n_pages(doc);
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(on_destroy), NULL);
    g_signal_connect(G_OBJECT(win), "expose-event", G_CALLBACK(on_expose), NULL);
    g_signal_connect(G_OBJECT(win), "key-press-event", G_CALLBACK(on_keypress), NULL);
    gtk_widget_set_app_paintable(win, TRUE);
    gtk_widget_show_all(win);
    gtk_main();
    g_object_unref(page);
    g_object_unref(doc);
}