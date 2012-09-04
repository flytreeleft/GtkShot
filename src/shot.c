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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "utils.h"

#include "shot.h"

#define IS_OUT_RECT(x, y, x0, y0, x1, y1) \
          ( ((x) < (x0) || (x) > (x1)) \
            || ((y) < (y0) || (y) > (y1)) )
#define IS_IN_RECT(x, y, x0, y0, x1, y1) \
          ( ((x) > (x0) && (x) < (x1)) \
            && ((y) > (y0) && (y) < (y1)) )

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
static gboolean on_shot_expose(GtkWidget *widget
                                      , GdkEventExpose *event);
static gboolean on_shot_button_press(GtkWidget *widget
                                        , GdkEventButton *event);
static gboolean on_shot_button_release(GtkWidget *widget
                                          , GdkEventButton *event);
static gboolean on_shot_motion_notify(GtkWidget *widget
                                          , GdkEventMotion *event);
static gboolean on_shot_key_press(GtkWidget *widget
                                          , GdkEventKey *event);
static gboolean on_shot_key_release(GtkWidget *widget
                                          , GdkEventKey *event);

// private(第一个参数为GtkShot时,函数名称以gtk_shot_开头)
static void gtk_shot_process_edit_mode(GtkShot *shot
                                          , GdkEventButton *event);
static void gtk_shot_draw_screen(GtkShot *shot, cairo_t *cr);
static void gtk_shot_draw_section(GtkShot *shot, cairo_t *cr);
static void gtk_shot_draw_doodle(GtkShot *shot, cairo_t *cr);
static void gtk_shot_draw_mask(GtkShot *shot, cairo_t *cr);
static void gtk_shot_draw_anchor(GtkShot *shot, cairo_t *cr);
static void gtk_shot_draw_message(GtkShot *shot, cairo_t *cr);
static void gtk_shot_clean_section(GtkShot *shot);
static void gtk_shot_clean_historic_pen(GtkShot *shot);
static void gtk_shot_move_section(GtkShot *shot, gint dx, gint dy);
static void gtk_shot_resize_section(GtkShot *shot, gint x, gint y);
static void gtk_shot_adjust_section(GtkShot *shot, gint x0, gint y0
                                                , gint x1, gint y1);
static void gtk_shot_get_zoom_anchor(GtkShot *shot, GdkPoint anchor[8]);
static GtkShotCursorPos gtk_shot_get_cursor_pos(GtkShot *shot
                                                    , gint x, gint y);
static void gtk_shot_change_cursor(GtkShot *shot);
static void gtk_shot_adjust_toolbar(GtkShot *shot);
static void gtk_shot_grab_keyboard(GtkShot *shot);

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
  widget_class->key_release_event = on_shot_key_release;
}

void gtk_shot_init(GtkShot *shot) {
  // 内部数据的初始化
  shot->mode = NORMAL_MODE;
  shot->dynamic = FALSE;
  shot->opacity = GTK_SHOT_OPACITY;
  shot->color = GTK_SHOT_COLOR;
  shot->anchor_border = GTK_SHOT_ANCHOR_BORDER;
  shot->section.x = shot->section.y = 0;
  shot->section.width = shot->section.height = 0;
  shot->section.border = GTK_SHOT_SECTION_BORDER;
  shot->section.color = GTK_SHOT_SECTION_COLOR;
  shot->move_start.x = shot->move_start.y = 0;
  shot->move_end.x = shot->move_end.y = 0;
  shot->cursor_pos = OUTER_OF_SECTION;
  shot->historic_pen = NULL;
  shot->pen = NULL;
  shot->pen_editor = gtk_shot_pen_editor_new(shot);
  shot->toolbar = gtk_shot_toolbar_new(shot);
  shot->input = gtk_shot_input_new(shot);
  shot->screen_pixbuf = NULL;
  shot->quit = NULL; // function
  shot->dblclick = NULL; // function

  gtk_window_set_decorated(GTK_WINDOW(shot), FALSE);
  gtk_window_set_keep_above(GTK_WINDOW(shot), TRUE);
  gtk_widget_set_can_focus(GTK_WIDGET(shot), TRUE);
  //gtk_window_set_skip_taskbar_hint(GTK_WINDOW(shot), TRUE);
  // 背景可透明
  GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(shot));
  GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);
  gtk_widget_set_app_paintable(GTK_WIDGET(shot), TRUE);
  gtk_widget_set_colormap(GTK_WIDGET(shot), colormap);

  shot->x = shot->y = 0;
  shot->width = gdk_screen_get_width(screen);
  shot->height = gdk_screen_get_height(screen);
  shot->mask_surface =
                cairo_image_surface_create(CAIRO_FORMAT_ARGB32
                                                , shot->width
                                                , shot->height);
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
                                | GDK_BUTTON_RELEASE_MASK
                                | GDK_KEY_PRESS_MASK
                                | GDK_KEY_RELEASE_MASK);
}

