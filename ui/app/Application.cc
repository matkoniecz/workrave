// Copyright (C) 2001 - 2021 Rob Caelers & Raymond Penners
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
#  include "config.h"
#endif

#include "Application.hh"

#include "Menus.hh"
#include "commonui/nls.h"
#include "core/IBreak.hh"
#include "core/ICore.hh"
#include "dbus/IDBus.hh"
#include "debug.hh"
#include "session/System.hh"
#include "ui/GUIConfig.hh"
#include "ui/IBreakWindow.hh"
#include "ui/IPlugin.hh"
#include "ui/Locale.hh"
#include "ui/SoundTheme.hh"
#include "ui/Text.hh"
#include "utils/Exception.hh"
#include "utils/Platform.hh"
#include "utils/Paths.hh"

#ifdef HAVE_DBUS
#  include "GenericDBusApplet.hh"
#endif

using namespace workrave;
using namespace workrave::utils;

Application::Application(int argc, char **argv, std::shared_ptr<IToolkit> toolkit)
  : toolkit(toolkit)
{
  TRACE_ENTER("Application:Application");

  this->argc = argc;
  this->argv = argv;

  TRACE_EXIT();
}

Application::~Application()
{
  TRACE_ENTER("Application:~Application");

  toolkit->deinit();

  core.reset();

  TRACE_EXIT();
}

void
Application::main()
{
  TRACE_ENTER("Application::main");

  System::init();
  srand((unsigned int)time(nullptr));

  init_core();
  init_nls();
  init_sound_player();
  init_dbus();

  menu_model = std::make_shared<MenuModel>();
  menus = std::make_shared<Menus>(shared_from_this());

  init_platform_pre();

  toolkit->init(shared_from_this());

  init_operation_mode_warning();
  init_updater();

  init_platform_post();

#if defined(HAVE_DBUS)
  register_plugin(std::make_shared<GenericDBusApplet>(shared_from_this()));
#endif

  connect(toolkit->signal_timer(), this, [this] { on_timer(); });
  connect(toolkit->signal_session_idle_changed(), this, [this](auto idle) { on_idle_changed(idle); });
  connect(toolkit->signal_main_window_closed(), this, [this] { on_main_window_closed(); });
  connect(toolkit->signal_status_icon_activated(), this, [this] { on_status_icon_activate(); });

  on_timer();

  init_ready = true;
  for (auto p: plugins)
    {
      p->init();
    }

  TRACE_MSG("Initialized. Entering event loop.");
  toolkit->run();
  TRACE_MSG("loop ended");

  System::clear();
  core->get_configurator()->save();

  TRACE_EXIT();
}

void
Application::register_plugin(std::shared_ptr<IPlugin> plugin)
{
  plugins.push_back(plugin);
  if (init_ready)
    {
      plugin->init();
    }
}

void
Application::init_nls()
{
#if defined(ENABLE_NLS)
  std::string language = GUIConfig::locale()();
  if (!language.empty())
    {
      Platform::setenv("LANGUAGE", language.c_str(), 1);
    }

  const char *locale_dir = nullptr;

#  if defined(PLATFORM_OS_WINDOWS)
  std::filesystem::path dir = Paths::get_application_directory();
  dir /= "lib";
  dir /= "locale";
  locale_dir = dir.u8string().c_str();
#  elif defined(PLATFORM_OS_MACOS)
  std::filesystem::path dir = Paths::get_application_directory();
  dir /= "Resources";
  dir /= "locale";
  locale_dir = dir.u8string().c_str();
#  else
  locale_dir = GNOMELOCALEDIR;
#  endif

#  ifdef HAVE_SETLOCALE
  setlocale(LC_ALL, "");
#  endif

#  if defined(PLATFORM_OS_WINDOWS)
  bindtextdomain("gtk20", locale_dir);
  bindtextdomain("iso_3166", locale_dir);
  bindtextdomain("iso_639", locale_dir);
  bindtextdomain("glib20", locale_dir);
  bind_textdomain_codeset("gk20", "UTF-8");
  bind_textdomain_codeset("glib20", "UTF-8");
  bind_textdomain_codeset("iso_3166", "UTF-8");
  bind_textdomain_codeset("iso_639", "UTF-8");

  GUIConfig::locale().connect(this, [&](const std::string &locale) {
    Locale::set_locale(locale);
    // TODO: menus->locale_changed();
  });
#  endif

  bindtextdomain(GETTEXT_PACKAGE, locale_dir);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

#endif
}

void
Application::init_core()
{
  core = CoreFactory::create();
#if defined(HAVE_CORE_NEXT)
  core->init(this, toolkit->get_display_name());
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      IBreak::Ptr b = core->get_break(BreakId(i));
      connect(b->signal_break_event(), this, std::bind(&Application::on_break_event, this, BreakId(i), std::placeholders::_1));
    }
