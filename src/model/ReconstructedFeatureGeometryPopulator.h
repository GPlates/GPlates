/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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
#include <boost/intrusive_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "FeatureVisitor.h"
#include "GeoTimeInstant.h"
#include "GpmlPlateId.h"
#include "ReconstructedFeatureGeometry.h"


// Forward declaration for intrusive-pointer.
// (We want to avoid the inclusion of "maths/PointOnSphere.h" and "maths/PolylineOnSphere.h" into
// this header file.)
namespace GPlatesMaths
{
	class PointOnSphere;
	class PolylineOnSphere;

	void
	intrusive_ptr_add_ref(
			const PointOnSphere *p);

	void
	intrusive_ptr_release(
			const PointOnSphere *p);

	void
	intrusive_ptr_add_ref(
			const PolylineOnSphere *p);

	void
	intrusive_ptr_release(
			const PolylineOnSphere *p);
}

namespace GPlatesModel
{
	class PropertyName;
	class ReconstructionTree;

	class ReconstructedFeatureGeometryPopulator:
			public FeatureVisitor
	{
	public:

		struct ReconstructedFeatureGeometryAccumulator
		{
			boost::scoped_ptr<PropertyName> d_most_recent_propname_read;
			boost::intrusive_ptr<GpmlPlateId> d_recon_plate_id;
			std::vector<boost::intrusive_ptr<const GPlatesMaths::PointOnSphere> >
					d_not_yet_reconstructed_points;
			std::vector<boost::intrusive_ptr<const GPlatesMaths::PolylineOnSphere> >
					d_not_yet_reconstructed_polylines;

			ReconstructedFeatureGeometryAccumulator():
				d_most_recent_propname_read(NULL),
				d_recon_plate_id(NULL)
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
				reconstructed_polylines_type &reconstructed_polylines);

		virtual
		~ReconstructedFeatureGeometryPopulator()
		{  }

		virtual
		void
		visit_feature_handle(
				FeatureHandle &feature_handle);

		virtual
		void
		visit_gml_line_string(
				GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_orientable_curve(
				GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gpml_constant_value(
				GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_single_valued_property_container(
				SingleValuedPropertyContainer &single_valued_property_container);

	private:

		const GeoTimeInstant d_recon_time;
		GpmlPlateId::integer_plate_id_type d_root_plate_id;
		ReconstructionTree *d_recon_tree_ptr;
		reconstructed_points_type *d_reconstructed_points_ptr;
		reconstructed_polylines_type *d_reconstructed_polylines_ptr;
		boost::scoped_ptr<ReconstructedFeatureGeometryAccumulator> d_accumulator;

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
