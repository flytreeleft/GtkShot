#include <config.h>

#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include "utils.h"

#include "pen.h"

#define PREPARE_PEN_AND_CAIRO(pen, cr) \
        do { \
          g_return_if_fail((pen) != NULL); \
          g_return_if_fail((cr) != NULL); \
          cairo_set_line_width((cr), (pen)->size); \
          cairo_set_source_rgb((cr), RGB_R((pen)->color) / 255.0 \
                                    , RGB_G((pen)->color) / 255.0 \
                                    , RGB_B((pen)->color) / 255.0); \
        } while(0)

GtkShotPen* gtk_shot_pen_new(GtkShotPenType type) {
  GtkShotPen *pen = g_new(GtkShotPen, 1);

  pen->size = GTK_SHOT_DEFAULT_PEN_SIZE;
  pen->color = GTK_SHOT_DEFAULT_PEN_COLOR;
  pen->tracks = NULL;
  pen->text.fontname = pen->text.content = NULL;
  pen->type = type;
  pen->save_track = gtk_shot_pen_save_general_track;
  switch(type) {
    case GTK_SHOT_PEN_RECT:
      pen->draw_track = gtk_shot_pen_draw_rectangle;
      break;
    case GTK_SHOT_PEN_ELLIPSE:
      pen->draw_track = gtk_shot_pen_draw_ellipse;
      break;
    case GTK_SHOT_PEN_ARROW:
      pen->draw_track = gtk_shot_pen_draw_arrow;
      break;
    case GTK_SHOT_PEN_LINE:
      pen->draw_track = gtk_shot_pen_draw_line;
      pen->save_track = gtk_shot_pen_save_line_track;
      break;
    case GTK_SHOT_PEN_TEXT:
      pen->size = 0;
      pen->text.fontname = g_strdup(GTK_SHOT_DEFAULT_PEN_FONT);
      pen->draw_track = gtk_shot_pen_draw_text;
      break;
  }

  return pen;
}

void gtk_shot_pen_free(GtkShotPen *pen) {
  if (!pen) return;
  if (pen->type == GTK_SHOT_PEN_LINE) {
    GSList *l = pen->tracks;
    for (l; l; l = l->next) {
      if (l->data) g_free(l->data);
    }
    g_slist_free(pen->tracks);
  } else if (pen->type == GTK_SHOT_PEN_TEXT) {
    g_free(pen->text.fontname);
    g_free(pen->text.content);
  }
  g_free(pen);
}

/** 画笔复位: 仅将其附属属性(attached)赋值为NULL */
void gtk_shot_pen_reset(GtkShotPen *pen) {
  if (pen) {
    pen->tracks = NULL;
    pen->text.fontname = pen->text.content = NULL;
    // 由于窗口绘制有延时,在画笔复位完成时,
    // 窗口可能还未完成绘制,故将画笔的起始位置进行调整,
    // 使其不可见 :-(
    pen->start.x = pen->start.y = -100;
    gdk_point_assign(pen->start, pen->end);
  }
}

void gtk_shot_pen_save_general_track(GtkShotPen *pen
                                      , gint x, gint y) {
  g_return_if_fail(pen != NULL);
  pen->end.x = x; pen->end.y = y;
}

/**
 * 保存线条的所有点,当新点和最后一个点相同时,不记录该点
 */
void gtk_shot_pen_save_line_track(GtkShotPen *pen
                                      , gint x, gint y) {
  g_return_if_fail(pen != NULL);
  pen->end.x = x; pen->end.y = y;

  GdkPoint *last = NULL;
  // 倒序排列
  last = pen->tracks != NULL
                  ? (GdkPoint*) pen->tracks->data
                    : NULL;
  if (last && gdk_point_is_equal(*last, pen->end)) {
    return;
  }
  last = g_new(GdkPoint, 1);
  last->x = x; last->y = y;
  pen->tracks = g_slist_prepend(pen->tracks, last);
}

void gtk_shot_pen_draw_rectangle(GtkShotPen *pen, cairo_t *cr) {
  PREPARE_PEN_AND_CAIRO(pen, cr);

  cairo_rectangle(cr, pen->start.x, pen->start.y
                    , pen->end.x - pen->start.x
                    , pen->end.y - pen->start.y);
  cairo_stroke(cr);
}

void gtk_shot_pen_draw_ellipse(GtkShotPen *pen, cairo_t *cr) {
  PREPARE_PEN_AND_CAIRO(pen, cr);

  gint dx = ABS(pen->end.x - pen->start.x);
  gint dy = ABS(pen->end.y - pen->start.y);

  cairo_save(cr);
  cairo_translate(cr, (pen->start.x + pen->end.x) / 2.0
                    , (pen->start.y + pen->end.y) / 2.0);
  if (dx > 0 && dy > 0) {
    cairo_scale(cr, 1.0, ((gfloat) dy) / dx);
    cairo_arc(cr, 0, 0, dx / 2.0, 0, 2 * M_PI);
  } else {
    // 高或宽为0时,则画矩形
    cairo_rectangle(cr, -dx / 2.0, -dy / 2.0, dx, dy);
  }
  cairo_restore(cr);

  cairo_stroke(cr);
}