GtkShot* gtk_shot_new(/* some arguments */) {
  GtkShot *shot;
  GdkWindowAttr attr;

  shot = g_object_new(GTK_SHOT_TYPE, NULL);
  GTK_WINDOW(shot)->type = GTK_WINDOW_TOPLEVEL;
  // 处理参数
  // ...

  return shot;
}
// End of GObject-related stuff

void gtk_shot_finalize(GObject *obj) {
  GtkShot *shot = GTK_SHOT(obj);
  // 做些清理工作
  g_object_unref(shot->screen_pixbuf);
  shot->screen_pixbuf = NULL;
  cairo_surface_destroy(shot->mask_surface);
  shot->mask_surface = NULL;
  gtk_shot_pen_free(shot->pen);
  shot->pen = NULL;
  gtk_shot_clean_historic_pen(shot);
  shot->historic_pen = NULL;
  gtk_shot_pen_editor_destroy(shot->pen_editor);
  shot->pen_editor = NULL;
  gtk_shot_toolbar_destroy(shot->toolbar);
  shot->toolbar = NULL;
  gtk_shot_input_destroy(shot->input);
  shot->input = NULL;
#ifdef GTK_SHOT_DEBUG
  debug("quit!\n");
#endif
}

void gtk_shot_destroy(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  gdk_keyboard_ungrab(GDK_CURRENT_TIME);
  gtk_widget_destroy(GTK_WIDGET(shot));
}

void gtk_shot_hide(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot)
                      && gtk_widget_get_visible(GTK_WIDGET(shot)));

  gtk_widget_hide_all(GTK_WIDGET(shot));
  gtk_shot_hide_toolbar(shot);
  gtk_shot_input_hide(shot->input);
  gdk_keyboard_ungrab(GDK_CURRENT_TIME);
}

void gtk_shot_show(GtkShot *shot, gboolean clean) {
  g_return_if_fail(IS_GTK_SHOT(shot)
                      && !gtk_widget_get_visible(GTK_WIDGET(shot)));

  if (clean) {
    shot->screen_pixbuf =
      gdk_pixbuf_get_from_drawable(shot->screen_pixbuf
                                    , gdk_get_default_root_window()
                                    , NULL
                                    , shot->x, shot->y
                                    , 0, 0
                                    , shot->width, shot->height);
    gtk_shot_clean_section(shot);
    shot->mode = NORMAL_MODE;
  }
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

gboolean gtk_shot_has_visible_section(GtkShot *shot) {
  gint x0, y0, x1, y1;
  gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);

  return (x1 - x0) > 0 && (y1 - y0) > 0;
}

/** 获取选区的实际边界(起点和终点坐标) */
void gtk_shot_get_section(GtkShot *shot, gint *x0, gint *y0
                                          , gint *x1, gint *y1) {
  *x0 = 0; *y0 = 0; *x1 = 0; *y1 = 0;
  g_return_if_fail(IS_GTK_SHOT(shot));

  // 所绘矩形的起始点在线宽的中心线上,其宽度和高度包含了一个线宽
  gint b = shot->section.border / 2 + shot->section.border % 2;
  gint w = MAX(shot->section.width - shot->section.border, 0);
  gint h = MAX(shot->section.height - shot->section.border, 0);

   // 修正坐标(去除边框和屏幕外的部分)
  *x0 = MAX(shot->section.x + b, shot->x);
  *y0 = MAX(shot->section.y + b, shot->y);
  *x1 = MIN(shot->section.x + w, shot->x + shot->width);
  *y1 = MIN(shot->section.y + h, shot->y + shot->height);
}

GdkPixbuf* gtk_shot_get_section_pixbuf(GtkShot *shot) {
  g_return_val_if_fail(gtk_shot_has_visible_section(shot), NULL);

  gint x0, y0, x1, y1;
  gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);
