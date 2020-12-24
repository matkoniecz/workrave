// Copyright (C) 2001 - 2013 Rob Caelers & Raymond Penners
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef PLATFORM_OS_WINDOWS
#include <gdk/gdkwin32.h>
#include <shellapi.h>
#undef __out
#undef __in
#endif

#include "commonui/nls.h"
#include "debug.hh"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "MainWindow.hh"

#include <list>
#include <gtkmm.h>

#include "commonui/Backend.hh"
#include "config/IConfigurator.hh"

#include "TimerBoxGtkView.hh"
#include "commonui/TimerBoxControl.hh"
#include "GUI.hh"
#include "GtkUtil.hh"
#include "Menus.hh"

#ifdef PLATFORM_OS_WINDOWS
const char *WIN32_MAIN_CLASS_NAME = "Workrave";
const UINT MYWM_TRAY_MESSAGE = WM_USER +0x100;
#endif

using namespace std;
using namespace workrave::utils;

//! Constructor.
/*!
 *  \param gui the main GUI entry point.
 *  \param control Interface to the controller.
 */
MainWindow::MainWindow() :
  enabled(true),
  can_close(true),
  timer_box_control(nullptr),
  timer_box_view(nullptr),
  window_location(-1, -1),
  window_head_location(-1, -1),
  window_relocated_location(-1, -1)
{
#ifdef PLATFORM_OS_UNIX
  leader = nullptr;
#endif
#ifdef PLATFORM_OS_WINDOWS
  show_retry_count = 0;
#endif
}


//! Destructor.
MainWindow::~MainWindow()
{
  TRACE_ENTER("MainWindow::~MainWindow");

  if (visible_connection.connected())
    {
      visible_connection.disconnect();
    }

#ifdef PLATFORM_OS_WINDOWS
  if (timeout_connection.connected())
    {
      timeout_connection.disconnect();
    }
#endif

  delete timer_box_control;
#ifdef PLATFORM_OS_WINDOWS
  win32_exit();
#endif
#ifdef PLATFORM_OS_UNIX
  delete leader;
#endif

  TRACE_EXIT();
}

bool
MainWindow::is_visible() const
{
#if defined(PLATFORM_OS_WINDOWS)
  const GtkWidget *window = Gtk::Widget::gobj();
  GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(window));
  HWND hwnd = (HWND) GDK_WINDOW_HWND(gdk_window);
  return IsWindowVisible(hwnd);
#else
  return get_visible();
#endif
}


void
MainWindow::toggle_window()
{
  TRACE_ENTER("MainWindow::toggle_window");

  bool visible = is_visible();
  if (visible)
    {
      close_window();
    }
  else
    {
      open_window();
    }
  TRACE_EXIT();
}


//! Opens the main window.
void
MainWindow::open_window()
{
  TRACE_ENTER("MainWindow::open_window");
  if (timer_box_view->get_visible_count() > 0)
    {
#ifdef PLATFORM_OS_WINDOWS
      win32_show(true);
      show_all();
#else
      show_all();
      deiconify();
#endif

      int x, y, head;
      set_position(Gtk::WIN_POS_NONE);
      set_gravity(Gdk::GRAVITY_STATIC);
      get_start_position(x, y, head);

      GtkRequisition min_size;
      GtkRequisition natural_size;
      get_preferred_size(min_size, natural_size);

      GUI::get_instance()->bound_head(x, y, min_size.width, min_size.height, head);

      GUI::get_instance()->map_from_head(x, y, head);
      TRACE_MSG("moving to " << x << " " << y);
      move(x, y);

      bool always_on_top = GUIConfig::main_window_always_on_top()();
      WindowHints::set_always_on_top(this, always_on_top);
      GUIConfig::timerbox_enabled("main_window").set(true);
    }
  TRACE_EXIT();
}



//! Closes the main window.
void
MainWindow::close_window()
{
  TRACE_ENTER("MainWindow::close_window");
#ifdef PLATFORM_OS_WINDOWS
  win32_show(false);
#elif defined(PLATFORM_OS_MACOS)
  hide();
#else
  if (can_close)
    {
      hide();
    }
  else
    {
      iconify();
    }
#endif

  GUIConfig::timerbox_enabled("main_window").set(false);
  TRACE_EXIT();
}


