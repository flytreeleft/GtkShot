#ifndef _GTK_SHOT_INPUT_H_
#define _GTK_SHOT_INPUT_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GTK_SHOT_INPUT(obj) ((GtkShotInput*) obj)

typedef struct _GtkShotInput {
  GtkWindow *window;
  GtkTextView *view;
} GtkShotInput;

GtkShotInput* gtk_shot_input_new(GtkWindow *parent);
void gtk_shot_input_show(GtkShotInput *input, gint x, gint y);
void gtk_shot_input_hide(GtkShotInput *input);
void gtk_shot_input_set_font(GtkShotInput *input
                                  , const char *fontname
                                  , gint color);
gchar* gtk_shot_input_get_text(GtkShotInput *input);

#ifdef __cplusplus
}
#endif

#endif

