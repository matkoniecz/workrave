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

#ifndef MENUS_HH
#define MENUS_HH

#include <memory>
#include <string>

#include "core/ICore.hh"
#include "utils/Signals.hh"

#include "ui/IToolkit.hh"
#include "ui/IApplication.hh"
#include "ui/MenuModel.hh"

class Menus : public workrave::utils::Trackable
{
public:
  typedef std::shared_ptr<Menus> Ptr;

  Menus(std::shared_ptr<IApplication> app);

  using sv = std::string_view;
  static constexpr std::string_view PREFERENCES = sv("workrave.preferences");
  static constexpr std::string_view EXERCISES = sv("workrave.exercises");
  static constexpr std::string_view REST_BREAK = sv("workrave.restbreak");
  static constexpr std::string_view MODE_MENU = sv("workrave.mode_menu");
  static constexpr std::string_view MODE = sv("workrave.mode");
  static constexpr std::string_view MODE_NORMAL = sv("workrave.mode_normal");
  static constexpr std::string_view MODE_QUIET = sv("workrave.mode_quiet");
  static constexpr std::string_view MODE_SUSPENDED = sv("workrave.mode_suspended");
  static constexpr std::string_view STATISTICS = sv("workrave.statistics");
  static constexpr std::string_view ABOUT = sv("workrave.about");
  static constexpr std::string_view MODE_READING = sv("workrave.mode_reading");
  static constexpr std::string_view OPEN = sv("workrave.open");
  static constexpr std::string_view QUIT = sv("workrave.quit");

private:
  void init();
  void set_operation_mode(workrave::OperationMode m);
  void set_usage_mode(workrave::UsageMode m);

  void on_menu_open_main_window();
  void on_menu_restbreak_now();
  void on_menu_about();
  void on_menu_quit();
  void on_menu_preferences();
  void on_menu_exercises();
  void on_menu_statistics();
  void on_menu_normal();
  void on_menu_suspend();
  void on_menu_quiet();
  void on_menu_reading();
  void on_menu_reading(bool on);
  void on_operation_mode_changed(const workrave::OperationMode m);
  void on_usage_mode_changed(const workrave::UsageMode m);

private:
  std::shared_ptr<IApplication> app;
  std::shared_ptr<IToolkit> toolkit;
  std::shared_ptr<workrave::ICore> core;

  MenuModel::Ptr menu_model;
  menus::RadioGroupNode::Ptr mode_group;
  menus::RadioNode::Ptr quiet_item;
  menus::RadioNode::Ptr suspended_item;
  menus::RadioNode::Ptr normal_item;
  menus::ToggleNode::Ptr reading_item;

  // TODO: DBUS stubs, refactor
  friend class org_workrave_ControlInterface_Stub;
};

#endif // MENUS_HH