#ifdef GTK_SHOT_DEBUG
  debug("screen shot(%d, %d: %d, %d)\n"
                  , x0, y0, x1 - x0, y1 - y0);
#endif
  GdkDrawable *drawable = NULL;
  if (shot->dynamic) {
    // 获取屏幕上的截图
    drawable = gdk_get_default_root_window();
  } else {
    // 截图和涂鸦绘制在新的画布上
    // Reference:
    // http://lists.cairographics.org/archives/cairo/2008-October/015479.html
    GdkColormap *colormap =
      gdk_drawable_get_colormap(GTK_WIDGET(shot)->window);
    drawable = gdk_pixmap_new(NULL, shot->width, shot->height
                                  , 32);
    gdk_drawable_set_colormap(drawable, colormap);

    cairo_t *cr = gdk_cairo_create(drawable);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    gtk_shot_draw_screen(shot, cr);
    gtk_shot_draw_doodle(shot, cr);
    cairo_destroy(cr);
  }
  return gdk_pixbuf_get_from_drawable(NULL, drawable
                                        , NULL
                                        , x0, y0
                                        , 0, 0
                                        , x1 - x0, y1 - y0);
}

void gtk_shot_save_section_to_clipboard(GtkShot *shot) {
  GdkPixbuf *pixbuf = gtk_shot_get_section_pixbuf(shot);
  if (pixbuf) {
    save_pixbuf_to_clipboard(pixbuf);
    g_object_unref(pixbuf);
  }
}

void gtk_shot_save_section_to_file(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  gchar *type = NULL;
  gboolean succ = FALSE;
  GdkPixbuf *pixbuf = gtk_shot_get_section_pixbuf(shot);
  gchar *filename =
        choose_and_get_filename(GTK_WINDOW(shot)
                                    , &type, NULL);
  if (filename) {
    GError *error = NULL;
    succ = gdk_pixbuf_save(pixbuf, filename, type
                                  , &error, NULL);
    if (!succ) {
      popup_message_dialog(GTK_WINDOW(shot), error->message);
    }
  }
  g_free(filename);
  g_free(type);
  
  if (succ) {
    gtk_shot_quit(shot);
  }
}

void gtk_shot_record(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));
}

void gtk_shot_hide_toolbar(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot));

  gtk_shot_toolbar_hide(shot->toolbar);
  gtk_shot_pen_editor_hide(shot->pen_editor);
}

void gtk_shot_show_toolbar(GtkShot *shot) {
  if (gtk_shot_has_visible_section(shot)) {
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

  if (shot->pen && shot->pen->type == GTK_SHOT_PEN_TEXT) {
    gtk_shot_input_hide(shot->input);
  }
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
  } else if (shot->mode == EDIT_MODE) {
    shot->mode = NORMAL_MODE;
  }
}

/**
 * 将画笔添加到历史画笔中(浅拷贝当前画笔)
 */
void gtk_shot_save_pen(GtkShot *shot, gboolean reset) {
  g_return_if_fail(IS_GTK_SHOT(shot) && shot->pen);

  shot->historic_pen
      = g_slist_append(shot->historic_pen
                        , gtk_shot_pen_flat_copy(shot->pen));
  if (reset) {
    gtk_shot_pen_reset(shot->pen);
  } else if (shot->pen->type == GTK_SHOT_PEN_TEXT) {
    // 由于是浅拷贝,故需对字体重新拷贝一份
    shot->pen->text.fontname
              = g_strdup(shot->pen->text.fontname);
    shot->pen->text.content = NULL;
  }
}

/** 撤销最后加入的历史画笔 */
void gtk_shot_undo_pen(GtkShot *shot) {
  g_return_if_fail(IS_GTK_SHOT(shot) && shot->historic_pen);

  GSList *l = g_slist_last(shot->historic_pen);

  gtk_shot_pen_free(GTK_SHOT_PEN(l->data));
  shot->historic_pen =
          g_slist_delete_link(shot->historic_pen, l);
}

/** 历史画笔是否为空 */
gboolean gtk_shot_has_empty_historic_pen(GtkShot *shot) {
  return IS_GTK_SHOT(shot) && !shot->historic_pen;
}

