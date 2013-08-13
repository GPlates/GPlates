/**
 * \file
 * $Revision: 8672 $
 * $Date: 2010-06-10 07:00:38 +0200 (to, 10 jun 2010) $
 *
 * Copyright (C) 2010 Geological Survey of Norway
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QDebug>

#include "Symbol.h"

boost::optional<GPlatesGui::Symbol::SymbolType>
GPlatesGui::get_symbol_type_from_string(
    const QString &symbol_string)
{
    static GPlatesGui::symbol_text_map_type map;
    map[QString("TRIANGLE")] = Symbol::TRIANGLE;
    map[QString("SQUARE")] = Symbol::SQUARE;
    map[QString("CIRCLE")] = Symbol::CIRCLE;
    map[QString("CROSS")] = Symbol::CROSS;
    map[QString("STRAIN_MARKER")] = Symbol::STRAIN_MARKER;

    GPlatesGui::symbol_text_map_type::const_iterator it = map.find(symbol_string);
    if (it == map.end())
    {
		return boost::none;
    }
    else
    {
		//qDebug() << "found symbol " << it->first;
		return it->second;
    }
}
