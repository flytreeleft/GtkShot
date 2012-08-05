#include <config.h>

#include <gtk/gtk.h>

#include "utils.h"

#include "input.h"

// Events
static gboolean on_input_window_expose(GtkWidget *widget
                                        , gpointer data);
static void on_input_window_show(GtkWidget *widget
                                      , gpointer data);

GtkShotInput* gtk_shot_input_new(GtkWindow *parent) {
  GtkShotInput *input = g_new(GtkShotInput, 1);

  GtkWindow *window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_transient_for(window, parent);
  gtk_widget_set_can_focus(GTK_WIDGET(window), TRUE);
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
  gtk_window_set_resizable(window, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(window), 0);
  /*gtk_widget_set_app_paintable(GTK_WIDGET(window), TRUE);
  // 背景可透明
  GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(window));
  GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);
  gtk_widget_set_colormap(GTK_WIDGET(window), colormap);
  g_signal_connect(G_OBJECT(window), "expose_event"
                      , G_CALLBACK(on_input_window_expose)
                      , NULL);*/
  /*g_signal_connect(G_OBJECT(window), "show"
                      , G_CALLBACK(on_input_window_show)
                      , NULL);*/

  GtkAlignment *align = GTK_ALIGNMENT(gtk_alignment_new(0, 0, 0, 0));
  gtk_alignment_set_padding(align, 0, 1, 0, 1);

  GtkScrolledWindow *scroll
        = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
  gtk_scrolled_window_set_shadow_type(scroll, GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(scroll, GTK_POLICY_ALWAYS
                                        , GTK_POLICY_ALWAYS);

  GtkTextView *view = GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_widget_set_size_request(GTK_WIDGET(view), 300, 80);

  gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(view));
  gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(scroll));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(align));

  input->window = window;
  input->view = view;
  return input;
}

void gtk_shot_input_show(GtkShotInput *input, gint x, gint y) {
  g_return_if_fail(input != NULL);

  GtkTextBuffer *buffer;
  buffer = gtk_text_view_get_buffer(input->view);
  gtk_text_buffer_set_text(buffer, "", -1);

  gtk_widget_show_all(GTK_WIDGET(input->window));
  gtk_window_move(input->window, x, y - SYSTEM_CURSOR_SIZE / 2);
}

void gtk_shot_input_hide(GtkShotInput *input) {
  g_return_if_fail(input != NULL);
  gtk_widget_hide_all(GTK_WIDGET(input->window));
}

void gtk_shot_input_set_font(GtkShotInput *input
                                  , const char *fontname
                                  , gint color) {
  g_return_if_fail(input != NULL);
#ifdef GTK_SHOT_DEBUG
  debug("font(%s), color(%x)\n", fontname, color);
#endif
  if (fontname) {
    PangoFontDescription *font_desc;

    font_desc = pango_font_description_from_string(fontname);
    gtk_widget_modify_font(GTK_WIDGET(input->view), font_desc);
    pango_font_description_free(font_desc);
  }
  GdkColor c;
  parse_to_gdk_color(color, &c);
  gtk_widget_modify_text(GTK_WIDGET(input->view), GTK_STATE_NORMAL, &c);
}

gchar* gtk_shot_input_get_text(GtkShotInput *input) {
  g_return_if_fail(input != NULL);

  GtkTextBuffer *buffer;
  GtkTextIter start, end;

  buffer = gtk_text_view_get_buffer(input->view);
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  return g_strstrip(gtk_text_buffer_get_text(buffer, &start, &end, FALSE));
}

gboolean on_input_window_expose(GtkWidget *widget, gpointer data) {
  cairo_t *cr;
  gint width, height;

  gtk_widget_get_size_request(widget, &width, &height);

  cr = gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);

  cairo_destroy(cr);

  return TRUE;
}

/**
 * For POPUP window's FOCUS
 * Reference:
 * http://stackoverflow.com/questions/1925568/how-to-give-keyboard-focus-to-a-pop-up-gtk-window
 */
void on_input_window_show(GtkWidget *widget, gpointer data) {
  /* grabbing might not succeed immediately */
  while (gdk_keyboard_grab(widget->window, FALSE, GDK_CURRENT_TIME)
          != GDK_GRAB_SUCCESS) {
    /* wait a while and try again */
    sleep(0.1);
  }
}

