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

#ifndef _GTK_SHOT_PEN_H_
#define _GTK_SHOT_PEN_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GtkShotPen GtkShotPen;
typedef enum _GtkShotPenType GtkShotPenType;

/* The color of pen */
#define GTK_SHOT_DEFAULT_PEN_COLOR 0xff0000
/* The font name of pen */
#define GTK_SHOT_DEFAULT_PEN_FONT "Sans 10"
/* The size of pen */
#define GTK_SHOT_DEFAULT_PEN_SIZE 2

#define GTK_SHOT_PEN(obj) ((GtkShotPen*) obj)

enum _GtkShotPenType{
  GTK_SHOT_PEN_RECT,
  GTK_SHOT_PEN_ELLIPSE,
  GTK_SHOT_PEN_ARROW,
  GTK_SHOT_PEN_LINE,
  GTK_SHOT_PEN_TEXT
};

struct _GtkShotPen {
  GtkShotPenType type;
  GdkPoint start, end;
  gint size, color;
  gboolean square; // 绘制正方形/圆形/直线
  union {
    struct {
      gchar *fontname; // 注: 使用动态开辟的空间
      gchar *content;
    } text;
    GSList *tracks;
  };
  void (*save_track) (GtkShotPen *pen, gint x, gint y);
  void (*draw_track) (GtkShotPen *pen, cairo_t *cr);
};

GtkShotPen* gtk_shot_pen_new(GtkShotPenType type);
void gtk_shot_pen_free(GtkShotPen *pen);
void gtk_shot_pen_reset(GtkShotPen *pen);
/**
 * 画笔的浅拷贝,内部指针仍然和被拷贝画笔相同,
 * 如需避免两者的内部指针调用问题,
 * 可使用gtk_shot_pen_reset将原画笔复位,
 * 或直接新建画笔
 */
#define gtk_shot_pen_flat_copy(pen) \
          g_memdup((pen), sizeof(GtkShotPen))
void gtk_shot_pen_save_general_track(GtkShotPen *pen
                                        , gint x, gint y);
void gtk_shot_pen_save_line_track(GtkShotPen *pen
                                        , gint x, gint y);
void gtk_shot_pen_draw_rectangle(GtkShotPen *pen, cairo_t *cr);
void gtk_shot_pen_draw_ellipse(GtkShotPen *pen, cairo_t *cr);
void gtk_shot_pen_draw_arrow(GtkShotPen *pen, cairo_t *cr);
void gtk_shot_pen_draw_line(GtkShotPen *pen, cairo_t *cr);
void gtk_shot_pen_draw_text(GtkShotPen *pen, cairo_t *cr);

#ifdef __cplusplus
}
#endif

#endif
