
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

#ifndef GPLATES_PRESENTATION_RECONSTRUCTIONGEOMETRYSYMBOLISER_H
#define GPLATES_PRESENTATION_RECONSTRUCTIONGEOMETRYSYMBOLISER_H

#include <boost/optional.hpp>
#include <QObject>

#include "PointSymboliser.h"
#include "PolygonSymboliser.h"
#include "PolylineSymboliser.h"
#include "Symbol.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
	class ReconstructionGeometry;
}

namespace GPlatesPresentation
{
	class ReconstructionGeometrySymboliser :
			public QObject,
			public GPlatesUtils::ReferenceCount<ReconstructionGeometrySymboliser>
	{
		Q_OBJECT

	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructionGeometrySymboliser.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometrySymboliser> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionGeometrySymboliser> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ReconstructionGeometrySymboliser());
		}


		boost::optional<Symbol::non_null_ptr_type>
		symbolise(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const;

		boost::optional<Symbol::non_null_ptr_type>
		symbolise(
				const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstruct_feature_geometry) const;

	Q_SIGNALS:

		/**
		 * Emitted when any aspect of any rule/symboliser has been modified.
		 */
		void
		modified();

	private:

		ReconstructionGeometrySymboliser();

		PointSymboliser::non_null_ptr_type d_point_symboliser;
		PolylineSymboliser::non_null_ptr_type d_polyline_symboliser;
		PolygonSymboliser::non_null_ptr_type d_polygon_symboliser;

	//
	// TODO: Remove the following when symbolisers are properly created and modified via the GUI...
	//
	// Note that these do not trigger the 'modified' signal since these changes have already
	// been signalled elsewhere.
	//
	public:
		void
		set_line_width(
				float line_width) const;

		void
		set_point_size(
				float point_size) const;
	};
}

#endif // GPLATES_PRESENTATION_RECONSTRUCTIONGEOMETRYSYMBOLISER_H
