#ifndef _GTK_SHOT_PEN_EDITOR_H_
#define _GTK_SHOT_PEN_EDITOR_H_

#include <gtk/gtk.h>

#include "pen.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GTK_SHOT_PEN_EDITOR(obj)  ((GtkShotPenEditor*) obj)

typedef struct _GtkShotPenEditor {
  GtkWindow *window;

  GtkShotPen *pen;
  GtkBox *left_box, *right_box;
  GtkBox *size_box, *font_box;
  gint x, y;
  gint width, height;

  // 当前设置的画笔属性
  gint size, color;
  gchar *fontname;
} GtkShotPenEditor;

GtkShotPenEditor* gtk_shot_pen_editor_new(GtkWindow *parent);
void gtk_shot_pen_editor_show(GtkShotPenEditor *editor);
void gtk_shot_pen_editor_hide(GtkShotPenEditor *editor);
void gtk_shot_pen_editor_set_pen(GtkShotPenEditor *editor
                                          , GtkShotPen *pen);

#ifdef __cplusplus
}
#endif

#endif

