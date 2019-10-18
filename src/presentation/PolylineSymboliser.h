
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

#ifndef GPLATES_PRESENTATION_POLYLINESYMBOLISER_H
#define GPLATES_PRESENTATION_POLYLINESYMBOLISER_H

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <vector>

#include "Symboliser.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesPresentation
{
	class PolylineSymboliser :
			public Symboliser
	{
	public:
		// Convenience typedefs for a shared pointer to a @a PolylineSymboliser.
		typedef GPlatesUtils::non_null_intrusive_ptr<PolylineSymboliser> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const PolylineSymboliser> non_null_ptr_to_const_type;


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

			boost::optional<SimpleLine &>
			get_simple_line()
			{
				if (SimpleLine *simple_line = boost::get<SimpleLine>(&d_layer))
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
			return non_null_ptr_type(new PolylineSymboliser());
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

		layer_seq_type &
		get_layers()
		{
			return d_layers;
		}


		virtual
		Symbol::non_null_ptr_type
		symbolise(
				const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry) const;

	protected:
		PolylineSymboliser()
		{  }

	private:
		layer_seq_type d_layers;
	};
}

#endif // GPLATES_PRESENTATION_POLYLINESYMBOLISER_H
