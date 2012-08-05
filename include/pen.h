#ifndef _GTK_SHOT_PEN_H_
#define _GTK_SHOT_PEN_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GTK_SHOT_PEN(obj) ((GtkShotPen*) obj)

typedef enum _GtkShotPenType{
  GTK_SHOT_PEN_RECT,
  GTK_SHOT_PEN_ELLIPSE,
  GTK_SHOT_PEN_ARROW,
  GTK_SHOT_PEN_LINE,
  GTK_SHOT_PEN_TEXT
} GtkShotPenType;

typedef struct _GtkShotPen GtkShotPen;
struct _GtkShotPen {
  GtkShotPenType type;
  GdkPoint start, end;
  gint size, color;
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

