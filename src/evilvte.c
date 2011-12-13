/* Forked from sakura 2.0.1, http://www.pleyades.net/david/sakura.php
 * Copyright (C) 2006-2008  David Gómez <david@pleyades.net>
 * Copyright (C) 2008-2011  Wen-Yen Chuang <caleb AT calno DOT com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <ctype.h>
#include <glib.h>
#ifndef G_CONST_RETURN
#define G_CONST_RETURN const
#endif
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(2,90,7)
#include <gdk/gdkkeysyms-compat.h>
#endif
#include <gdk/gdkkeysyms.h>
#include <libintl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <vte/vte.h>

#ifndef VTE_CHECK_VERSION
#define VTE_CHECK_VERSION(x,y,z) 0
#endif

#define AUTO            VTE_ERASE_AUTO
#define BACKSPACE       VTE_ERASE_ASCII_BACKSPACE
#define DELETE          VTE_ERASE_ASCII_DELETE
#define DELETE_SEQUENCE VTE_ERASE_DELETE_SEQUENCE
#define ERASE_TTY       VTE_ERASE_TTY
#if !VTE_CHECK_VERSION(0,20,4)
#undef ERASE_TTY
#define ERASE_TTY       VTE_ERASE_AUTO
#endif

#define BLOCK     VTE_CURSOR_SHAPE_BLOCK
#define IBEAM     VTE_CURSOR_SHAPE_IBEAM
#define UNDERLINE VTE_CURSOR_SHAPE_UNDERLINE

#define LINUX     1
#define RXVT      2
#define TANGO     3
#define VTE_FIXED 4
#define XTERM     5
#define ZENBURN   6

#define LEFT   0
#define RIGHT  1
#define TOP    2
#define BOTTOM 3
#define OFF_L  4
#define OFF_R  5

#define CTRL event->keyval ==

#include "custom.h"
#include "evilvte.h"

#if !GTK_CHECK_VERSION(2,13,0)
#undef HOTKEY_MIMIC_SCROLL_UP
#undef HOTKEY_MIMIC_SCROLL_DOWN
#undef ONLY_ONE_MENU_ITEM
#endif

#if !GTK_CHECK_VERSION(2,13,4)
#define gtk_color_selection_dialog_get_color_selection(x) (x)->colorsel
#define gtk_dialog_get_content_area(x) (x)->vbox
#endif

#if !GTK_CHECK_VERSION(2,17,5)
#define gtk_widget_set_can_focus(x,y)
#endif

#if !GTK_CHECK_VERSION(2,90,0)
#define gtk_scrollbar_new(x,y) gtk_vscrollbar_new(y)
#define gtk_scale_new_with_range(w,x,y,z) gtk_hscale_new_with_range(x,y,z)
#endif

#if GTK_CHECK_VERSION(2,90,7)
#define GTK_DIALOG_NO_SEPARATOR 0
#endif

#if GTK_CHECK_VERSION(2,91,0)
#define gtk_widget_set_colormap gtk_widget_set_visual
#define gdk_screen_get_rgba_colormap gdk_screen_get_rgba_visual
#endif

#if !GTK_CHECK_VERSION(2,91,1)
#define gtk_window_set_has_resize_grip(x,y)
#endif

#if GTK_CHECK_VERSION(2,91,2) && defined(USE_GTK_GRID)
#undef GTK_BOX
#define GTK_BOX GTK_GRID
#define gtk_hbox_new(x,y) gtk_grid_new()
#define gtk_vbox_new(x,y) gtk_grid_new()
#define gtk_box_pack_start(v,w,x,y,z) gtk_container_add(GTK_CONTAINER(v),w)
#endif

#if !GTK_CHECK_VERSION(2,99,0)
#define gdk_device_warp(w,x,y,z) gdk_display_warp_pointer(display,x,y,z)
#define gdk_device_get_position(w,x,y,z) gdk_display_get_pointer(display,x,y,z,NULL)
#endif

#if !VTE_CHECK_VERSION(0,25,1)
#undef HOTKEY_SEARCH_STRING
#undef HOTKEY_SEARCH_PREVIOUS
#undef HOTKEY_SEARCH_NEXT
#endif

#if !defined(INNER_BORDER_H) || !defined(INNER_BORDER_W)
#if !GTK_CHECK_VERSION(2,91,6)
#define INNER_BORDER_H (inner_border ? (inner_border->top + inner_border->bottom) : 0)
#define INNER_BORDER_W (inner_border ? (inner_border->left + inner_border->right) : 0)
#endif
#if GTK_CHECK_VERSION(2,91,6)
#define INNER_BORDER_H 2
#define INNER_BORDER_W 2
#endif
#endif

#ifndef VTE_FUNNY
#define VTE_FUNNY 1
#endif

#if !VTE_FUNNY || !VTE_CHECK_VERSION(0,27,1) || !GTK_CHECK_VERSION(2,91,1)
#undef VTE_FUNNY
#endif

#if VTE_CHECK_VERSION(0,17,1) && (COLOR_STYLE == VTE_FIXED)
#undef COLOR_STYLE
#endif

#ifndef VTE_FORK_CMD_OLD
#define VTE_FORK_CMD_OLD 1
#endif

#define VTE_WINDOW_RESIZE(x,y,z) gtk_window_resize(x,y,z)
#if VTE_CHECK_VERSION(0,27,1) && GTK_CHECK_VERSION(2,91,1)
#ifndef VTE_FUNNY
#undef VTE_WINDOW_RESIZE
#define VTE_WINDOW_RESIZE(x,y,z)
#endif
GtkBorder *inner_border;
#ifndef VTE_COLUMNS
#define VTE_COLUMNS 80
#endif
#ifndef VTE_ROWS
#define VTE_ROWS 24
#endif
#endif

#define GET_CURRENT_TAB(x) term = (struct terminal*)g_object_get_data(G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), (x))), "current_tab")

#if CURSOR_BLINKS
#define VTE_CURSOR_BLINKS VTE_CURSOR_BLINK_ON
#endif
#if !CURSOR_BLINKS
#define VTE_CURSOR_BLINKS VTE_CURSOR_BLINK_OFF
#endif

#ifndef DEFAULT_COMMAND
#define DEFAULT_COMMAND g_getenv("SHELL")
#endif

#ifndef RECORD_LASTLOG
#define RECORD_LASTLOG 1
#endif
#define EVILVTE_PTY_NO_LASTLOG 0
#if !RECORD_LASTLOG
#undef EVILVTE_PTY_NO_LASTLOG
#define EVILVTE_PTY_NO_LASTLOG VTE_PTY_NO_LASTLOG
#endif

#ifndef RECORD_UTMP
#define RECORD_UTMP 1
#endif
#define EVILVTE_PTY_NO_UTMP 0
#if !RECORD_UTMP
#undef EVILVTE_PTY_NO_UTMP
#define EVILVTE_PTY_NO_UTMP VTE_PTY_NO_UTMP
#endif

#ifndef RECORD_WTMP
#define RECORD_WTMP 1
#endif
#define EVILVTE_PTY_NO_WTMP 0
#if !RECORD_WTMP
#undef EVILVTE_PTY_NO_WTMP
#define EVILVTE_PTY_NO_WTMP VTE_PTY_NO_WTMP
#endif

#define EVILVTE_PTY_NO_HELPER 0
#if !RECORD_LASTLOG && !RECORD_UTMP && !RECORD_WTMP
#undef EVILVTE_PTY_NO_HELPER
#define EVILVTE_PTY_NO_HELPER VTE_PTY_NO_HELPER
#endif

#ifndef LABEL_DEFAULT_ENCODING
#define LABEL_DEFAULT_ENCODING "_Default Encoding"
#endif

#ifndef LABEL_DIALOG_BACKGROUND_TINT
#define LABEL_DIALOG_BACKGROUND_TINT "_Background tint color"
#endif

#ifndef LABEL_DIALOG_CLOSE
#define LABEL_DIALOG_CLOSE "Do you really want to close it?"
#endif

#ifndef LABEL_DIALOG_SEARCH
#define LABEL_DIALOG_SEARCH "Find"
#endif

#ifndef LABEL_MENU_SATURATION
#define LABEL_MENU_SATURATION "_Adjust saturation"
#endif

#ifndef LABEL_MENU_TOGGLE_ANTI_ALIAS
#define LABEL_MENU_TOGGLE_ANTI_ALIAS "_Toggle anti-alias"
#endif

#ifndef LABEL_MENU_TOGGLE_BG
#define LABEL_MENU_TOGGLE_BG "_Toggle background"
#endif

#ifndef LABEL_MENU_TOGGLE_DECORATED
#define LABEL_MENU_TOGGLE_DECORATED "_Toggle window decorated"
#endif

#ifndef LABEL_MENU_TOGGLE_FULLSCREEN
#define LABEL_MENU_TOGGLE_FULLSCREEN "_Toggle fullscreen"
#endif

#ifndef LABEL_MENU_TOGGLE_HOTKEYS
#define LABEL_MENU_TOGGLE_HOTKEYS "_Toggle hotkeys locking"
#endif

#ifndef LABEL_MENU_TOGGLE_ON_TOP
#define LABEL_MENU_TOGGLE_ON_TOP "_Toggle always on top"
#endif

#ifndef LABEL_MENU_TOGGLE_SCROLLBAR
#define LABEL_MENU_TOGGLE_SCROLLBAR "_Toggle scrollbar"
#endif

#ifndef LABEL_MENU_TOGGLE_STATUS_BAR
#define LABEL_MENU_TOGGLE_STATUS_BAR "_Toggle status bar"
#endif

#ifndef LABEL_MENU_TOGGLE_TABBAR
#define LABEL_MENU_TOGGLE_TABBAR "_Toggle tabbar"
#endif

#ifndef LABEL_SUBMENU_ENCODING
#define LABEL_SUBMENU_ENCODING "_Character Encoding"
#endif

#ifndef LABEL_SUBMENU_IME
#define LABEL_SUBMENU_IME "_Input Methods"
#endif

#define MATCH_HTTP_DATA "((f|F)|(h|H)(t|T))(t|T)(p|P)(s|S)?://(([^|.< \t\r\n\\\"]*([.][^|< \t\r\n\\\"])?[^|.< \t\r\n\\\"]*)*[^< \t\r\n,;|\\\"]*[^|.< \t\r\n\\\"])?/*"
#define MATCH_FILE_DATA "(f|F)(i|I)(l|L)(e|E):///(([^|.< \t\r\n\\\"]*([.][^|< \t\r\n\\\"])?[^|.< \t\r\n\\\"]*)*[^< \t\r\n,;|\\\"]*[^|.< \t\r\n\\\"])?/*"
#define MATCH_MAIL_DATA "(m|M)(a|A)(i|I)(l|L)(t|T)(o|O):(([^|.< \t\r\n\\\"]*([.][^|< \t\r\n\\\"])?[^|.< \t\r\n\\\"]*)*@[^< \t\r\n,;|\\\"]*[^|.< \t\r\n\\\"])?/*"

#if CLOSE_DIALOG
#define DEL_TAB del_tab
#endif

#ifndef CLOSE_DIALOG
#define CLOSE_DIALOG 0
#endif

#ifndef DEL_TAB
#define DEL_TAB(x,y) del_tab()
#endif

#if DEF_TAB_LABEL && !defined(TAB_LABEL)
#define TAB_LABEL "Page %u"
#endif

#if !TAB
#undef COMMAND_TAB_NUMBERS
#undef SHOW_WINDOW_BORDER
#undef TAB_BORDER
#undef TAB_CLOSE_BUTTON
#undef TAB_EXPANDED_WIDTH
#undef TAB_LABEL
#undef TAB_LABEL_DYNAMIC
#undef TAB_LABEL_CUSTOM
#undef TAB_NEW_PATH_EQUAL_OLD
#undef TAB_REORDERABLE
#undef TAB_SHOW_INFO_AT_TITLE
#undef TABBAR
#undef TABBAR_PLACE
#undef TABBAR_AUTOHIDE
#undef TABBAR_SCROLLABLE
#undef TABBAR_MENU_SELECT_TAB
#define SHOW_WINDOW_BORDER 0
#define TABBAR 0
#endif

#define GET_VTE_CHILD_PID NULL
#if VTE_FORK_CMD_OLD
#undef GET_VTE_CHILD_PID
#define GET_VTE_CHILD_PID
#endif

#if TAB_NEW_PATH_EQUAL_OLD || CLOSE_DIALOG || CLOSE_SAFELY
#undef GET_VTE_CHILD_PID
#define GET_VTE_CHILD_PID &(term->pid)
#if VTE_FORK_CMD_OLD
#undef GET_VTE_CHILD_PID
#define GET_VTE_CHILD_PID term->pid =
#endif
#endif

#if TAB_NEW_PATH_EQUAL_OLD
#ifndef DEFAULT_DIRECTORY
#define DEFAULT_DIRECTORY g_get_current_dir()
#endif
#define VTE_DEFAULT_DIRECTORY default_directory
#endif

#if !TAB_NEW_PATH_EQUAL_OLD
#ifndef DEFAULT_DIRECTORY
#define DEFAULT_DIRECTORY NULL
#endif
#define VTE_DEFAULT_DIRECTORY DEFAULT_DIRECTORY
#endif

#define KEY_MOD_MASK GDK_CONTROL_MASK

#if CTRL_ALT
#undef HOTKEY
#define HOTKEY TRUE
#undef KEY_MOD_MASK
#define KEY_MOD_MASK (GDK_CONTROL_MASK | GDK_MOD1_MASK)
#endif

#if !HOTKEY
#undef MENU_TOGGLE_HOTKEYS
#undef HOTKEY_COLOR_BACKGROUND
#undef HOTKEY_FONT_BIGGER
#undef HOTKEY_FONT_SMALLER
#undef HOTKEY_FONT_DEFAULT_SIZE
#undef HOTKEY_FONT_SELECT
#undef HOTKEY_MIMIC_SCROLL_UP
#undef HOTKEY_MIMIC_SCROLL_DOWN
#undef HOTKEY_SATURATION_DIALOG
#undef HOTKEY_SATURATION_MORE
#undef HOTKEY_SATURATION_LESS
#undef HOTKEY_SEARCH_STRING
#undef HOTKEY_SEARCH_PREVIOUS
#undef HOTKEY_SEARCH_NEXT
#undef HOTKEY_TOGGLE_ANTI_ALIAS
#undef HOTKEY_TOGGLE_DECORATED
#undef HOTKEY_TOGGLE_FULLSCREEN
#undef HOTKEY_TOGGLE_HOTKEYS
#undef HOTKEY_TOGGLE_ON_TOP
#undef HOTKEY_TOGGLE_SCROLLBAR
#undef HOTKEY_TOGGLE_STATUS_BAR
#undef HOTKEY_TOGGLE_BACKGROUND
#endif

#if !TAB || !HOTKEY
#undef HOTKEY_TAB_ADD
#undef HOTKEY_TAB_REMOVE
#undef HOTKEY_TAB_PREVIOUS
#undef HOTKEY_TAB_NEXT
#undef HOTKEY_TAB_FIRST
#undef HOTKEY_TAB_LAST
#undef HOTKEY_TAB_GO_TO_NUMBER
#undef HOTKEY_TAB_EDIT_LABEL
#undef HOTKEY_TOGGLE_TABBAR
#endif

#if BUTTON_ORDER_BY_RCFILE
int button_order = 0;
#endif

#define MENU_INPUT_METHOD 1

#if !MENU
#undef MENU_INPUT_METHOD
#undef MENU_ENCODING_LIST
#undef MENU_MATCH_STRING_EXEC
#undef MENU_CUSTOM
#endif

#ifndef MENU_CUSTOM_SIZE
#undef MENU_CUSTOM
#endif

#ifndef MENU_ENCODING_LIST_SIZE
#undef MENU_ENCODING_LIST
#endif

#ifndef TOGGLE_BG_ORDER_SIZE
#undef TOGGLE_BG_ORDER
#endif

#ifndef MENU_CUSTOM
#undef MENU_COPY
#undef MENU_PASTE
#undef MENU_SELECT_ALL
#undef MENU_COLOR_BACKGROUND
#undef MENU_OPEN_NEW_WINDOW
#undef MENU_QUIT
#undef MENU_FONT_BIGGER
#undef MENU_FONT_SMALLER
#undef MENU_FONT_DEFAULT_SIZE
#undef MENU_RESET_TERMINAL
#undef MENU_RESET_AND_CLEAR
#undef MENU_FONT_SELECT
#undef MENU_SEPARATOR
#undef SUBMENU_ENCODING_LIST
#undef SUBMENU_INPUT_METHOD
#undef MENU_TOGGLE_BACKGROUND
#undef MENU_TOGGLE_HOTKEYS
#undef MENU_TOGGLE_ON_TOP
#undef MENU_TOGGLE_SCROLLBAR
#undef MENU_TOGGLE_STATUS_BAR
#undef MENU_TOGGLE_DECORATED
#undef MENU_TOGGLE_FULLSCREEN
#undef MENU_TOGGLE_ANTI_ALIAS
#undef MENU_CHANGE_SATURATION
#undef ONLY_ONE_MENU_ITEM
#endif

#if !TAB || !defined(MENU_CUSTOM)
#undef MENU_TAB_ADD
#undef MENU_TAB_REMOVE
#undef MENU_TAB_EDIT_LABEL
#undef MENU_TOGGLE_TABBAR
#endif

#if defined(HOTKEY_TOGGLE_ON_TOP) || MENU_TOGGLE_ON_TOP
#ifndef PROGRAM_ALWAYS_ON_TOP
#define PROGRAM_ALWAYS_ON_TOP 0
#endif
#endif

#if defined(HOTKEY_SATURATION_DIALOG) || MENU_CHANGE_SATURATION
GtkWidget *adjustment;
#endif

#if defined(HOTKEY_TOGGLE_HOTKEYS) || MENU_TOGGLE_HOTKEYS
int hotkey_status = 1;
#endif

#ifdef MENU_ENCODING_LIST
char *encoding[] = { MENU_ENCODING_LIST };
#endif

#if !defined(MENU_ENCODING_LIST) && MENU
GtkWidget *encoding_item;
#endif

#if TOGGLE_BG_ORDER_SIZE == 1
#undef HOTKEY_TOGGLE_BACKGROUND
#undef MENU_TOGGLE_BACKGROUND
#endif

#if defined(HOTKEY_TOGGLE_BACKGROUND) || MENU_TOGGLE_BACKGROUND
#ifndef BACKGROUND_IMAGE
#define BACKGROUND_IMAGE ".config/evilvte/background.png"
#endif
#ifndef TOGGLE_BG_ORDER
#define TOGGLE_BG_ORDER "Image", "Transparent", "No background", "Opacity"
#define TOGGLE_BG_ORDER_SIZE 4
#define TOGGLE_BG_OPACITY 1
#define TOGGLE_BG_IMAGE 1
#define TOGGLE_BG_TRANSPARENT 1
#define TOGGLE_BG_NO_BACKGROUND 1
#endif
#if TOGGLE_BG_OPACITY
#define INIT_OPACITY 1
#endif
#define DO_TOGGLE_BACKGROUND 1
const char *background_order[] = { TOGGLE_BG_ORDER };
int background_status = 0;
#endif

#ifdef BACKGROUND_IMAGE
char imgstr[sizeof(BACKGROUND_IMAGE) + 64];
#endif

#ifdef PROGRAM_ICON
char iconstr[sizeof(PROGRAM_ICON) + 64];
#endif

#if defined(BACKGROUND_IMAGE) || BACKGROUND_TRANSPARENT || TOGGLE_BG_TRANSPARENT
#define BACKGROUND_EXIST 1
#endif

#if defined(HOTKEY_COLOR_BACKGROUND) || MENU_COLOR_BACKGROUND
#ifndef BACKGROUND_TINT_COLOR
#define BACKGROUND_TINT_COLOR "black"
#endif
#endif

#if defined(BACKGROUND_TINT_COLOR) && BACKGROUND_EXIST
GdkColor color_tint;
#endif

#if defined(HOTKEY_TOGGLE_SCROLLBAR) || MENU_TOGGLE_SCROLLBAR
#ifndef SCROLLBAR
#define SCROLLBAR RIGHT
#endif
int scrollbar_status = (SCROLLBAR < 3);
#endif

#if !defined(HOTKEY_TOGGLE_SCROLLBAR) && !MENU_TOGGLE_SCROLLBAR && (SCROLLBAR > 3)
#undef SCROLLBAR
#endif

#ifdef SCROLLBAR
#define VTE_HBOX term->hbox
#endif
#ifndef SCROLLBAR
#define VTE_HBOX term->vte
#endif

#if defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
#ifndef STATUS_BAR
#define STATUS_BAR 1
#endif
int status_bar_status = STATUS_BAR;
int status_bar_resize_grip = 0;
#endif

#if defined(HOTKEY_TOGGLE_DECORATED) || MENU_TOGGLE_DECORATED
#ifdef SHOW_WINDOW_DECORATED
int window_decorated_status = SHOW_WINDOW_DECORATED;
#endif
#ifndef SHOW_WINDOW_DECORATED
int window_decorated_status = 1;
#endif
#endif

#if defined(HOTKEY_TOGGLE_FULLSCREEN) || MENU_TOGGLE_FULLSCREEN || defined(VTE_FUNNY)
int window_fullscreen_status = 0;
#endif

#if defined(HOTKEY_TOGGLE_ANTI_ALIAS) || MENU_TOGGLE_ANTI_ALIAS
#if defined(FONT_ANTI_ALIAS) && !FONT_ANTI_ALIAS
int antialias_status = VTE_ANTI_ALIAS_FORCE_DISABLE;
#endif
#if !defined(FONT_ANTI_ALIAS) || FONT_ANTI_ALIAS
int antialias_status = VTE_ANTI_ALIAS_FORCE_ENABLE;
#endif
#endif

#define ONLY_ONE_MENU_HANDLE
#ifdef ONLY_ONE_MENU_ITEM
#undef ONLY_ONE_MENU_HANDLE
#define ONLY_ONE_MENU_HANDLE || (menu_item_success == 1)
#endif

#ifdef MENU_CUSTOM
const char *menu_custom[] = { MENU_CUSTOM };
int menu_item_success = 0;
#endif

#if TABBAR_AUTOHIDE && !defined(TABBAR)
#define TABBAR 1
#endif

#if defined(HOTKEY_TOGGLE_TABBAR) || MENU_TOGGLE_TABBAR
#ifdef TABBAR
int tabbar_status = TABBAR;
#endif
#ifndef TABBAR
int tabbar_status = 1;
#endif
#define VTE_TABBAR tabbar_status
#endif

#if !defined(HOTKEY_TOGGLE_TABBAR) && !MENU_TOGGLE_TABBAR
#define VTE_TABBAR TABBAR
#endif

#if defined(MENU_MATCH_STRING_EXEC) || defined(MATCH_STRING_L) || defined(MATCH_STRING_M)
#if !MATCH_STRING_HTTP && !MATCH_STRING_MAIL && !MATCH_STRING_FILE && !defined(MATCH_STRING)
#undef MATCH_STRING_HTTP
#define MATCH_STRING_HTTP 1
#endif
#endif

#ifdef FONT_ANTI_ALIAS
#define VTE_ANTI_ALIAS VTE_ANTI_ALIAS_FORCE_ENABLE
#if !FONT_ANTI_ALIAS
#undef VTE_ANTI_ALIAS
#define VTE_ANTI_ALIAS VTE_ANTI_ALIAS_FORCE_DISABLE
#endif
#endif

#ifndef FONT_ANTI_ALIAS
#define VTE_ANTI_ALIAS VTE_ANTI_ALIAS_USE_DEFAULT
#endif

#define EVILVTE_SET_FONT(x,y,z) vte_terminal_set_font_from_string(x,y)
#if defined(FONT_ANTI_ALIAS) || MENU_TOGGLE_ANTI_ALIAS || defined(HOTKEY_TOGGLE_ANTI_ALIAS)
#undef EVILVTE_SET_FONT
#define EVILVTE_SET_FONT vte_terminal_set_font_from_string_full
#endif

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "evilvte"
#define UPPER_PROGRAM_NAME "Evilvte"
#endif

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION EVILVTE_VERSION
#endif

#if PROGRAM_WM_CLASS
char *wm_class_name = PROGRAM_NAME;
char *wm_class_class = UPPER_PROGRAM_NAME;
#endif

#define VTE_REGEX 0
#if defined(SEARCH_CASE_SENSITIVE) && !SEARCH_CASE_SENSITIVE
#undef VTE_REGEX
#define VTE_REGEX G_REGEX_CASELESS
#endif

#define VTE_PROGRAM_NAME PROGRAM_NAME
#if COMMAND_SET_TITLE
#undef VTE_PROGRAM_NAME
#define VTE_PROGRAM_NAME program_name
char *program_name = PROGRAM_NAME;
#endif

#if COMMAND_FONT
char *command_font = NULL;
#endif

#if COMMAND_GEOMETRY
char *command_geometry = NULL;
#endif

GtkWidget *main_window;
GtkWidget *notebook;

#define VTE_LABEL NULL
#if defined(TAB_LABEL) || defined(TAB_LABEL_CUSTOM)
#undef VTE_LABEL
#define VTE_LABEL label
#endif

#if TAB_CLOSE_BUTTON
#ifndef TAB_LABEL
#define TAB_LABEL "Page %u"
#endif
#undef VTE_LABEL
#define VTE_LABEL term->label
#endif

#ifdef TAB_LABEL_CUSTOM
#undef TAB_LABEL
const char *label_style_custom[] = { TAB_LABEL_CUSTOM };
const int label_style_size = sizeof(label_style_custom) / sizeof(label_style_custom[0]);
#endif

#ifdef COLOR_STYLE
const GdkColor color_style[16] = {
#if COLOR_STYLE == LINUX
  { 0, 0x0000, 0x0000, 0x0000 },
  { 0, 0xaaaa, 0x0000, 0x0000 },
  { 0, 0x0000, 0xaaaa, 0x0000 },
  { 0, 0xaaaa, 0x5555, 0x0000 },
  { 0, 0x0000, 0x0000, 0xaaaa },
  { 0, 0xaaaa, 0x0000, 0xaaaa },
  { 0, 0x0000, 0xaaaa, 0xaaaa },
  { 0, 0xaaaa, 0xaaaa, 0xaaaa },
  { 0, 0x5555, 0x5555, 0x5555 },
  { 0, 0xffff, 0x5555, 0x5555 },
  { 0, 0x5555, 0xffff, 0x5555 },
  { 0, 0xffff, 0xffff, 0x5555 },
  { 0, 0x5555, 0x5555, 0xffff },
  { 0, 0xffff, 0x5555, 0xffff },
  { 0, 0x5555, 0xffff, 0xffff },
  { 0, 0xffff, 0xffff, 0xffff }
#endif
#if COLOR_STYLE == RXVT
  { 0, 0x0000, 0x0000, 0x0000 },
  { 0, 0xcdcd, 0x0000, 0x0000 },
  { 0, 0x0000, 0xcdcd, 0x0000 },
  { 0, 0xcdcd, 0xcdcd, 0x0000 },
  { 0, 0x0000, 0x0000, 0xcdcd },
  { 0, 0xcdcd, 0x0000, 0xcdcd },
  { 0, 0x0000, 0xcdcd, 0xcdcd },
  { 0, 0xfafa, 0xebeb, 0xd7d7 },
  { 0, 0x4040, 0x4040, 0x4040 },
  { 0, 0xffff, 0x0000, 0x0000 },
  { 0, 0x0000, 0xffff, 0x0000 },
  { 0, 0xffff, 0xffff, 0x0000 },
  { 0, 0x0000, 0x0000, 0xffff },
  { 0, 0xffff, 0x0000, 0xffff },
  { 0, 0x0000, 0xffff, 0xffff },
  { 0, 0xffff, 0xffff, 0xffff }
#endif
#if COLOR_STYLE == TANGO
  { 0, 0x2e2e, 0x3434, 0x3636 },
  { 0, 0xcccc, 0x0000, 0x0000 },
  { 0, 0x4e4e, 0x9a9a, 0x0606 },
  { 0, 0xc4c4, 0xa0a0, 0x0000 },
  { 0, 0x3434, 0x6565, 0xa4a4 },
  { 0, 0x7575, 0x5050, 0x7b7b },
  { 0, 0x0606, 0x9820, 0x9a9a },
  { 0, 0xd3d3, 0xd7d7, 0xcfcf },
  { 0, 0x5555, 0x5757, 0x5353 },
  { 0, 0xefef, 0x2929, 0x2929 },
  { 0, 0x8a8a, 0xe2e2, 0x3434 },
  { 0, 0xfcfc, 0xe9e9, 0x4f4f },
  { 0, 0x7272, 0x9f9f, 0xcfcf },
  { 0, 0xadad, 0x7f7f, 0xa8a8 },
  { 0, 0x3434, 0xe2e2, 0xe2e2 },
  { 0, 0xeeee, 0xeeee, 0xecec }
#endif
#if COLOR_STYLE == VTE_FIXED
  { 0, 0x0000, 0x0000, 0x0000 },
  { 0, 0xc0c0, 0x0000, 0x0000 },
  { 0, 0x0000, 0xc0c0, 0x0000 },
  { 0, 0xc0c0, 0xc0c0, 0x0000 },
  { 0, 0x0000, 0x0000, 0xc0c0 },
  { 0, 0xc0c0, 0x0000, 0xc0c0 },
  { 0, 0x0000, 0xc0c0, 0xc0c0 },
  { 0, 0xc0c0, 0xc0c0, 0xc0c0 },
  { 0, 0x3f3f, 0x3f3f, 0x3f3f },
  { 0, 0xffff, 0x3f3f, 0x3f3f },
  { 0, 0x3f3f, 0xffff, 0x3f3f },
  { 0, 0xffff, 0xffff, 0x3f3f },
  { 0, 0x3f3f, 0x3f3f, 0xffff },
  { 0, 0xffff, 0x3f3f, 0xffff },
  { 0, 0x3f3f, 0xffff, 0xffff },
  { 0, 0xffff, 0xffff, 0xffff }
#endif
#if COLOR_STYLE == XTERM
  { 0, 0x0000, 0x0000, 0x0000 },
  { 0, 0xcdcb, 0x0000, 0x0000 },
  { 0, 0x0000, 0xcdcb, 0x0000 },
  { 0, 0xcdcb, 0xcdcb, 0x0000 },
  { 0, 0x1e1a, 0x908f, 0xffff },
  { 0, 0xcdcb, 0x0000, 0xcdcb },
  { 0, 0x0000, 0xcdcb, 0xcdcb },
  { 0, 0xe5e2, 0xe5e2, 0xe5e2 },
  { 0, 0x4ccc, 0x4ccc, 0x4ccc },
  { 0, 0xffff, 0x0000, 0x0000 },
  { 0, 0x0000, 0xffff, 0x0000 },
  { 0, 0xffff, 0xffff, 0x0000 },
  { 0, 0x4645, 0x8281, 0xb4ae },
  { 0, 0xffff, 0x0000, 0xffff },
  { 0, 0x0000, 0xffff, 0xffff },
  { 0, 0xffff, 0xffff, 0xffff }
#endif
#if COLOR_STYLE == ZENBURN
  { 0, 0x0000, 0x0000, 0x0000 },
  { 0, 0x9e9e, 0x1818, 0x2828 },
  { 0, 0xaeae, 0xcece, 0x9292 },
  { 0, 0x9696, 0x8a8a, 0x3838 },
  { 0, 0x4141, 0x4141, 0x7171 },
  { 0, 0x9696, 0x3c3c, 0x5959 },
  { 0, 0x4141, 0x8181, 0x7979 },
  { 0, 0xbebe, 0xbebe, 0xbebe },
  { 0, 0x6666, 0x6666, 0x6666 },
  { 0, 0xcfcf, 0x6161, 0x7171 },
  { 0, 0xc5c5, 0xf7f7, 0x7979 },
  { 0, 0xffff, 0xf7f7, 0x9696 },
  { 0, 0x4141, 0x8686, 0xbebe },
  { 0, 0xcfcf, 0x9e9e, 0xbebe },
  { 0, 0x7171, 0xbebe, 0xbebe },
  { 0, 0xffff, 0xffff, 0xffff }
#endif
};
#endif

#if defined(FONT_ANTI_ALIAS) || MENU_FONT_BIGGER || MENU_FONT_SMALLER || MENU_FONT_SELECT || MENU_TOGGLE_ANTI_ALIAS || defined(HOTKEY_TOGGLE_ANTI_ALIAS) || defined(HOTKEY_FONT_BIGGER) || defined(HOTKEY_FONT_SMALLER) || defined(HOTKEY_FONT_SELECT) || COMMAND_FONT
#ifndef FONT
#define FONT "Monospace 10"
#endif
#endif

#if !MENU_FONT_BIGGER && !MENU_FONT_SMALLER && !MENU_FONT_SELECT && !defined(HOTKEY_FONT_BIGGER) && !defined(HOTKEY_FONT_SMALLER) && !defined(HOTKEY_FONT_SELECT)
#undef MENU_FONT_DEFAULT_SIZE
#undef HOTKEY_FONT_DEFAULT_SIZE
#endif

#ifdef FONT
char font_name[125];
char font_str[128];
int font_size;
#if defined(HOTKEY_FONT_DEFAULT_SIZE) || MENU_FONT_DEFAULT_SIZE
int font_size_default;
#endif
#endif

#if COMMAND_EXEC_PROGRAM
#define VTE_DEFAULT_COMMAND default_command
#define DEFAULT_ARGV default_argv
char default_command[256];
char **default_argv = NULL;
#endif

#if !COMMAND_EXEC_PROGRAM
#define VTE_DEFAULT_COMMAND DEFAULT_COMMAND
#define DEFAULT_ARGV NULL
#endif

#if COMMAND_LOGIN_SHELL && VTE_FORK_CMD_OLD
char *login_shell[] = { "-", NULL };
#endif

#if COMMAND_LOGIN_SHELL || (!VTE_FORK_CMD_OLD && COMMAND_EXEC_PROGRAM)
int login_shell_flag = 0;
#endif

#if defined(HOTKEY_SATURATION_MORE) || defined(HOTKEY_SATURATION_LESS) || MOUSE_CTRL_SATURATION || BACKGROUND_OPACITY || defined(HOTKEY_SATURATION_DIALOG) || MENU_CHANGE_SATURATION || defined(HOTKEY_TOGGLE_BACKGROUND) || MENU_TOGGLE_BACKGROUND
#ifndef BACKGROUND_SATURATION
#define BACKGROUND_SATURATION 0.4
#endif
#endif

#ifdef PROGRAM_ALWAYS_ON_TOP
int always_on_top = PROGRAM_ALWAYS_ON_TOP;
#endif

#ifdef BACKGROUND_SATURATION
double saturation_level = BACKGROUND_SATURATION;
#endif

#if COMMAND_AT_ROOT_WINDOW
int at_root_window = 0;
#endif

#if MENU
GtkWidget *menu;
#endif

#if MENU_COPY
GtkWidget *menu_copy;
#endif

#if MENU_PASTE
GtkWidget *menu_paste;
#endif

#if MENU_FONT_DEFAULT_SIZE
GtkWidget *menu_zoom_100;
#endif

#ifdef MENU_MATCH_STRING_EXEC
GtkWidget *match_open;
GtkWidget *match_copy;
GtkWidget *match_item;
#endif

#if defined(MENU_MATCH_STRING_EXEC) || defined(MATCH_STRING_L) || defined(MATCH_STRING_M)
char *matched_url = (char*)NULL;
#endif

#if STATUS_BAR || defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
GtkWidget *statusbar;
GtkWidget *vbox;
#endif

#if defined(HOTKEY_SEARCH_STRING) || defined(HOTKEY_SEARCH_PREVIOUS) || defined(HOTKEY_SEARCH_NEXT)
  GtkWidget *global_search_string;
#endif

struct terminal {
  GtkWidget *vte;
#if CLOSE_DIALOG || CLOSE_SAFELY || TAB_NEW_PATH_EQUAL_OLD
  int pid;
#endif
#if TAB_CLOSE_BUTTON
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *label_edit;
#endif
#ifdef SCROLLBAR
  GtkWidget *hbox;
  GtkWidget *scrollbar;
#endif
#if defined(HOTKEY_TAB_EDIT_LABEL) || MENU_TAB_EDIT_LABEL
  int label_exist;
#endif
#if defined(HOTKEY_SEARCH_STRING) || defined(HOTKEY_SEARCH_PREVIOUS) || defined(HOTKEY_SEARCH_NEXT)
  GtkWidget *search_string;
  int global_string;
#endif
};

struct terminal *term;

#if CLOSE_DIALOG
GtkWidget* make_close_dialog()
{
  GtkWidget *dialog;
  GtkWidget *dialog_hbox;
#if BUTTON_ORDER_BY_RCFILE
  if (button_order)
    dialog = gtk_dialog_new_with_buttons(LABEL_DIALOG_CLOSE, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  else
#endif
    dialog = gtk_dialog_new_with_buttons(LABEL_DIALOG_CLOSE, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
  dialog_hbox = (GtkWidget*)gtk_hbox_new(0, 0);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), dialog_hbox);
  gtk_box_pack_start(GTK_BOX(dialog_hbox), gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG), 0, 0, 0);
  gtk_box_pack_start(GTK_BOX(dialog_hbox), gtk_label_new(LABEL_DIALOG_CLOSE), 1, 0, 0);
  return dialog;
}
#endif

void DEL_TAB(GtkWidget *widget, int do_close_dialog)
{
  int index = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
#if CLOSE_DIALOG
  GET_CURRENT_TAB(index);
#if CLOSE_SAFELY
  int user_want_to_close = 0;
#endif
  if (do_close_dialog) {
    /* Known to work on FreeBSD *with* linprocfs mounted */
    char *stat = NULL;
    char **stats = NULL;
    gsize length;
    if (g_file_get_contents(g_strdup_printf("/proc/%d/stat", term->pid), &stat, &length, NULL)) {
      stats = g_strsplit_set(stat, " ", 8);
      if (atoi(stats[7]) != term->pid) {
        GtkWidget *dialog = make_close_dialog();
        gtk_widget_show_all(dialog);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_CLOSE) {
          gtk_widget_destroy(dialog);
#if CLOSE_SAFELY
          user_want_to_close = 1;
#endif
        } else {
          gtk_widget_destroy(dialog);
          return;
        }
      }
    }
  }
