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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "utils.h"

#include "xpm.h"
#include "shot.h"
#include "toolbar.h"

typedef struct _PenButton {
  const char **xpm;
  const char *tips;
  GtkShotPenType type;
} PenButton;
static PenButton pen_btns[] = {
  {.xpm = rectangle_xpm, .tips = N_("draw rectangle"), .type = GTK_SHOT_PEN_RECT},
  {.xpm = ellipse_xpm, .tips = N_("draw ellipse"), .type = GTK_SHOT_PEN_ELLIPSE},
  {.xpm = arrow_xpm, .tips = N_("draw arrow"), .type = GTK_SHOT_PEN_ARROW},
  {.xpm = line_xpm, .tips = N_("draw line"), .type = GTK_SHOT_PEN_LINE},
  {.xpm = text_xpm, .tips = N_("draw text"), .type = GTK_SHOT_PEN_TEXT}
};

// Button Events
static gboolean on_change_pen(GtkToggleButton *btn
                                  , GtkShotToolbar *toolbar);
static gboolean on_undo(GtkButton *btn
                            , GtkShotToolbar *toolbar);
static gboolean on_save_to_file(GtkButton *btn
                                    , GtkShotToolbar *toolbar); 
static gboolean on_save_to_clipboard(GtkButton *btn
                                        , GtkShotToolbar *toolbar); 
static gboolean on_quit(GtkButton *btn, GtkShotToolbar *toolbar);

static void adjust_toolbar(GtkShotToolbar *toolbar);
static GtkBox* create_pen_box(GtkShotToolbar *toolbar);
static GtkBox* create_op_box(GtkShotToolbar *toolbar);

GtkShotToolbar* gtk_shot_toolbar_new(GtkShot *shot) {
  gint width = 338, height = 36;
  GtkShotToolbar *toolbar = g_new(GtkShotToolbar, 1);
  GtkWindow *window =
        create_popup_window(GTK_WINDOW(shot), width, height);
  
  GtkBox *hbox = GTK_BOX(gtk_hbox_new(FALSE, 2));
  toolbar->pen_box = create_pen_box(toolbar);
  toolbar->op_box = create_op_box(toolbar);

  pack_to_box(hbox, toolbar->pen_box);
  pack_to_box(hbox, gtk_vseparator_new());
  pack_to_box(hbox, toolbar->op_box);
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));

  toolbar->shot = shot;
  toolbar->pen_editor = gtk_shot_pen_editor_new(shot);
  toolbar->window = window;
  toolbar->x = 0;
  toolbar->y = 0;
  toolbar->width = width;
  toolbar->height = height;

  return toolbar;
}

void gtk_shot_toolbar_destroy(GtkShotToolbar *toolbar) {
  if (toolbar) {
    gtk_shot_pen_editor_destroy(toolbar->pen_editor);
    gtk_widget_destroy(GTK_WIDGET(toolbar->window));
    g_free(toolbar);
  }
}

void gtk_shot_toolbar_show(GtkShotToolbar *toolbar) {
  if (toolbar) {
    adjust_toolbar(toolbar);
    gtk_widget_show_all(GTK_WIDGET(toolbar->window));
  }
}

void gtk_shot_toolbar_hide(GtkShotToolbar *toolbar) {
  if (gtk_shot_toolbar_visible(toolbar)) {
    gtk_shot_pen_editor_hide(toolbar->pen_editor);
    // 取消所有按钮的激活态
    GList *l = gtk_container_get_children(GTK_CONTAINER(toolbar->pen_box));
    set_all_toggle_button_inactive(l);
    gtk_widget_hide_all(GTK_WIDGET(toolbar->window));
  }
}

void gtk_shot_toolbar_move(GtkShotToolbar *toolbar, gint x, gint y) {
  if (toolbar) {
    toolbar->x = x; toolbar->y = y;
    gtk_window_move(toolbar->window, x, y);
  }
}

gboolean on_change_pen(GtkToggleButton *btn
                                , GtkShotToolbar *toolbar) {
  GList *l = gtk_container_get_children(GTK_CONTAINER(toolbar->pen_box));
  // 查找其他已激活按钮(只会有一个)
  GtkToggleButton *act = get_active_toggle_button_except(l, btn);

  if (!btn->active && !act) {
    // 没有已激活的按钮,则移除画笔
    gtk_shot_remove_pen(toolbar->shot);
    gtk_shot_pen_editor_set_pen(toolbar->pen_editor, NULL);
    gtk_shot_pen_editor_hide(toolbar->pen_editor);
  } else if (btn->active) {
    if (act) { // 取消其他已激活按钮的激活状态
      gtk_toggle_button_set_active(act, FALSE);
    }
    GtkShotPenType type = (GtkShotPenType)
                GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn)
                                              , "pen-type"));
    GtkShotPen *pen = gtk_shot_pen_new(type);
    gtk_shot_set_pen(toolbar->shot, pen);
    gtk_shot_pen_editor_set_pen(toolbar->pen_editor, pen);
    gtk_shot_pen_editor_show(toolbar->pen_editor);
  }

  return TRUE;
}

