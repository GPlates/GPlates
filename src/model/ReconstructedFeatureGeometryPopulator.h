/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRYPOPULATOR_H
#define GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRYPOPULATOR_H

#include <vector>
#include <boost/optional.hpp>

#include "FeatureVisitor.h"
#include "ReconstructedFeatureGeometry.h"
#include "types.h"
#include "PropertyName.h"
#include "property-values/GeoTimeInstant.h"
#include "maths/GeometryForwardDeclarations.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	class ReconstructionTree;

	class ReconstructedFeatureGeometryPopulator:
			public FeatureVisitor
	{
	public:

		struct ReconstructedFeatureGeometryAccumulator
		{
			/**
			 * Whether or not the current feature is defined at this reconstruction
			 * time.
			 *
			 * The value of this member defaults to true; it's only set to false if
			 * both: (i) a "gml:validTime" property is encountered which contains a
			 * "gml:TimePeriod" structural type; and (ii) the reconstruction time lies
			 * outside the range of the valid time.
			 */
			bool d_feature_is_defined_at_recon_time;

			boost::optional<PropertyName> d_most_recent_propname_read;
			boost::optional<integer_plate_id_type> d_recon_plate_id;
			std::vector<GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> >
					d_not_yet_reconstructed_points;
			std::vector<GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> >
					d_not_yet_reconstructed_polylines;

			ReconstructedFeatureGeometryAccumulator():
				d_feature_is_defined_at_recon_time(true)
			{  }

		};

		typedef std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >
				reconstructed_points_type;
		typedef std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >
				reconstructed_polylines_type;

		ReconstructedFeatureGeometryPopulator(
				const double &recon_time,
				unsigned long root_plate_id,
				ReconstructionTree &recon_tree,
				reconstructed_points_type &reconstructed_points,
				reconstructed_polylines_type &reconstructed_polylines,
				bool should_keep_features_without_recon_plate_id = true);

		virtual
		~ReconstructedFeatureGeometryPopulator()
		{  }

		virtual
		void
		visit_feature_handle(
				FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				InlinePropertyContainer &inline_property_container);

		virtual
		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string);


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
		visit_gml_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

	private:

		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;
		GPlatesModel::integer_plate_id_type d_root_plate_id;
		ReconstructionTree *d_recon_tree_ptr;
		reconstructed_points_type *d_reconstructed_points_to_populate;
		reconstructed_polylines_type *d_reconstructed_polylines_to_populate;
		boost::optional<ReconstructedFeatureGeometryAccumulator> d_accumulator;
		bool d_should_keep_features_without_recon_plate_id;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		ReconstructedFeatureGeometryPopulator(
				const ReconstructedFeatureGeometryPopulator &);

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		ReconstructedFeatureGeometryPopulator &
		operator=(
				const ReconstructedFeatureGeometryPopulator &);
	};
}

#endif  // GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRYPOPULATOR_H