#endif

#if CLOSE_SAFELY
  GET_CURRENT_TAB(index);
#if CLOSE_DIALOG
  if (user_want_to_close || !do_close_dialog)
    kill(term->pid, 9);
  else
#endif
  {
    /* Known to work on FreeBSD *with* linprocfs mounted */
    char *stat = NULL;
    char **stats = NULL;
    gsize length;
    if (g_file_get_contents(g_strdup_printf("/proc/%d/stat", term->pid), &stat, &length, NULL)) {
      stats = g_strsplit_set(stat, " ", 8);
      if (atoi(stats[7]) != term->pid)
        return;
      else
        kill(term->pid, 9);
    }
  }
#endif

  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  g_free(term);
  (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) < 2) ? gtk_main_quit() : gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), index);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), index);

#if TABBAR_AUTOHIDE
  if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 1
#if defined(HOTKEY_TOGGLE_TABBAR) || MENU_TOGGLE_TABBAR
      && tabbar_status
#endif
     ) {
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), 0);
#ifdef VTE_FUNNY
    if (!window_fullscreen_status)
#endif
      VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
  }
#endif
}

#if TAB_CLOSE_BUTTON
void button_clicked(GtkWidget *widget, void *data)
{
  int index = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  int i = 0;
  int killed = 0;
  for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
    GET_CURRENT_TAB(i);
    if (data == term->button) {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), i);
      DEL_TAB(NULL, CLOSE_DIALOG);
      killed = i;
    }
  }
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), (killed < index) ? index - 1 : index);
}
#endif