#else
  core->init(argc, argv, this, toolkit->get_display_name());
  core->set_core_events_listener(this);
#endif
  GUIConfig::init(shared_from_this());
}

void
Application::init_dbus()
{
  auto dbus = get_core()->get_dbus();

  if (dbus->is_available())
    {
      if (dbus->is_running("org.workrave.Workrave"))
        {
          // TODO:
          // Gtk::MessageDialog dialog(_("Workrave failed to start"), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
          // dialog.set_secondary_text(_("Is Workrave already running?"));
          // dialog.show();
          // dialog.run();
          exit(1);
        }

      try
        {
          dbus->register_object_path("/org/workrave/Workrave/UI");
          dbus->connect("/org/workrave/Workrave/UI", "org.workrave.ControlInterface", menus.get());
        }
      catch (workrave::dbus::DBusException &)
        {
        }
    }

#ifdef HAVE_DBUS
  try
    {
      extern void init_DBusGUI(workrave::dbus::IDBus::Ptr dbus);
      init_DBusGUI(dbus);
    }
  catch (workrave::dbus::DBusException &)
    {
    }
#endif
}

void
Application::init_operation_mode_warning()
{
  OperationMode mode = core->get_operation_mode();
  if (mode != OperationMode::Normal)
    {
      toolkit->create_oneshot_timer(5000, [this]() { on_operation_mode_warning_timer(); });
    }
}

void
Application::init_updater()
{
  // TODO:
  // updater = workrave::updater::std::make_shared<Updater>("http://snapshots.workrave.org/appcast/");
  // if (updater)
  //   {
  //     updater->check_for_updates();
  //   }
}

void
Application::init_sound_player()
{
  TRACE_ENTER("GUI:init_sound_player");
  try
    {
      // Tell pulseaudio were are playing sound events
      Platform::setenv("PULSE_PROP_media.role", "event", 1);

      sound_theme = std::make_shared<SoundTheme>(core->get_configurator());
      sound_theme->init();
    }
  catch (workrave::utils::Exception &)
    {
      TRACE_MSG("No sound");
    }
  TRACE_EXIT();
}

#if defined(HAVE_CORE_NEXT)
void
Application::on_break_event(BreakId break_id, BreakEvent event)
{
  TRACE_ENTER_MSG("Application::on_break_event", break_id << " " << static_cast<std::underlying_type<BreakEvent>::type>(event));

  struct EventMap
  {
    BreakId id;
    BreakEvent break_event;
    SoundEvent sound_event;
  } event_mappings[] = {
    {BREAK_ID_MICRO_BREAK, BreakEvent::ShowPrelude, SoundEvent::BreakPrelude},
    {BREAK_ID_MICRO_BREAK, BreakEvent::BreakIgnored, SoundEvent::BreakIgnored},
    {BREAK_ID_MICRO_BREAK, BreakEvent::ShowBreak, SoundEvent::MicroBreakStarted},
    {BREAK_ID_MICRO_BREAK, BreakEvent::BreakTaken, SoundEvent::MicroBreakEnded},
    {BREAK_ID_REST_BREAK, BreakEvent::ShowPrelude, SoundEvent::BreakPrelude},
    {BREAK_ID_REST_BREAK, BreakEvent::BreakIgnored, SoundEvent::BreakIgnored},
    {BREAK_ID_REST_BREAK, BreakEvent::ShowBreak, SoundEvent::RestBreakStarted},
    {BREAK_ID_REST_BREAK, BreakEvent::BreakTaken, SoundEvent::RestBreakEnded},
    {BREAK_ID_DAILY_LIMIT, BreakEvent::ShowPrelude, SoundEvent::BreakPrelude},
    {BREAK_ID_DAILY_LIMIT, BreakEvent::BreakIgnored, SoundEvent::BreakIgnored},
    {BREAK_ID_DAILY_LIMIT, BreakEvent::ShowBreak, SoundEvent::MicroBreakEnded},
  };

  for (auto &event_mapping: event_mappings)
    {
      if (event_mapping.id == break_id && event_mapping.break_event == event)
        {
          bool mute = false;
          SoundEvent snd = event_mapping.sound_event;
          TRACE_MSG("play " << static_cast<std::underlying_type<BreakEvent>::type>(event));

          mute = sound_theme->sound_mute()();
          if (mute)
            {
              muted = true;
            }
          TRACE_MSG("Mute after playback " << mute);
          sound_theme->play_sound(snd, mute);
        }
    }
}
#else
void
Application::core_event_notify(const CoreEvent event)
{
  TRACE_ENTER_MSG("Application::core_event_sound_notify", event);

  if (event >= CORE_EVENT_SOUND_FIRST && event <= CORE_EVENT_SOUND_LAST)
    {
      bool mute = false;
      SoundEvent snd = (SoundEvent)((int)event - CORE_EVENT_SOUND_FIRST);
      TRACE_MSG("play " << event);

      if (event == CORE_EVENT_SOUND_REST_BREAK_STARTED || event == CORE_EVENT_SOUND_DAILY_LIMIT)
        {
          bool mute = sound_theme->sound_mute()();
          if (mute)
            {
              muted = true;
            }
        }
      TRACE_MSG("Mute after playback " << mute);
      sound_theme->play_sound(snd, mute);
    }

  if (event == CORE_EVENT_MONITOR_FAILURE)
    {
      std::string msg = _("Workrave could not monitor your keyboard and mouse activity.\n");

      toolkit->show_notification("failed_monitor", "Workrave", msg, []() {});
      toolkit->terminate();
    }
  TRACE_EXIT();
}
#endif

