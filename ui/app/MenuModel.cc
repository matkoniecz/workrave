// Copyright (C) 2013 - 2021 Rob Caelers <robc@krandor.nl>
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

#include <iostream>
#include <utility>

#include "ui/MenuModel.hh"

MenuModel::MenuModel()
{
  root = std::make_shared<menus::SubMenuNode>();
}

auto
MenuModel::get_root() const -> menus::SubMenuNode::Ptr
{
  return root;
}

void
MenuModel::update()
{
  update_signal();
}

auto
MenuModel::signal_update() -> boost::signals2::signal<void()> &
{
  return update_signal;
}

menus::Node::Node()
  : id("workrave:root")
{
}

menus::Node::Node(std::string_view id, std::string text, Activated activated)
  : id(id)
  , text(std::move(text))
  , activated(std::move(activated))
{
}

auto
menus::Node::get_id() const -> std::string
{
  return id;
}

auto
menus::Node::get_text() const -> std::string
{
  return text;
}

auto
menus::Node::get_text_no_accel() const -> std::string
{
  auto ret = text;
  ret.erase(std::remove(ret.begin(), ret.end(), '_'), ret.end());
  return ret;
}

auto
menus::Node::is_visible() const -> bool
{
  return visible;
}

void
menus::Node::set_text(std::string text)
{
  if (this->text != text)
    {
      this->text = text;
    }
}

void
menus::Node::set_visible(bool visible)
{
  if (this->visible != visible)
    {
      this->visible = visible;
      changed_signal();
    }
}

auto
menus::Node::signal_changed() -> boost::signals2::signal<void()> &
{
  return changed_signal;
}

void
menus::Node::activate()
{
  activated();
}

auto
menus::SubMenuNode::create(std::string_view id, std::string text) -> menus::SubMenuNode::Ptr
{
  return std::make_shared<menus::SubMenuNode>(id, text);
}

menus::SubMenuNode::SubMenuNode(std::string_view id, std::string text)
  : Node(id, text)
{
}

void
menus::SubMenuNode::add(menus::Node::Ptr submenu, menus::Node::Ptr before)
{
  auto pos = children.end();
  if (before)
    {
      pos = std::find(children.begin(), children.end(), before);
    }

  children.insert(pos, submenu);
}

void
menus::SubMenuNode::remove(menus::Node::Ptr submenu)
{
  children.remove(submenu);
}

auto
menus::SubMenuNode::get_children() const -> std::list<menus::Node::Ptr>
{
  return children;
}

auto
menus::ActionNode::create(std::string_view id, std::string text, Activated activated) -> menus::ActionNode::Ptr
{
  return std::make_shared<menus::ActionNode>(id, text, activated);
}

menus::ActionNode::ActionNode(std::string_view id, std::string text, Activated activated)
  : Node(id, text, activated)
{
}

auto
menus::ToggleNode::create(std::string_view id, std::string text, Activated activated) -> menus::ToggleNode::Ptr
{
  return std::make_shared<menus::ToggleNode>(id, text, activated);
}

menus::ToggleNode::ToggleNode(std::string_view id, std::string text, Activated activated)
  : Node(id, text, activated)
{
}

auto
menus::ToggleNode::is_checked() const -> bool
{
  return checked;
}

void
menus::ToggleNode::set_checked(bool checked)
{
  if (this->checked != checked)
    {
      this->checked = checked;
      changed_signal();
    }
}

void
menus::ToggleNode::activate()
{
  set_checked(!is_checked());
  if (activated)
    {
      activated();
    }
}

void
menus::ToggleNode::activate(bool state)
{
  set_checked(state);
  if (activated)
    {
      activated();
    }
}

auto
menus::RadioNode::create(std::shared_ptr<RadioGroupNode> group, std::string_view id, std::string text, int value, Activated activated)
  -> menus::RadioNode::Ptr
{
  return std::make_shared<menus::RadioNode>(group, id, text, value, activated);
}