#if TAB_CLOSE_BUTTON
void tab_close_button(GtkWidget *tab_label)
{
  term->label = (GtkWidget*)gtk_hbox_new(0, 0);
  term->button = gtk_button_new();
  gtk_button_set_image(GTK_BUTTON(term->button), gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
  gtk_button_set_relief(GTK_BUTTON(term->button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click(GTK_BUTTON(term->button), 0);
#if !GTK_CHECK_VERSION(2,91,6)
  GtkRcStyle *rcstyle = gtk_rc_style_new();
  rcstyle->xthickness = 0;
  rcstyle->ythickness = 0;
  gtk_widget_modify_style(term->button, rcstyle);
  g_object_unref(rcstyle);
#endif
  term->label_edit = tab_label;
#if GTK_CHECK_VERSION(2,91,2) && defined(USE_GTK_GRID) && TAB_EXPANDED_WIDTH
  gtk_widget_set_hexpand(term->label_edit, TRUE);
#endif
  gtk_box_pack_start(GTK_BOX(term->label), term->label_edit, 1, 1, 0);
  gtk_box_pack_start(GTK_BOX(term->label), term->button, 0, 0, 0);
  gtk_widget_show_all(term->label);
  g_signal_connect(term->button, "clicked", G_CALLBACK(button_clicked), term->button);
}
#endif

#if WINDOW_TITLE_DYNAMIC || TAB_LABEL_DYNAMIC
void do_title_changed()
{
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
#if TAB_LABEL_DYNAMIC
#if TAB_CLOSE_BUTTON
  tab_close_button(gtk_label_new(vte_terminal_get_window_title(VTE_TERMINAL(term->vte))));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), VTE_HBOX, term->label);
#endif
#if !TAB_CLOSE_BUTTON
  gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), VTE_HBOX, vte_terminal_get_window_title(VTE_TERMINAL(term->vte)));
#endif
#endif
#if WINDOW_TITLE_DYNAMIC
  gtk_window_set_title(GTK_WINDOW(main_window), vte_terminal_get_window_title(VTE_TERMINAL(term->vte)));
#endif
}
#endif

#if BELL_URGENT
void do_beep()
{
  gtk_window_set_urgency_hint(GTK_WINDOW(main_window), TRUE);
}
#endif

#if MENU || defined(MATCH_STRING_L) || defined(MATCH_STRING_M)
int menu_popup(GtkWidget *widget, GdkEventButton *event)
{
#ifdef ONLY_ONE_MENU_ITEM
  int x = 0;
  int y = 0;
#endif
#if defined(ONLY_ONE_MENU_ITEM) && GTK_CHECK_VERSION(2,99,0)
  GdkDevice *device = gtk_get_current_event_device();
#endif
#if defined(ONLY_ONE_MENU_ITEM) || MENU_PASTE
  GdkDisplay *display = gdk_display_get_default();
#endif

#if defined(MENU_MATCH_STRING_EXEC) || defined(MATCH_STRING_L) || defined(MATCH_STRING_M)
  int tag = -1;
#endif

#ifdef MATCH_STRING_L
  if (event->button == 1) {
    GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
    matched_url = vte_terminal_match_check(VTE_TERMINAL(term->vte), event->x / vte_terminal_get_char_width(VTE_TERMINAL(term->vte)), event->y / vte_terminal_get_char_height(VTE_TERMINAL(term->vte)), &tag);
    if (matched_url != NULL) {
      char new_window_str[256];
      if (event->button == 1)
        g_snprintf(new_window_str, sizeof(new_window_str), "%s '%s' &", MATCH_STRING_L, matched_url);
      system(new_window_str);
      matched_url = NULL;
      return TRUE;
    }
  }
#endif

#ifdef MATCH_STRING_M
  if (event->button == 2) {
    GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
    matched_url = vte_terminal_match_check(VTE_TERMINAL(term->vte), event->x / vte_terminal_get_char_width(VTE_TERMINAL(term->vte)), event->y / vte_terminal_get_char_height(VTE_TERMINAL(term->vte)), &tag);
    if (matched_url != NULL) {
      char new_window_str[256];
      if (event->button == 2)
        g_snprintf(new_window_str, sizeof(new_window_str), "%s '%s' &", MATCH_STRING_M, matched_url);
      system(new_window_str);
      matched_url = NULL;
      return TRUE;
    }
  }
#endif

#if MENU
  if (event->button == 3) {
    GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
#if MENU_COPY
    if (vte_terminal_get_has_selection(VTE_TERMINAL(term->vte))ONLY_ONE_MENU_HANDLE)
      gtk_widget_set_sensitive(menu_copy, 1);
    else
      gtk_widget_set_sensitive(menu_copy, 0);
#endif
#if MENU_PASTE
    if (gtk_clipboard_wait_is_text_available(gtk_clipboard_get_for_display(display, GDK_SELECTION_CLIPBOARD))ONLY_ONE_MENU_HANDLE)
      gtk_widget_set_sensitive(menu_paste, 1);
    else
      gtk_widget_set_sensitive(menu_paste, 0);
#endif
#if MENU_FONT_DEFAULT_SIZE
    if ((font_size != font_size_default)ONLY_ONE_MENU_HANDLE)
      gtk_widget_set_sensitive(menu_zoom_100, 1);
    else
      gtk_widget_set_sensitive(menu_zoom_100, 0);
#endif
#ifdef MENU_MATCH_STRING_EXEC
      matched_url = vte_terminal_match_check(VTE_TERMINAL(term->vte), event->x / vte_terminal_get_char_width(VTE_TERMINAL(term->vte)), event->y / vte_terminal_get_char_height(VTE_TERMINAL(term->vte)), &tag);
      if (matched_url != NULL) {
        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
      } else {
        gtk_widget_hide(match_open);
        gtk_widget_hide(match_copy);
        if (match_item)
          gtk_widget_hide(match_item);
#endif
#ifdef MENU_CUSTOM
        if (menu_item_success)
#endif
          gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
#ifdef ONLY_ONE_MENU_ITEM
        if (menu_item_success == 1) {
          gdk_device_get_position(device, NULL, &x, &y);
          gtk_test_widget_send_key(menu, GDK_Down, 0);
          gtk_test_widget_send_key(menu, GDK_Return, 0);
          gdk_device_warp(device, gdk_display_get_default_screen(display), x, y);
        }
#endif
#ifdef MENU_MATCH_STRING_EXEC
      }
#endif
      return TRUE;
  }
#endif /* MENU */
  return FALSE;
}
#endif

#ifdef BACKGROUND_SATURATION
void do_saturation_routine()
{
  vte_terminal_set_background_saturation(VTE_TERMINAL(term->vte), saturation_level);
#if BACKGROUND_OPACITY
  vte_terminal_set_opacity(VTE_TERMINAL(term->vte), (1 - saturation_level) * 65535);
#endif
#if DO_TOGGLE_BACKGROUND && TOGGLE_BG_OPACITY
  if (!strncmp(background_order[background_status], "Opacity", 8))
    vte_terminal_set_opacity(VTE_TERMINAL(term->vte), (1 - saturation_level) * 65535);
#endif
}
#endif

#ifdef BACKGROUND_SATURATION
void saturation_routine()
{
  int i = 0;
  for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
    GET_CURRENT_TAB(i);
    do_saturation_routine();
  }
}
#endif

