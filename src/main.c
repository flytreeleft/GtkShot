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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "utils.h"
#include "xpm.h"

#include "shot.h"

#define WAKE_UP_SIGNAL    SIGUSR1
#define LOCK_FILE ("/tmp/gtkshot.lock")

static GtkShot *shot = NULL;

static void wake_up(gint signo);
static void exit_clean(gint signo);
static void quit();
static void save_to_clipboard();
static void remove_lock_file();
static gint new_lock_file();

int main(int argc, char *argv[]) {
#ifdef ENABLE_NLS
  bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);
#endif

  gint pid = new_lock_file();

  if (pid > 0) {
    if (kill(pid, WAKE_UP_SIGNAL) == 0) {
      exit(0); // 进程确已存在,则退出新进程
    } else { // 否则,重新创建锁定文件
      remove_lock_file();
      new_lock_file();
    }
  }
  signal(WAKE_UP_SIGNAL, wake_up);
  signal(SIGINT, exit_clean);

  gtk_init(&argc, &argv);

  shot = gtk_shot_new();
  shot->quit = quit;
  shot->dblclick = save_to_clipboard;

  gtk_window_set_title(GTK_WINDOW(shot), GTK_SHOT_NAME);
  GdkPixbuf *icon = gdk_pixbuf_new_from_xpm_data(gtkshot_xpm);
  gtk_window_set_icon(GTK_WINDOW(shot), icon);
  g_object_unref(icon);

  gtk_shot_show(shot, TRUE);
  gtk_main();

  return 0;
}

void wake_up(gint signo) {
  gtk_shot_show(shot, TRUE);
  debug("GtkShot has been wake up...\n");
}

void exit_clean(gint signo) {
  quit();
}

void quit() {
  debug("GtkShot has exit" \
                  ", you will not get shot image any more...\n");

  remove_lock_file();
  gtk_main_quit();
  exit(0);
}

void save_to_clipboard() {
  if (gtk_shot_has_visible_section(shot)) {
    gtk_shot_save_section_to_clipboard(shot);
    gtk_shot_hide(shot);
  } else {
    gtk_shot_quit(shot);
  }
}

void remove_lock_file() {
  if (remove(LOCK_FILE) == 0) {
    debug("clean lock file [%s] successfully...\n", LOCK_FILE);
  } else {
    debug("can not remove lock file [%s]: %s" \
              ", please remove it by yourself...\n"
                                  , LOCK_FILE, strerror(errno));
  }
}

/**
 * 创建锁文件,并返回进程PID
 * @return (= 0), 进程不存在,新建锁文件;
 *         (> 0), 进程已存在,且PID为该返回值
 *                ,并保留原锁文件;
 */
gint new_lock_file() {
  FILE *f = fopen(LOCK_FILE, "r");
  gint pid = 0;

  if (f) {
    fscanf(f, "gtk-shot-pid: %d", &pid);
    //fread(&pid, sizeof(gint), 1, f);
#ifdef GTK_SHOT_DEBUG
    debug("GtkShot exists: pid is %d\n", pid);
#endif
  } else if ((f = fopen(LOCK_FILE, "w+"))) {
    pid = getpid();
    fprintf(f, "gtk-shot-pid: %d", pid);
    //fwrite(&pid, sizeof(gint), 1, f);
#ifdef GTK_SHOT_DEBUG
    debug("new GtkShot: pid is %d\n", pid);
#endif
    pid = 0;
  } else {
    debug("no lock file and create lock file failed[%s]: %s\n"
                                , LOCK_FILE, strerror(errno));
    exit(-1);
  }
  if (f) fclose(f);

  return pid;
}
