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

#ifndef TOOLKIT_FACTORY_HH
#define TOOLKIT_FACTORY_HH

#include <memory>

#include "ui/IToolkit.hh"

class ToolkitFactory
{
public:
  static auto create(int argc, char **argv) -> std::shared_ptr<IToolkit>;
};

#endif // TOOLKIT_FACTORY_HH
