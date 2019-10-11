/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_LINESYMBOL_H
#define GPLATES_PRESENTATION_LINESYMBOL_H

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "utils/ReferenceCount.h"


namespace GPlatesPresentation
{
	class LineSymbol :
			public GPlatesUtils::ReferenceCount<LineSymbol>
	{
	public:
		// Convenience typedefs for a shared pointer to a @a LineSymbol.
		typedef GPlatesUtils::non_null_intrusive_ptr<LineSymbol> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const LineSymbol> non_null_ptr_to_const_type;


		class SimpleLine
		{
		public:
			explicit
			SimpleLine(
					const double &line_width_ = 1.0) :
				line_width(line_width_)
			{  }

			double line_width;
		};

		class MarkerLine
		{
		public:
		};


		class Layer
		{
		public:
			explicit
			Layer(
					const SimpleLine &simple_line) :
				d_layer(simple_line)
			{  }

			explicit
			Layer(
					const MarkerLine &marker_line) :
				d_layer(marker_line)
			{  }

			boost::optional<const SimpleLine &>
			get_simple_line() const
			{
				if (const SimpleLine *simple_line = boost::get<SimpleLine>(&d_layer))
				{
					return *simple_line;
				}

				return boost::none;
			}

			boost::optional<const MarkerLine &>
			get_marker_line() const
			{
				if (const MarkerLine *marker_line = boost::get<MarkerLine>(&d_layer))
				{
					return *marker_line;
				}

				return boost::none;
			}

		private:
			typedef boost::variant<SimpleLine, MarkerLine> layer_type;
			layer_type d_layer;
		};

		typedef std::vector<Layer> layer_seq_type;


		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new LineSymbol());
		}

		void
		add_layer(
				const Layer &layer)
		{
			d_layers.push_back(layer);
		}

		const layer_seq_type &
		get_layers() const
		{
			return d_layers;
		}

	private:
		LineSymbol()
		{  }

		layer_seq_type d_layers;
	};
}

#endif // GPLATES_PRESENTATION_LINESYMBOL_H

