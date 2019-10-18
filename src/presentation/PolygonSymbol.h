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

#ifndef GPLATES_PRESENTATION_POLYGONSYMBOL_H
#define GPLATES_PRESENTATION_POLYGONSYMBOL_H

#include <vector>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "Symbol.h"

#include "utils/ReferenceCount.h"


namespace GPlatesPresentation
{
	class PolygonSymbol :
			public Symbol
	{
	public:
		// Convenience typedefs for a shared pointer to a @a PolygonSymbol.
		typedef GPlatesUtils::non_null_intrusive_ptr<PolygonSymbol> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const PolygonSymbol> non_null_ptr_to_const_type;


		class SimpleOutline
		{
		public:
			explicit
			SimpleOutline(
					float line_width_ = 1.0f) :
				line_width(line_width_)
			{  }

			float line_width;
		};

		class MarkerOutline
		{
		public:
		};

		class FillInterior
		{
		public:
		};


		class Layer
		{
		public:
			explicit
			Layer(
					const SimpleOutline &simple_outline) :
				d_layer(simple_outline)
			{  }

			explicit
			Layer(
					const MarkerOutline &marker_outline) :
				d_layer(marker_outline)
			{  }

			explicit
			Layer(
					const FillInterior &fill_interior) :
				d_layer(fill_interior)
			{  }


			boost::optional<const SimpleOutline &>
			get_simple_outline() const
			{
				if (const SimpleOutline *simple_outline = boost::get<SimpleOutline>(&d_layer))
				{
					return *simple_outline;
				}

				return boost::none;
			}

			boost::optional<const MarkerOutline &>
			get_marker_outline() const
			{
				if (const MarkerOutline *marker_outline = boost::get<MarkerOutline>(&d_layer))
				{
					return *marker_outline;
				}

				return boost::none;
			}

			boost::optional<const FillInterior &>
			get_fill_interior() const
			{
				if (const FillInterior *fill_interior = boost::get<FillInterior>(&d_layer))
				{
					return *fill_interior;
				}

				return boost::none;
			}

		private:
			typedef boost::variant<SimpleOutline, MarkerOutline, FillInterior> layer_type;
			layer_type d_layer;
		};

		typedef std::vector<Layer> layer_seq_type;


		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new PolygonSymbol());
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


		/**
		 * Accept a ConstSymbolVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstSymbolVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

		/**
		 * Accept a SymbolVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				SymbolVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

	protected:
		PolygonSymbol()
		{  }

	private:
		layer_seq_type d_layers;
	};
}

#endif // GPLATES_PRESENTATION_POLYGONSYMBOL_H

