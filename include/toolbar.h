#ifndef _GTK_SHOT_TOOLBAR_H_
#define _GTK_SHOT_TOOLBAR_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GTK_SHOT_TOOLBAR(obj) ((GtkShotToolbar*) obj)

typedef struct _GtkShotToolbar {
  GtkWindow *window;
  GtkBox *pen_box, *op_box;
  GSList *buttons, *toggle_buttons;
  gint x, y;
  gint width, height;

  struct _GtkShot *shot;
} GtkShotToolbar;

GtkShotToolbar* gtk_shot_toolbar_new(GtkWindow *parent);
void gtk_shot_toolbar_show(GtkShotToolbar *toolbar);
void gtk_shot_toolbar_hide(GtkShotToolbar *toolbar);

#ifdef __cplusplus
}
#endif

#endif
