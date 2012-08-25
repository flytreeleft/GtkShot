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

#ifndef _GTK_SHOT_UTILS_H_
#define _GTK_SHOT_UTILS_H_

#include <gtk/gtk.h>

#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX
  #define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
  #define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef ABS
  #define ABS(x) ((x) < 0 ? -(x) : (x))
#endif
#define RGB(r, g, b) ( ((r) << 16) | ((g) << 8) | ((b) << 0) )
#define RGB_R(rgb) ( ((rgb) & 0xff0000) >> 16 )
#define RGB_G(rgb) ( ((rgb) & 0x00ff00) >> 8 )
#define RGB_B(rgb) ( ((rgb) & 0x0000ff) >> 0 )
/** cairo的RGB颜色值为0 ~ 1, 需对R/G/B值分别除以255.0 */
#define SET_CAIRO_RGB(cr, rgb) \
          cairo_set_source_rgb(cr, RGB_R(rgb) / 255.0 \
                                  , RGB_G(rgb) / 255.0 \
                                  , RGB_B(rgb) / 255.0)
#define SET_CAIRO_RGBA(cr, rgb, a) \
          cairo_set_source_rgba(cr, RGB_R(rgb) / 255.0 \
                                  , RGB_G(rgb) / 255.0 \
                                  , RGB_B(rgb) / 255.0, a)

/** 在BOX中加入控件,并取消控件的可聚焦性,防止按钮上出现TAB键切换聚焦的虚线 */
#define pack_to_box(box, thing) \
        do { \
          gtk_widget_set_can_focus(GTK_WIDGET(thing), FALSE); \
          gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(thing), FALSE, FALSE, 0); \
        } while(0);
void parse_to_gdk_color(gint color, GdkColor *c);
#define parse_gdk_color(c) \
          RGB((c).red & 0xff, (c).green & 0xff, (c).blue & 0xff)
gchar* choose_and_get_filename(GtkWindow *parent, char **type
                                  , const char *path);
GtkWindow* create_popup_window(GtkWindow *parent
                                  , gint width, gint height);
GtkWidget* create_icon_button(const char *icon, const char *tip
                                , GCallback cb, gboolean toggle
                                , gpointer data);
GtkWidget* create_image_button(GtkWidget *img, const char *tip
                                  , GCallback cb, gboolean toggle
                                  , gpointer data);
GtkWidget* create_xpm_button(const char **xpm, const char *tip
                                , GCallback cb, gboolean toggle
                                , gpointer data);
GtkWidget* create_color_button(gint color, gint width, gint height
                                  , const char *tip
                                  , GCallback cb, gboolean toggle
                                  , gpointer data);
void popup_message_dialog(GtkWindow *parent, const char *msg);
void save_pixbuf_to_clipboard(GdkPixbuf *pixbuf);
void set_all_toggle_button_inactive(GList *list);
GtkToggleButton*
    get_active_toggle_button_except(GList *list
                                      , GtkToggleButton *btn);
void cairo_draw_text(cairo_t *cr, char *text, char *fontname);
void cairo_round_rect(cairo_t *cr, gfloat x, gfloat y
                                  , gfloat width, gfloat height
                                  , gfloat radius);
/**
 * 仅设置Toggle按钮的激活状态,但不产生激活事件
 */
#define gtk_toggle_button_mark_active(btn, act) \
          do { \
            GTK_BUTTON(btn)->depressed = (act); \
            GTK_TOGGLE_BUTTON(btn)->active = (act); \
          } while(0)
#define gdk_point_is_equal(p0, p1) \
          ((p0).x == (p1).x && (p0).y == (p1).y)
#define gdk_point_assign(left, right) \
          do { \
            (left).x = (right).x; (left).y = (right).y; \
          } while(0)

#ifdef __cplusplus
}
#endif

#endif
