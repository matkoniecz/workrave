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

#ifndef APPLICATION_HH
#define APPLICATION_HH

#include <list>
#include <vector>
#include <string>
#include <memory>

#ifdef HAVE_QT
#  include <QCoreApplication>
#endif

#include "Menus.hh"
#include "core/IApp.hh"
#include "core/ICore.hh"
#include "ui/IApplication.hh"
#include "ui/IBreakWindow.hh"
#include "ui/IPreludeWindow.hh"
#include "ui/IToolkit.hh"
#include "ui/SoundTheme.hh"
#include "updater/Updater.hh"
#include "utils/Signals.hh"

class Application
  : public IApplication
  , public workrave::IApp
#if !defined(HAVE_CORE_NEXT)
  , public workrave::ICoreEventListener
#endif
  , public workrave::utils::Trackable
{
public:
  Application(int argc, char **argv, std::shared_ptr<IToolkit> toolkit);
  ~Application() override;

  auto get_sound_theme() const -> SoundTheme::Ptr override;
  auto get_menu_model() const -> MenuModel::Ptr override;
  auto get_core() const -> workrave::ICore::Ptr override;
  auto get_toolkit() const -> IToolkit::Ptr override;

  void register_plugin(std::shared_ptr<IPlugin> plugin) override;

  void main();

  virtual void init_platform_pre() = 0;
  virtual void init_platform_post() = 0;

  // IApp methods
  void create_prelude_window(workrave::BreakId break_id) override;
  void create_break_window(workrave::BreakId break_id, workrave::utils::Flags<workrave::BreakHint> break_hint) override;
  void hide_break_window() override;
  void show_break_window() override;
  void refresh_break_window() override;
  void set_break_progress(int value, int max_value) override;
  void set_prelude_stage(PreludeStage stage) override;
  void set_prelude_progress_text(PreludeProgressText text) override;

#if !defined(HAVE_CORE_NEXT)
  void core_event_notify(const workrave::CoreEvent event) override;
#endif

private:
  auto get_timers_tooltip() -> std::string;
  void on_timer();
  void init_nls();
  void init_core();
  void init_sound_player();
  void init_dbus();
  void init_operation_mode_warning();
  void init_updater();

  void on_status_icon_activate();
  void on_main_window_closed();
  void on_idle_changed(bool new_idle);
  void on_operation_mode_warning_timer();

#if defined(HAVE_CORE_NEXT)
  void on_break_event(workrave::BreakId break_id, workrave::BreakEvent event);
#endif

private:
  std::shared_ptr<IToolkit> toolkit;
  std::shared_ptr<workrave::ICore> core;
  std::shared_ptr<Menus> menus;
  std::shared_ptr<MenuModel> menu_model;
  std::shared_ptr<workrave::updater::Updater> updater;
  std::shared_ptr<SoundTheme> sound_theme;
  int argc{0};
  char **argv{nullptr};
  std::vector<IBreakWindow::Ptr> break_windows;
  std::vector<IPreludeWindow::Ptr> prelude_windows;
  workrave::BreakId active_break_id{workrave::BREAK_ID_NONE};
  std::list<std::shared_ptr<IPlugin>> plugins;
  bool init_ready{false};
  bool muted{false};
  bool closewarn_shown{false};
  bool is_idle{false};
  bool taking{false};
};

inline auto
Application::get_sound_theme() const -> SoundTheme::Ptr
{
  return sound_theme;
}

inline auto
Application::get_menu_model() const -> MenuModel::Ptr
{
  return menu_model;
}

inline auto
Application::get_core() const -> workrave::ICore::Ptr
{
  return core;
}

inline auto
Application::get_toolkit() const -> IToolkit::Ptr
{
  return toolkit;
}

#endif // APPLICATION_HH
