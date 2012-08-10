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

#include <math.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "utils.h"

#include "shot.h"

#define IS_OUT_RECT(x, y, x0, y0, w, h) \
          ( ((x) < (x0) || (x) > ((x0) + (w))) \
            || ((y) < (y0) || (y) > ((y0) + (h))) )
#define IS_IN_RECT(x, y, x0, y0, w, h, border) \
          ( ((x) > ((x0) + (border)) && (x) < ((x0) + (w) - (border))) \
            && ((y) > ((y0) + (border)) && (y) < ((y0) + (h) - (border))) )
#define HAS_VISIBLE_SECTION(section) \
          ( (section).width > 2 * (section).border \
              && (section).height > 2 * (section).border )

// 鼠标位置与鼠标类型的映射
static GdkCursorType cursor_pos_type[] = {
  [LEFT_TOP_OF_SECTION] = GDK_TOP_LEFT_CORNER,
  [TOP_OF_SECTION] = GDK_TOP_SIDE,
  [RIGHT_TOP_OF_SECTION] = GDK_TOP_RIGHT_CORNER,
  [RIGHT_OF_SECTION] = GDK_RIGHT_SIDE,
  [RIGHT_BOTTOM_OF_SECTION] = GDK_BOTTOM_RIGHT_CORNER,
  [BOTTOM_OF_SECTION] = GDK_BOTTOM_SIDE,
  [LEFT_BOTTOM_OF_SECTION] = GDK_BOTTOM_LEFT_CORNER,
  [LEFT_OF_SECTION] = GDK_LEFT_SIDE,
  [INNER_OF_SECTION] = GDK_FLEUR,
  [OUTER_OF_SECTION] = GDK_LEFT_PTR
};

static void gtk_shot_class_init(GtkShotClass *klass);
static void gtk_shot_init(GtkShot *shot);
static void gtk_shot_realize(GtkWidget *widget);
static void gtk_shot_finalize(GObject *obj);
// Events
static gboolean on_shot_expose(GtkWidget *widget, GdkEventExpose *event);
static gboolean on_shot_button_press(GtkWidget *widget, GdkEventButton *event);
static gboolean on_shot_button_release(GtkWidget *widget, GdkEventButton *event);
static gboolean on_shot_motion_notify(GtkWidget *widget, GdkEventMotion *event);
static gboolean on_shot_key_press(GtkWidget *widget, GdkEventKey *event);

// private(第一个参数为GtkShot时,函数名称以gtk_shot_开头)
static void gtk_shot_draw_screen(GtkShot *shot, cairo_t *cr);
static void gtk_shot_draw_section(GtkShot *shot, cairo_t *cr);
static void gtk_shot_clean_section(GtkShot *shot);
static void gtk_shot_move_section(GtkShot *shot, gint x, gint y);
static void gtk_shot_resize_section(GtkShot *shot, gint x, gint y);
static void gtk_shot_adjust_section(GtkShot *shot, gint x0, gint y0
                                                      , gint x1, gint y1);
static void gtk_shot_get_section_coordinate(GtkShot *shot
                                              , gint *x0, gint *y0
                                              , gint *x1, gint *y1);
static void gtk_shot_get_zoom_anchor(GtkShot *shot, GdkPoint anchor[8]);
static GtkShotCursorPos gtk_shot_get_cursor_pos(GtkShot *shot, gint x, gint y);
static void gtk_shot_change_cursor(GtkShot *shot);
static void gtk_shot_adjust_toolbar(GtkShot *shot);

static void save_pixbuf_to_clipboard(GdkPixbuf *pixbuf);
static void draw_rect_border(cairo_t *cr, gint x, gint y
                                          , gint width, gint height
                                          , gint border, gint color);
static void draw_rect_area(cairo_t *cr, gint x, gint y
                                      , gint width, gint height
                                      , gint color, gdouble opacity);
static void draw_zoom_anchor(cairo_t *cr, GdkPoint anchor[8], gint border, gint color);

// Begin of GObject-related stuff
G_DEFINE_TYPE(GtkShot, gtk_shot, GTK_TYPE_WINDOW)