menus::RadioNode::RadioNode(std::shared_ptr<RadioGroupNode> group, std::string_view id, std::string text, int value, Activated activated)
  : Node(id, text, activated)
  , value(value)
  , group(group)
{
}

auto
menus::RadioNode::get_value() const -> int
{
  return value;
}

void
menus::RadioNode::set_value(int value)
{
  if (this->value != value)
    {
      this->value = value;
      changed_signal();
    }
}

auto
menus::RadioNode::is_checked() const -> bool
{
  return checked;
}

void
menus::RadioNode::set_checked(bool checked)
{
  if (this->checked != checked)
    {
      this->checked = checked;

      if (checked)
        {
          std::shared_ptr<RadioGroupNode> rg = group.lock();
          if (rg)
            {
              rg->select(get_id());
            }
        }
      changed_signal();
    }
}

auto
menus::RadioNode::get_group_id() const -> std::string
{
  std::shared_ptr<RadioGroupNode> g = group.lock();
  if (g)
    {
      return g->get_id();
    }

  return "";
}

void
menus::RadioNode::activate()
{
  set_checked(true);
  if (activated)
    {
      activated();
    }
}

auto
menus::RadioGroupNode::create(std::string_view id, std::string text, Activated activated) -> menus::RadioGroupNode::Ptr
{
  return std::make_shared<menus::RadioGroupNode>(id, text, activated);
}

menus::RadioGroupNode::RadioGroupNode(std::string_view id, std::string text, Activated activated)
  : Node(id, text, activated)
{
}

void
menus::RadioGroupNode::add(menus::RadioNode::Ptr submenu, menus::RadioNode::Ptr before)
{
  auto pos = children.end();
  if (before)
    {
      pos = std::find(children.begin(), children.end(), before);
    }

  children.insert(pos, submenu);
}

void
menus::RadioGroupNode::remove(menus::RadioNode::Ptr submenu)
{
  children.remove(submenu);
}

auto
menus::RadioGroupNode::get_children() const -> std::list<menus::RadioNode::Ptr>
{
  return children;
}

auto
menus::RadioGroupNode::get_selected_node() const -> menus::RadioNode::Ptr
{
  auto i = std::find_if(children.begin(), children.end(), [this](auto n) { return n->get_id() == selected_id; });
  if (i != children.end())
    {
      return (*i);
    }
  return menus::RadioNode::Ptr();
}

auto
menus::RadioGroupNode::get_selected_value() const -> int
{
  auto node = get_selected_node();
  if (node)
    {
      return node->get_value();
    }

  return 0;
}

auto
menus::RadioGroupNode::get_selected_id() const -> std::string
{
  return selected_id;
}

void
menus::RadioGroupNode::select(std::string id)
{
  if (selected_id == id)
    {
      return;
    }
  selected_id = id;

  for (auto &node: children)
    {
      node->set_checked(node->get_id() == id);
    }
  changed_signal();
}

void
menus::RadioGroupNode::select(int value)
{
  bool changed{false};
  for (auto &node: children)
    {
      if (node->get_value() == value)
        {
          if (!node->is_checked())
            {
              selected_id = node->get_id();
              node->set_checked(true);
              changed = true;
            }
        }
      else
        {
          if (node->is_checked())
            {
              node->set_checked(false);
              changed = true;
            }
        }
    }
  if (changed)
    {
      changed_signal();
    }
}

void
menus::RadioGroupNode::activate()
{
  auto node = get_selected_node();
  if (node)
    {
      node->activate();
    }
  if (activated)
    {
      activated();
    }
}

void
menus::RadioGroupNode::activate(int value)
{
  auto i = std::find_if(children.begin(), children.end(), [value](auto n) { return n->get_value() == value; });
  if (i != children.end())
    {
      auto node = *i;
      node->activate();
    }

  if (activated)
    {
      activated();
    }
}

auto
menus::SeparatorNode::create() -> menus::SeparatorNode::Ptr
{
  return std::make_shared<menus::SeparatorNode>();
}