gboolean on_shot_expose(GtkWidget *widget
                              , GdkEventExpose *event) {
  GtkShot *shot = GTK_SHOT(widget);
  cairo_t *cr, *mask_cr;

  cr = gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_set_operator(cr, shot->dynamic ?
                            CAIRO_OPERATOR_SOURCE
                            : CAIRO_OPERATOR_OVER);

  mask_cr = cairo_create(shot->mask_surface);
  cairo_set_operator(mask_cr, CAIRO_OPERATOR_SOURCE);
  // 窗口上绘制截屏图像
  gtk_shot_draw_screen(shot, cr);
  // mask层绘制选区边框和涂鸦
  gtk_shot_draw_section(shot, mask_cr);
  // 绘制信息窗口
  gtk_shot_draw_message(shot, mask_cr);
  // 将mask层合并到窗口上
  cairo_set_source_surface(cr, shot->mask_surface, 0, 0);
  cairo_paint(cr);

  cairo_destroy(mask_cr);
  cairo_destroy(cr);
  // 控制键盘
  gtk_shot_grab_keyboard(shot);

  return TRUE;
}

gboolean on_shot_button_press(GtkWidget *widget
                                    , GdkEventButton *event) {
  GtkShot *shot = GTK_SHOT(widget);

  if (event->button != 1) return FALSE;

  if (event->type == GDK_2BUTTON_PRESS) {
    if (shot->dblclick) {
      shot->dblclick();
    }
  } else if (event->type == GDK_BUTTON_PRESS) {
    gdk_point_assign(shot->move_start, *event);
    gdk_point_assign(shot->move_end, *event);

    if (shot->mode == NORMAL_MODE) {
      if (shot->cursor_pos == OUTER_OF_SECTION) {
        shot->mode = DRAW_MODE;
        gdk_point_assign(shot->section, *event);
        shot->section.width = shot->section.height = 0;
        gtk_shot_refresh(shot);
      } else if (shot->cursor_pos == INNER_OF_SECTION) {
        shot->mode = MOVE_MODE;
      } else {
        shot->mode = ZOOM_MODE;
      }
      gtk_shot_hide_toolbar(shot);
    } else if (shot->mode == EDIT_MODE) {
      gtk_shot_process_edit_mode(shot, event);
    }
  }
  return TRUE;
}

gboolean on_shot_button_release(GtkWidget *widget
                                      , GdkEventButton *event) {
  GtkShot *shot = GTK_SHOT(widget);

  if (event->button != 1 || event->type != GDK_BUTTON_RELEASE) {
    return FALSE;
  }
  switch(shot->mode) {
    case DRAW_MODE:
    case MOVE_MODE:
    case ZOOM_MODE:
      shot->mode = NORMAL_MODE;
      gtk_shot_show_toolbar(shot);
      break;
    case EDIT_MODE:
      if (shot->pen->type != GTK_SHOT_PEN_TEXT) {
        gdk_point_assign(shot->pen->end, *event);
        // 将画笔放到历史链表
        gtk_shot_save_pen(shot, TRUE);
      }
      break;
  }

  return TRUE;
}

gboolean on_shot_motion_notify(GtkWidget *widget
                                    , GdkEventMotion *event) {
  GtkShot *shot = GTK_SHOT(widget);
  GdkModifierType state;
  GdkPoint cursor;

  gtk_widget_get_pointer(widget, &cursor.x, &cursor.y);
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
        gtk_shot_move_section(shot, cursor.x - shot->move_start.x
                                  , cursor.y - shot->move_start.y);
        break;
      case ZOOM_MODE:
        gtk_shot_resize_section(shot, cursor.x, cursor.y);
        gtk_shot_change_cursor(shot);
        break;
      case EDIT_MODE:
        if (shot->pen->type != GTK_SHOT_PEN_TEXT) {
          shot->pen->save_track(shot->pen, cursor.x, cursor.y);
        }
        break;
    }
    if (shot->mode != NORMAL_MODE && shot->mode != SAVE_MODE) {
      gtk_shot_refresh(shot);
    }
  } else {
    shot->cursor_pos =
      gtk_shot_get_cursor_pos(shot, cursor.x, cursor.y);
    gtk_shot_change_cursor(shot);
  }
  return TRUE;
}