void gtk_shot_class_init(GtkShotClass *klass) {
  GObjectClass *obj_class;
  GtkWidgetClass *widget_class;

  obj_class = (GObjectClass*) klass;
  obj_class->finalize = gtk_shot_finalize;

  widget_class = GTK_WIDGET_CLASS(klass);
  widget_class->expose_event = on_shot_expose;
  widget_class->button_press_event = on_shot_button_press;
  widget_class->button_release_event = on_shot_button_release;
  widget_class->motion_notify_event = on_shot_motion_notify;
  widget_class->key_press_event = on_shot_key_press;
}

void gtk_shot_init(GtkShot *shot) {
  // 内部数据的初始化
  shot->mode = NORMAL_MODE;
  shot->opacity = GTK_SHOT_OPACITY;
  shot->color = GTK_SHOT_COLOR;
  shot->anchor_border = GTK_SHOT_ANCHOR_BORDER;
#if GTK_SHOT_NO_ANCHOR
  shot->has_anchor = FALSE;
#else
  shot->has_anchor = TRUE;
#endif
  shot->section.x = shot->section.y = 0;
  shot->section.width = shot->section.height = 0;
  shot->section.border = GTK_SHOT_SECTION_BORDER;
  shot->section.color = GTK_SHOT_SECTION_COLOR;
  shot->move_start.x = shot->move_start.y = 0;
  shot->move_end.x = shot->move_end.y = 0;
  shot->cursor_pos = OUTER_OF_SECTION;
  shot->historic_pen = NULL;
  shot->pen = NULL;
  shot->pen_editor = gtk_shot_pen_editor_new(GTK_WINDOW(shot));
  shot->toolbar = gtk_shot_toolbar_new(GTK_WINDOW(shot));
  shot->input = gtk_shot_input_new(GTK_WINDOW(shot));

  shot->quit = NULL;

  gtk_window_set_title(GTK_WINDOW(shot), GTK_SHOT_NAME);
  gtk_widget_set_can_focus(GTK_WIDGET(shot), TRUE);
  gtk_window_set_decorated(GTK_WINDOW(shot), FALSE);
  gtk_window_set_keep_above(GTK_WINDOW(shot), TRUE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(shot), TRUE);
  // 背景可透明
  GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(shot));
  GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);
  gtk_widget_set_app_paintable(GTK_WIDGET(shot), TRUE);
  gtk_widget_set_colormap(GTK_WIDGET(shot), colormap);

  shot->x = shot->y = 0;
  shot->width = gdk_screen_get_width(screen);
  shot->height = gdk_screen_get_height(screen);
  shot->screen_pixbuf = get_screen_pixbuf(shot->x, shot->y
                                            , shot->width
                                            , shot->height);
  shot->mask_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32
                                                    , shot->width
                                                    , shot->height);
#ifdef GTK_SHOT_DEBUG
  debug("window size(%d, %d), section color(%x: %x, %x, %x)\n"
                    , shot->width, shot->height
                    , shot->section.color
                    , RGB_R(shot->section.color)
                    , RGB_G(shot->section.color)
                    , RGB_B(shot->section.color));
#endif
  // 全屏窗口
  gtk_window_set_default_size(GTK_WINDOW(shot)
                                , shot->width, shot->height);
  // events
  gtk_widget_set_events(GTK_WIDGET(shot)
                            , gtk_widget_get_events(GTK_WIDGET(shot))
                                | GDK_BUTTON_MOTION_MASK
                                | GDK_POINTER_MOTION_MASK
                                | GDK_POINTER_MOTION_HINT_MASK
                                | GDK_BUTTON_PRESS_MASK
                                | GDK_BUTTON_RELEASE_MASK);
}

GtkShot* gtk_shot_new(/* some arguments */) {
  GtkShot *shot;
  GdkWindowAttr attr;

  shot = g_object_new(GTK_SHOT_TYPE, NULL);
  // POPUP window
  //GTK_WINDOW(shot)->type = GTK_WINDOW_POPUP;
  // 处理参数
  // ...

  return shot;
}
// End of GObject-related stuff

void gtk_shot_finalize(GObject *obj) {
  GtkShot *shot;

  g_return_if_fail(obj != NULL);
  g_return_if_fail(IS_GTK_SHOT(obj));

  shot = GTK_SHOT(obj);
  // 做些清理工作
  // ...
  debug("quit!\n");
}

