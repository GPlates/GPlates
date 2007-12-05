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

#include <boost/none.hpp>  // boost::none

#include "ReconstructedFeatureGeometryPopulator.h"
#include "ReconstructionTree.h"
#include "FeatureHandle.h"
#include "InlinePropertyContainer.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


GPlatesModel::ReconstructedFeatureGeometryPopulator::ReconstructedFeatureGeometryPopulator(
		const double &recon_time,
		unsigned long root_plate_id,
		ReconstructionTree &recon_tree,
		reconstructed_points_type &reconstructed_points,
		reconstructed_polylines_type &reconstructed_polylines,
		bool should_keep_features_without_recon_plate_id):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_root_plate_id(GPlatesModel::integer_plate_id_type(root_plate_id)),
	d_recon_tree_ptr(&recon_tree),
	d_reconstructed_points_to_populate(&reconstructed_points),
	d_reconstructed_polylines_to_populate(&reconstructed_polylines),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id)
{  }


namespace
{
	/**
	 * A Reconstructor is an abstract base class which provides member functions which
	 * reconstruct points and polylines.
	 */
	struct Reconstructor
	{
		virtual
		~Reconstructor()
		{  }

		virtual
		const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		reconstruct_point(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type p) const = 0;

		virtual
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		reconstruct_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type p) const = 0;
	};


	/**
	 * This is a reconstructor which uses a supplied ReconstructionTree to reconstruct points
	 * and polylines according to a supplied plate ID.
	 */
	struct ReconstructAccordingToPlateId: public Reconstructor
	{
		ReconstructAccordingToPlateId(
				const GPlatesModel::ReconstructionTree &recon_tree,
				GPlatesModel::integer_plate_id_type plate_id):
			d_recon_tree_ptr(&recon_tree),
			d_plate_id(plate_id)
		{  }

		virtual
		const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		reconstruct_point(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type p) const
		{
			// FIXME:  Do we care about the reconstruction circumstance?
			// (For example, there may have been no match for the reconstruction plate
			// ID.)
			return (d_recon_tree_ptr->reconstruct_point(*p, d_plate_id)).first;
		}

		virtual
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		reconstruct_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type p) const
		{
			// FIXME:  Do we care about the reconstruction circumstance?
			// (For example, there may have been no match for the reconstruction plate
			// ID.)
			return (d_recon_tree_ptr->reconstruct_polyline(*p, d_plate_id)).first;
		}

		const GPlatesModel::ReconstructionTree *d_recon_tree_ptr;
		GPlatesModel::integer_plate_id_type d_plate_id;
	};


	/**
	 * This is a reconstructor which simply returns the supplied points and polylines, as if
	 * they had been reconstructed using the identity rotation.
	 */
	struct IdentityReconstructor: public Reconstructor
	{
		IdentityReconstructor()
		{  }

		virtual
		const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		reconstruct_point(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type p) const
		{
			return p;
		}

		virtual
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		reconstruct_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type p) const
		{
			return p;
		}
	};


	void
	reconstruct_points_and_polylines(
			GPlatesModel::ReconstructedFeatureGeometryPopulator::reconstructed_points_type *
					reconstructed_points_to_populate,
			GPlatesModel::ReconstructedFeatureGeometryPopulator::reconstructed_polylines_type *
					reconstructed_polylines_to_populate,
			const std::vector<GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> > &
					not_yet_reconstructed_points,
			const std::vector<GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> > &
					not_yet_reconstructed_polylines,
			GPlatesModel::FeatureHandle &feature_handle,
			const Reconstructor &reconstructor)
	{
		using namespace GPlatesMaths;

		std::vector<PointOnSphere::non_null_ptr_to_const_type>::const_iterator point_iter =
				not_yet_reconstructed_points.begin();
		std::vector<PointOnSphere::non_null_ptr_to_const_type>::const_iterator point_end =
				not_yet_reconstructed_points.end();
		for ( ; point_iter != point_end; ++point_iter) {
			PointOnSphere::non_null_ptr_to_const_type reconstructed_point =
					reconstructor.reconstruct_point(*point_iter);

			GPlatesModel::ReconstructedFeatureGeometry<PointOnSphere> rfg(
					reconstructed_point, feature_handle);
			reconstructed_points_to_populate->push_back(rfg);
		}

		std::vector<PolylineOnSphere::non_null_ptr_to_const_type>::const_iterator polyline_iter =
				not_yet_reconstructed_polylines.begin();
		std::vector<PolylineOnSphere::non_null_ptr_to_const_type>::const_iterator polyline_end =
				not_yet_reconstructed_polylines.end();
		for ( ; polyline_iter != polyline_end; ++polyline_iter) {
			PolylineOnSphere::non_null_ptr_to_const_type reconstructed_polyline =
					reconstructor.reconstruct_polyline(*polyline_iter);

			GPlatesModel::ReconstructedFeatureGeometry<PolylineOnSphere> rfg(
					reconstructed_polyline, feature_handle);
			reconstructed_polylines_to_populate->push_back(rfg);
		}
	}
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_feature_handle(
		FeatureHandle &feature_handle)
{
	d_accumulator = ReconstructedFeatureGeometryAccumulator();

	// Now visit each of the properties in turn.
	visit_feature_properties(feature_handle);

	// So now we've visited the contents of this feature.  Let's find out if we were able to
	// obtain all the information we need.
	if ( ! d_accumulator->d_recon_plate_id) {
		// We couldn't obtain the reconstruction plate ID.

		// So, how do we react to this situation?  Do we ignore features which don't have a
		// reconsruction plate ID, or do we "reconstruct" their geometries using the
		// identity rotation, so the features simply sit still on the globe?  Fortunately,
		// the client code has already told us how it wants us to behave...
		if ( ! d_should_keep_features_without_recon_plate_id) {
			d_accumulator = boost::none;
			return;
		}
		IdentityReconstructor reconstructor;

		reconstruct_points_and_polylines(
				d_reconstructed_points_to_populate,
				d_reconstructed_polylines_to_populate,
				d_accumulator->d_not_yet_reconstructed_points,
				d_accumulator->d_not_yet_reconstructed_polylines,
				feature_handle,
				reconstructor);
	} else {
		// We obtained the reconstruction plate ID.  We now have all the information we
		// need to reconstruct according to the reconstruction plate ID.
		ReconstructAccordingToPlateId reconstructor(*d_recon_tree_ptr, *(d_accumulator->d_recon_plate_id));

		reconstruct_points_and_polylines(
				d_reconstructed_points_to_populate,
				d_reconstructed_polylines_to_populate,
				d_accumulator->d_not_yet_reconstructed_points,
				d_accumulator->d_not_yet_reconstructed_polylines,
				feature_handle,
				reconstructor);
	}
	d_accumulator = boost::none;
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_inline_property_container(
		InlinePropertyContainer &inline_property_container)
{
	d_accumulator->d_most_recent_propname_read = inline_property_container.property_name();

	visit_property_values(inline_property_container);
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_accumulator->d_not_yet_reconstructed_polylines.push_back(gml_line_string.polyline());
}

void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}

void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_accumulator->d_not_yet_reconstructed_points.push_back(gml_point.point());
}

void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static PropertyName reconstruction_plate_id_property_name("gpml:reconstructionPlateId");

	// Note that we're going to assume that we've read a property name...
	if (*(d_accumulator->d_most_recent_propname_read) == reconstruction_plate_id_property_name) {
		// This plate ID is the reconstruction plate ID.
		d_accumulator->d_recon_plate_id = gpml_plate_id.value();
	}
}