gboolean on_shot_key_press(GtkWidget *widget
                                , GdkEventKey *event) {
  GtkShot *shot = GTK_SHOT(widget);
  gboolean is_ctrl = event->state && GDK_CONTROL_MASK;
#ifdef GTK_SHOT_DEBUG
  debug("key press: %s, ctrl mask: %d\n"
                , gdk_keyval_name(event->keyval)
                , is_ctrl);
#endif
  if (shot->pen
        && (event->keyval == GDK_Control_L
              || event->keyval == GDK_Control_R)) {
    shot->pen->square = !shot->pen->square;
    gtk_shot_refresh(shot);
    return TRUE;
  }

  gint dx = 0, dy = 0;
  switch (event->keyval) {
    case GDK_KP_Up:
    case GDK_Up: dy -= 2;
    case GDK_KP_Down:
    case GDK_Down: dy += 1; dx += 1;
    case GDK_KP_Left:
    case GDK_Left: dx -= 2;
    case GDK_KP_Right:
    case GDK_Right: dx += 1;
      if (!is_ctrl) { // move
        gtk_shot_move_section(shot, dx, dy);
      } else { // resize
        gint w = ABS(shot->section.width + dx);
        gint h = ABS(shot->section.height + dy);
        if (w == 0 || h == 0) break;
        shot->section.width = w;
        shot->section.height = h;
      }
      gtk_shot_refresh(shot);
      gtk_shot_show_toolbar(shot);
      break;
  }
  if (is_ctrl) {
    switch (event->keyval) {
      case GDK_s: // save
        gtk_shot_save_section_to_file(shot);
        break;
      case GDK_o: // ok
        gtk_shot_save_section_to_clipboard(shot);
        gtk_shot_hide(shot);
        break;
      case GDK_a: // select whole
        shot->section.x = shot->x - shot->section.border;
        shot->section.y = shot->y - shot->section.border;
        shot->section.width = shot->width + 2 * shot->section.border;
        shot->section.height = shot->height + 2 * shot->section.border;
        gtk_shot_refresh(shot);
        gtk_shot_show_toolbar(shot);
        if (gtk_shot_pen_editor_visible(shot->pen_editor)) {
          gtk_shot_pen_editor_show(shot->pen_editor);
        }
        break;
      case GDK_d: // dynamic
        break;
      case GDK_r: // record
        gtk_shot_record(shot);
        break;
      case GDK_z: // undo
        gtk_shot_undo_pen(shot);
        gtk_shot_refresh(shot);
        break;
      case GDK_y: // redo
        break;
      case GDK_q: // quit
        gtk_shot_quit(shot);
        break;
    }
  }
  return TRUE;
}

gboolean on_shot_key_release(GtkWidget *widget
                                      , GdkEventKey *event) {
  return FALSE;
}

void gtk_shot_process_edit_mode(GtkShot *shot
                                    , GdkEventButton *event) {
  if (shot->pen->type != GTK_SHOT_PEN_TEXT) {
    gdk_point_assign(shot->pen->start, *event);
    gdk_point_assign(shot->pen->end, *event);
  } else {
    gint x0, y0, x1, y1;
    gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);
    if (!IS_IN_RECT(event->x, event->y, x0, y0, x1, y1)) {
      gtk_shot_input_hide(shot->input);
      gtk_shot_grab_keyboard(shot);
    } else {
      // 完成输入,并保存当前画笔
      char *text = gtk_shot_input_get_text(shot->input);
      if (text != NULL && strlen(text) > 0) {
        shot->pen->text.content = text;
        gtk_shot_save_pen(shot, FALSE);
        gtk_shot_refresh(shot);
        gtk_shot_input_hide(shot->input);
        gtk_shot_grab_keyboard(shot);
      } else {
        // 弹出文本输入框,接受输入
        gtk_shot_input_set_font(shot->input
                                  , shot->pen->text.fontname
                                  , shot->pen->color);
        gdk_point_assign(shot->pen->start, *event);
        gdk_point_assign(shot->pen->end, *event);
        gdk_keyboard_ungrab(GDK_CURRENT_TIME);
        gtk_shot_input_show(shot->input, event->x, event->y);
      }
    }
  }
}

void gtk_shot_draw_screen(GtkShot *shot, cairo_t *cr) {
  if (shot->screen_pixbuf) {
    gdk_cairo_set_source_pixbuf(cr, shot->screen_pixbuf
                                  , 0, 0);
    cairo_paint(cr);
  }
}