void gtk_shot_get_section(GtkShot *shot, gint *x0, gint *y0
                                          , gint *x1, gint *y1) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  // border为线宽,所绘矩形的起始点在线宽的中心线上
  gint border = shot->section.border / 2;

  gtk_shot_get_section_coordinate(shot, x0, y0, x1, y1);

  *x0 += border; *y0 += border;
  *x1 -= border; *y1 -= border;
  // 修正坐标(去除屏幕外的部分)
  *x0 = MAX(*x0, 0);
  *y0 = MAX(*y0, 0);
  *x1 = MIN(*x1, shot->width);
  *y1 = MIN(*y1, shot->height);
}

GdkPixbuf* gtk_shot_get_section_pixbuf(GtkShot *shot) {
  GdkPixbuf *pixbuf = NULL;

  g_return_if_fail(IS_GTK_SHOT(shot));
#ifdef GTK_SHOT_DEBUG
  debug("screen shot(%d, %d: %d, %d)\n"
                  , shot->section.x, shot->section.y
                  , shot->section.width, shot->section.height);
#endif
  if (HAS_VISIBLE_SECTION(shot->section)) {
    gint x0, y0, x1, y1;
    gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);
    pixbuf = get_screen_pixbuf(x0, y0, x1 - x0, y1 - y0);
  } else {
    debug("no selected screen area...\n");
  }

  return pixbuf;
}

void save_pixbuf_to_clipboard(GdkPixbuf *pixbuf) {
  GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_image(clipboard, pixbuf);
  gtk_clipboard_set_can_store(clipboard, NULL, 0);
  gtk_clipboard_store(clipboard);
}

void gtk_shot_save_section_to_clipboard(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  GdkPixbuf *pixbuf = gtk_shot_get_section_pixbuf(shot);
  g_return_if_fail(pixbuf != NULL);

  save_pixbuf_to_clipboard(pixbuf);
}

void gtk_shot_hide(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  gtk_widget_hide_all(GTK_WIDGET(shot));
  gtk_shot_hide_toolbar(shot);
  gtk_shot_input_hide(shot->input);
}

void gtk_shot_show(GtkShot *shot, gboolean clean) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  if (clean) gtk_shot_clean_section(shot);
  gtk_widget_show(GTK_WIDGET(shot));
}

void gtk_shot_quit(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  if (shot->quit) {
    shot->quit();
  } else {
    gtk_shot_hide(shot);
  }
}

void gtk_shot_hide_toolbar(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  gtk_shot_toolbar_hide(shot->toolbar);
  gtk_shot_pen_editor_hide(shot->pen_editor);
}

void gtk_shot_show_toolbar(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  if (HAS_VISIBLE_SECTION(shot->section)) {
    gtk_shot_adjust_toolbar(shot);
    gtk_shot_toolbar_show(shot->toolbar);
  }
}

/**
 * 移除当前使用的画笔并隐藏画笔设置栏,
 * 如果其当前画笔不为NULL,则设置模式为SAVE_MODE
 */
void gtk_shot_remove_pen(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));
  if (shot->pen) {
    if (shot->pen->type == GTK_SHOT_PEN_TEXT) {
      gtk_shot_input_hide(shot->input);
    }
    shot->mode = SAVE_MODE;
  }
  gtk_shot_pen_free(shot->pen);
  shot->pen = NULL;
  gtk_shot_pen_editor_set_pen(shot->pen_editor, NULL);
  gtk_shot_pen_editor_hide(shot->pen_editor);
}

/**
 * 设置当前使用的画笔,显示画笔设置栏并修改模式为EDIT_MODE 
 */
void gtk_shot_set_pen(GtkShot *shot, GtkShotPen *pen) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  shot->pen = pen;
  gtk_shot_pen_editor_set_pen(shot->pen_editor, pen);
  gtk_shot_pen_editor_show(shot->pen_editor);

  if (pen) {
    shot->mode = EDIT_MODE;
    switch(pen->type) {
      case GTK_SHOT_PEN_RECT:
      case GTK_SHOT_PEN_ELLIPSE:
        shot->edit_cursor = GDK_CROSSHAIR; break;
      case GTK_SHOT_PEN_ARROW:
        shot->edit_cursor = GDK_LEFT_PTR; break;
      case GTK_SHOT_PEN_LINE:
        shot->edit_cursor = GDK_PENCIL; break;
      case GTK_SHOT_PEN_TEXT:
        shot->edit_cursor = GDK_XTERM; break;
    }
  }
}

/**
 * 将画笔添加到历史画笔中(浅拷贝当前画笔)
 */
