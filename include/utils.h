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
/**
 * cairo在的RGB颜色值设置时需为浮点型,
 * 类型转换时会出现精度损失,造成所绘颜色的不一致,
 * 需对R/G/B值分别除以255.0
 */
#define RGB(r, g, b) ( ((r) << 16) | ((g) << 8) | ((b) << 0) )
#define RGB_R(rgb) ( ((rgb) & 0xff0000) >> 16 )
#define RGB_G(rgb) ( ((rgb) & 0x00ff00) >> 8 )
#define RGB_B(rgb) ( ((rgb) & 0x0000ff) >> 0 )

#define pack_to_box(box, thing) \
          gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(thing), FALSE, FALSE, 0)
void parse_to_gdk_color(gint color, GdkColor *c);
#define parse_gdk_color(c) \
          RGB((c).red & 0xff, (c).green & 0xff, (c).blue & 0xff)
gchar* choose_and_get_filename(GtkWindow *parent, char **type);
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
GdkPixbuf* get_screen_pixbuf(gint x, gint y, gint width, gint height);
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
