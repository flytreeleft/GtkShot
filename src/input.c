/*
 * GtkShot - A screen capture programme using GtkLib
 * Copyright (C) 2012 flytreeleft @ CrazyDan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>

#include <gtk/gtk.h>

#include "utils.h"

#include "shot.h"
#include "input.h"

GtkShotInput* gtk_shot_input_new(GtkShot *shot) {
  gint width = 300, height = 80;
  GtkShotInput *input = g_new(GtkShotInput, 1);
  GtkWindow *window =
      create_popup_window(GTK_WINDOW(shot), width, height);

  gtk_container_set_border_width(GTK_CONTAINER(window), 0);

  GtkAlignment *align =
                GTK_ALIGNMENT(gtk_alignment_new(0, 0, 0, 0));
  gtk_alignment_set_padding(align, 0, 1, 0, 1);

  GtkScrolledWindow *scroll
        = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
  gtk_scrolled_window_set_shadow_type(scroll, GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(scroll, GTK_POLICY_ALWAYS
                                        , GTK_POLICY_ALWAYS);

  GtkTextView *view = GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_widget_set_size_request(GTK_WIDGET(view), width, height);

  gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(view));
  gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(scroll));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(align));

  input->window = window;
  input->view = view;
  return input;
}

void gtk_shot_input_destroy(GtkShotInput *input) {
  g_return_if_fail(input != NULL);

  gtk_widget_destroy(GTK_WIDGET(input->window));
  g_free(input);
}

void gtk_shot_input_show_at(GtkShotInput *input, gint x, gint y) {
  g_return_if_fail(input != NULL);

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(input->view);
  gtk_text_buffer_set_text(buffer, "", -1);

  gtk_widget_show_all(GTK_WIDGET(input->window));
  gtk_window_move(input->window, x, y - SYSTEM_CURSOR_SIZE / 2);
  gdk_keyboard_grab(GTK_WIDGET(input->window)->window
                        , FALSE, GDK_CURRENT_TIME);
}

void gtk_shot_input_hide(GtkShotInput *input) {
  if (gtk_shot_input_visible(input)) {
    gtk_widget_hide_all(GTK_WIDGET(input->window));
  }
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
  if (!gtk_shot_input_visible(input)) return NULL;

  GtkTextBuffer *buffer;
  GtkTextIter start, end;

  buffer = gtk_text_view_get_buffer(input->view);
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  return g_strstrip(gtk_text_buffer_get_text(buffer
                                                , &start, &end
                                                , FALSE));
}