void gtk_shot_save_pen(GtkShot *shot, gboolean reset) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  if (!shot->pen) return;

  shot->historic_pen
      = g_slist_append(shot->historic_pen
                          , gtk_shot_pen_flat_copy(shot->pen));
  if (reset) {
    gtk_shot_pen_reset(shot->pen);
  }
}

/** 撤销最后一个历史画笔 */
void gtk_shot_undo_pen(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));
  if (shot->historic_pen) {
    GSList *l = shot->historic_pen;
    gtk_shot_pen_free(GTK_SHOT_PEN(l->data));
    shot->historic_pen = g_slist_delete_link(shot->historic_pen, l);
  }
}

gboolean on_shot_expose(GtkWidget *widget, GdkEventExpose *event) {
  GtkShot *shot = GTK_SHOT(widget);
  cairo_t *cr, *mask_cr;

  cr = gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  mask_cr = cairo_create(shot->mask_surface);
  cairo_set_operator(mask_cr, CAIRO_OPERATOR_SOURCE);
  // 窗口上绘制截屏图像
  gtk_shot_draw_screen(shot, cr);
  // mask层绘制选区边框和涂鸦
  gtk_shot_draw_section(shot, mask_cr);
  // 将mask层合并到窗口上
  cairo_set_source_surface(cr, shot->mask_surface, 0, 0);
  cairo_paint(cr);

  cairo_destroy(mask_cr);
  cairo_destroy(cr);

  return TRUE;
}

gboolean on_shot_button_press(GtkWidget *widget, GdkEventButton *event) {
  GtkShot *shot = GTK_SHOT(widget);

  if (event->button != 1) return FALSE;
#ifdef GTK_SHOT_DEBUG
  debug("event(%f, %f)\n"
              , event->x, event->y);
#endif
  if (event->type == GDK_2BUTTON_PRESS) {
    shot->mode = SAVE_MODE;

    if (HAS_VISIBLE_SECTION(shot->section)) {
      gtk_shot_save_section_to_clipboard(shot);
      gtk_shot_hide(shot);
    } else {
      gtk_shot_quit(shot);
    }
  } else if (event->type == GDK_BUTTON_PRESS) {
    gdk_point_assign(shot->move_start, *event);
    gdk_point_assign(shot->move_end, *event);

    if (shot->mode == NORMAL_MODE) {
      if (shot->cursor_pos == OUTER_OF_SECTION) {
        shot->mode = DRAW_MODE;

        shot->section.x = event->x;
        shot->section.y = event->y;
        shot->section.width = shot->section.height = 0;

        gtk_shot_refresh(shot);
      } else if (shot->cursor_pos == INNER_OF_SECTION) {
        shot->mode = MOVE_MODE;
      } else {
        shot->mode = ZOOM_MODE;
      }
      gtk_shot_hide_toolbar(shot);
    } else if (shot->mode == EDIT_MODE) {
      if (shot->pen) {
        gdk_point_assign(shot->pen->start, *event);
        gdk_point_assign(shot->pen->end, *event);

        gboolean is_in_rect = IS_IN_RECT(event->x, event->y
                                          , shot->section.x
                                          , shot->section.y
                                          , shot->section.width
                                          , shot->section.height
                                          , shot->section.border);
        if (shot->pen->type == GTK_SHOT_PEN_TEXT
                                  && is_in_rect) {
          shot->mode = EDIT_TEXT_MODE;
          // 弹出文本输入框,接收输入
          gtk_shot_input_set_font(shot->input
                      , shot->pen->text.fontname
                      , shot->pen->color);
          gtk_shot_input_show(shot->input, event->x, event->y);
        }
      }
    } else if (shot->mode == EDIT_TEXT_MODE) {
      gboolean is_in_rect = IS_IN_RECT(event->x, event->y
                                        , shot->section.x
                                        , shot->section.y
                                        , shot->section.width
                                        , shot->section.height
                                        , shot->section.border);
      // 完成输入,并保存当前画笔,重新定位输入窗口
      char *text = gtk_shot_input_get_text(shot->input);
      if (text != NULL && strlen(text) > 0) {
        shot->mode = EDIT_MODE;
        shot->pen->text.content = text;
        gtk_shot_save_pen(shot, FALSE);
        // 由于是浅拷贝,故需对字体重新拷贝一份
        shot->pen->text.fontname
                  = g_strdup(shot->pen->text.fontname);
        shot->pen->text.content = NULL;
        gtk_shot_refresh(shot);
        gtk_shot_input_hide(shot->input);
      } else if (is_in_rect) {
        gdk_point_assign(shot->pen->start, *event);
        gdk_point_assign(shot->pen->end, *event);
        gtk_shot_input_show(shot->input, event->x, event->y);
      }
    }
  }
  return TRUE;
}

