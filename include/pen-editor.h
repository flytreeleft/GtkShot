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

#ifndef _GTK_SHOT_PEN_EDITOR_H_
#define _GTK_SHOT_PEN_EDITOR_H_

#include <gtk/gtk.h>

#include "pen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GtkShotPenEditor GtkShotPenEditor;

#define GTK_SHOT_PEN_EDITOR(obj)  ((GtkShotPenEditor*) obj)

struct _GtkShotPenEditor {
  GtkWindow *window;

  GtkShotPen *pen;
  GtkBox *left_box, *right_box;
  GtkBox *size_box, *font_box;
  gint width, height;

  // 当前设置的画笔属性
  gint size, color;
  gchar *fontname;
};

GtkShotPenEditor* gtk_shot_pen_editor_new(GtkShot *shot);
void gtk_shot_pen_editor_destroy(GtkShotPenEditor *editor);
void gtk_shot_pen_editor_show(GtkShotPenEditor *editor);
void gtk_shot_pen_editor_hide(GtkShotPenEditor *editor);
void gtk_shot_pen_editor_set_pen(GtkShotPenEditor *editor
                                          , GtkShotPen *pen);
#define gtk_shot_pen_editor_visible(editor) \
        ((editor) && gtk_widget_get_visible(GTK_WIDGET((editor)->window)))
#define gtk_shot_pen_editor_move(editor, x, y) \
        gtk_window_move(editor->window, x, y)

#ifdef __cplusplus
}
#endif

#endif