void
Application::create_prelude_window(BreakId break_id)
{
  TRACE_ENTER_MSG("Application::create_prelude_window", break_id);
  hide_break_window();
  active_break_id = break_id;

  for (int i = 0; i < toolkit->get_head_count(); i++)
    {
      prelude_windows.push_back(toolkit->create_prelude_window(i, break_id));
    }

  TRACE_EXIT();
}

void
Application::create_break_window(BreakId break_id, workrave::utils::Flags<BreakHint> break_hint)
{
  TRACE_ENTER_MSG("Application::create_break_window", break_id << " " << break_hint);
  hide_break_window();

  BreakFlags break_flags = BREAK_FLAGS_NONE;
  bool ignorable = GUIConfig::break_ignorable(break_id)();
  bool skippable = GUIConfig::break_skippable(break_id)();

  if (break_hint & BreakHint::UserInitiated)
    {
      break_flags = (BREAK_FLAGS_POSTPONABLE | BREAK_FLAGS_USER_INITIATED);

      if (skippable)
        {
          break_flags |= BREAK_FLAGS_SKIPPABLE;
        }
    }
  else
    {
      if (ignorable)
        {
          break_flags |= BREAK_FLAGS_POSTPONABLE;
        }

      if (skippable)
        {
          break_flags |= BREAK_FLAGS_SKIPPABLE;
        }
    }

  if (break_hint & BreakHint::NaturalBreak)
    {
      break_flags |= (BREAK_FLAGS_NO_EXERCISES | BREAK_FLAGS_NATURAL | BREAK_FLAGS_POSTPONABLE);
    }

  active_break_id = break_id;

  for (int i = 0; i < toolkit->get_head_count(); i++)
    {
      IBreakWindow::Ptr break_window = toolkit->create_break_window(i, break_id, break_flags);

      break_windows.push_back(break_window);
      break_window->init();

      break_flags |= BREAK_FLAGS_NO_EXERCISES;
    }

  TRACE_EXIT();
}

void
Application::hide_break_window()
{
  TRACE_ENTER("Application::hide_break_window");
  active_break_id = BREAK_ID_NONE;

  for (auto &window: prelude_windows)
    {
      window->stop();
    }

  for (auto &window: break_windows)
    {
      window->stop();
    }

  break_windows.clear();
  prelude_windows.clear();

  toolkit->get_locker()->unlock();
  TRACE_EXIT();
}

void
Application::show_break_window()
{
  TRACE_ENTER("Application::show_break_window");

  for (auto &window: prelude_windows)
    {
      window->start();
    }

  for (auto &window: break_windows)
    {
      window->start();
    }

  if (GUIConfig::block_mode()() != GUIConfig::BLOCK_MODE_NONE)
    {
      toolkit->get_locker()->lock();
    }

  TRACE_EXIT();
}

void
Application::refresh_break_window()
{
  for (auto &window: prelude_windows)
    {
      window->refresh();
    }

  for (auto &window: break_windows)
    {
      window->refresh();
    }
}

void
Application::set_break_progress(int value, int max_value)
{
  for (auto &window: prelude_windows)
    {
      window->set_progress(value, max_value);
    }

  for (auto &window: break_windows)
    {
      window->set_progress(value, max_value);
    }
}

void
Application::set_prelude_stage(PreludeStage stage)
{
  for (auto &window: prelude_windows)
    {
      window->set_stage(stage);
    }
}

void
Application::set_prelude_progress_text(PreludeProgressText text)
{
  for (auto &window: prelude_windows)
    {
      window->set_progress_text(text);
    }
}