gboolean on_shot_button_release(GtkWidget *widget, GdkEventButton *event) {
  GtkShot *shot = GTK_SHOT(widget);

  if (event->button != 1 || event->type != GDK_BUTTON_RELEASE) return FALSE;
#ifdef GTK_SHOT_DEBUG
  debug("event(%f, %f)\n"
                , event->x, event->y);
#endif
  switch(shot->mode) {
    case DRAW_MODE:
    case MOVE_MODE:
    case ZOOM_MODE:
      shot->mode = NORMAL_MODE;
      gtk_shot_show_toolbar(shot);
      break;
    case EDIT_MODE:
      if (shot->pen) {
        gdk_point_assign(shot->pen->end, *event);
        if (shot->pen->type != GTK_SHOT_PEN_TEXT) {
          // 将画笔放到历史链表,并复位当前画笔
          gtk_shot_save_pen(shot, TRUE);
        }
      }
      break;
  }

  return TRUE;
}

gboolean on_shot_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
  GtkShot *shot = GTK_SHOT(widget);
  GdkModifierType state;
  GdkPoint cursor;

  gtk_widget_get_pointer(widget, &cursor.x, &cursor.y);
#ifdef GTK_SHOT_DEBUG
  debug("event(%f, %f), cursor(%d, %d), mouse press(%d)\n"
                , event->x, event->y
                , cursor.x, cursor.y
                , event->state & GDK_BUTTON1_MASK);
#endif
  if (event->state & GDK_BUTTON1_MASK) { // 鼠标被按下
    switch(shot->mode) {
      case DRAW_MODE:
        if (!gdk_point_is_equal(shot->move_end, cursor)) {
          gdk_point_assign(shot->move_end, *event);
          gtk_shot_adjust_section(shot, shot->move_start.x
                                    , shot->move_start.y
                                    , shot->move_end.x
                                    , shot->move_end.y);
        }
        break;
      case MOVE_MODE:
        gtk_shot_move_section(shot, cursor.x, cursor.y);
        break;
      case ZOOM_MODE:
        gtk_shot_resize_section(shot, cursor.x, cursor.y);
        gtk_shot_change_cursor(shot);
        break;
      case EDIT_MODE:
        if (shot->pen
              && shot->pen->type != GTK_SHOT_PEN_TEXT) {
          shot->pen->save_track(shot->pen, cursor.x, cursor.y);
        }
        break;
    }
    if (shot->mode != NORMAL_MODE && shot->mode != SAVE_MODE) {
      gtk_shot_refresh(shot);
    }
  } else {
    shot->cursor_pos = gtk_shot_get_cursor_pos(shot
                                                  , cursor.x
                                                  , cursor.y);
    gtk_shot_change_cursor(shot);
  }
  return TRUE;
}

gboolean on_shot_key_press(GtkWidget *widget, GdkEventKey *event) {
  return TRUE;
}

void gtk_shot_draw_screen(GtkShot *shot, cairo_t *cr) {
  gdk_cairo_set_source_pixbuf(cr, shot->screen_pixbuf, 0, 0);
  cairo_paint(cr);
}

