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

#ifndef _GTK_SHOT_H_
#define _GTK_SHOT_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GtkShotClass GtkShotClass;
typedef enum _GtkShotMode GtkShotMode;
typedef enum _GtkShotCursorPos GtkShotCursorPos;
typedef struct _GtkShotSection GtkShotSection;
typedef struct _GtkShot GtkShot;

#include "pen.h"
#include "input.h"
#include "toolbar.h"
#include "pen-editor.h"

/* The border of anchor */
#define GTK_SHOT_ANCHOR_BORDER 6
/* The color of mask */
#define GTK_SHOT_COLOR 0x000000
/* The opacity of mask */
#define GTK_SHOT_OPACITY 0.8
/* The border of selection of capture */
#define GTK_SHOT_SECTION_BORDER 2
/* The color of selection of capture */
#define GTK_SHOT_SECTION_COLOR 0x00ff00
/* The count of trying grab key */
#define GRAB_KEY_TRY_COUNT 0

#define GTK_SHOT_TYPE    (gtk_shot_get_type())
#define GTK_SHOT(obj) \
          (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_SHOT_TYPE, GtkShot))
#define GTK_SHOT_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_CAST((klass), GTK_SHOT_TYPE, GtkShotClass))
#define IS_GTK_SHOT(obj) \
          (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_SHOT_TYPE))
#define IS_GTK_SHOT_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_SHOT_TYPE))
#define GTK_SHOT_GET_CLASS(obj) \
          (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_SHOT_TYPE, GtkShotClass))

struct _GtkShotClass {
  GtkWindowClass parent_class;
};

enum _GtkShotMode {
  NORMAL_MODE = 0, /* 一般模式 */
  DRAW_MODE, /* 绘制模式 */
  MOVE_MODE, /* 移动模式 */
  ZOOM_MODE, /* 缩放模式 */
  EDIT_MODE,  /* 编辑模式 */
  /**
   * 保存模式:
   * 该模式为冻结窗口模式,
   * 即只能进行编辑,不能移动和缩放
   */
  SAVE_MODE
};

enum _GtkShotCursorPos {
  /* 以下方位需要保持相邻
   * ,并且按逆/顺时针排序
   * ,第一个方位值必须为0
   */
  TOP_OF_SECTION = 0,
  RIGHT_TOP_OF_SECTION,
  RIGHT_OF_SECTION,
  RIGHT_BOTTOM_OF_SECTION,
  BOTTOM_OF_SECTION,
  LEFT_BOTTOM_OF_SECTION,
  LEFT_OF_SECTION,
  LEFT_TOP_OF_SECTION,
  OUTER_OF_SECTION,
  INNER_OF_SECTION
};

struct _GtkShotSection {
  gint x, y;
  gint width, height;
  gint border; // 选区边框大小
  gint color; // 选区边框颜色
};

struct _GtkShot {
  GtkWindow parent;

  GdkPixbuf *screen_pixbuf; // 整个屏幕的截图
  cairo_surface_t *mask_surface; // 遮罩层

  GtkShotMode mode;
  gboolean dynamic; // 是否动态截图
  gint width, height; // 窗口的宽度和高度
  gint x, y; // 窗口的起始位置坐标
  gdouble opacity;  // 窗口透明度
  gint color; // 16进制背景色
  gint anchor_border; // 缩放锚点的宽度

  GtkShotSection section; // 选择区域
  GtkShotCursorPos cursor_pos; // 鼠标位置
  GdkCursorType edit_cursor; // 编辑模式下的鼠标样式
  GdkPoint move_start, move_end; // 移动时的起点和终点

  GtkShotToolbar *toolbar; // 工具条
  GtkShotPenEditor *pen_editor; // 画笔编辑器
  GtkShotPen *pen; // 当前使用的画笔
  GSList *historic_pen; // 历史画笔
  GtkShotInput *input; // 文本输入窗口

  // FUNCTION
  void (*dblclick)();
  void (*quit)();
};

GtkShot* gtk_shot_new();
void gtk_shot_destroy(GtkShot *shot);

void gtk_shot_hide(GtkShot *shot);
void gtk_shot_show(GtkShot *shot, gboolean clean);
void gtk_shot_quit(GtkShot *shot);

void gtk_shot_hide_toolbar(GtkShot *shot);
void gtk_shot_show_toolbar(GtkShot *shot);
#define gtk_shot_refresh(shot) \
            gdk_window_invalidate_rect(GTK_WIDGET(shot)->window, NULL, FALSE)
gboolean gtk_shot_has_visible_section(GtkShot *shot);
void gtk_shot_get_section(GtkShot *shot
                              , gint *x0, gint *y0
                              , gint *x1, gint *y1);
GdkPixbuf* gtk_shot_get_section_pixbuf(GtkShot *shot);
void gtk_shot_save_section_to_clipboard(GtkShot *shot);
void gtk_shot_save_section_to_file(GtkShot *shot);
void gtk_shot_record(GtkShot *shot);

void gtk_shot_set_pen(GtkShot *shot, GtkShotPen *pen);
void gtk_shot_save_pen(GtkShot *shot, gboolean reset);
void gtk_shot_remove_pen(GtkShot *shot);
void gtk_shot_undo_pen(GtkShot *shot);
gboolean gtk_shot_has_empty_historic_pen(GtkShot *shot);

GType gtk_shot_get_type();

#ifdef __cplusplus
}
#endif
#endif
