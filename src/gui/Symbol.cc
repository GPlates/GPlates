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

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


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


GPlatesScribe::TranscribeResult
GPlatesGui::Symbol::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const Symbol DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_symbol_type, "symbol_type"))
	{
		d_symbol_type = DEFAULT_PARAMS.d_symbol_type;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_size, "size"))
	{
		d_size = DEFAULT_PARAMS.d_size;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_filled, "filled"))
	{
		d_filled = DEFAULT_PARAMS.d_filled;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_scale_x, "scale_x"))
	{
		d_scale_x = DEFAULT_PARAMS.d_scale_x;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_scale_y, "scale_y"))
	{
		d_scale_y = DEFAULT_PARAMS.d_scale_y;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_angle, "angle"))
	{
		d_angle = DEFAULT_PARAMS.d_angle;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesGui::transcribe(
		GPlatesScribe::Scribe &scribe,
		Symbol::SymbolType &symbol_type,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("TRIANGLE", Symbol::TRIANGLE),
		GPlatesScribe::EnumValue("SQUARE", Symbol::SQUARE),
		GPlatesScribe::EnumValue("CIRCLE", Symbol::CIRCLE),
		GPlatesScribe::EnumValue("CROSS", Symbol::CROSS),
		GPlatesScribe::EnumValue("STRAIN_MARKER", Symbol::STRAIN_MARKER)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			symbol_type,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