void
MainWindow::set_can_close(bool can_close)
{
  TRACE_ENTER_MSG("MainWindow::set_can_close", can_close);
  this->can_close = can_close;

  TRACE_MSG(enabled);
  if (!enabled)
    {
      if (can_close)
        {
          TRACE_MSG("hide");
          hide();
        }
      else
        {
          TRACE_MSG("iconify");
          iconify();
          show_all();
        }
    }
  TRACE_EXIT();
}


//! Updates the main window.
void
MainWindow::update()
{
  timer_box_control->update();
}


//! Initializes the main window.
void
MainWindow::init()
{
  TRACE_ENTER("MainWindow::init");

  set_border_width(2);
  set_resizable(false);

  std::list<Glib::RefPtr<Gdk::Pixbuf> > icons;

  const char *icon_files[] =
    {
#ifndef PLATFORM_OS_WIN32
       // This causes a crash on windows
       "scalable" G_DIR_SEPARATOR_S "workrave.svg",
#endif
       "16x16" G_DIR_SEPARATOR_S "workrave.png",
       "24x24" G_DIR_SEPARATOR_S "workrave.png",
       "32x32" G_DIR_SEPARATOR_S "workrave.png",
       "48x48" G_DIR_SEPARATOR_S "workrave.png",
       "64x64" G_DIR_SEPARATOR_S "workrave.png",
       "96x96" G_DIR_SEPARATOR_S "workrave.png",
       "128x128" G_DIR_SEPARATOR_S "workrave.png",
    };

  for (unsigned int i = 0; i < sizeof(icon_files) / sizeof(char *); i++)
    {
      Glib::RefPtr<Gdk::Pixbuf> pixbuf = GtkUtil::create_pixbuf(icon_files[i]);
      if (pixbuf)
      {
        icons.push_back(pixbuf);
      }
    }

  Glib::ListHandle<Glib::RefPtr<Gdk::Pixbuf> > icon_list(icons);
  Gtk::Window::set_default_icon_list(icon_list);
  //Gtk::Window::set_default_icon_name("workrave");

  enabled = GUIConfig::timerbox_enabled("main_window")();

  timer_box_view = Gtk::manage(new TimerBoxGtkView(Menus::MENU_MAINWINDOW));
  timer_box_control = new TimerBoxControl("main_window", timer_box_view);
  timer_box_view->set_geometry(ORIENTATION_LEFT, -1);
  timer_box_control->update();

  Gtk::EventBox *eventbox = new Gtk::EventBox;
  eventbox->set_visible_window(false);
  eventbox->set_events(eventbox->get_events() | Gdk::BUTTON_PRESS_MASK);
#ifndef PLATFORM_OS_MACOS
  // No popup menu on OS X
  eventbox->signal_button_press_event().connect(sigc::mem_fun(*this,
                                                              &MainWindow::on_timer_view_button_press_event));

#endif
  eventbox->add(*timer_box_view);
  add(*eventbox);

  // Necessary for popup menu
  realize_if_needed();

  Glib::RefPtr<Gdk::Window> window = get_window();

  // Window decorators
  window->set_decorations(Gdk::DECOR_BORDER
                          |Gdk::DECOR_TITLE
                          |Gdk::DECOR_MENU);
  // This used to be W32 only:
  //   window->set_functions(Gdk::FUNC_CLOSE|Gdk::FUNC_MOVE);

  // (end window decorators)

#ifdef PLATFORM_OS_UNIX
  // HACK. this sets a different group leader in the WM_HINTS....
  // Without this hack, metacity makes ALL windows on-top.
  leader = new Gtk::Window(Gtk::WINDOW_POPUP);
  gtk_widget_realize(GTK_WIDGET(leader->gobj()));
  Glib::RefPtr<Gdk::Window> leader_window = leader->get_window();
  window->set_group(leader_window);
#endif

  stick();
  setup();

#ifdef PLATFORM_OS_WINDOWS

  win32_init();
  set_gravity(Gdk::GRAVITY_STATIC);
  set_position(Gtk::WIN_POS_NONE);

#ifdef HAVE_NOT_PROPER_SIZED_MAIN_WINDOW_ON_STARTUP
  // This is the proper code, see hacked code below.
  if (!enabled)
    {
      move(-1024, 0);
      show_all();
      win32_show(false);
      move_to_start_position();
    }
  else
    {
      move_to_start_position();
      show_all();
    }
#else // Hack deprecated: Since GTK+ 2.10 no longer necessary

  // Hack: since GTK+ 2.2.4 the window is too wide, it incorporates room
  // for the "_ [ ] [X]" buttons somehow. This hack fixes just that.
  move(-1024, 0); // Move off-screen to reduce wide->narrow flickering
  show_all();
  HWND hwnd = (HWND) GDK_WINDOW_HWND(window->gobj());
  SetWindowPos(hwnd, NULL, 0, 0, 1, 1,
               SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOACTIVATE
               |SWP_NOOWNERZORDER|SWP_NOMOVE);
  if (! enabled)
    {
      win32_show(false);
    }
  move_to_start_position();
  // (end of hack)
#endif

#else
  set_gravity(Gdk::GRAVITY_STATIC);
  set_position(Gtk::WIN_POS_NONE);
  show_all();
  move_to_start_position();

  if (!enabled) //  || get_start_in_tray())
    {
#ifndef PLATFORM_OS_MACOS
      iconify();
#endif
      close_window();
    }
#endif
  setup();
  set_title("Workrave");

  connections.add(GUIConfig::key_timerbox("main_window").connect([&] () {
        setup();
      }));

  visible_connection = property_visible().signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_visibility_changed));

  TRACE_EXIT();
}