void gtk_shot_draw_section(GtkShot *shot, cairo_t *cr) {
  if (shot->section.width > 0 || shot->section.height > 0) {
    // transparent section
    draw_rect_area(cr, shot->section.x
                      , shot->section.y
                      , shot->section.width
                      , shot->section.height
                      , 0x000000, 1);
    // draw doodle
    GSList *l = shot->historic_pen;
    GtkShotPen *pen;
    for (l; l; l = l->next) {
      pen = GTK_SHOT_PEN(l->data);
      pen->draw_track(pen, cr);
    }
    pen = shot->pen;
    if (pen) {
      pen->draw_track(pen, cr);
    }
    // draw shot around section
    gint x0, y0, x1, y1;
    gint border = shot->section.border / 2;
    gtk_shot_get_section_coordinate(shot, &x0, &y0, &x1, &y1);
    x0 -= border; y0 -= border;
    x1 += border; y1 += border;
    draw_rect_area(cr, shot->x, shot->y
                      , shot->width, y0 - shot->y
                      , shot->color, shot->opacity);
    draw_rect_area(cr, shot->x, y1
                      , shot->width, shot->height + shot->y - y1
                      , shot->color, shot->opacity);
    draw_rect_area(cr, shot->x, y0
                      , x0 - shot->x, y1 - y0
                      , shot->color, shot->opacity);
    draw_rect_area(cr, x1, y0
                      , shot->width + shot->x - x1, y1 - y0
                      , shot->color, shot->opacity);
    // draw section border
    draw_rect_border(cr, shot->section.x
                          , shot->section.y
                          , shot->section.width
                          , shot->section.height
                          , shot->section.border
                          , shot->section.color);
    if (shot->has_anchor) {
      // draw zoom anchor
      GdkPoint anchor[8] = {{.x = 0, .y = 0}};
      gtk_shot_get_zoom_anchor(shot, anchor);
      draw_zoom_anchor(cr, anchor, shot->anchor_border
                                  , shot->section.color);
    }
  } else {
    // 整体清除
    draw_rect_area(cr, shot->x, shot->y
                      , shot->width, shot->height
                      , shot->color, shot->opacity);
  }
}

void gtk_shot_clean_section(GtkShot *shot) {
  gdk_point_assign(shot->move_start, shot->move_end);
  shot->section.width = shot->section.height = 0;

  gtk_shot_remove_pen(shot);

  GSList *l = shot->historic_pen;
  for (l; l; l = l->next) {
    gtk_shot_pen_free(GTK_SHOT_PEN(l->data));
  }
  g_slist_free(shot->historic_pen);
}

void gtk_shot_move_section(GtkShot *shot, gint x, gint y) {
  gint dx = x - shot->move_start.x;
  gint dy = y - shot->move_start.y;

  shot->section.x += dx;
  shot->section.y += dy;

  shot->move_start.x += dx;
  shot->move_start.y += dy;
  shot->move_end.x += dx;
  shot->move_end.y += dy;
}

void gtk_shot_resize_section(GtkShot *shot, gint x, gint y) {
  gint x0, y0, x1, y1;

  gtk_shot_get_section_coordinate(shot, &x0, &y0, &x1, &y1);

  switch(shot->cursor_pos) {
    case LEFT_TOP_OF_SECTION: x0 = x;
    case TOP_OF_SECTION: y0 = y; break;
    case RIGHT_TOP_OF_SECTION: y0 = y;
    case RIGHT_OF_SECTION: x1 = x; break;
    case RIGHT_BOTTOM_OF_SECTION: x1 = x;
    case BOTTOM_OF_SECTION: y1 = y; break;
    case LEFT_BOTTOM_OF_SECTION: y1 = y;
    case LEFT_OF_SECTION: x0 = x; break;
  }
  
  gint dx = x1 - x0, dy = y1 - y0;
  // 缩放反向,初始时的对角线方向矢量一定为正,故仅判断变换后的矢量.
  // 对角点的奇偶性相同; 对边点的奇偶性相同;
  // dx*dy > 0: 相对的对角点互换; 两对角点的值相差4;
  // dx*dy < 0: 相邻的对角点互换(值相差2或6),或对边点互换(值相差4);
  if (dx < 0 || dy < 0) {
    gint diff = 4; // 互换点间的差值
    if (dx * dy < 0 && shot->cursor_pos % 2 == LEFT_TOP_OF_SECTION % 2) {
      if (shot->cursor_pos == LEFT_TOP_OF_SECTION
            || shot->cursor_pos == RIGHT_BOTTOM_OF_SECTION) {
        diff = dx < 0 ? 2 : 6; // dx < 0: 与顺时针方向的相邻对角点互换
      } else {
        diff = dx < 0 ? 6 : 2; // dx < 0: 与逆时针方向的相邻对角点互换
      }
    }
    shot->cursor_pos = (GtkShotCursorPos) ((shot->cursor_pos + diff) % 8);
  }
  gtk_shot_adjust_section(shot, x0, y0, x1, y1);
}

void gtk_shot_adjust_section(GtkShot *shot, gint x0, gint y0
                                              , gint x1, gint y1) {
  shot->section.x = MIN(x0, x1);
  shot->section.y = MIN(y0, y1);
  shot->section.width = ABS(x0 - x1);
  shot->section.height = ABS(y0 -y1);
}

