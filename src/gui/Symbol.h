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

#ifndef GPLATES_GUI_SYMBOL_H
#define GPLATES_GUI_SYMBOL_H

#include <map>
#include <boost/optional.hpp>

#include "model/FeatureType.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesGui
{


    struct Symbol
    {
	enum SymbolType
	{
	    TRIANGLE,
	    SQUARE,
	    CIRCLE,
	    CROSS,
	    STRAIN_MARKER,

		// NOTE: Any new values should also be added to @a transcribe.

	    NUM_SYMBOLS
	};

	Symbol(
	    SymbolType symbol_type = TRIANGLE,
	    unsigned int size = 1, // FIXME: Make this floating-point.
	    bool filled = false,
	    boost::optional<double> s_x = boost::none,
	    boost::optional<double> s_y = boost::none,
	    boost::optional<double> a = boost::none):
			d_symbol_type(symbol_type),
			d_size(size),
			d_filled(filled),
			d_scale_x(s_x),
			d_scale_y(s_y),
			d_angle(a)
	{ };

		SymbolType d_symbol_type;
		unsigned int d_size;
		bool d_filled;
		boost::optional<double> d_scale_x;
		boost::optional<double> d_scale_y;
		boost::optional<double> d_angle;

	private: // Transcribe for sessions/projects...

		friend class GPlatesScribe::Access;

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
    };

    typedef std::pair<GPlatesModel::FeatureType,Symbol> feature_type_symbol_pair_type;

    typedef std::map<GPlatesModel::FeatureType,Symbol> symbol_map_type;

    typedef std::map<QString,Symbol::SymbolType> symbol_text_map_type;

    boost::optional<Symbol::SymbolType>
    get_symbol_type_from_string( 
		const QString &symbol_string);


	/**
	 * Transcribe for sessions/projects.
	 */
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			Symbol::SymbolType &symbol_type,
			bool transcribed_construct_data);
}

#endif // GPLATES_GUI_SYMBOL_H
