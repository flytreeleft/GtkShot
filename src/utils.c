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

#include <stdlib.h>
#include <unistd.h>

#include <glib/gi18n.h>

#include "utils.h"

#define FILE_NAME ("gtkshot")

void parse_to_gdk_color(gint color, GdkColor *c) {
  char *color_str;
  color_str = g_strdup_printf("#%02x%02x%02x"
                                , RGB_R(color)
                                , RGB_G(color)
                                , RGB_B(color)
                                , NULL);
  gdk_color_parse(color_str, c);
  g_free(color_str);
}

/* source from gpicview-0.2.2 START */
static void on_file_save_filter_changed(GObject* obj, GParamSpec* pspec
                                          , gpointer user_data) {
  GtkFileChooser *dialog = (GtkFileChooser*) obj;
  GtkFileFilter *filter = gtk_file_chooser_get_filter(dialog);
  const char *type = (const char*) g_object_get_data(G_OBJECT(filter), "type");
  char *name = NULL;

  name = g_strdup_printf("%s.%s", FILE_NAME, type, NULL);
#ifdef GTK_SHOT_DEBUG
  debug("choose type: %s, file name: %s\n", type, name);
#endif
  gtk_file_chooser_set_current_name(dialog, name);
  g_free(name);
}

gchar* choose_and_get_filename(GtkWindow *parent, char **type) {
  gchar *filename = NULL;
  GtkFileChooser *dialog = (GtkFileChooser*)
    gtk_file_chooser_dialog_new(NULL, parent
                                  , GTK_FILE_CHOOSER_ACTION_SAVE
                                  , GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
                                  , GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
  g_signal_connect(dialog, "notify::filter", G_CALLBACK(on_file_save_filter_changed), NULL);

  GSList *modules, *module;
  GtkFileFilter *filter, *default_filter;

  modules = gdk_pixbuf_get_formats();
  for (module = modules; module; module = module->next) {
    char *name, *desc, *tmp;
    char **exts, **mimes, **mime;
    GdkPixbufFormat *format = (GdkPixbufFormat*) module->data;

    if (!gdk_pixbuf_format_is_writable(format)) {
      continue;
    }
    name = gdk_pixbuf_format_get_name(format);
    // ico的尺寸不能太大,故直接去除
    if (strncmp(name, "ico", 3) == 0) {
      continue;
    }

    filter = gtk_file_filter_new();
    desc = gdk_pixbuf_format_get_description(format);
    exts = gdk_pixbuf_format_get_extensions(format);
    mimes = gdk_pixbuf_format_get_mime_types(format);
    tmp = g_strdup_printf("%s (*.%s)", desc, exts[0], NULL);

    g_object_set_data_full(G_OBJECT(filter), "type", name, (GDestroyNotify) g_free);
    g_strfreev(exts);
    g_free(desc);
    gtk_file_filter_set_name(filter, tmp);
    g_free(tmp);

    for (mime = mimes; *mime ; ++mime) {
      gtk_file_filter_add_mime_type(filter, *mime);
    }
    g_strfreev(mimes);
    // PNG为默认存储格式
    if (strncmp(name, "png", 3) == 0) {
      default_filter = filter;
    }
    gtk_file_chooser_add_filter(dialog, filter);
  }
  g_slist_free(modules);

  gtk_file_chooser_set_filter(dialog, default_filter);
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filter = gtk_file_chooser_get_filter(dialog);
    filename = g_locale_from_utf8(gtk_file_chooser_get_filename(dialog)
                                      , -1, NULL, NULL, NULL);
    *type = g_object_steal_data(G_OBJECT(filter), "type");
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));

  return filename;
}

GtkWidget* create_icon_button(const char *icon, const char *tip
                                , GCallback cb, gboolean toggle
                                , gpointer data) {
  GtkWidget *img;
  if (g_str_has_prefix(icon, "gtk-")) {
    img = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_SMALL_TOOLBAR);
  } else {
    img = gtk_image_new_from_icon_name(icon, GTK_ICON_SIZE_SMALL_TOOLBAR);
  }
  return create_image_button(img, tip, cb, toggle, data);
}

GtkWidget* create_image_button(GtkWidget *img, const char *tip
                                  , GCallback cb, gboolean toggle
                                  , gpointer data) {
  GtkWidget *btn;
  if (G_UNLIKELY(toggle)) {
    btn = gtk_toggle_button_new();
    g_signal_connect(btn, "toggled", cb, data);
  } else {
    btn = gtk_button_new();
    g_signal_connect(btn, "clicked", cb, data);
  }
  gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click(GTK_BUTTON(btn), FALSE);
  gtk_container_add(GTK_CONTAINER(btn), img);
  gtk_widget_set_tooltip_text(btn, tip);

  return btn;
}
/* source from gpicview-0.2.2 END */

GtkWidget* create_xpm_button(const char **xpm, const char *tip
                                , GCallback cb, gboolean toggle
                                , gpointer data) {
  GtkWidget *img = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data(xpm));
  return create_image_button(img, tip, cb, toggle, data);
}

GtkWidget* create_color_button(gint color, gint width, gint height
                                  , const char *tip
                                  , GCallback cb, gboolean toggle
                                  , gpointer data) {
  GdkPixbuf *pixbuf, *img;

  pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
  gdk_pixbuf_fill(pixbuf, color << 8); // 此处的颜色应为RRGGBBAA形式

  img = gtk_image_new_from_pixbuf(pixbuf);
            
  g_object_unref(pixbuf);

  return create_image_button(img, tip, cb, toggle, data);
}

void popup_message_dialog(GtkWindow *parent, const char *msg) {
  GtkWidget *msg_dlg = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL
                                                , GTK_MESSAGE_ERROR
                                                , GTK_BUTTONS_OK
                                                , "%s", msg);
  gtk_dialog_run(GTK_DIALOG(msg_dlg));
  gtk_widget_destroy(msg_dlg);
}

GdkPixbuf* get_screen_pixbuf(gint x, gint y, gint width, gint height) {
  GdkWindow *root = gdk_get_default_root_window();
  return gdk_pixbuf_get_from_drawable(NULL, root, NULL
                                          , x, y, 0, 0
                                          , width, height);
}