#if MOUSE_CTRL_SATURATION
int scroll_event(GtkWidget *widget, GdkEventScroll *event)
{
#if HOTKEY
  if ((event->state & KEY_MOD_MASK) == KEY_MOD_MASK) {
    if (event->direction == GDK_SCROLL_UP) {
      saturation_level += 0.025;
      if (saturation_level > 1)
        saturation_level = 1;
      saturation_routine();
      return TRUE;
    }
    if (event->direction == GDK_SCROLL_DOWN) {
      saturation_level -= 0.025;
      if (saturation_level < 0)
        saturation_level = 0;
      saturation_routine();
      return TRUE;
    }
  }
#endif
  if (event->direction == GDK_SCROLL_RIGHT) {
    saturation_level += 0.025;
    if (saturation_level > 1)
      saturation_level = 1;
    saturation_routine();
    return TRUE;
  }
  if (event->direction == GDK_SCROLL_LEFT) {
    saturation_level -= 0.025;
    if (saturation_level < 0)
      saturation_level = 0;
    saturation_routine();
    return TRUE;
  }
  return FALSE;
}
#endif

#if defined(HOTKEY_TOGGLE_SCROLLBAR) || MENU_TOGGLE_SCROLLBAR
void hide_scrollbar()
{
    int i = 0;
    for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
      GET_CURRENT_TAB(i);
      gtk_widget_hide(term->scrollbar);
    }
}
#endif

void add_tab()
{
  term = g_malloc(sizeof(struct terminal));
#if defined(HOTKEY_SEARCH_STRING) || defined(HOTKEY_SEARCH_PREVIOUS) || defined(HOTKEY_SEARCH_NEXT)
  term->search_string = gtk_entry_new();
  term->global_string = 1;
#endif

#if defined(HOTKEY_TAB_EDIT_LABEL) || MENU_TAB_EDIT_LABEL
  int label_exist = 0;
#endif

#if defined(TAB_LABEL) || defined(TAB_LABEL_CUSTOM)
  GtkWidget *label;
#endif

#ifdef TAB_LABEL
  label = gtk_label_new(g_strdup_printf(gettext(TAB_LABEL), (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) + 1)));
#if defined(HOTKEY_TAB_EDIT_LABEL) || MENU_TAB_EDIT_LABEL
  label_exist = 1;
#endif
#endif

#ifdef TAB_LABEL_CUSTOM
  if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) < label_style_size) {
    label = gtk_label_new(g_strdup_printf("%s", label_style_custom[gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook))]));
#if defined(HOTKEY_TAB_EDIT_LABEL) || MENU_TAB_EDIT_LABEL
    label_exist = 1;
#endif
  } else
    label = NULL;
#endif

#if TAB_CLOSE_BUTTON
  tab_close_button(label);
#endif

  term->vte = vte_terminal_new();

#ifdef SCROLLBAR
  term->hbox = (GtkWidget*)gtk_hbox_new(0, 0);
  term->scrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, vte_terminal_get_adjustment(VTE_TERMINAL(term->vte)));
#if !(SCROLLBAR & 1)
  gtk_box_pack_start(GTK_BOX(term->hbox), term->scrollbar, 0, 0, 0);
#endif
  gtk_box_pack_start(GTK_BOX(term->hbox), term->vte, 1, 1, 0);
#if (SCROLLBAR & 1)
  gtk_box_pack_start(GTK_BOX(term->hbox), term->scrollbar, 0, 0, 0);
#endif
#endif

#if TAB_NEW_PATH_EQUAL_OLD
  char *default_directory = (char*)DEFAULT_DIRECTORY;
  if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 0)
    default_directory = g_file_read_link(g_strdup_printf("/proc/%d/cwd", ((struct terminal*)g_object_get_data(G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))))), "current_tab"))->pid), NULL);
#endif

#if !VTE_FORK_CMD_OLD
  char **evilvte_argv = NULL;
#if COMMAND_EXEC_PROGRAM
  if (login_shell_flag & 2)
    evilvte_argv = default_argv;
  else
#endif
    g_shell_parse_argv(VTE_DEFAULT_COMMAND, NULL, &evilvte_argv, NULL);
#if COMMAND_LOGIN_SHELL
  if (login_shell_flag == 1)
    evilvte_argv[1] = "-";
#endif
  vte_terminal_fork_command_full(VTE_TERMINAL(term->vte), EVILVTE_PTY_NO_LASTLOG | EVILVTE_PTY_NO_UTMP | EVILVTE_PTY_NO_WTMP | EVILVTE_PTY_NO_HELPER, VTE_DEFAULT_DIRECTORY, evilvte_argv, NULL, G_SPAWN_CHILD_INHERITS_STDIN | G_SPAWN_SEARCH_PATH | G_SPAWN_FILE_AND_ARGV_ZERO, NULL, NULL, GET_VTE_CHILD_PID, NULL);
#endif

#if VTE_FORK_CMD_OLD
#if COMMAND_LOGIN_SHELL
  if (login_shell_flag == 1)
    GET_VTE_CHILD_PID vte_terminal_fork_command(VTE_TERMINAL(term->vte), VTE_DEFAULT_COMMAND, login_shell, NULL, VTE_DEFAULT_DIRECTORY, RECORD_LASTLOG, RECORD_UTMP, RECORD_WTMP);
  else
#endif
    GET_VTE_CHILD_PID vte_terminal_fork_command(VTE_TERMINAL(term->vte), VTE_DEFAULT_COMMAND, DEFAULT_ARGV, NULL, VTE_DEFAULT_DIRECTORY, RECORD_LASTLOG, RECORD_UTMP, RECORD_WTMP);
#endif

#ifdef MATCH_STRING
#if !VTE_CHECK_VERSION(0,17,1)
  vte_terminal_match_add(VTE_TERMINAL(term->vte), MATCH_STRING);
#endif
#if VTE_CHECK_VERSION(0,17,1)
  vte_terminal_match_add_gregex(VTE_TERMINAL(term->vte), g_regex_new(MATCH_STRING, 0, 0, NULL), 0);
#endif
#endif

#if MATCH_STRING_HTTP
#if !VTE_CHECK_VERSION(0,17,1)
  vte_terminal_match_add(VTE_TERMINAL(term->vte), MATCH_HTTP_DATA);
#endif
#if VTE_CHECK_VERSION(0,17,1)
  vte_terminal_match_add_gregex(VTE_TERMINAL(term->vte), g_regex_new(MATCH_HTTP_DATA, 0, 0, NULL), 0);
#endif
#endif

#if MATCH_STRING_MAIL
#if !VTE_CHECK_VERSION(0,17,1)
  vte_terminal_match_add(VTE_TERMINAL(term->vte), MATCH_MAIL_DATA);
#endif
#if VTE_CHECK_VERSION(0,17,1)
  vte_terminal_match_add_gregex(VTE_TERMINAL(term->vte), g_regex_new(MATCH_MAIL_DATA, 0, 0, NULL), 0);
#endif
#endif

#if MATCH_STRING_FILE
#if !VTE_CHECK_VERSION(0,17,1)
  vte_terminal_match_add(VTE_TERMINAL(term->vte), MATCH_FILE_DATA);
#endif
#if VTE_CHECK_VERSION(0,17,1)
  vte_terminal_match_add_gregex(VTE_TERMINAL(term->vte), g_regex_new(MATCH_FILE_DATA, 0, 0, NULL), 0);
#endif
#endif

#ifdef DEFAULT_EMULATION_TYPE
  vte_terminal_set_emulation(VTE_TERMINAL(term->vte), DEFAULT_EMULATION_TYPE);
#endif

#ifdef FONT_ENABLE_BOLD_TEXT
  vte_terminal_set_allow_bold(VTE_TERMINAL(term->vte), FONT_ENABLE_BOLD_TEXT);
#endif

#ifdef BELL_AUDIBLE
  vte_terminal_set_audible_bell(VTE_TERMINAL(term->vte), BELL_AUDIBLE);
#endif

#ifdef BELL_VISIBLE
  vte_terminal_set_visible_bell(VTE_TERMINAL(term->vte), BELL_VISIBLE);
#endif

#ifdef BACKGROUND_IMAGE
  vte_terminal_set_background_image_file(VTE_TERMINAL(term->vte), imgstr);
#endif

#if BACKGROUND_OPACITY
  vte_terminal_set_opacity(VTE_TERMINAL(term->vte), (1 - saturation_level) * 65535);
#endif

#if defined(BACKGROUND_TINT_COLOR) && BACKGROUND_EXIST
  vte_terminal_set_background_tint_color(VTE_TERMINAL(term->vte), &color_tint);
#endif

#if defined(BACKGROUND_SATURATION) && BACKGROUND_EXIST
  vte_terminal_set_background_saturation(VTE_TERMINAL(term->vte), saturation_level);
#endif

#ifdef BACKGROUND_TRANSPARENT
  vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), BACKGROUND_TRANSPARENT);
#endif

#ifdef BACKSPACE_KEY
  vte_terminal_set_backspace_binding(VTE_TERMINAL(term->vte), BACKSPACE_KEY);
#endif

#ifdef DELETE_KEY
  vte_terminal_set_delete_binding(VTE_TERMINAL(term->vte), DELETE_KEY);
#endif

#ifdef COLOR_STYLE
  vte_terminal_set_colors(VTE_TERMINAL(term->vte), NULL, NULL, color_style, 16);
#endif

#if defined(COLOR_BACKGROUND) || defined(COLOR_TEXT_BOLD) || defined(CURSOR_COLOR) || defined(COLOR_TEXT_DIM) || defined(COLOR_FOREGROUND) || defined(COLOR_TEXT_HIGHLIGHTED)
#ifndef COLOR_STYLE
  vte_terminal_set_default_colors(VTE_TERMINAL(term->vte));
#endif
#endif

#ifdef COLOR_BACKGROUND
  GdkColor color_bg;
  gdk_color_parse(COLOR_BACKGROUND, &color_bg);
  vte_terminal_set_color_background(VTE_TERMINAL(term->vte), &color_bg);
#endif

#ifdef COLOR_TEXT_BOLD
  GdkColor color_bold;
  gdk_color_parse(COLOR_TEXT_BOLD, &color_bold);
  vte_terminal_set_color_bold(VTE_TERMINAL(term->vte), &color_bold);
#endif

#ifdef CURSOR_COLOR
  GdkColor color_cursor;
  gdk_color_parse(CURSOR_COLOR, &color_cursor);
  vte_terminal_set_color_cursor(VTE_TERMINAL(term->vte), &color_cursor);
#endif

#ifdef COLOR_TEXT_DIM
  GdkColor color_dim;
  gdk_color_parse(COLOR_TEXT_DIM, &color_dim);
  vte_terminal_set_color_dim(VTE_TERMINAL(term->vte), &color_dim);
#endif

#ifdef COLOR_FOREGROUND
  GdkColor color_fg;
  gdk_color_parse(COLOR_FOREGROUND, &color_fg);
  vte_terminal_set_color_foreground(VTE_TERMINAL(term->vte), &color_fg);
#endif

#ifdef COLOR_TEXT_HIGHLIGHTED
  GdkColor color_highlight;
  gdk_color_parse(COLOR_TEXT_HIGHLIGHTED, &color_highlight);
  vte_terminal_set_color_highlight(VTE_TERMINAL(term->vte), &color_highlight);
#endif

#ifdef CURSOR_BLINKS
#if !VTE_CHECK_VERSION(0,17,1)
  vte_terminal_set_cursor_blinks(VTE_TERMINAL(term->vte), CURSOR_BLINKS);
#endif
#if VTE_CHECK_VERSION(0,17,1)
  vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(term->vte), VTE_CURSOR_BLINKS);
#endif
#endif

#if defined(CURSOR_SHAPE) && VTE_CHECK_VERSION(0,19,1)
  vte_terminal_set_cursor_shape(VTE_TERMINAL(term->vte), CURSOR_SHAPE);
#endif

#ifdef DEFAULT_ENCODING
  vte_terminal_set_encoding(VTE_TERMINAL(term->vte), DEFAULT_ENCODING);
#endif

#ifdef FONT
#if defined(HOTKEY_TOGGLE_ANTI_ALIAS) || MENU_TOGGLE_ANTI_ALIAS
  EVILVTE_SET_FONT(VTE_TERMINAL(term->vte), font_str, antialias_status);
#endif
#if !defined(HOTKEY_TOGGLE_ANTI_ALIAS) && !MENU_TOGGLE_ANTI_ALIAS
  EVILVTE_SET_FONT(VTE_TERMINAL(term->vte), font_str, VTE_ANTI_ALIAS);
#endif
#endif

#ifdef MOUSE_CURSOR_AUTOHIDE
  vte_terminal_set_mouse_autohide(VTE_TERMINAL(term->vte), MOUSE_CURSOR_AUTOHIDE);
#endif

#ifdef BACKGROUND_SCROLLABLE
  vte_terminal_set_scroll_background(VTE_TERMINAL(term->vte), BACKGROUND_SCROLLABLE);
#endif

#ifdef SCROLL_ON_KEYSTROKE
  vte_terminal_set_scroll_on_keystroke(VTE_TERMINAL(term->vte), SCROLL_ON_KEYSTROKE);
#endif

#ifdef SCROLL_ON_OUTPUT
  vte_terminal_set_scroll_on_output(VTE_TERMINAL(term->vte), SCROLL_ON_OUTPUT);
#endif

#ifdef SCROLL_LINES
  vte_terminal_set_scrollback_lines(VTE_TERMINAL(term->vte), SCROLL_LINES);
#endif

#if !VTE_CHECK_VERSION(0,27,1) || !GTK_CHECK_VERSION(2,91,1)
#if VTE_COLUMNS && VTE_ROWS
  vte_terminal_set_size(VTE_TERMINAL(term->vte), VTE_COLUMNS, VTE_ROWS);
#endif
#endif

#ifdef WORD_CHARS
  vte_terminal_set_word_chars(VTE_TERMINAL(term->vte), WORD_CHARS);
#endif

  gtk_widget_show_all(VTE_HBOX);

  int index = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), VTE_HBOX, VTE_LABEL);
  g_object_set_data(G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), index)), "current_tab", term);

  g_signal_connect(term->vte, "child-exited", G_CALLBACK(del_tab), (int*)CLOSE_DIALOG);

#if WINDOW_TITLE_DYNAMIC || TAB_LABEL_DYNAMIC
  g_signal_connect(term->vte, "window-title-changed", do_title_changed, NULL);
#endif

#if BELL_URGENT
  g_signal_connect(term->vte, "beep", do_beep, NULL);
#endif

#if MENU || defined(MATCH_STRING_L) || defined(MATCH_STRING_M)
  g_signal_connect(term->vte, "button-press-event", G_CALLBACK(menu_popup), NULL);
#endif

#if MOUSE_CTRL_SATURATION
  g_signal_connect(term->vte, "scroll-event", G_CALLBACK(scroll_event), NULL);
#endif

#if TABBAR_AUTOHIDE
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 1) ? 0 : VTE_TABBAR);
#ifndef VTE_FUNNY
  VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
#endif
  gtk_widget_set_can_focus(notebook, 0);
#endif

#if DO_TOGGLE_BACKGROUND
#if TOGGLE_BG_TRANSPARENT
  if (!strncmp(background_order[background_status], "Transparent", 12))
    vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), 1);
#endif
#if TOGGLE_BG_IMAGE
  if (!strncmp(background_order[background_status], "Image", 6)) {
    vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), 0);
    vte_terminal_set_background_image_file(VTE_TERMINAL(term->vte), imgstr);
  }
#endif
#if TOGGLE_BG_NO_BACKGROUND
  if (!strncmp(background_order[background_status], "No background", 14)) {
    vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), 0);
    vte_terminal_set_background_image_file(VTE_TERMINAL(term->vte), "/dev/null");
