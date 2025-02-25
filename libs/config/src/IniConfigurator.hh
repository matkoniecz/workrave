// Copyright (C) 2001 - 2013 Rob Caelers <robc@krandor.nl>
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

#ifndef INICONFIGURATOR_HH
#define INICONFIGURATOR_HH

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <string>

#include "IConfigBackend.hh"

class IniConfigurator : public virtual IConfigBackend
{
public:
  IniConfigurator() = default;
  ~IniConfigurator() override = default;

  bool load(std::string filename) override;
  bool save(std::string filename) override;
  bool save() override;

  bool remove_key(const std::string &key) override;
  bool has_user_value(const std::string &key) override;
  bool get_value(const std::string &key, VariantType type, Variant &value) const override;
  bool set_value(const std::string &key, Variant &value) override;

private:
  boost::property_tree::ptree::path_type path(const std::string &key) const;

private:
  boost::property_tree::ptree pt;

  std::string last_filename;
};

#endif // INICONFIGURATOR_HH
