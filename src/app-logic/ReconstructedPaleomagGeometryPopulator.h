/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDPALEOMAGGEOMETRYPOPULATOR_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDPALEOMAGGEOMETRYPOPULATOR_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ReconstructedPaleomagGeometry.h"
#include "ReconstructionFeatureProperties.h"

#include "maths/FiniteRotation.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	class ReconstructionTree;
	class ReconstructionGeometryCollection;

	/**
	 * Reconstructs (rotates) VGP feature geometries using a @a ReconstructionTree.
	 */
	class ReconstructedPaleomagGeometryPopulator:
			public GPlatesModel::FeatureVisitor,
			private boost::noncopyable
	{
	public:
		ReconstructedPaleomagGeometryPopulator(
				ReconstructionGeometryCollection &reconstruction_geometry_collection,
				bool should_keep_features_without_recon_plate_id = true);

		virtual
		~ReconstructedPaleomagGeometryPopulator()
		{  }

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
		void
		visit_gml_orientable_curve(
				GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point);
		
		virtual
		void
		visit_gml_polygon(
				GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

	private:
		ReconstructionGeometryCollection &d_reconstruction_geometry_collection;

		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;
		const ReconstructionTree &d_reconstruction_tree;

		ReconstructionFeatureProperties d_reconstruction_params;
		boost::optional<GPlatesMaths::FiniteRotation> d_recon_rotation;

		bool d_should_keep_features_without_recon_plate_id;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTEDPALEOMAGGEOMETRYPOPULATOR_H