#if INIT_OPACITY || BACKGROUND_OPACITY
    vte_terminal_set_opacity(VTE_TERMINAL(term->vte), 65535);
#endif
  }
#endif
#if TOGGLE_BG_OPACITY
  if (!strncmp(background_order[background_status], "Opacity", 8)) {
    vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), 0);
    vte_terminal_set_background_image_file(VTE_TERMINAL(term->vte), "/dev/null");
    vte_terminal_set_opacity(VTE_TERMINAL(term->vte), (1 - saturation_level) * 65535);
  }
#endif
#endif

#if VTE_CHECK_VERSION(0,27,1) && GTK_CHECK_VERSION(2,91,1) && defined(VTE_FUNNY)
  gtk_widget_set_size_request(term->vte, VTE_COLUMNS * vte_terminal_get_char_width(VTE_TERMINAL(term->vte)) + INNER_BORDER_W, VTE_ROWS * vte_terminal_get_char_height(VTE_TERMINAL(term->vte)) + INNER_BORDER_H);
#endif

  gtk_widget_show_all(notebook);

#if defined(HOTKEY_TOGGLE_SCROLLBAR) || MENU_TOGGLE_SCROLLBAR
  if (!scrollbar_status) {
    hide_scrollbar();
#ifndef VTE_FUNNY
    VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
#endif
  }
#endif

  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), index);

#if TAB_EXPANDED_WIDTH
  gtk_container_child_set(GTK_CONTAINER(notebook), VTE_HBOX, "tab-expand", 1, NULL);
#endif
#if TAB_REORDERABLE && GTK_CHECK_VERSION(2,9,0)
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), VTE_HBOX, 1);
#endif

  gtk_window_set_focus(GTK_WINDOW(main_window), term->vte);

#if defined(HOTKEY_TAB_EDIT_LABEL) || MENU_TAB_EDIT_LABEL
  term->label_exist = label_exist;
#endif
}

#if defined(HOTKEY_SATURATION_DIALOG) || MENU_CHANGE_SATURATION
void do_change_saturation()
{
  saturation_level = gtk_range_get_value(GTK_RANGE(adjustment));
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  do_saturation_routine();
}
#endif

#if BELL_URGENT
gboolean focus_in_event()
{
  gtk_window_set_urgency_hint(GTK_WINDOW(main_window), FALSE);
  return FALSE;
}
#endif

#if defined(HOTKEY_TOGGLE_ON_TOP) || MENU_TOGGLE_ON_TOP
void do_always_on_top()
{
  always_on_top ^= 1;
  gtk_window_set_keep_above(GTK_WINDOW(main_window), always_on_top);
#if COMMAND_AT_ROOT_WINDOW
  if (at_root_window && !always_on_top) {
    gtk_window_set_keep_below(GTK_WINDOW(main_window), 1);
    gtk_window_set_decorated(GTK_WINDOW(main_window), 0);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(main_window), 1);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(main_window), 1);
  }
#endif
}
#endif

#if defined(HOTKEY_SEARCH_STRING) || defined(HOTKEY_SEARCH_PREVIOUS) || defined(HOTKEY_SEARCH_NEXT)
void do_edit_search()
{
  GtkWidget *entry = gtk_entry_new();
  GtkWidget *dialog;
#if BUTTON_ORDER_BY_RCFILE
  if (button_order)
    dialog = gtk_dialog_new_with_buttons(LABEL_DIALOG_SEARCH, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  else
#endif
    dialog = gtk_dialog_new_with_buttons(LABEL_DIALOG_SEARCH, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
  if (strlen(gtk_entry_get_text(GTK_ENTRY(term->search_string))))
    gtk_entry_set_text(GTK_ENTRY(entry), gtk_entry_get_text(GTK_ENTRY(term->search_string)));
  else {
    if (term->global_string)
      gtk_entry_set_text(GTK_ENTRY(entry), gtk_entry_get_text(GTK_ENTRY(global_search_string)));
  }
  gtk_entry_set_activates_default(GTK_ENTRY(entry), 1);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry);
  gtk_widget_show_all(dialog);
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    term->global_string = 0;
    if (strlen(gtk_entry_get_text(GTK_ENTRY(entry))))
      gtk_entry_set_text(GTK_ENTRY(global_search_string), gtk_entry_get_text(GTK_ENTRY(entry)));
    gtk_entry_set_text(GTK_ENTRY(term->search_string), gtk_entry_get_text(GTK_ENTRY(entry)));
    vte_terminal_search_set_gregex(VTE_TERMINAL(term->vte), g_regex_new(gtk_entry_get_text(GTK_ENTRY(term->search_string)), VTE_REGEX, 0, NULL));
    vte_terminal_search_set_wrap_around(VTE_TERMINAL(term->vte), TRUE);
  }
  gtk_widget_destroy(dialog);
}
#endif

#if defined(HOTKEY_TAB_EDIT_LABEL) || MENU_TAB_EDIT_LABEL
void do_edit_label()
{
  char *label_name = "";
  int index = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  GET_CURRENT_TAB(index);
  if (term->label_exist) {
#if TAB_CLOSE_BUTTON
    label_name = (char*)gtk_label_get_text(GTK_LABEL(term->label_edit));
#endif
#if !TAB_CLOSE_BUTTON
    label_name = (char*)gtk_label_get_text(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), index))));
#endif
  }
  GtkWidget *entry = gtk_entry_new();
  GtkWidget *dialog;
#if BUTTON_ORDER_BY_RCFILE
  if (button_order)
    dialog = gtk_dialog_new_with_buttons(label_name, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  else
#endif
    dialog = gtk_dialog_new_with_buttons(label_name, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
  gtk_entry_set_text(GTK_ENTRY(entry), label_name);
  gtk_entry_set_activates_default(GTK_ENTRY(entry), 1);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry);
  gtk_widget_show_all(dialog);
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
#if TAB_CLOSE_BUTTON
    tab_close_button(gtk_label_new(gtk_entry_get_text(GTK_ENTRY(entry))));
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), VTE_HBOX, term->label);
#endif
#if !TAB_CLOSE_BUTTON
    gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), VTE_HBOX, gtk_entry_get_text(GTK_ENTRY(entry)));
#endif
    term->label_exist = 1;
  }
  gtk_widget_destroy(dialog);
}
#endif

#if defined(HOTKEY_SATURATION_DIALOG) || MENU_CHANGE_SATURATION
void do_menu_saturation()
{
  double saturation_level_old = saturation_level;
  GtkWidget *dialog;
#if BUTTON_ORDER_BY_RCFILE
  if (button_order)
    dialog = gtk_dialog_new_with_buttons(LABEL_MENU_SATURATION, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  else
#endif
    dialog = gtk_dialog_new_with_buttons(LABEL_MENU_SATURATION, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  adjustment = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
  gtk_range_set_value(GTK_RANGE(adjustment), saturation_level_old);
  g_signal_connect_after(adjustment, "change-value", do_change_saturation, NULL);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), adjustment);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
  gtk_widget_show_all(dialog);
  saturation_level = (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) ? gtk_range_get_value(GTK_RANGE(adjustment)) : saturation_level_old;
  gtk_widget_destroy(dialog);
  saturation_routine();
}
#endif

#if defined(HOTKEY_COLOR_BACKGROUND) || MENU_COLOR_BACKGROUND
void do_menu_tint_color()
{
  GtkColorSelectionDialog *color_tint_dialog = (GtkColorSelectionDialog*)gtk_color_selection_dialog_new(LABEL_DIALOG_BACKGROUND_TINT);
  gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(color_tint_dialog)), &color_tint);
  if (GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(color_tint_dialog))) {
    gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(color_tint_dialog)), &color_tint);
    int i = 0;
    for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
      GET_CURRENT_TAB(i);
      vte_terminal_set_background_tint_color(VTE_TERMINAL(term->vte), &color_tint);
    }
  }
  gtk_widget_destroy(GTK_WIDGET(color_tint_dialog));
}
#endif

#ifdef FONT
void calculate_font()
{
    const char *number_char[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    int len = strlen(font_name) - 1;
    int loop = 1;
    font_size = 0;
    int i = 0;
    while (len > 0 && isdigit(font_name[len])) {
      for (i = 0 ; i < 10 ; i++) {
        if (font_name[len] == *number_char[i])
          font_size += i * loop;
      }
      loop *= 10;
      font_name[len--] = 0;
    }
    while (len > 0 && font_name[len] == ' ')
      font_name[len--] = 0;
}
#endif

#if defined(HOTKEY_FONT_DEFAULT_SIZE) || MENU_FONT_DEFAULT_SIZE || defined(HOTKEY_FONT_BIGGER) || MENU_FONT_BIGGER || defined(HOTKEY_FONT_SMALLER) || MENU_FONT_SMALLER || defined(HOTKEY_FONT_SELECT) || MENU_FONT_SELECT
void do_zoom_routine()
{
  g_snprintf(font_str, sizeof(font_str), "%s %d", font_name, font_size);
  int i = 0;
  for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
    GET_CURRENT_TAB(i);
    vte_terminal_set_font_from_string(VTE_TERMINAL(term->vte), font_str);
#ifdef VTE_FUNNY
    gtk_widget_set_size_request(term->vte, VTE_COLUMNS * vte_terminal_get_char_width(VTE_TERMINAL(term->vte)) + INNER_BORDER_W, VTE_ROWS * vte_terminal_get_char_height(VTE_TERMINAL(term->vte)) + INNER_BORDER_H);
#endif
  }
#ifndef VTE_FUNNY
  VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
#endif
}
#endif

#if defined(HOTKEY_FONT_SELECT) || MENU_FONT_SELECT
void do_select_font()
{
  GtkWidget *font_dialog = gtk_font_selection_dialog_new(font_str);
  gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(font_dialog), font_str);
  if (GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(font_dialog))) {
    g_snprintf(font_name, sizeof(font_name), "%s", gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(font_dialog)));
    calculate_font();
    do_zoom_routine();
  }
  gtk_widget_destroy(font_dialog);
}
#endif

#if defined(HOTKEY_TOGGLE_ANTI_ALIAS) || MENU_TOGGLE_ANTI_ALIAS
void do_toggle_antialias()
{
  antialias_status++;
  if (antialias_status > VTE_ANTI_ALIAS_FORCE_DISABLE)
    antialias_status = VTE_ANTI_ALIAS_FORCE_ENABLE;
  int i = 0;
  for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
    GET_CURRENT_TAB(i);
    vte_terminal_set_font_from_string_full(VTE_TERMINAL(term->vte), font_str, antialias_status);
  }
}
#endif

#if defined(HOTKEY_TOGGLE_BACKGROUND) || MENU_TOGGLE_BACKGROUND
void do_toggle_bg()
{
  int i = 0;
  background_status++;
  if (background_status >= TOGGLE_BG_ORDER_SIZE)
    background_status = 0;
#if TOGGLE_BG_TRANSPARENT
  if (!strncmp(background_order[background_status], "Transparent", 12)) {
    for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
      GET_CURRENT_TAB(i);
      vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), 1);
    }
  }
#endif
#if TOGGLE_BG_IMAGE
  if (!strncmp(background_order[background_status], "Image", 6)) {
    for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
      GET_CURRENT_TAB(i);
      vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), 0);
      vte_terminal_set_background_image_file(VTE_TERMINAL(term->vte), imgstr);
    }
  }
#endif
#if TOGGLE_BG_NO_BACKGROUND
  if (!strncmp(background_order[background_status], "No background", 14)) {
    for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
      GET_CURRENT_TAB(i);
      vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), 0);
      vte_terminal_set_background_image_file(VTE_TERMINAL(term->vte), "/dev/null");
#if INIT_OPACITY || BACKGROUND_OPACITY
      vte_terminal_set_opacity(VTE_TERMINAL(term->vte), 65535);
#endif
    }
  }
#endif
#if TOGGLE_BG_OPACITY
  if (!strncmp(background_order[background_status], "Opacity", 8)) {
    for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
      GET_CURRENT_TAB(i);
      vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), 0);
      vte_terminal_set_background_image_file(VTE_TERMINAL(term->vte), "/dev/null");
      vte_terminal_set_opacity(VTE_TERMINAL(term->vte), (1 - saturation_level) * 65535);
    }
  }
#endif
}
#endif

#if defined(HOTKEY_TOGGLE_DECORATED) || MENU_TOGGLE_DECORATED
void do_toggle_decorated()
{
  window_decorated_status ^= 1;
  gtk_window_set_decorated(GTK_WINDOW(main_window), window_decorated_status);
  int index = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  GET_CURRENT_TAB(index);
  gtk_widget_hide(main_window);
  gtk_widget_show_all(main_window);
#if defined(HOTKEY_TOGGLE_SCROLLBAR) || MENU_TOGGLE_SCROLLBAR
  if (!scrollbar_status)
    hide_scrollbar();
#endif
#if defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
  if (status_bar_status)
    gtk_window_set_has_resize_grip(GTK_WINDOW(main_window), status_bar_resize_grip);
  else {
    gtk_widget_hide(statusbar);
    gtk_window_set_has_resize_grip(GTK_WINDOW(main_window), 0);
  }
#endif
#ifndef VTE_FUNNY
  VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
#endif
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), index);
  gtk_window_set_focus(GTK_WINDOW(main_window), term->vte);
}
#endif

#if defined(HOTKEY_TOGGLE_SCROLLBAR) || MENU_TOGGLE_SCROLLBAR
void do_toggle_scrollbar()
{
  scrollbar_status ^= 1;
  if (scrollbar_status) {
    int i = 0;
    for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
      GET_CURRENT_TAB(i);
      gtk_widget_show(term->scrollbar);
    }
  } else
    hide_scrollbar();
#ifndef VTE_FUNNY
  VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
#endif
}
#endif

#if defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
void do_toggle_status_bar()
{
  status_bar_status ^= 1;
  if (status_bar_status) {
    gtk_widget_show(statusbar);
    gtk_window_set_has_resize_grip(GTK_WINDOW(main_window), status_bar_resize_grip);
  } else {
    gtk_widget_hide(statusbar);
    gtk_window_set_has_resize_grip(GTK_WINDOW(main_window), 0);
  }
#ifndef VTE_FUNNY
  VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
#endif
}
#endif

#if defined(HOTKEY_TOGGLE_TABBAR) || MENU_TOGGLE_TABBAR
void do_toggle_tabbar()
{
#if TABBAR_AUTOHIDE
  tabbar_status = gtk_notebook_get_show_tabs(GTK_NOTEBOOK(notebook));
#endif
  tabbar_status ^= 1;
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), tabbar_status);
#ifndef VTE_FUNNY
  VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
#endif
  gtk_widget_set_can_focus(notebook, 0);
}
#endif

#if defined(HOTKEY_FONT_DEFAULT_SIZE) || MENU_FONT_DEFAULT_SIZE
void do_zoom_100()
{
  font_size = font_size_default;
  do_zoom_routine();
}
#endif

#if defined(HOTKEY_FONT_BIGGER) || MENU_FONT_BIGGER
void do_zoom_in()
{
  font_size++;
  do_zoom_routine();
}
#endif

#if defined(HOTKEY_FONT_SMALLER) || MENU_FONT_SMALLER
void do_zoom_out()
{
  if (font_size > 1) {
    font_size--;
    do_zoom_routine();
  }
}
#endif