void gtk_shot_draw_section(GtkShot *shot, cairo_t *cr) {
  if (shot->section.width > 0
          || shot->section.height > 0) {
    // transparent section
    SET_CAIRO_RGBA(cr, 0, 0);
    cairo_rectangle(cr, shot->section.x
                      , shot->section.y
                      , shot->section.width
                      , shot->section.height);
    cairo_fill(cr);
    // draw doodle
    gtk_shot_draw_doodle(shot, cr);
    // draw mask around section
    gtk_shot_draw_mask(shot, cr);
    // draw section border
    cairo_set_line_width(cr, shot->section.border);
    SET_CAIRO_RGB(cr, shot->section.color);
    cairo_rectangle(cr, shot->section.x
                      , shot->section.y
                      , shot->section.width
                      , shot->section.height);
    cairo_stroke(cr);
    // draw zoom anchor
    gtk_shot_draw_anchor(shot, cr);
  } else {
    // 选区高宽为0时,所画MASK必然覆盖整个画布
    gtk_shot_draw_mask(shot, cr);
  }
}

void gtk_shot_draw_doodle(GtkShot *shot, cairo_t *cr) {
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
}

void gtk_shot_draw_mask(GtkShot *shot, cairo_t *cr) {
  gint x0, y0, x1, y1;
  gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);

  SET_CAIRO_RGBA(cr, shot->color, 1 - shot->opacity);
  cairo_rectangle(cr, shot->x, shot->y
                    , shot->width, y0 - shot->y);
  cairo_rectangle(cr, shot->x, y1
                    , shot->width, shot->height + shot->y - y1);
  cairo_rectangle(cr, shot->x, y0
                    , x0 - shot->x, y1 - y0);
  cairo_rectangle(cr, x1, y0
                    , shot->width + shot->x - x1, y1 - y0);
  cairo_fill(cr);
}

void gtk_shot_draw_anchor(GtkShot *shot, cairo_t *cr) {
  GdkPoint anchor[8] = {{.x = 0, .y = 0}};

  gtk_shot_get_zoom_anchor(shot, anchor);
  SET_CAIRO_RGB(cr, shot->section.color);

  gint i = 0;
  gfloat b = shot->anchor_border / 2.0;
  for (i = 0; i < 8; i++) {
    cairo_arc(cr, anchor[i].x + b
                , anchor[i].y + b
                , b, 0, 2 * M_PI);
    cairo_fill(cr);
  }
}

void gtk_shot_draw_message(GtkShot *shot, cairo_t *cr) {
  // 选区不存在,或在保存和编辑模式时,不显示信息窗口
  if ((shot->section.width == 0 && shot->section.height == 0)
        || shot->mode == EDIT_MODE
        || shot->mode == SAVE_MODE) {
    return;
  }
  gint x0, y0, x1, y1;
  gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);

  gint radius = 8, width = 145, height = 50;
  gint b = MAX(shot->section.border, shot->anchor_border);
  gint x = x0 + shot->section.border;
  gint y = y0 - b - height;

  if (x1 >= shot->x + shot->width && x + width > x1) {
    x = x1 - width - b; // 保持在屏幕内
  }
  if (y < shot->y) {
    y = y0 + shot->section.border;
  }
  gchar *msg = g_strdup_printf("x:%4d, y:%4d\nw:%4d, h:%4d\n"
                                    , x0, y0
                                    , MAX(x1 - x0, 0)
                                    , MAX(y1 - y0, 0));
  SET_CAIRO_RGBA(cr, 0x232126, shot->opacity / 2.0);
  cairo_round_rect(cr, x, y, width, height, radius);
  cairo_fill(cr);

  SET_CAIRO_RGB(cr, 0xFFFFFF);
  cairo_move_to(cr, x + radius, y + radius / 3.0);
  cairo_draw_text(cr, msg, "");

  g_free(msg);
}

void gtk_shot_clean_section(GtkShot *shot) {
  gdk_point_assign(shot->move_start, shot->move_end);
  shot->section.width = shot->section.height = 0;

  gtk_shot_remove_pen(shot);
  gtk_shot_clean_historic_pen(shot);
}

void gtk_shot_clean_historic_pen(GtkShot *shot) {
  GSList *l = shot->historic_pen;
  for (l; l; l = l->next) {
    gtk_shot_pen_free(GTK_SHOT_PEN(l->data));
  }
  g_slist_free(shot->historic_pen);
  shot->historic_pen = NULL;
}

void gtk_shot_move_section(GtkShot *shot, gint dx, gint dy) {
  shot->section.x += dx;
  shot->section.y += dy;

  shot->move_start.x += dx;
  shot->move_start.y += dy;
  shot->move_end.x += dx;
  shot->move_end.y += dy;
}

