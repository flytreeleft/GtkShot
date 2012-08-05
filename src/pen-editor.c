#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "utils.h"

#include "xpm.h"
#include "pen-editor.h"

typedef struct _ColorMap {
  const char *name;
  gint color;
} ColorMap;
static ColorMap color_maps[] = {
  {.name = N_("white"), .color = 0xffffff},
  {.name = N_("gray"), .color = 0xc0c0c0},
  {.name = N_("red"), .color = 0xff0000},
  {.name = N_("yellow"), .color = 0xffff00},
  {.name = N_("green"), .color = 0x00ff00},
  {.name = N_("blue"), .color = 0x0000ff},
  {.name = N_("pink"), .color = 0xff00ff},
  {.name = N_("wathet"), .color = 0x00ffff},
  {.name = N_("black"), .color = 0x000000},
  {.name = N_("dark gray"), .color = 0x808080},
  {.name = N_("dark red"), .color = 0x800000},
  {.name = N_("dark yellow"), .color = 0x808000},
  {.name = N_("dark green"), .color = 0x008000},
  {.name = N_("dark blue"), .color = 0x000080},
  {.name = N_("dark pink"), .color = 0x800080},
  {.name = N_("dark wathet"), .color = 0x008080}
};

// Button Events
static void
    on_set_pen_size(GtkToggleButton *btn, GtkShotPenEditor *editor);
static void
    on_set_pen_font(GtkFontButton *btn, GParamSpec *pspec
                            , GtkShotPenEditor *editor);
static void
    on_set_pen_color(GtkColorButton *btn, GParamSpec *pspec
                            , GtkShotPenEditor *editor);
static void
    on_change_color(GtkButton *btn, GtkColorButton *color_btn);
static gboolean
    on_color_button_expose(GtkWidget *widget, GdkEventExpose *event
                              , gpointer *color);

static GtkBox* create_size_box(GtkShotPenEditor *editor);
static GtkBox* create_font_box(GtkShotPenEditor *editor);
static GtkBox* create_color_box(GtkShotPenEditor *editor);

GtkShotPenEditor* gtk_shot_pen_editor_new(GtkWindow *parent) {
  GtkShotPenEditor *editor = g_new(GtkShotPenEditor, 1);

  GtkWindow *window =
            GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_transient_for(window, parent);
  gtk_widget_set_can_focus(GTK_WIDGET(window), TRUE);
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
  gtk_window_set_resizable(window, FALSE);
  gtk_window_set_default_size(window, 305, 36);
  
  GtkBox *hbox = GTK_BOX(gtk_hbox_new(FALSE, 2));
  GtkBox *left_box = GTK_BOX(gtk_hbox_new(FALSE, 2));
  GtkBox *right_box = GTK_BOX(gtk_hbox_new(FALSE, 2));
  GtkBox *size_box = create_size_box(editor);
  GtkBox *font_box = create_font_box(editor);
  GtkBox *color_box = create_color_box(editor);

  pack_to_box(left_box, size_box);
  pack_to_box(right_box, color_box);
  pack_to_box(hbox, left_box);
  pack_to_box(hbox, gtk_vseparator_new());
  pack_to_box(hbox, right_box);

  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));

  editor->pen = NULL;
  editor->window = window;
  editor->left_box = left_box;
  editor->right_box = right_box;
  editor->size_box = size_box;
  editor->font_box = font_box;
  editor->x = editor->y = 0;
  editor->width = 305;
  editor->height = 36;

  editor->size = GTK_SHOT_DEFAULT_PEN_SIZE;
  editor->color = GTK_SHOT_DEFAULT_PEN_COLOR;
  editor->fontname = GTK_SHOT_DEFAULT_PEN_FONT;

  return editor;
}