//! Setup configuration settings.
void
MainWindow::setup()
{
  TRACE_ENTER("MainWindow::setup");

  bool new_enabled = GUIConfig::timerbox_enabled("main_window")();
  bool always_on_top = GUIConfig::main_window_always_on_top()();

  TRACE_MSG("can_close " << new_enabled);
  TRACE_MSG("enabled " << new_enabled);
  TRACE_MSG("on top " << always_on_top);

  if (enabled != new_enabled)
    {
      enabled = new_enabled;
      if (enabled)
        {
          open_window();
        }
      else
        {
          close_window();
        }
    }

  bool visible = is_visible();

  if (visible)
    {
      WindowHints::set_always_on_top(this, always_on_top);
    }

  if (visible && always_on_top)
    {
      raise();
    }

  TRACE_EXIT();
}


//! User has closed the main window.
bool
MainWindow::on_delete_event(GdkEventAny *)
{
  TRACE_ENTER("MainWindow::on_delete_event");

#if defined(PLATFORM_OS_WINDOWS)
  win32_show(false);
  closed_signal.emit();
  GUIConfig::timerbox_enabled("main_window").set(false);
#elif defined(PLATFORM_OS_MACOS)
  close_window();
  GUIConfig::timerbox_enabled("main_window").set(false);
#else
  if (can_close)
    {
      close_window();
      GUIConfig::timerbox_enabled("main_window").set(false);
    }
  else
    {
      IGUI *gui = GUI::get_instance();
      gui->terminate();
    }
#endif

  TRACE_EXIT();
  return true;
}

bool
MainWindow::on_timer_view_button_press_event(GdkEventButton *event)
{
  TRACE_ENTER("MainWindow::on_timer_view_button_press_event");
  bool ret = false;

  (void) event;

  if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
    {
      IGUI *gui = GUI::get_instance();
      Menus *menus = gui->get_menus();
      menus->popup(Menus::MENU_MAINWINDOW, event->button, event->time);
      ret = true;
    }

  TRACE_EXIT();
  return ret;
}