void gtk_shot_resize_section(GtkShot *shot, gint x, gint y) {
  gint x0, y0, x1, y1;
  x0 = shot->section.x;
  y0 = shot->section.y;
  x1 = x0 + shot->section.width;
  y1 = y0 + shot->section.height;

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
    if (dx * dy < 0
            && shot->cursor_pos % 2 == LEFT_TOP_OF_SECTION % 2) {
      if (shot->cursor_pos == LEFT_TOP_OF_SECTION
            || shot->cursor_pos == RIGHT_BOTTOM_OF_SECTION) {
        diff = dx < 0 ? 2 : 6; // dx < 0: 与顺时针方向的相邻对角点互换
      } else {
        diff = dx < 0 ? 6 : 2; // dx < 0: 与逆时针方向的相邻对角点互换
      }
    }
    shot->cursor_pos =
            (GtkShotCursorPos) ((shot->cursor_pos + diff) % 8);
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

void gtk_shot_adjust_toolbar(GtkShot *shot) {
  gint x = 0, y = 0, x0, y0, x1, y1;
  gint b = MAX(shot->section.border, shot->anchor_border);
  gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);

  // 扩展到锚点外部矩形范围
  x0 -= b;
  y0 -= b;
  x1 += b;
  y1 += b;
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

  gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);

  x0 -= shot->anchor_border;
  y0 -= shot->anchor_border;

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

GtkShotCursorPos gtk_shot_get_cursor_pos(GtkShot *shot
                                              , gint x, gint y) {
  GtkShotCursorPos pos = OUTER_OF_SECTION;

  // 没有选择区域,自然在外部了 :-)
  if (shot->section.width <= 0 && shot->section.height <= 0) {
    return OUTER_OF_SECTION;
  }
  gint x0, y0, x1, y1;
  gtk_shot_get_section(shot, &x0, &y0, &x1, &y1);

  if (IS_IN_RECT(x, y, x0, y0, x1, y1)) {
    pos = INNER_OF_SECTION;
  } else if (IS_OUT_RECT(x, y
                          , x0 - shot->section.border
                          , y0 - shot->section.border
                          , x1 + shot->section.border
                          , y1 + shot->section.border)) {
    pos = OUTER_OF_SECTION;
    // 继续缩放锚点上的位置判断
    GdkPoint anchor[8] = {{.x = 0, .y = 0}};
    gint i = 0;
    gtk_shot_get_zoom_anchor(shot, anchor);
    for (i = 0; i < 8; i++) {
      if (IS_IN_RECT(x, y, anchor[i].x, anchor[i].y
                        , anchor[i].x + shot->anchor_border
                        , anchor[i].y + shot->anchor_border)) {
        pos = (GtkShotCursorPos) i; // :-)
      }
    }
  } else {
    if (x <= x0) {
      if (y <= y0) {
        pos = LEFT_TOP_OF_SECTION;
      } else if (y >= y1) {
        pos = LEFT_BOTTOM_OF_SECTION;
      } else {
        pos = LEFT_OF_SECTION;
      }
    } else if (x >= x1) {
      if (y <= y0) {
        pos = RIGHT_TOP_OF_SECTION;
      } else if (y >= y1) {
        pos = RIGHT_BOTTOM_OF_SECTION;
      } else {
        pos = RIGHT_OF_SECTION;
      }
    } else if (y <= y0) {
      pos = TOP_OF_SECTION;
    } else {
      pos = BOTTOM_OF_SECTION;
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
  } else if (shot->mode == EDIT_MODE) {
    if (shot->cursor_pos == INNER_OF_SECTION) {
      cursor = shot->edit_cursor;
    } else {
      cursor = GDK_LEFT_PTR;
    }
  }
  gdk_window_set_cursor(GTK_WIDGET(shot)->window
                          , gdk_cursor_new(cursor));
}

void gtk_shot_grab_keyboard(GtkShot *shot) {
  gint i = 0;
  while (gdk_keyboard_grab(GTK_WIDGET(shot)->window
                                , FALSE
                                , GDK_CURRENT_TIME)
            != GDK_GRAB_SUCCESS) {
    if (i++ < GRAB_KEY_TRY_COUNT) {
      debug("grab keyboard again after 0.1s ...\n");
      sleep(0.1);
    } else {
      debug("grab keyboard failed after 10 times.\n");
      break;
    }
  }
}