#if HOTKEY
int key_press_event(GtkWidget *widget, GdkEventKey *event)
{
#if BELL_URGENT
  gtk_window_set_urgency_hint(GTK_WINDOW(main_window), FALSE);
#endif

  if ((event->state & KEY_MOD_MASK) == KEY_MOD_MASK) {

#ifdef HOTKEY_TOGGLE_HOTKEYS
    if (HOTKEY_TOGGLE_HOTKEYS) {
      hotkey_status ^= 1;
      return TRUE;
    }
#endif

#if defined(HOTKEY_TOGGLE_HOTKEYS) || MENU_TOGGLE_HOTKEYS
    if (hotkey_status) {
#endif

#ifdef HOTKEY_COLOR_BACKGROUND
      if (HOTKEY_COLOR_BACKGROUND) {
        do_menu_tint_color();
        return TRUE;
      }
#endif

#ifdef HOTKEY_SATURATION_DIALOG
      if (HOTKEY_SATURATION_DIALOG) {
        do_menu_saturation();
        return TRUE;
      }
#endif

#ifdef HOTKEY_TOGGLE_ANTI_ALIAS
      if (HOTKEY_TOGGLE_ANTI_ALIAS) {
        do_toggle_antialias();
        return TRUE;
      }
#endif

#ifdef HOTKEY_TOGGLE_DECORATED
      if (HOTKEY_TOGGLE_DECORATED) {
        do_toggle_decorated();
        return TRUE;
      }
#endif

#ifdef HOTKEY_TOGGLE_FULLSCREEN
      if (HOTKEY_TOGGLE_FULLSCREEN) {
        window_fullscreen_status ^= 1;
        window_fullscreen_status ? gtk_window_maximize(GTK_WINDOW(main_window)) : gtk_window_unmaximize(GTK_WINDOW(main_window));
        return TRUE;
      }
#endif

#ifdef HOTKEY_TOGGLE_TABBAR
      if (HOTKEY_TOGGLE_TABBAR) {
        do_toggle_tabbar();
        return TRUE;
      }
#endif

#ifdef HOTKEY_TOGGLE_STATUS_BAR
      if (HOTKEY_TOGGLE_STATUS_BAR) {
        do_toggle_status_bar();
        return TRUE;
      }
#endif

#if defined(HOTKEY_TOGGLE_SCROLLBAR) && defined(SCROLLBAR)
      if (HOTKEY_TOGGLE_SCROLLBAR) {
        do_toggle_scrollbar();
        return TRUE;
      }
#endif

#ifdef HOTKEY_OPEN_NEW_WINDOW
      if (HOTKEY_OPEN_NEW_WINDOW) {
        system(PROGRAM_NAME " &");
        return TRUE;
      }
#endif

#ifdef HOTKEY_TOGGLE_ON_TOP
      if (HOTKEY_TOGGLE_ON_TOP) {
        do_always_on_top();
        return TRUE;
      }
#endif

#ifdef HOTKEY_SEARCH_STRING
      if (HOTKEY_SEARCH_STRING) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        do_edit_search();
        vte_terminal_search_find_previous(VTE_TERMINAL(term->vte));
        return TRUE;
      }
#endif

#ifdef HOTKEY_SEARCH_PREVIOUS
      if (HOTKEY_SEARCH_PREVIOUS) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        if (strlen(gtk_entry_get_text(GTK_ENTRY(term->search_string))))
          vte_terminal_search_find_previous(VTE_TERMINAL(term->vte));
        else {
          if (term->global_string && strlen(gtk_entry_get_text(GTK_ENTRY(global_search_string)))) {
            term->global_string = 0;
            gtk_entry_set_text(GTK_ENTRY(term->search_string), gtk_entry_get_text(GTK_ENTRY(global_search_string)));
            vte_terminal_search_set_gregex(VTE_TERMINAL(term->vte), g_regex_new(gtk_entry_get_text(GTK_ENTRY(term->search_string)), VTE_REGEX, 0, NULL));
            vte_terminal_search_set_wrap_around(VTE_TERMINAL(term->vte), TRUE);
          } else
            do_edit_search();
          vte_terminal_search_find_previous(VTE_TERMINAL(term->vte));
        }
        return TRUE;
      }
#endif

#ifdef HOTKEY_SEARCH_NEXT
      if (HOTKEY_SEARCH_NEXT) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        if (strlen(gtk_entry_get_text(GTK_ENTRY(term->search_string))))
          vte_terminal_search_find_next(VTE_TERMINAL(term->vte));
        else {
          if (term->global_string && strlen(gtk_entry_get_text(GTK_ENTRY(global_search_string)))) {
            term->global_string = 0;
            gtk_entry_set_text(GTK_ENTRY(term->search_string), gtk_entry_get_text(GTK_ENTRY(global_search_string)));
            vte_terminal_search_set_gregex(VTE_TERMINAL(term->vte), g_regex_new(gtk_entry_get_text(GTK_ENTRY(term->search_string)), VTE_REGEX, 0, NULL));
            vte_terminal_search_set_wrap_around(VTE_TERMINAL(term->vte), TRUE);
          } else
            do_edit_search();
          vte_terminal_search_find_next(VTE_TERMINAL(term->vte));
        }
        return TRUE;
      }
#endif

#ifdef HOTKEY_SELECT_ALL
      if (HOTKEY_SELECT_ALL) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        vte_terminal_select_all(VTE_TERMINAL(term->vte));
        return TRUE;
      }
#endif

#ifdef HOTKEY_EDIT_ENCODING
      if (HOTKEY_EDIT_ENCODING) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        const char *encoding_name = vte_terminal_get_encoding(VTE_TERMINAL(term->vte));
        GtkWidget *encoding_entry = gtk_entry_new();
        GtkWidget *dialog;
#if BUTTON_ORDER_BY_RCFILE
        if (button_order)
          dialog = gtk_dialog_new_with_buttons(encoding_name, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        else
#endif
          dialog = gtk_dialog_new_with_buttons(encoding_name, GTK_WINDOW(main_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
        gtk_entry_set_text(GTK_ENTRY(encoding_entry), encoding_name);
        gtk_entry_set_activates_default(GTK_ENTRY(encoding_entry), 1);
        gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), encoding_entry);
        gtk_widget_show_all(dialog);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
          vte_terminal_set_encoding(VTE_TERMINAL(term->vte), gtk_entry_get_text(GTK_ENTRY(encoding_entry)));
#if STATUS_BAR || defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
          gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, vte_terminal_get_encoding(VTE_TERMINAL(term->vte)));
#endif
        }
        gtk_widget_destroy(dialog);
        return TRUE;
      }
#endif

#ifdef HOTKEY_TAB_EDIT_LABEL
      if (HOTKEY_TAB_EDIT_LABEL) {
        do_edit_label();
        return TRUE;
      }
#endif

#if HOTKEY_TAB_GO_TO_NUMBER
      if (event->keyval == GDK_1) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
        return TRUE;
      }
      if ((event->keyval == GDK_2) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 1)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 1);
        return TRUE;
      }
      if ((event->keyval == GDK_3) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 2)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 2);
        return TRUE;
      }
      if ((event->keyval == GDK_4) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 3)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 3);
        return TRUE;
      }
      if ((event->keyval == GDK_5) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 4)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 4);
        return TRUE;
      }
      if ((event->keyval == GDK_6) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 5)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 5);
        return TRUE;
      }
      if ((event->keyval == GDK_7) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 6)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 6);
        return TRUE;
      }
      if ((event->keyval == GDK_8) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 7)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 7);
        return TRUE;
      }
      if ((event->keyval == GDK_9) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 8)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 8);
        return TRUE;
      }
      if ((event->keyval == GDK_0) && (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 9)) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 9);
        return TRUE;
      }
#endif

#ifdef HOTKEY_TOGGLE_BACKGROUND
      if (HOTKEY_TOGGLE_BACKGROUND) {
        do_toggle_bg();
        return TRUE;
      }
#endif

#ifdef HOTKEY_SATURATION_MORE
      if (HOTKEY_SATURATION_MORE) {
        saturation_level += 0.1;
        if (saturation_level > 1)
          saturation_level = 1;
        saturation_routine();
        return TRUE;
      }
#endif

#ifdef HOTKEY_SATURATION_LESS
      if (HOTKEY_SATURATION_LESS) {
        saturation_level -= 0.1;
        if (saturation_level < 0)
          saturation_level = 0;
        saturation_routine();
        return TRUE;
      }
#endif

#ifdef HOTKEY_RESET_TERMINAL
      if (HOTKEY_RESET_TERMINAL) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        vte_terminal_reset(VTE_TERMINAL(term->vte), 1, 0);
        return TRUE;
      }
#endif

#ifdef HOTKEY_RESET_AND_CLEAR
      if (HOTKEY_RESET_AND_CLEAR) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        vte_terminal_reset(VTE_TERMINAL(term->vte), 1, 1);
        return TRUE;
      }
#endif

#ifdef HOTKEY_COPY
      if (HOTKEY_COPY) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        vte_terminal_copy_clipboard(VTE_TERMINAL(term->vte));
        return TRUE;
      }
#endif

#ifdef HOTKEY_PASTE
      if (HOTKEY_PASTE) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        vte_terminal_paste_clipboard(VTE_TERMINAL(term->vte));
        return TRUE;
      }
#endif

#ifdef HOTKEY_FONT_BIGGER
      if (HOTKEY_FONT_BIGGER) {
        do_zoom_in();
        return TRUE;
      }
#endif

#ifdef HOTKEY_FONT_SMALLER
      if (HOTKEY_FONT_SMALLER) {
        do_zoom_out();
        return TRUE;
      }
#endif

#ifdef HOTKEY_FONT_DEFAULT_SIZE
      if (HOTKEY_FONT_DEFAULT_SIZE) {
        do_zoom_100();
        return TRUE;
      }
#endif

#ifdef HOTKEY_FONT_SELECT
      if (HOTKEY_FONT_SELECT) {
        do_select_font();
        return TRUE;
      }
#endif

#ifdef HOTKEY_TAB_FIRST
      if (HOTKEY_TAB_FIRST) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
        return TRUE;
      }
#endif

#ifdef HOTKEY_TAB_LAST
      if (HOTKEY_TAB_LAST) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) - 1);
        return TRUE;
      }
#endif

#ifdef HOTKEY_MIMIC_SCROLL_UP
      if (HOTKEY_MIMIC_SCROLL_UP) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        gtk_test_widget_click(term->vte, 4, 0);
        return TRUE;
      }
#endif

#ifdef HOTKEY_MIMIC_SCROLL_DOWN
      if (HOTKEY_MIMIC_SCROLL_DOWN) {
        GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
        gtk_test_widget_click(term->vte, 5, 0);
        return TRUE;
      }
#endif

#ifdef HOTKEY_TAB_PREVIOUS
      if (HOTKEY_TAB_PREVIOUS) {
        int index = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), index ? index - 1 : gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) - 1);
        return TRUE;
      }
#endif

#ifdef HOTKEY_TAB_NEXT
      if (HOTKEY_TAB_NEXT) {
        int index = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), (index == (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) - 1)) ? 0 : index + 1);
        return TRUE;
      }
#endif

#ifdef HOTKEY_TAB_ADD
      if (HOTKEY_TAB_ADD) {
        add_tab();
        return TRUE;
      }
#endif

#ifdef HOTKEY_TAB_REMOVE
      if (HOTKEY_TAB_REMOVE) {
        DEL_TAB(NULL, CLOSE_DIALOG);
        return TRUE;
      }
#endif

#if defined(HOTKEY_TOGGLE_HOTKEYS) || MENU_TOGGLE_HOTKEYS
    }
#endif

  }
  return FALSE;
}
#endif /* HOTKEY */

#if CLOSE_DIALOG
gboolean delete_event() {
  if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 1) {
    GtkWidget *dialog = make_close_dialog();
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_CLOSE) {
      gtk_widget_destroy(dialog);
#if !CLOSE_SAFELY
      gtk_main_quit();
#endif
    } else {
      gtk_widget_destroy(dialog);
      return TRUE;
    }
  }
#endif
#if !CLOSE_DIALOG
void delete_event() {
#endif
  int i = 0;
  for (i = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i > 0 ; i--) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), i - 1);
    DEL_TAB(NULL, 0);
  }
#if CLOSE_DIALOG
  return FALSE;
#endif
}

#if MENU_RESET_AND_CLEAR
void do_clear()
{
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  vte_terminal_reset(VTE_TERMINAL(term->vte), 1, 1);
}
#endif

#if MENU_COPY
void do_copy()
{
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  vte_terminal_copy_clipboard(VTE_TERMINAL(term->vte));
}
#endif

#ifdef MENU_MATCH_STRING_EXEC
void do_match_copy()
{
  gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), matched_url, -1);
  matched_url = NULL;
}
#endif

#ifdef MENU_MATCH_STRING_EXEC
void do_match_open()
{
  char new_window_str[256];
  g_snprintf(new_window_str, sizeof(new_window_str), "%s '%s' &", MENU_MATCH_STRING_EXEC, matched_url);
  system(new_window_str);
  matched_url = NULL;
}
#endif

#if MENU_OPEN_NEW_WINDOW
void do_new_window()
{
  system(PROGRAM_NAME " &");
}
#endif

#if MENU_PASTE
void do_paste()
{
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  vte_terminal_paste_clipboard(VTE_TERMINAL(term->vte));
}
#endif

#if MENU_RESET_TERMINAL
void do_reset()
{
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  vte_terminal_reset(VTE_TERMINAL(term->vte), 1, 0);
}
#endif

#if MENU_SELECT_ALL
void do_select_all()
{
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  vte_terminal_select_all(VTE_TERMINAL(term->vte));
}
#endif

#if MENU_TOGGLE_FULLSCREEN
void do_toggle_fullscreen()
{
  window_fullscreen_status ^= 1;
  window_fullscreen_status ? gtk_window_maximize(GTK_WINDOW(main_window)) : gtk_window_unmaximize(GTK_WINDOW(main_window));
}
#endif

#if MENU_TOGGLE_HOTKEYS
void do_toggle_hotkeys()
{
  hotkey_status ^= 1;
}
#endif

#if MENU
void set_encoding(GtkWidget *widget, void *data)
{
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  vte_terminal_set_encoding(VTE_TERMINAL(term->vte), (char*)data);
#if STATUS_BAR || defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, (char*)data);
#endif
}
#endif

void switch_page()
{
#if STATUS_BAR || defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR || WINDOW_TITLE_DYNAMIC
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
#endif
#if STATUS_BAR || defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, vte_terminal_get_encoding(VTE_TERMINAL(term->vte)));
#endif
#if TAB_SHOW_INFO_AT_TITLE
  int index = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
  if (index == 1)
    gtk_window_set_title(GTK_WINDOW(main_window), VTE_PROGRAM_NAME);
  else {
    char tabtitle[64];
    g_snprintf(tabtitle, sizeof(tabtitle), "%s - tab %d of %d", VTE_PROGRAM_NAME, gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) + 1, index);
    gtk_window_set_title(GTK_WINDOW(main_window), tabtitle);
  }
#endif
#if WINDOW_TITLE_DYNAMIC
  gtk_window_set_title(GTK_WINDOW(main_window), vte_terminal_get_window_title(VTE_TERMINAL(term->vte)));
#endif
}

#if defined(HOTKEY_TOGGLE_FULLSCREEN) || MENU_TOGGLE_FULLSCREEN || defined(VTE_FUNNY)
void window_state_event(GtkWidget *widget, GdkEventWindowState *event)
{
  window_fullscreen_status = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) ? 1 : 0;
}
#endif