#ifdef PLATFORM_OS_WINDOWS
void
MainWindow::win32_show(bool b)
{
  TRACE_ENTER_MSG("MainWindow::win32_show", b);
  bool retry = false;

  // Gtk's hide() seems to quit the program.
  GtkWidget *window = Gtk::Widget::gobj();
  GdkWindow *gdk_window = gtk_widget_get_window(window);
  HWND hwnd = (HWND) GDK_WINDOW_HWND(gdk_window);
  ShowWindow(hwnd, b ? SW_SHOWNORMAL : SW_HIDE);
  visibility_changed_signal.emit();

  if (b)
    {
      present();

      if (hwnd != GetForegroundWindow())
        {
          if (show_retry_count == 0)
            {
              show_retry_count = 20;
            }
          else
            {
              show_retry_count--;
            }

          TRACE_MSG("2 " << show_retry_count);
          retry = true;
        }
    }

  if (retry)
    {
      if (show_retry_count > 0)
        {
          timeout_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &MainWindow::win32_show_retry), 50);
        }
    }
  else
    {
      show_retry_count = 0;
    }
  TRACE_EXIT();
}

bool
MainWindow::win32_show_retry()
{
  TRACE_ENTER("MainWindow::win32_show_retry");
  if (show_retry_count > 0)
  {
     TRACE_MSG("retry");
     win32_show(true);
  }
  TRACE_EXIT();
  return false;
}



void
MainWindow::win32_init()
{
  TRACE_ENTER("MainWindow::win32_init");

  win32_hinstance = (HINSTANCE) GetModuleHandle(NULL);

  WNDCLASSEXA wclass =
    {
      sizeof(WNDCLASSEXA),
      0,
      win32_window_proc,
      0,
      0,
      win32_hinstance,
      NULL,
      NULL,
      NULL,
      NULL,
      WIN32_MAIN_CLASS_NAME,
      NULL
    };
  /* ATOM atom = */ RegisterClassExA(&wclass);

  win32_main_hwnd = CreateWindowExA(WS_EX_TOOLWINDOW,
                                   WIN32_MAIN_CLASS_NAME,
                                   "Workrave",
                                   WS_OVERLAPPED,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   (HWND)NULL,
                                   (HMENU)NULL,
                                   win32_hinstance,
                                   (LPSTR)NULL);
  ShowWindow(win32_main_hwnd, SW_HIDE);

  // User data
  SetWindowLongPtr(win32_main_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  // Reassign ownership
  GtkWidget *window = Gtk::Widget::gobj();
  GdkWindow *gdk_window = gtk_widget_get_window(window);
  HWND hwnd = (HWND) GDK_WINDOW_HWND(gdk_window);
  SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win32_main_hwnd));

  TRACE_EXIT();
}

void
MainWindow::win32_exit()
{
  DestroyWindow(win32_main_hwnd);
  UnregisterClassA(WIN32_MAIN_CLASS_NAME, GetModuleHandle(NULL));
}

#endif

void
MainWindow::get_start_position(int &x, int &y, int &head)
{
  TRACE_ENTER("MainWindow::get_start_position");
  x = GUIConfig::main_window_x()();
  y = GUIConfig::main_window_y()();
  head = GUIConfig::main_window_head()();
  if (head < 0)
    {
      head = 0;
    }
  TRACE_MSG(x << " " << y << " " << head);
  TRACE_EXIT();
}


void
MainWindow::set_start_position(int x, int y, int head)
{
  TRACE_ENTER_MSG("MainWindow::set_start_position", x << " " << y << " " << head);
  GUIConfig::main_window_x().set(x);
  GUIConfig::main_window_y().set(y);
  GUIConfig::main_window_head().set(head);
  TRACE_EXIT();
}



void
MainWindow::move_to_start_position()
{
  TRACE_ENTER("MainWindow::move_to_start_position");

  int x, y, head;
  get_start_position(x, y, head);

  GtkRequisition min_size;
  GtkRequisition natural_size;
  get_preferred_size(min_size, natural_size);

  GUI::get_instance()->bound_head(x, y, min_size.width, min_size.height, head);

  window_head_location.set_x(x);
  window_head_location.set_y(y);

  GUI::get_instance()->map_from_head(x, y, head);

  TRACE_MSG("Main window size " << min_size.width << " " << min_size.height);

  window_location.set_x(x);
  window_location.set_y(y);
  window_relocated_location.set_x(x);
  window_relocated_location.set_y(y);
  TRACE_MSG("moving to " << x << " " << y);

  move(x, y);
}