gboolean on_undo(GtkButton *btn, GtkShotToolbar *toolbar) {
  g_return_val_if_fail(!gtk_shot_has_empty_historic_pen(toolbar->shot), FALSE);

  gtk_shot_undo_pen(toolbar->shot);
  /*if (gtk_shot_has_empty_historic_pen(toolbar->shot)) {
    GList *l = gtk_container_get_children(GTK_CONTAINER(toolbar->pen_box));
    set_all_toggle_button_inactive(l);
  }*/
  gtk_shot_refresh(toolbar->shot);

  return TRUE;
}

gboolean on_save_to_file(GtkButton *btn, GtkShotToolbar *toolbar) {
  gtk_shot_save_section_to_file(toolbar->shot);
  return TRUE;
}

gboolean on_save_to_clipboard(GtkButton *btn
                                , GtkShotToolbar *toolbar) {
  gtk_shot_save_section_to_clipboard(toolbar->shot);
  gtk_shot_hide(toolbar->shot);
  return TRUE;
}

gboolean on_quit(GtkButton *btn, GtkShotToolbar *toolbar) {
  gtk_shot_quit(toolbar->shot);
  return TRUE;
}

GtkBox* create_pen_box(GtkShotToolbar *toolbar) {
  GtkWidget *btn;
  GtkBox *box = GTK_BOX(gtk_hbox_new(FALSE, 2));
  gint i = 0, size = sizeof(pen_btns) / sizeof(PenButton);

  for (i = 0; i < size; i++) {
    btn = create_xpm_button(pen_btns[i].xpm
                                , _(pen_btns[i].tips)
                                , G_CALLBACK(on_change_pen)
                                , TRUE, toolbar);
    g_object_set_data(G_OBJECT(btn)
                          , "pen-type"
                          , GINT_TO_POINTER(pen_btns[i].type));
    pack_to_box(box, btn);
  }

  return box;
}

GtkBox* create_op_box(GtkShotToolbar *toolbar) {
  GtkWidget *btn;
  GtkBox *box = GTK_BOX(gtk_hbox_new(FALSE, 2));

  btn = create_icon_button(GTK_STOCK_UNDO
                                  , _("undo")
                                  , G_CALLBACK(on_undo)
                                  , FALSE, toolbar);
  pack_to_box(box, btn);
  btn = create_icon_button(GTK_STOCK_SAVE
                                  , _("save to file")
                                  , G_CALLBACK(on_save_to_file)
                                  , FALSE, toolbar);
  pack_to_box(box, btn);
  pack_to_box(box, gtk_vseparator_new());
  btn = create_icon_button(GTK_STOCK_QUIT
                                  , _("exit")
                                  , G_CALLBACK(on_quit)
                                  , FALSE, toolbar);
  pack_to_box(box, btn);
  btn = create_icon_button(GTK_STOCK_APPLY
                                  , _("finish")
                                  , G_CALLBACK(on_save_to_clipboard)
                                  , FALSE, toolbar);
  pack_to_box(box, btn);

  return box;
}

void adjust_toolbar(GtkShotToolbar *toolbar) {
  GtkShot *shot = toolbar->shot;
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

  x = x0 + ((x1 - x0) - toolbar->width) / 2;
  x = MAX(MIN(x, shot->width - toolbar->width), 0);
#define TS_SPACE  2
  if (y1 <= shot->height
              - toolbar->height
              - toolbar->pen_editor->height
              - TS_SPACE) {
    y = y1;
  } else {
    y = MAX(y0 - toolbar->height, 0);
  }
  x += shot->x; y += shot->y;

  gtk_shot_toolbar_move(toolbar, x, y);
  if (y != y1 && y0 >= toolbar->height
                        + toolbar->pen_editor->height
                        + TS_SPACE) {
    y -= toolbar->pen_editor->height + TS_SPACE;
  } else {
    y += toolbar->height + TS_SPACE;
  }
  gtk_shot_pen_editor_move(toolbar->pen_editor, x, y);
}