int main(int argc, char **argv)
{
#if COMMAND_DOCK_MODE
int at_dock_mode = 0;
#endif

#if COMMAND_EXEC_PROGRAM || COMMAND_TAB_NUMBERS || defined(MENU_ENCODING_LIST)
  int i = 0;
#endif
#if COMMAND_AT_ROOT_WINDOW || COMMAND_DOCK_MODE || COMMAND_FULLSCREEN || COMMAND_LOGIN_SHELL || PROGRAM_WM_CLASS || COMMAND_SET_TITLE || COMMAND_FONT || COMMAND_GEOMETRY || COMMAND_SHOW_HELP || COMMAND_SHOW_OPTIONS || COMMAND_SHOW_VERSION || COMMAND_TAB_NUMBERS || defined(MENU_CUSTOM)
  int j = 0;
#endif

#if COMMAND_EXEC_PROGRAM
  g_snprintf(default_command, sizeof(default_command), "%s", DEFAULT_COMMAND);
  if (argc > 2) {
    for (i = 0 ; i < argc ; i++) {
      if (!strncmp(argv[i], "-e", 3)) {
        if (i < argc - 1) {
          g_snprintf(default_command, sizeof(default_command), "%s", argv[i + 1]);
#if COMMAND_LOGIN_SHELL || !VTE_FORK_CMD_OLD
          login_shell_flag = 2;
#endif
        }
        if (++i != argc)
#if VTE_FORK_CMD_OLD
          default_argv = &(argv[i]);
#endif
#if !VTE_FORK_CMD_OLD
          default_argv = &(argv[i - 1]);
        default_argv[0] = default_command;
#endif
        break;
      }
    }
  }
#endif

#if PROGRAM_WM_CLASS
  if (g_getenv("RESOURCE_NAME"))
    wm_class_name = (char*)g_getenv("RESOURCE_NAME");
#endif

#if COMMAND_AT_ROOT_WINDOW || COMMAND_DOCK_MODE || COMMAND_LOGIN_SHELL || PROGRAM_WM_CLASS || COMMAND_SET_TITLE || COMMAND_FONT || COMMAND_GEOMETRY || COMMAND_SHOW_HELP || COMMAND_SHOW_OPTIONS || COMMAND_SHOW_VERSION || COMMAND_TAB_NUMBERS
  j = 1;
  while ((j < argc) && strncmp(argv[j], "-e", 3)) {
#if PROGRAM_WM_CLASS
    if (argc > (j + 1) && !strncmp(argv[j], "--name", 7))
      wm_class_name = argv[j + 1];
    if (argc > (j + 1) && !strncmp(argv[j], "--class", 8))
      wm_class_class = argv[j + 1];
#endif

#if COMMAND_SET_TITLE
    if (argc > (j + 1) && (!strncmp(argv[j], "-title", 7) || !strncmp(argv[j], "-T", 3)))
      program_name = argv[j + 1];
#endif

#if COMMAND_FONT
    if (argc > (j + 1) && !strncmp(argv[j], "-fn", 4))
      command_font = argv[j + 1];
#endif

#if COMMAND_GEOMETRY
    if (argc > (j + 1) && !strncmp(argv[j], "-g", 3))
      command_geometry = argv[j + 1];
#endif

#if COMMAND_SHOW_VERSION
    if (!strncmp(argv[j], "-v", 3)) {
      printf("%s, version %s\n", PROGRAM_NAME, PROGRAM_VERSION);
      return 0;
    }
#endif

#if COMMAND_SHOW_OPTIONS
    if (!strncmp(argv[j], "-o", 3)) {
      system("showvte");
      return 0;
    }
#endif

#if COMMAND_SHOW_HELP
    if (!strncmp(argv[j], "-h", 3)) {
      printf("%s, version %s\n\nUsage:\n\t%s [options]\n\nOption%s:\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_NAME,
#if COMMAND_AT_ROOT_WINDOW || COMMAND_DOCK_MODE || COMMAND_EXEC_PROGRAM || COMMAND_FULLSCREEN || COMMAND_LOGIN_SHELL || COMMAND_SET_TITLE || COMMAND_FONT || COMMAND_GEOMETRY || COMMAND_SHOW_OPTIONS || COMMAND_SHOW_VERSION || COMMAND_TAB_NUMBERS
             "s");
#endif
#if !COMMAND_AT_ROOT_WINDOW && !COMMAND_DOCK_MODE && !COMMAND_EXEC_PROGRAM && !COMMAND_FULLSCREEN && !COMMAND_LOGIN_SHELL && !COMMAND_SET_TITLE && !COMMAND_FONT && !COMMAND_GEOMETRY && !COMMAND_SHOW_OPTIONS && !COMMAND_SHOW_VERSION && !COMMAND_TAB_NUMBERS
             "");
#endif
#if COMMAND_DOCK_MODE
      printf("\t-d                    \tstart %s as a dock\n", PROGRAM_NAME);
#endif
#if COMMAND_EXEC_PROGRAM
      printf("\t-e [program] [options]\tspecify the program to be run in %s\n", PROGRAM_NAME);
#endif
#if COMMAND_FULLSCREEN
      printf("\t-f                    \tstart %s in fullscreen mode\n", PROGRAM_NAME);
#endif
#if COMMAND_FONT
      printf("\t-fn \"[font] [size]\"   \tspecify font and font size\n");
#endif
#if COMMAND_GEOMETRY
      printf("\t-g +X+Y               \tspecify geometry\n");
#endif
      printf("\t-h                    \tshow this help\n");
#if COMMAND_LOGIN_SHELL
      printf("\t-ls                   \tuse login shell\n");
#endif
#if COMMAND_SHOW_OPTIONS
      printf("\t-o                    \tshow build-time configuration\n");
#endif
#if COMMAND_AT_ROOT_WINDOW
      printf("\t-r                    \tmake %s run in root window\n", PROGRAM_NAME);
#endif
#if COMMAND_SET_TITLE
      printf("\t-T [string]           \tspecify program title\n\t-title [string]       \tspecify program title\n");
#endif
#if COMMAND_SHOW_VERSION
      printf("\t-v                    \tshow version\n");
#endif
#if COMMAND_TAB_NUMBERS
      printf("\t-2 to -9              \tspecify the initial tab numbers\n");
#endif
      printf("\nGTK+ Options:\n\t--class [string]      \tspecify WM_CLASS class\n\t--name [string]       \tspecify WM_CLASS name\n");
#ifdef BACKGROUND_IMAGE
      printf("\nBackground image:\n\t$HOME/%s\n", BACKGROUND_IMAGE);
#endif
      printf("\n");
      return 0;
    }
#endif

#if COMMAND_LOGIN_SHELL
    if (!strncmp(argv[j], "-ls", 4))
      login_shell_flag |= 1;
#endif

#if COMMAND_AT_ROOT_WINDOW
    if (!strncmp(argv[j], "-r", 3))
      at_root_window = 1;
#endif

#if COMMAND_DOCK_MODE
    if (!strncmp(argv[j], "-d", 3))
      at_dock_mode = 1;
#endif

#if COMMAND_TAB_NUMBERS
    i = 1;
    if (!strncmp(argv[j], "-2", 3))
      i = 2;
    if (!strncmp(argv[j], "-3", 3))
      i = 3;
    if (!strncmp(argv[j], "-4", 3))
      i = 4;
    if (!strncmp(argv[j], "-5", 3))
      i = 5;
    if (!strncmp(argv[j], "-6", 3))
      i = 6;
    if (!strncmp(argv[j], "-7", 3))
      i = 7;
    if (!strncmp(argv[j], "-8", 3))
      i = 8;
    if (!strncmp(argv[j], "-9", 3))
      i = 9;
#endif

    j++;
  }
#endif /* COMMAND_AT_ROOT_WINDOW || COMMAND_DOCK_MODE || COMMAND_LOGIN_SHELL || PROGRAM_WM_CLASS || COMMAND_SET_TITLE || COMMAND_FONT || COMMAND_GEOMETRY || COMMAND_SHOW_HELP || COMMAND_SHOW_OPTIONS || COMMAND_SHOW_VERSION || COMMAND_TAB_NUMBERS */

#ifdef BACKGROUND_IMAGE
  g_snprintf(imgstr, sizeof(imgstr), "%s/%s", g_getenv("HOME"), BACKGROUND_IMAGE);
#endif

#ifdef PROGRAM_ICON
  g_snprintf(iconstr, sizeof(iconstr), "%s/%s", g_getenv("HOME"), PROGRAM_ICON);
#endif

#ifdef FONT
#if COMMAND_FONT
  if (command_font)
    g_snprintf(font_name, sizeof(font_name), "%s", command_font);
  else
#endif
    g_snprintf(font_name, sizeof(font_name), "%s", FONT);
  calculate_font();
  if (font_size < 1)
    font_size = 10;
  g_snprintf(font_str, sizeof(font_str), "%s %d", font_name, font_size);
#if defined(HOTKEY_FONT_DEFAULT_SIZE) || MENU_FONT_DEFAULT_SIZE
  font_size_default = font_size;
#endif
#endif

#if GTK_CHECK_VERSION(2,91,7)
#if defined(HOTKEY_MIMIC_SCROLL_UP) || defined(HOTKEY_MIMIC_SCROLL_DOWN) || defined(ONLY_ONE_MENU_ITEM)
  gdk_disable_multidevice();
#endif
#endif

  gtk_init(&argc, &argv);

#if defined(GTK3_CSS) && GTK_CHECK_VERSION(2,91,6)
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider, GTK3_CSS, -1, NULL);
  gtk_style_context_add_provider_for_screen(gdk_display_get_default_screen(gdk_display_get_default()), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(provider);
#endif

#ifdef TAB_LABEL
#if GTK_CHECK_VERSION(2,90,0)
  textdomain("gtk30");
#endif
#if !GTK_CHECK_VERSION(2,90,0)
  textdomain("gtk20");
#endif
#endif

#if defined(HOTKEY_SEARCH_STRING) || defined(HOTKEY_SEARCH_PREVIOUS) || defined(HOTKEY_SEARCH_NEXT)
  global_search_string = gtk_entry_new();
#endif

#if BUTTON_ORDER_BY_RCFILE
  g_object_get(gtk_settings_get_default(), "gtk-alternative-button-order", &button_order, NULL);
#endif

#if defined(BACKGROUND_TINT_COLOR) && BACKGROUND_EXIST
  gdk_color_parse(BACKGROUND_TINT_COLOR, &color_tint);
#endif

  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

#if GTK_CHECK_VERSION(2,91,1)
#if defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
  status_bar_resize_grip = gtk_window_resize_grip_is_visible(GTK_WINDOW(main_window));
  if (!status_bar_status)
    gtk_window_set_has_resize_grip(GTK_WINDOW(main_window), 0);
#endif
#if !STATUS_BAR && !defined(HOTKEY_TOGGLE_STATUS_BAR) && !MENU_TOGGLE_STATUS_BAR
  gtk_window_set_has_resize_grip(GTK_WINDOW(main_window), 0);
#endif
#endif

#if PROGRAM_WM_CLASS
  gtk_window_set_wmclass(GTK_WINDOW(main_window), wm_class_name, wm_class_class);
#endif

#ifdef SHOW_WINDOW_DECORATED
  gtk_window_set_decorated(GTK_WINDOW(main_window), SHOW_WINDOW_DECORATED);
#endif

#if COMMAND_SET_TITLE
  gtk_window_set_title(GTK_WINDOW(main_window), VTE_PROGRAM_NAME);
#endif

#ifdef PROGRAM_ICON
  if (!gtk_window_set_icon_from_file(GTK_WINDOW(main_window), iconstr, NULL))
    gtk_window_set_icon_from_file(GTK_WINDOW(main_window), ICON_DIR"/evilvte.png", NULL);
#endif

#if !defined(PROGRAM_ICON) && SHOW_WINDOW_ICON
  gtk_window_set_icon_from_file(GTK_WINDOW(main_window), ICON_DIR"/evilvte.png", NULL);
#endif

#if STATUS_BAR || defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
  vbox = (GtkWidget*)gtk_vbox_new(0, 0);
#if GTK_CHECK_VERSION(2,91,2) && defined(USE_GTK_GRID)
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);
#endif
  gtk_container_add(GTK_CONTAINER(main_window), vbox);
#endif

  notebook = gtk_notebook_new();

#ifdef SHOW_WINDOW_BORDER
  gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), SHOW_WINDOW_BORDER);
#endif

#if !GTK_CHECK_VERSION(2,90,0)
#ifdef TAB_BORDER
  gtk_notebook_set_tab_border(GTK_NOTEBOOK(notebook), TAB_BORDER);
#endif
#ifdef TAB_BORDER_VERTICAL
  gtk_notebook_set_tab_hborder(GTK_NOTEBOOK(notebook), TAB_BORDER_VERTICAL);
#endif
#ifdef TAB_BORDER_HORIZONTAL
  gtk_notebook_set_tab_vborder(GTK_NOTEBOOK(notebook), TAB_BORDER_HORIZONTAL);
#endif
#endif

#ifdef TABBAR
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), VTE_TABBAR);
#endif

#if TABBAR_SCROLLABLE
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TABBAR_SCROLLABLE);
#endif
#if TABBAR_MENU_SELECT_TAB
  gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
#endif
#ifdef TABBAR_PLACE
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), TABBAR_PLACE);
#endif

  gtk_widget_set_can_focus(notebook, 0);

#if INIT_OPACITY || BACKGROUND_OPACITY
  gtk_widget_set_colormap(main_window, gdk_screen_get_rgba_colormap(gtk_widget_get_screen(main_window)));
#endif

  add_tab();

#if COMMAND_EXEC_PROGRAM
  g_snprintf(default_command, sizeof(default_command), "%s", DEFAULT_COMMAND);
  default_argv = NULL;
#if COMMAND_LOGIN_SHELL || !VTE_FORK_CMD_OLD
  login_shell_flag = (login_shell_flag & 1) ? 1 : 0;
#endif
#endif

#if COMMAND_TAB_NUMBERS
  while (i > 1) {
    add_tab();
    i--;
  }
  GET_CURRENT_TAB(0);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
  gtk_window_set_focus(GTK_WINDOW(main_window), term->vte);
#endif

#if STATUS_BAR || defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
  gtk_box_pack_start(GTK_BOX(vbox), notebook, 1, 1, 0);
  statusbar = gtk_statusbar_new();
  GET_CURRENT_TAB(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, vte_terminal_get_encoding(VTE_TERMINAL(term->vte)));
  gtk_box_pack_start(GTK_BOX(vbox), statusbar, 0, 1, 0);
#endif
#if !STATUS_BAR && !defined(HOTKEY_TOGGLE_STATUS_BAR) && !MENU_TOGGLE_STATUS_BAR
  gtk_container_add(GTK_CONTAINER(main_window), notebook);
#endif

  g_signal_connect(main_window, "delete_event", G_CALLBACK(delete_event), NULL);

#if HOTKEY
  g_signal_connect(main_window, "key-press-event", G_CALLBACK(key_press_event), NULL);
#endif

#if BELL_URGENT
  g_signal_connect(main_window, "focus-in-event", G_CALLBACK(focus_in_event), NULL);
#endif

#if defined(HOTKEY_TOGGLE_FULLSCREEN) || MENU_TOGGLE_FULLSCREEN || defined(VTE_FUNNY)
  g_signal_connect(main_window, "window_state_event", G_CALLBACK(window_state_event), NULL);
#endif

  g_signal_connect_after(notebook, "switch-page", switch_page, NULL);

#if TAB_REORDERABLE && GTK_CHECK_VERSION(2,9,0)
  g_signal_connect(notebook, "page-reordered", switch_page, NULL);
#endif

#if COMMAND_AT_ROOT_WINDOW
  if (at_root_window) {
    gtk_window_set_keep_below(GTK_WINDOW(main_window), 1);
#if defined(HOTKEY_TOGGLE_DECORATED) || MENU_TOGGLE_DECORATED
    window_decorated_status = 0;
#endif
    gtk_window_set_decorated(GTK_WINDOW(main_window), 0);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(main_window), 1);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(main_window), 1);
  }
#endif

#ifdef PROGRAM_ALWAYS_ON_TOP
#if COMMAND_AT_ROOT_WINDOW
  if (!at_root_window)
#endif
    gtk_window_set_keep_above(GTK_WINDOW(main_window), always_on_top);
#endif

#if COMMAND_DOCK_MODE
  if (at_dock_mode)
    gtk_window_set_type_hint(GTK_WINDOW(main_window), GDK_WINDOW_TYPE_HINT_DOCK);
#endif

