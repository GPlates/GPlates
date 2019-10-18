
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

#ifndef GPLATES_PRESENTATION_POINTSYMBOLISER_H
#define GPLATES_PRESENTATION_POINTSYMBOLISER_H

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
	class PointSymboliser :
			public Symboliser
	{
	public:
		// Convenience typedefs for a shared pointer to a @a PointSymboliser.
		typedef GPlatesUtils::non_null_intrusive_ptr<PointSymboliser> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const PointSymboliser> non_null_ptr_to_const_type;


		class SimplePoint
		{
		public:
			explicit
			SimplePoint(
					const double &point_size_ = 1.0) :
				point_size(point_size_)
			{  }

			double point_size;
		};

		class MarkerPoint
		{
		public:
		};


		class Layer
		{
		public:
			explicit
			Layer(
					const SimplePoint &simple_point) :
				d_layer(simple_point)
			{  }

			explicit
			Layer(
					const MarkerPoint &marker_point) :
				d_layer(marker_point)
			{  }


			boost::optional<const SimplePoint &>
			get_simple_point() const
			{
				if (const SimplePoint *simple_point = boost::get<SimplePoint>(&d_layer))
				{
					return *simple_point;
				}

				return boost::none;
			}

			boost::optional<SimplePoint &>
			get_simple_point()
			{
				if (SimplePoint *simple_point = boost::get<SimplePoint>(&d_layer))
				{
					return *simple_point;
				}

				return boost::none;
			}

			boost::optional<const MarkerPoint &>
			get_marker_point() const
			{
				if (const MarkerPoint *marker_point = boost::get<MarkerPoint>(&d_layer))
				{
					return *marker_point;
				}

				return boost::none;
			}

		private:
			typedef boost::variant<SimplePoint, MarkerPoint> layer_type;
			layer_type d_layer;
		};

		typedef std::vector<Layer> layer_seq_type;


		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new PointSymboliser());
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


		Symbol::non_null_ptr_type
		symbolise(
				const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry) const;

	protected:
		PointSymboliser()
		{  }

	private:
		layer_seq_type d_layers;
	};
}

#endif // GPLATES_PRESENTATION_POINTSYMBOLISER_H