void
Application::on_timer()
{
  std::string tip = get_timers_tooltip();

  core->heartbeat();

  // TODO: tip changed.
  // applet_control->set_tooltip(tip);
  toolkit->show_tooltip(tip);

  if (!break_windows.empty() && muted)
    {
      bool user_active = core->is_user_active();

      if (user_active)
        {
          sound_theme->restore_mute();
          muted = false;
        }
    }
}

void
Application::on_main_window_closed()
{
  TRACE_ENTER("Application::on_main_window_closed");
  bool closewarn = GUIConfig::closewarn_enabled()();
  TRACE_MSG(closewarn);
  if (closewarn && !closewarn_shown)
    {
      toolkit->show_notification("closewarn",
                                 "Workrave",
                                 _("Workrave is still running. "
                                   "You can access Workrave by clicking on the white sheep icon. "
                                   "Click on this balloon to disable this message"),
                                 []() { GUIConfig::closewarn_enabled().set(false); });
      closewarn_shown = true;
    }

  TRACE_EXIT();
}

void
Application::on_operation_mode_warning_timer()
{
  OperationMode mode = core->get_operation_mode();
  if (mode == OperationMode::Suspended)
    {
      toolkit->show_notification("operation_mode",
                                 "Workrave",
                                 _("Workrave is in suspended mode.\n"
                                   "Mouse and keyboard activity will not be monitored."),
                                 []() {});
    }
  else if (mode == OperationMode::Quiet)
    {
      toolkit->show_notification("operation_mode",
                                 "Workrave",
                                 _("Workrave is in quiet mode. "
                                   "No break windows will appear."),
                                 []() {});
    }
}

std::string
Application::get_timers_tooltip()
{
  // FIXME: duplicate
  const char *labels[] = {_("Micro-break"), _("Rest break"), _("Daily limit")};
  std::string tip = "";

  OperationMode mode = core->get_operation_mode();
  switch (mode)
    {
    case OperationMode::Suspended:
      tip = std::string(_("Mode: ")) + _("Suspended");
      break;

    case OperationMode::Quiet:
      tip = std::string(_("Mode: ")) + _("Quiet");
      break;

    case OperationMode::Normal:
    default:
#if !defined(PLATFORM_OS_WINDOWS)
      // Win32 tip is limited in length
      tip = "Workrave";
#endif
      break;
    }

  for (int count = 0; count < BREAK_ID_SIZEOF; count++)
    {
      auto b = core->get_break(BreakId(count));
      bool on = b->is_enabled();

      if (b != nullptr && on)
        {
          // Collect some data.
          int64_t maxActiveTime = b->get_limit();
          int64_t activeTime = b->get_elapsed_time();
          std::string text;

          // Set the text
          if (b->is_limit_enabled() && maxActiveTime != 0)
            {
              text = Text::time_to_string(maxActiveTime - activeTime);
            }
          else
            {
              text = Text::time_to_string(activeTime);
            }

          if (!tip.empty())
            {
              tip += "\n";
            }

          tip += labels[count];
          tip += ": " + text;
        }
    }

  return tip;
}

void
Application::on_status_icon_activate()
{
  toolkit->show_window(IToolkit::WindowType::Main);
}

void
Application::on_idle_changed(bool new_idle)
{
  TRACE_ENTER_MSG("Application::on_idle_changed", new_idle);

  bool auto_natural = GUIConfig::break_auto_natural(BREAK_ID_REST_BREAK)();
  auto core = get_core();

  if (core->get_usage_mode() == UsageMode::Reading)
    {
      core->force_idle();
    }

  if (new_idle && !is_idle)
    {
      TRACE_MSG("Now idle");
      auto rest_break = core->get_break(BREAK_ID_REST_BREAK);

      taking = rest_break->is_taking();
      TRACE_MSG("taking " << taking);
      if (!taking)
        {
          core->set_operation_mode_override(OperationMode::Suspended, "screensaver");
        }
    }
  else if (!new_idle && is_idle && !taking)
    {
      TRACE_MSG("No longer idle");
      core->remove_operation_mode_override("screensaver");

      if (auto_natural)
        {
          TRACE_MSG("Automatic natural break enabled");

          auto rest_break = core->get_break(BREAK_ID_REST_BREAK);

          if (core->get_operation_mode() == OperationMode::Normal && rest_break->get_elapsed_idle_time() < rest_break->get_auto_reset()
              && rest_break->is_enabled() && !rest_break->is_taking())
            {
              bool overdue = (rest_break->get_limit() < rest_break->get_elapsed_time());

              if (overdue)
                {
                  core->force_break(BREAK_ID_REST_BREAK, BreakHint::Normal);
                }
              else
                {
                  core->force_break(BREAK_ID_REST_BREAK, BreakHint::NaturalBreak);
                }
            }
        }
    }

  is_idle = new_idle;
  TRACE_EXIT();
}
