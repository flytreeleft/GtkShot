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

#ifndef _GTK_SHOT_TOOLBAR_H_
#define _GTK_SHOT_TOOLBAR_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GtkShotToolbar GtkShotToolbar;

#define GTK_SHOT_TOOLBAR(obj) ((GtkShotToolbar*) obj)

struct _GtkShotToolbar {
  GtkWindow *window;
  GtkBox *pen_box, *op_box;
  GSList *buttons, *toggle_buttons;
  gint x, y;
  gint width, height;

  GtkShot *shot;
};

GtkShotToolbar* gtk_shot_toolbar_new(GtkShot *shot);
void gtk_shot_toolbar_destroy(GtkShotToolbar *toolbar);
void gtk_shot_toolbar_show(GtkShotToolbar *toolbar);
void gtk_shot_toolbar_hide(GtkShotToolbar *toolbar);

#ifdef __cplusplus
}
#endif

#endif