bool
MainWindow::on_configure_event(GdkEventConfigure *event)
{
  TRACE_ENTER_MSG("MainWindow::on_configure_event",
                  event->x << " " << event->y);
  locate_window(event);
  bool ret =  Widget::on_configure_event(event);
  TRACE_EXIT();
  return ret;
}

void
MainWindow::locate_window(GdkEventConfigure *event)
{
  TRACE_ENTER("MainWindow::locate_window");
  int x, y;
  int width, height;

  (void) event;

  Glib::RefPtr<Gdk::Window> window = get_window();
  if ((window->get_state() & (Gdk::WINDOW_STATE_ICONIFIED | Gdk::WINDOW_STATE_WITHDRAWN)) != 0)
    {
      TRACE_EXIT();
      return;
    }

#ifndef PLATFORM_OS_WINDOWS
  // Returns bogus results on windows...sometime.
  if (event != nullptr)
    {
      x = event->x;
      y = event->y;
      width = event->width;
      height = event->height;
    }
  else
#endif
    {
      (void) event;

      get_position(x, y);

      GtkRequisition min_size;
      GtkRequisition natural_size;
      get_preferred_size(min_size, natural_size);

      width = min_size.width;
      height = min_size.height;
    }

  TRACE_MSG("main window = " << x << " " << y);

  if (x <= 0 && y <= 0)
    {
      TRACE_EXIT();
      return;
    }

  if (x != window_relocated_location.get_x() ||
      y != window_relocated_location.get_y())
    {
      window_location.set_x(x);
      window_location.set_y(y);
      window_relocated_location.set_x(x);
      window_relocated_location.set_y(y);

      int head = GUI::get_instance()->map_to_head(x, y);
      TRACE_MSG("main window head = " << x << " " << y << " " << head);

      bool rc = GUI::get_instance()->bound_head(x, y, width, height, head);
      TRACE_MSG("main window bounded = " << x << " " << y);

      window_head_location.set_x(x);
      window_head_location.set_y(y);
      set_start_position(x, y, head);

      if (rc)
        {
          move_to_start_position();
        }
    }
  TRACE_EXIT();
}


void
MainWindow::relocate_window(int width, int height)
{
  TRACE_ENTER_MSG("MainWindow::relocate_window", width << " " << height);
  int x = window_location.get_x();
  int y = window_location.get_y();

  if (x <= 0 || y <= 0)
    {
      TRACE_MSG("invalid " << x << " " << y);
    }
  else if (x <= width && y <= height)
    {
      TRACE_MSG(x << " " << y);
      TRACE_MSG("fits, moving to");
      set_position(Gtk::WIN_POS_NONE);
      move(x, y);
    }
  else
    {
      TRACE_MSG("move to differt head");
      x = window_head_location.get_x();
      y = window_head_location.get_y();

      IGUI *gui = GUI::get_instance();
      int num_heads = gui->get_number_of_heads();
      for (int i = 0; i < num_heads; i++)
        {
          GtkRequisition min_size;
          GtkRequisition natural_size;
          get_preferred_size(min_size, natural_size);

          GUI::get_instance()->bound_head(x, y, min_size.width, min_size.height, i);

          gui->map_from_head(x, y, i);
          break;
        }

      if (x < 0)
        {
          x = 0;
        }
      if (y < 0)
        {
          y = 0;
        }

      TRACE_MSG("moving to " << x << " " << y);
      window_relocated_location.set_x(x);
      window_relocated_location.set_y(y);

      set_position(Gtk::WIN_POS_NONE);
      move(x, y);
    }

  TRACE_EXIT();
}

#ifdef PLATFORM_OS_WINDOWS

LRESULT CALLBACK
MainWindow::win32_window_proc(HWND hwnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam)
{
  TRACE_ENTER_MSG("MainWindow::win32_window_proc", uMsg << " " << wParam);
  TRACE_EXIT();
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif


void
MainWindow::on_visibility_changed()
{
  TRACE_ENTER("MainWindow::on_visibility_changed");
  TRACE_MSG(is_visible());
  visibility_changed_signal.emit();
  TRACE_EXIT();
}

sigc::signal<void> &
MainWindow::signal_closed()
{
  return closed_signal;
}

sigc::signal<void> &
MainWindow::signal_visibility_changed()
{
  return visibility_changed_signal;
}