void gtk_shot_get_section_coordinate(GtkShot *shot
                                              , gint *x0, gint *y0
                                              , gint *x1, gint *y1) {
  *x0 = shot->section.x;
  *y0 = shot->section.y;
  *x1 = shot->section.x + shot->section.width;
  *y1 = shot->section.y + shot->section.height;
}

void gtk_shot_adjust_toolbar(GtkShot *shot) {
  gint x = 0, y = 0, x0, y0, x1, y1;
  gint toolbar_w, toolbar_h, editor_w, editor_h;

  gtk_shot_get_section_coordinate(shot, &x0, &y0, &x1, &y1);

  // 扩展到锚点外部矩形范围
  x0 -= shot->anchor_border;
  y0 -= shot->anchor_border;
  x1 += shot->anchor_border;
  y1 += shot->anchor_border;
  // 限定在MASK区域,并转换坐标系为相对于MASK的坐标系
  x0 = MAX(x0, shot->x) - shot->x;
  y0 = MAX(y0, shot->y) - shot->y;
  x1 = MIN(x1, shot->x + shot->width) - shot->x;
  y1 = MIN(y1, shot->y + shot->height) - shot->y;

  x = x0 + ((x1 - x0) - shot->toolbar->width) / 2;
  x = MAX(MIN(x, shot->width - shot->toolbar->width), 0);
#define TS_SPACE  2
  if (y1 <= shot->height
              - shot->toolbar->height
              - shot->pen_editor->height
              - TS_SPACE) {
    y = y1;
  } else {
    y = MAX(y0 - shot->toolbar->height, 0);
  }
  x += shot->x; y += shot->y;

  shot->toolbar->x = x; shot->toolbar->y = y;
  if (y != y1 && y0 >= shot->toolbar->height
                        + shot->pen_editor->height
                        + TS_SPACE) {
    y -= shot->pen_editor->height + TS_SPACE;
  } else {
    y += shot->toolbar->height + TS_SPACE;
  }
  shot->pen_editor->x = x; shot->pen_editor->y = y;
}

/**
 * 获取8个矩形缩放锚点的起始坐标(矩形框)
 */
void gtk_shot_get_zoom_anchor(GtkShot *shot, GdkPoint anchor[8]) {
  gint x0, y0, x1, y1;
  gint border = shot->section.border / 2;

  // NOTE: 选择边框在选区内部
  gtk_shot_get_section_coordinate(shot, &x0, &y0, &x1, &y1);

  x0 += border - shot->anchor_border;
  y0 += border - shot->anchor_border;
  x1 -= border;
  y1 -= border;

  anchor[LEFT_TOP_OF_SECTION].x = x0;
  anchor[LEFT_TOP_OF_SECTION].y = y0;

  anchor[TOP_OF_SECTION].x = (x0 + x1) / 2;
  anchor[TOP_OF_SECTION].y = y0;
  anchor[RIGHT_TOP_OF_SECTION].x = x1;
  anchor[RIGHT_TOP_OF_SECTION].y = y0;
  anchor[RIGHT_OF_SECTION].x = x1;
  anchor[RIGHT_OF_SECTION].y = (y0 + y1) / 2;

  anchor[RIGHT_BOTTOM_OF_SECTION].x = x1;
  anchor[RIGHT_BOTTOM_OF_SECTION].y = y1;

  anchor[BOTTOM_OF_SECTION].x = (x0 + x1) / 2;
  anchor[BOTTOM_OF_SECTION].y = y1;
  anchor[LEFT_BOTTOM_OF_SECTION].x = x0;
  anchor[LEFT_BOTTOM_OF_SECTION].y = y1;
  anchor[LEFT_OF_SECTION].x = x0;
  anchor[LEFT_OF_SECTION].y = (y0 + y1) / 2;
}

