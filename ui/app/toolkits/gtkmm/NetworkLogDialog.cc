// NetworkLogDialog.cc --- NetworkLog dialog
//
// Copyright (C) 2002, 2003, 2006, 2007, 2008, 2011 Rob Caelers & Raymond Penners
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

#ifdef HAVE_DISTRIBUTION

#  include <cassert>

#  include <gtkmm/textview.h>
#  include <gtkmm/textbuffer.h>
#  include <gtkmm/adjustment.h>
#  include <gtkmm/stock.h>

#  include "commonui/nls.h"
#  include "debug.hh"

#  include "NetworkLogDialog.hh"

#  include "core/ICore.hh"
#  include "core/IDistributionManager.hh"

using namespace std;
using namespace workrave;

NetworkLogDialog::NetworkLogDialog(std::shared_ptr<IApplication> app)
  : Gtk::Dialog(_("Network log"), false)
  , app(app)
{
  TRACE_ENTER("NetworkLogDialog::NetworkLogDialog");

  set_default_size(600, 400);

  text_buffer = Gtk::TextBuffer::create();

  text_view = Gtk::manage(new Gtk::TextView(text_buffer));
  text_view->set_cursor_visible(false);
  text_view->set_editable(false);

  scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  scrolled_window.add(*text_view);

  Gtk::HBox *box = Gtk::manage(new Gtk::HBox(false, 6));
  box->pack_start(scrolled_window, true, true, 0);

  get_vbox()->pack_start(*box, true, true, 0);

  add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);

  show_all();

  TRACE_EXIT();
}

//! Destructor.
NetworkLogDialog::~NetworkLogDialog()
{
  TRACE_ENTER("NetworkLogDialog::~NetworkLogDialog");
  auto core = app->get_core();
  IDistributionManager *dist_manager = core->get_distribution_manager();
  if (dist_manager != nullptr)
    {
      dist_manager->remove_log_listener(this);
    }
  TRACE_EXIT();
}

void
NetworkLogDialog::distribution_log(std::string msg)
{
  Gtk::TextIter iter = text_buffer->end();
  iter = text_buffer->insert(iter, msg);
  Glib::RefPtr<Gtk::Adjustment> a = scrolled_window.get_vadjustment();
  a->set_value(a->get_upper());
}

void
NetworkLogDialog::init()
{
  auto core = app->get_core();
  IDistributionManager *dist_manager = core->get_distribution_manager();

  Gtk::TextIter iter = text_buffer->end();

  if (dist_manager != nullptr)
    {
      list<string> logs = dist_manager->get_logs();

      for (list<string>::iterator i = logs.begin(); i != logs.end(); i++)
        {
          iter = text_buffer->insert(iter, (*i));
        }

      dist_manager->add_log_listener(this);
      Glib::RefPtr<Gtk::Adjustment> a = scrolled_window.get_vadjustment();
      a->set_value(a->get_upper());
    }
}

int
NetworkLogDialog::run()
{
  TRACE_ENTER("NetworkLogDialog::run");
  init();

  show_all();
  TRACE_EXIT();
  return 0;
}

void
NetworkLogDialog::on_response(int response)
{
  (void)response;
  TRACE_ENTER("NetworkLogDialog::on_response");
  auto core = app->get_core();
  IDistributionManager *dist_manager = core->get_distribution_manager();
  if (dist_manager != nullptr)
    {
      dist_manager->remove_log_listener(this);
    }
  TRACE_EXIT();
}

#endif