void gtk_shot_pen_editor_show(GtkShotPenEditor *editor) {
  g_return_if_fail(editor != NULL);
  if (!editor->pen) return;
  
  GList *l =
        gtk_container_get_children(GTK_CONTAINER(editor->left_box));
  GtkWidget *child = GTK_WIDGET(g_list_nth_data(l, 0));

  if (editor->pen->type == GTK_SHOT_PEN_TEXT
                      && child == editor->size_box) {
    g_object_ref(editor->size_box); // 增加引用,防止被删除
    gtk_container_remove(GTK_CONTAINER(editor->left_box)
                            , GTK_WIDGET(editor->size_box));
    gtk_container_add(GTK_CONTAINER(editor->left_box)
                            , GTK_WIDGET(editor->font_box));

  } else if (editor->pen->type != GTK_SHOT_PEN_TEXT
                      && child == editor->font_box) {
    g_object_ref(editor->font_box);
    gtk_container_remove(GTK_CONTAINER(editor->left_box)
                            , GTK_WIDGET(editor->font_box));
    gtk_container_add(GTK_CONTAINER(editor->left_box)
                            , GTK_WIDGET(editor->size_box));
  }

  gtk_widget_show_all(GTK_WIDGET(editor->window));
  gtk_window_move(editor->window, editor->x, editor->y);
}

void gtk_shot_pen_editor_hide(GtkShotPenEditor *editor) {
  g_return_if_fail(editor != NULL);

  gtk_widget_hide_all(GTK_WIDGET(editor->window));
}

void gtk_shot_pen_editor_set_pen(GtkShotPenEditor *editor
                                          , GtkShotPen *pen) {
  g_return_if_fail(editor != NULL);
  
  if (pen) {
    pen->color = editor->color;
    if (pen->type == GTK_SHOT_PEN_TEXT) {
      pen->text.fontname = g_strdup(editor->fontname);
    } else {
      pen->size = editor->size;
    }
  }
  editor->pen = pen;
}

GtkBox* create_size_box(GtkShotPenEditor *editor) {
  GtkBox *hbox = GTK_BOX(gtk_hbox_new(FALSE, 2));
  GtkWidget *btn;

  btn = create_xpm_button(small_xpm
                              , _("small")
                              , G_CALLBACK(on_set_pen_size)
                              , TRUE, editor);
  g_object_set_data(G_OBJECT(btn)
                          , "pen-size"
                          , GINT_TO_POINTER(GTK_SHOT_DEFAULT_PEN_SIZE));
  pack_to_box(hbox, btn);
  // press HACK :-)
  gtk_toggle_button_mark_active(btn, TRUE);

  btn = create_xpm_button(normal_xpm
                            , _("normal")
                            , G_CALLBACK(on_set_pen_size)
                            , TRUE, editor);
  g_object_set_data(G_OBJECT(btn)
                          , "pen-size"
                          , GINT_TO_POINTER(GTK_SHOT_DEFAULT_PEN_SIZE * 2));
  pack_to_box(hbox, btn);

  btn = create_xpm_button(big_xpm
                              , _("big")
                              , G_CALLBACK(on_set_pen_size)
                              , TRUE, editor);
  g_object_set_data(G_OBJECT(btn)
                          , "pen-size"
                          , GINT_TO_POINTER(GTK_SHOT_DEFAULT_PEN_SIZE * 4));
  pack_to_box(hbox, btn);

  return hbox;
}

GtkBox* create_font_box(GtkShotPenEditor *editor) {
  GtkBox *hbox = GTK_BOX(gtk_hbox_new(FALSE, 2));
  GtkWidget *btn =
            gtk_font_button_new_with_font(GTK_SHOT_DEFAULT_PEN_FONT);

  g_signal_connect(btn, "notify::font-name"
                      , G_CALLBACK(on_set_pen_font)
                      , editor);
  pack_to_box(hbox, btn);

  return hbox;
}