GtkShotCursorPos gtk_shot_get_cursor_pos(GtkShot *shot, gint x, gint y) {
  GtkShotCursorPos pos = OUTER_OF_SECTION;
  gint border = shot->section.border;

  // 获取从左上角出发的矩形区域及起始坐标(将边框划到矩形框内部)
  gint w = shot->section.width + border, h = shot->section.height + border;
  gint x0 = shot->section.x - border / 2, y0 = shot->section.y - border / 2;
  // 没有选择区域,自然在外部了 :-)
  if (w == 0 && h == 0) return OUTER_OF_SECTION;
  if (IS_OUT_RECT(x, y, x0, y0, w, h)) {
    pos = OUTER_OF_SECTION;
  } else if (IS_IN_RECT(x, y, x0, y0, w, h, border)) {
    pos = INNER_OF_SECTION;
  } else {
    x0 = x - x0; y0 = y - y0;
    if (x0 <= border) {
      if (y0 <= border) {
        pos = LEFT_TOP_OF_SECTION;
      } else if (y0 >= h - border) {
        pos = LEFT_BOTTOM_OF_SECTION;
      } else {
        pos = LEFT_OF_SECTION;
      }
    } else if (x0 >= w - border) {
      if (y0 <= border) {
        pos = RIGHT_TOP_OF_SECTION;
      } else if (y0 >= h - border) {
        pos = RIGHT_BOTTOM_OF_SECTION;
      } else {
        pos = RIGHT_OF_SECTION;
      }
    } else if (y0 <= border) {
      pos = TOP_OF_SECTION;
    } else if (y0 >= h - border) {
      pos = BOTTOM_OF_SECTION;
    }
  }
  // 缩放锚点上的位置判断
  GdkPoint anchor[8] = {{.x = 0, .y = 0}};
  gint i = 0;

  gtk_shot_get_zoom_anchor(shot, anchor);
  border = shot->anchor_border;
  for (i = 0; i < 8; i++) {
    if (!IS_OUT_RECT(x, y, anchor[i].x, anchor[i].y
                          , border, border)) {
      pos = (GtkShotCursorPos) i; // :-)
    }
  }

  return pos;
}

void gtk_shot_change_cursor(GtkShot *shot) {
  GdkCursorType cursor = cursor_pos_type[shot->cursor_pos];

  if (shot->mode == SAVE_MODE) {
    // 如果什么都没画的话,则回到NORMAL_MODE,鼠标按正常方式显示,
    // 否则,仅将鼠标变为指针
    if (!shot->historic_pen) {
      shot->mode = NORMAL_MODE;
    } else {
      cursor = GDK_LEFT_PTR;
    }
  } else if (shot->mode == EDIT_MODE
                || shot->mode == EDIT_TEXT_MODE) {
    // GDK_PENCIL(line), GDK_CROSSHAIR(rectangle,ellipse),
    // GDK_XTERM(text), GDK_LEFT_PTR(other)
    if (shot->cursor_pos == INNER_OF_SECTION) {
      cursor = shot->edit_cursor;
    } else {
      cursor = GDK_LEFT_PTR;
    }
  }
  gdk_window_set_cursor(GTK_WIDGET(shot)->window
                          , gdk_cursor_new(cursor));
}

void draw_rect_area(cairo_t *cr, gint x, gint y
                                , gint width, gint height
                                , gint color, gdouble opacity) {
  cairo_set_source_rgba(cr, RGB_R(color) / 255.0
                          , RGB_G(color) / 255.0
                          , RGB_B(color) / 255.0
                          , 1 - opacity);
  cairo_rectangle(cr, x, y, width, height);
  cairo_fill(cr);
}

/** 所绘图形至少有border的宽度或高度 */
void draw_rect_border(cairo_t *cr, gint x, gint y
                                  , gint width, gint height
                                  , gint border, gint color) {
  cairo_set_line_width(cr, border);
  cairo_set_source_rgb(cr, RGB_R(color) / 255.0
                          , RGB_G(color) / 255.0
                          , RGB_B(color) / 255.0);
  cairo_rectangle(cr, x, y
                    , width > 0 ? width : border
                    , height > 0 ? height : border);
  cairo_stroke(cr);
}

void draw_zoom_anchor(cairo_t *cr, GdkPoint anchor[8]
                                  , gint border, gint color) {
  gint i = 0;

  cairo_set_source_rgb(cr, RGB_R(color) / 255.0
                          , RGB_G(color) / 255.0
                          , RGB_B(color) / 255.0);
  for (i = 0; i < 8; i++) {
    // 为了回避缩放锚点在截屏时不能有效清除的问题,
    // 故画圆形锚点,并让其不占用内部选择区域(SECTION)
    //cairo_rectangle(cr, anchor[i].x, anchor[i].y, border, border);
    cairo_arc(cr, anchor[i].x + border / 2.0
                , anchor[i].y + border / 2.0
                , border / 2.0, 0, 2 * M_PI);
    cairo_fill(cr);
  }
}