/**
 * 绘制箭头线(由箭头和线身两部分构成)<br>
 * 箭头为等腰三角形,其顶点为箭头线的终点,
 * 腰长为sqrt(2)倍线宽(实际绘制时,线宽为指定size的2倍),
 * 高度等于线宽.
 * 箭头线的线身长为整个箭头线的长度减去1/2线宽
 */
void gtk_shot_pen_draw_arrow(GtkShotPen *pen, cairo_t *cr) {
  PREPARE_PEN_AND_CAIRO(pen, cr);

  gfloat alpha = 45 * (M_PI / 180); // 箭头腰与直线的夹角
  gfloat l = sqrt(2) * (2 * pen->size); // 箭头腰长
  // (a,b)为起点到终点的方向向量
  gfloat a = pen->end.x - pen->start.x;
  gfloat b = pen->end.y - pen->start.y;
  gfloat a2b2 = a * a + b * b;
  // 箭头腰向量与直线的向量积
  gfloat m = sqrt(a2b2)*l*cos(alpha);
  // 箭头腰向量(x0,y0),(x1,y1)
  gfloat x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  // 线身的终点坐标
  gfloat x = pen->end.x, y = pen->end.y;

  // 不可将浮点变量用"=="或"!="与任何数字比较
  // 参考: http://hi.baidu.com/ecgql/blog/item/fde8d617c496f50ec83d6d7a.html
  if (a2b2 < -FLT_EPSILON || a2b2 > FLT_EPSILON) {
    // for speed :-(
    gfloat temp = sqrt(a*a*m*m - a2b2*(m*m - b*b*l*l));
    x0 = (a*m + temp) / a2b2;
    x1 = (a*m - temp) / a2b2;
    if (b < -FLT_EPSILON || b > FLT_EPSILON) {
      y0 = (m - a*x0) / b;
      y1 = (m - a*x1) / b;
    } else { // 两个极小数的平方和不一定为极小数(可视为0的数)
      temp = sqrt(b*b*m*m - a2b2*(m*m - a*a*l*l));
      y0 = (b*m + temp) / a2b2;
      y1 = (b*m - temp) / a2b2;
    }
    x = a - (a*pen->size/(sqrt(a2b2))) + pen->start.x;
    y = b - (b*pen->size/(sqrt(a2b2))) + pen->start.y;
  }
  // 箭头另外两点坐标
  x0 = pen->end.x - x0;
  y0 = pen->end.y - y0;
  x1 = pen->end.x - x1;
  y1 = pen->end.y - y1;

  // 绘制线身
  cairo_move_to(cr, pen->start.x, pen->start.y);
  cairo_line_to(cr, x, y);
  cairo_stroke(cr);
  // 绘制箭头(填充)
  cairo_set_line_width(cr, 1);
  cairo_move_to(cr, pen->end.x, pen->end.y);
  cairo_line_to(cr, x0, y0);
  cairo_line_to(cr, x1, y1);
  cairo_close_path(cr);
  cairo_fill(cr);
}

void gtk_shot_pen_draw_line(GtkShotPen *pen, cairo_t *cr) {
  PREPARE_PEN_AND_CAIRO(pen, cr);
  // 从尾部到头部画线
  cairo_move_to(cr, pen->end.x, pen->end.y);

  GSList *l = pen->tracks;
  for (l; l; l = l->next) {
    GdkPoint *p = (GdkPoint*) l->data;
    cairo_line_to(cr, p->x, p->y);
  }
  cairo_line_to(cr, pen->start.x, pen->start.y);
  cairo_stroke(cr);
}

void gtk_shot_pen_draw_text(GtkShotPen *pen, cairo_t *cr) {
  PREPARE_PEN_AND_CAIRO(pen, cr);
  if (!pen->text.content) {
    return;
  }

  // Thanks deepin-screenshot START
  cairo_move_to(cr, pen->start.x, pen->start.y - SYSTEM_CURSOR_SIZE / 2);

  PangoLayout *layout = pango_cairo_create_layout(cr);
  PangoFontDescription *desc
        = pango_font_description_from_string(pen->text.fontname);
  pango_layout_set_text(layout, pen->text.content, -1);
  pango_layout_set_font_description(layout, desc);
  pango_font_description_free(desc);

  pango_cairo_update_layout(cr, layout);
  pango_cairo_show_layout(cr, layout);
  // Thanks deepin-screenshot END
}