GtkBox* create_color_box(GtkShotPenEditor *editor) {
  GtkBox *hbox = GTK_BOX(gtk_hbox_new(FALSE, 2));
  GdkColor c;

  parse_to_gdk_color(GTK_SHOT_DEFAULT_PEN_COLOR, &c);
  GtkWidget *color_btn =
              gtk_color_button_new_with_color(&c);
  GtkBox *left, *right;
  left = GTK_BOX(gtk_hbox_new(FALSE, 2));
  right = GTK_BOX(gtk_vbox_new(FALSE, 2));

  g_signal_connect(color_btn, "notify::color"
                            , G_CALLBACK(on_set_pen_color)
                            , editor);
  pack_to_box(left, color_btn);

  GtkBox *up_hbox, *down_hbox;
  GtkWidget *btn;
  gint i, size = sizeof(color_maps) / sizeof(color_maps[0]);

  up_hbox = GTK_BOX(gtk_hbox_new(FALSE, 2));
  down_hbox = GTK_BOX(gtk_hbox_new(FALSE, 2));
  for (i = 0; i < size; i++) {
    btn = gtk_button_new();
    g_signal_connect(btn, "expose-event"
                        , G_CALLBACK(on_color_button_expose)
                        , GINT_TO_POINTER(color_maps[i].color));
    g_signal_connect(btn, "clicked"
                        , G_CALLBACK(on_change_color)
                        , color_btn);
    gtk_button_set_focus_on_click(GTK_BUTTON(btn), FALSE);
    gtk_widget_set_tooltip_text(btn, color_maps[i].name);
    g_object_set_data(G_OBJECT(btn)
                          , "pen-color"
                          , GINT_TO_POINTER(color_maps[i].color));
    gtk_widget_set_size_request(btn, 14, 14);

    if (i < size / 2) {
      pack_to_box(up_hbox, btn);
    } else {
      pack_to_box(down_hbox, btn);
    }
  }
  pack_to_box(right, up_hbox);
  pack_to_box(right, down_hbox);

  GtkWidget *right_align = gtk_alignment_new(0.6, 0.6, 0, 0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(right_align), 0, 0, 1, 2);
  gtk_container_add(GTK_CONTAINER(right_align), GTK_WIDGET(right));

  pack_to_box(hbox, left);
  pack_to_box(hbox, right_align);

  return hbox;
}

void on_set_pen_size(GtkToggleButton *btn, GtkShotPenEditor *editor) {
  // 查找其他已激活按钮(只会有一个)
  GList *l =
        gtk_container_get_children(GTK_CONTAINER(editor->size_box));
  GtkToggleButton *act = NULL;
  for (l; l; l = l->next) {
    GtkToggleButton *b = GTK_TOGGLE_BUTTON(l->data);
    if (b != btn && b->active) {
      act = b; break;
    }
  }
  // 点击的按钮非激活且没有其他激活的按钮,则保持该按钮的激活状态
  if (!btn->active && !act) {
    gtk_toggle_button_mark_active(btn, TRUE);
  } else if (btn->active) {
    if (act) {
      gtk_toggle_button_set_active(act, FALSE);
    }
    if (editor->pen) {
      editor->size =
            GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn)
                                    , "pen-size"));
      editor->pen->size = editor->size;
    }
  }
}

void on_set_pen_font(GtkFontButton *btn, GParamSpec *pspec
                                        , GtkShotPenEditor *editor) {
  if (editor->pen) {
    // 从FontButton中获取的字体不能被free,故将其复制一份
    if (editor->pen->text.fontname) {
      g_free(editor->pen->text.fontname);
    }
    editor->fontname =
              gtk_font_button_get_font_name(GTK_FONT_BUTTON(btn));
    editor->pen->text.fontname = g_strdup(editor->fontname);
#ifdef GTK_SHOT_DEBUG
    debug("font: %s\n", editor->pen->text.fontname);
#endif
  }
}

void on_set_pen_color(GtkColorButton *btn, GParamSpec *pspec
                                          , GtkShotPenEditor *editor) {
  if (editor->pen) {
    GdkColor c;
    gtk_color_button_get_color(btn, &c);
    editor->color = parse_gdk_color(c);
    editor->pen->color = editor->color;
#ifdef GTK_SHOT_DEBUG
    debug("%s, %06x\n", gdk_color_to_string(&c), editor->pen->color);
#endif
  }
}

void on_change_color(GtkButton *btn, GtkColorButton *color_btn) {
  GdkColor c;
  parse_to_gdk_color(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn)
                                          , "pen-color"))
                            , &c);
  gtk_color_button_set_color(color_btn, &c);
}

gboolean on_color_button_expose(GtkWidget *widget, GdkEventExpose *event
                                                  , gpointer *color) {
  cairo_t *cr;
  gint c = GPOINTER_TO_INT(color);

  cr = gdk_cairo_create(widget->window);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgb(cr, RGB_R(c) / 255.0
                          , RGB_G(c) / 255.0
                          , RGB_B(c) / 255.0);
  cairo_rectangle(cr, widget->allocation.x
                    , widget->allocation.y
                    , widget->allocation.width
                    , widget->allocation.height);
  cairo_fill(cr);

  cairo_destroy(cr);

  return TRUE;
}