#if COMMAND_GEOMETRY
  if (command_geometry)
    gtk_window_parse_geometry(GTK_WINDOW(main_window), command_geometry);
#endif

#if VTE_CHECK_VERSION(0,27,1) && GTK_CHECK_VERSION(2,91,1)
  gtk_widget_style_get(term->vte, "inner-border", &inner_border, NULL);
#ifndef VTE_FUNNY
  gtk_window_set_default_geometry(GTK_WINDOW(main_window), VTE_COLUMNS * vte_terminal_get_char_width(VTE_TERMINAL(term->vte)) + INNER_BORDER_W, VTE_ROWS * vte_terminal_get_char_height(VTE_TERMINAL(term->vte)) + INNER_BORDER_H);
#endif
#ifdef VTE_FUNNY
#if COMMAND_TAB_NUMBERS
  for (i = 0 ; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) ; i++) {
    GET_CURRENT_TAB(i);
#endif
    gtk_widget_set_size_request(term->vte, VTE_COLUMNS * vte_terminal_get_char_width(VTE_TERMINAL(term->vte)) + INNER_BORDER_W, VTE_ROWS * vte_terminal_get_char_height(VTE_TERMINAL(term->vte)) + INNER_BORDER_H);
#if COMMAND_TAB_NUMBERS
  }
#endif
#endif
#endif

  gtk_widget_show_all(main_window);

#if defined(HOTKEY_TOGGLE_STATUS_BAR) || MENU_TOGGLE_STATUS_BAR
  if (!status_bar_status) {
    gtk_widget_hide(statusbar);
    VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
  }
#endif

#if defined(HOTKEY_TOGGLE_SCROLLBAR) || MENU_TOGGLE_SCROLLBAR
  if (!scrollbar_status) {
    hide_scrollbar();
    VTE_WINDOW_RESIZE(GTK_WINDOW(main_window), 1, 1);
  }
#endif

#if COMMAND_FULLSCREEN
  j = 1;
  while ((j < argc) && strncmp(argv[j], "-e", 3)) {
    if (!strncmp(argv[j], "-f", 3))
      gtk_window_maximize(GTK_WINDOW(main_window));
    j++;
  }
#endif

#if MENU
  menu = gtk_menu_new();
#endif

#ifdef MENU_CUSTOM
  for (j = 0 ; j < MENU_CUSTOM_SIZE ; j++)
#endif
  {
#if MENU_COPY
    if (!strncmp(menu_custom[j], "Copy", 5)) {
      menu_copy = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_copy);
      g_signal_connect(menu_copy, "activate", do_copy, NULL);
      menu_item_success++;
    }
#endif

#if MENU_PASTE
    if (!strncmp(menu_custom[j], "Paste", 6)) {
      menu_paste = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_paste);
      g_signal_connect(menu_paste, "activate", do_paste, NULL);
      menu_item_success++;
    }
#endif

#if MENU_SELECT_ALL
    if (!strncmp(menu_custom[j], "Select all", 11)) {
      GtkWidget *menu_select_all = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_ALL, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_select_all);
      g_signal_connect(menu_select_all, "activate", do_select_all, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TAB_ADD
    if (!strncmp(menu_custom[j], "Add tab", 8)) {
      GtkWidget *menu_add_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_add_tab);
      g_signal_connect(menu_add_tab, "activate", add_tab, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TAB_REMOVE
    if (!strncmp(menu_custom[j], "Remove tab", 11)) {
      GtkWidget *menu_close_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_close_tab);
      g_signal_connect(menu_close_tab, "activate", G_CALLBACK(del_tab), (int*)CLOSE_DIALOG);
      menu_item_success++;
    }
#endif

#if MENU_TAB_EDIT_LABEL
    if (!strncmp(menu_custom[j], "Edit label", 11)) {
      GtkWidget *menu_edit_label = gtk_image_menu_item_new_from_stock(GTK_STOCK_EDIT, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_edit_label);
      g_signal_connect(menu_edit_label, "activate", do_edit_label, NULL);
      menu_item_success++;
    }
#endif

#if MENU_OPEN_NEW_WINDOW
    if (!strncmp(menu_custom[j], "New window", 11)) {
      GtkWidget *menu_new_window = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_new_window);
      g_signal_connect(menu_new_window, "activate", do_new_window, NULL);
      menu_item_success++;
    }
#endif

#if MENU_QUIT
    if (!strncmp(menu_custom[j], "Quit", 5)) {
      GtkWidget *menu_quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_quit);
      g_signal_connect(menu_quit, "activate", G_CALLBACK(delete_event), NULL);
      menu_item_success++;
    }
#endif

#if MENU_FONT_BIGGER
    if (!strncmp(menu_custom[j], "Zoom in", 8)) {
      GtkWidget *menu_zoom_in = gtk_image_menu_item_new_from_stock(GTK_STOCK_ZOOM_IN, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_zoom_in);
      g_signal_connect(menu_zoom_in, "activate", do_zoom_in, NULL);
      menu_item_success++;
    }
#endif

#if MENU_FONT_SMALLER
    if (!strncmp(menu_custom[j], "Zoom out", 9)) {
      GtkWidget *menu_zoom_out = gtk_image_menu_item_new_from_stock(GTK_STOCK_ZOOM_OUT, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_zoom_out);
      g_signal_connect(menu_zoom_out, "activate", do_zoom_out, NULL);
      menu_item_success++;
    }
#endif

#if MENU_FONT_DEFAULT_SIZE
    if (!strncmp(menu_custom[j], "Zoom default", 13)) {
      menu_zoom_100 = gtk_image_menu_item_new_from_stock(GTK_STOCK_ZOOM_100, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_zoom_100);
      g_signal_connect(menu_zoom_100, "activate", do_zoom_100, NULL);
      menu_item_success++;
    }
#endif

#if MENU_RESET_TERMINAL
    if (!strncmp(menu_custom[j], "Reset terminal", 15)) {
      GtkWidget *menu_reset = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_reset);
      g_signal_connect(menu_reset, "activate", do_reset, NULL);
      menu_item_success++;
    }
#endif

#if MENU_RESET_AND_CLEAR
    if (!strncmp(menu_custom[j], "Reset and clear", 16)) {
      GtkWidget *menu_clear = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_clear);
      g_signal_connect(menu_clear, "activate", do_clear, NULL);
      menu_item_success++;
    }
#endif

#if MENU_FONT_SELECT
    if (!strncmp(menu_custom[j], "Select font", 12)) {
      GtkWidget *menu_font = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_FONT, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_font);
      g_signal_connect(menu_font, "activate", do_select_font, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_ON_TOP
    if (!strncmp(menu_custom[j], "Toggle always on top", 21)) {
      GtkWidget *menu_toggle_on_top = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_ON_TOP);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_on_top), gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_on_top);
      g_signal_connect(menu_toggle_on_top, "activate", do_always_on_top, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_BACKGROUND
    if (!strncmp(menu_custom[j], "Toggle background", 18)) {
      GtkWidget *menu_toggle_bg = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_BG);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_bg), gtk_image_new_from_stock(GTK_STOCK_DND_MULTIPLE, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_bg);
      g_signal_connect(menu_toggle_bg, "activate", do_toggle_bg, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_HOTKEYS
    if (!strncmp(menu_custom[j], "Toggle hotkeys locking", 23)) {
      GtkWidget *menu_toggle_hotkeys = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_HOTKEYS);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_hotkeys), gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_hotkeys);
      g_signal_connect(menu_toggle_hotkeys, "activate", do_toggle_hotkeys, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_SCROLLBAR && defined(SCROLLBAR)
    if (!strncmp(menu_custom[j], "Toggle scrollbar", 17)) {
      GtkWidget *menu_toggle_scrollbar = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_SCROLLBAR);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_scrollbar), gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_scrollbar);
      g_signal_connect(menu_toggle_scrollbar, "activate", do_toggle_scrollbar, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_STATUS_BAR
    if (!strncmp(menu_custom[j], "Toggle status bar", 18)) {
      GtkWidget *menu_toggle_status_bar = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_STATUS_BAR);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_status_bar), gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_status_bar);
      g_signal_connect(menu_toggle_status_bar, "activate", do_toggle_status_bar, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_TABBAR
    if (!strncmp(menu_custom[j], "Toggle tabbar", 14)) {
      GtkWidget *menu_toggle_tabbar = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_TABBAR);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_tabbar), gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_tabbar);
      g_signal_connect(menu_toggle_tabbar, "activate", do_toggle_tabbar, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_DECORATED
    if (!strncmp(menu_custom[j], "Toggle window decorated", 24)) {
      GtkWidget *menu_toggle_decorated = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_DECORATED);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_decorated), gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_decorated);
      g_signal_connect(menu_toggle_decorated, "activate", do_toggle_decorated, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_FULLSCREEN
    if (!strncmp(menu_custom[j], "Toggle fullscreen", 18)) {
      GtkWidget *menu_toggle_fullscreen = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_FULLSCREEN);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_fullscreen), gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_fullscreen);
      g_signal_connect(menu_toggle_fullscreen, "activate", do_toggle_fullscreen, NULL);
      menu_item_success++;
    }
#endif

#if MENU_TOGGLE_ANTI_ALIAS
    if (!strncmp(menu_custom[j], "Toggle anti-alias", 18)) {
      GtkWidget *menu_toggle_antialias = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_TOGGLE_ANTI_ALIAS);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_toggle_antialias), gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_toggle_antialias);
      g_signal_connect(menu_toggle_antialias, "activate", do_toggle_antialias, NULL);
      menu_item_success++;
    }
#endif

#if MENU_CHANGE_SATURATION
    if (!strncmp(menu_custom[j], "Adjust saturation", 18)) {
      GtkWidget *menu_change_saturation = gtk_image_menu_item_new_with_mnemonic(LABEL_MENU_SATURATION);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_change_saturation), gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_change_saturation);
      g_signal_connect(menu_change_saturation, "activate", do_menu_saturation, NULL);
      menu_item_success++;
    }
#endif

#if MENU_COLOR_BACKGROUND
    if (!strncmp(menu_custom[j], "Background tint", 16)) {
      GtkWidget *menu_change_tint = gtk_image_menu_item_new_with_mnemonic(LABEL_DIALOG_BACKGROUND_TINT);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_change_tint), gtk_image_new_from_stock(GTK_STOCK_SELECT_COLOR, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_change_tint);
      g_signal_connect(menu_change_tint, "activate", do_menu_tint_color, NULL);
      menu_item_success++;
    }
#endif

#if MENU_SEPARATOR
    if (!strncmp(menu_custom[j], "Separator", 10))
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
#endif

#ifdef MENU_CUSTOM
    if (!strncmp(menu_custom[j], "Encoding list", 14))
#endif
#ifdef MENU_ENCODING_LIST
    {
      GtkWidget *encoding_item[MENU_ENCODING_LIST_SIZE];
      for (i = 0 ; i < MENU_ENCODING_LIST_SIZE ; i++) {
#if MENU_DEFAULT_ENCODING
        if (!strncmp(encoding[i], "Default Encoding", 17)) {
          encoding[i] = (char*)vte_terminal_get_encoding(VTE_TERMINAL(term->vte));
#ifndef MENU_CUSTOM
          encoding_item[i] = gtk_menu_item_new_with_mnemonic(LABEL_DEFAULT_ENCODING);
#endif
#ifdef MENU_CUSTOM
          encoding_item[i] = gtk_image_menu_item_new_with_mnemonic(LABEL_DEFAULT_ENCODING);
#endif
        } else
#endif
#ifndef MENU_CUSTOM
          encoding_item[i] = gtk_menu_item_new_with_label(encoding[i]);
#endif
#ifdef MENU_CUSTOM
          encoding_item[i] = gtk_image_menu_item_new_with_label(encoding[i]);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(encoding_item[i]), gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
        menu_item_success++;
#endif
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), encoding_item[i]);
        g_signal_connect(encoding_item[i], "activate", G_CALLBACK(set_encoding), encoding[i]);
      }
    }
#endif /* MENU_ENCODING_LIST */
#if !defined(MENU_ENCODING_LIST) && MENU
    {
#ifndef MENU_CUSTOM
      encoding_item = gtk_menu_item_new_with_label("UTF-8");
#endif
#ifdef MENU_CUSTOM
      encoding_item = gtk_image_menu_item_new_with_label("UTF-8");
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(encoding_item), gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
      menu_item_success++;
#endif
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), encoding_item);
      g_signal_connect(encoding_item, "activate", G_CALLBACK(set_encoding), "UTF-8");
    }
#endif

#ifdef MENU_CUSTOM
    if (!strncmp(menu_custom[j], "Input method", 13))
#endif
#if MENU_INPUT_METHOD
    {
      vte_terminal_im_append_menuitems(VTE_TERMINAL(term->vte), GTK_MENU_SHELL(menu));
#ifdef MENU_CUSTOM
      menu_item_success++;
#endif
    }
#endif

#if SUBMENU_ENCODING_LIST
    if (!strncmp(menu_custom[j], "Submenu encoding list", 22)) {
      GtkWidget *subitem_enc = gtk_image_menu_item_new_with_mnemonic(LABEL_SUBMENU_ENCODING);
      GtkWidget *submenu_enc = gtk_menu_new();
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), subitem_enc);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem_enc), gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(subitem_enc), submenu_enc);
#ifdef MENU_ENCODING_LIST
      GtkWidget *encoding_sub[MENU_ENCODING_LIST_SIZE];
      for (i = 0 ; i < MENU_ENCODING_LIST_SIZE ; i++) {
#if MENU_DEFAULT_ENCODING
        if (!strncmp(encoding[i], "Default Encoding", 17)) {
          encoding[i] = (char*)vte_terminal_get_encoding(VTE_TERMINAL(term->vte));
          encoding_sub[i] = gtk_menu_item_new_with_mnemonic(LABEL_DEFAULT_ENCODING);
        } else
#endif
          encoding_sub[i] = gtk_menu_item_new_with_label(encoding[i]);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu_enc), encoding_sub[i]);
        g_signal_connect(encoding_sub[i], "activate", G_CALLBACK(set_encoding), encoding[i]);
      }
#endif
#ifndef MENU_ENCODING_LIST
      encoding_item = gtk_menu_item_new_with_label("UTF-8");
      gtk_menu_shell_append(GTK_MENU_SHELL(submenu_enc), encoding_item);
      g_signal_connect(encoding_item, "activate", G_CALLBACK(set_encoding), "UTF-8");
#endif
      menu_item_success++;
    }
#endif

#if SUBMENU_INPUT_METHOD
    if (!strncmp(menu_custom[j], "Submenu input method", 21)) {
      GtkWidget *subitem_ime = gtk_image_menu_item_new_with_mnemonic(LABEL_SUBMENU_IME);
      GtkWidget *submenu_ime = gtk_menu_new();
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), subitem_ime);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem_ime), gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_MENU));
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(subitem_ime), submenu_ime);
      vte_terminal_im_append_menuitems(VTE_TERMINAL(term->vte), GTK_MENU_SHELL(submenu_ime));
      menu_item_success++;
    }
#endif
  }

#if MENU
#ifdef MENU_MATCH_STRING_EXEC
#ifdef MENU_CUSTOM
  if (menu_item_success > 0) {
#endif
    match_item = gtk_separator_menu_item_new();
    gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), match_item);
#ifdef MENU_CUSTOM
  }
#endif
  match_copy = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), match_copy);
  g_signal_connect(match_copy, "activate", do_match_copy, NULL);
  match_open = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), match_open);
  g_signal_connect(match_open, "activate", do_match_open, NULL);
#endif
  gtk_widget_show_all(menu);
#endif

#ifdef CLIPBOARD_CLEAR
  GtkClipboard *clipboard = gtk_clipboard_get_for_display(gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text(clipboard, "", 0);
  gtk_clipboard_clear(clipboard);
#endif

  gtk_main();
  return 0;
}
