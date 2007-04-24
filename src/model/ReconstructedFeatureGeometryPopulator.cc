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

#include "ReconstructedFeatureGeometryPopulator.h"
#include "ReconstructionTree.h"
#include "FeatureHandle.h"
#include "GmlLineString.h"
#include "GmlOrientableCurve.h"
#include "GpmlConstantValue.h"
#include "InlinePropertyContainer.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


GPlatesModel::ReconstructedFeatureGeometryPopulator::ReconstructedFeatureGeometryPopulator(
		const double &recon_time,
		unsigned long root_plate_id,
		ReconstructionTree &recon_tree,
		reconstructed_points_type &reconstructed_points,
		reconstructed_polylines_type &reconstructed_polylines):
	d_recon_time(GeoTimeInstant(recon_time)),
	d_root_plate_id(GpmlPlateId::integer_plate_id_type(root_plate_id)),
	d_recon_tree_ptr(&recon_tree),
	d_reconstructed_points_ptr(&reconstructed_points),
	d_reconstructed_polylines_ptr(&reconstructed_polylines)
{  }


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_feature_handle(
		FeatureHandle &feature_handle)
{
	static FeatureType isochron_feature_type =
			FeatureType(UnicodeString("gpml:Isochron"));

	// If a feature handle isn't for a feature of type "gpml:Isochron", we're not interested.
	if (feature_handle.feature_type() == isochron_feature_type) {
		d_accumulator.reset(new ReconstructedFeatureGeometryAccumulator());

		// Now visit each of the properties in turn.
		GPlatesModel::FeatureHandle::properties_iterator iter =
				feature_handle.properties_begin();
		GPlatesModel::FeatureHandle::properties_iterator end =
				feature_handle.properties_end();
		for ( ; iter != end; ++iter) {
			// Elements of this properties vector can be NULL pointers.  (See the
			// comment in "model/FeatureRevision.h" for more details.)
			if (*iter != NULL) {
				d_accumulator->d_most_recent_propname_read.reset(
						new PropertyName((*iter)->property_name()));
				(*iter)->accept_visitor(*this);
			}
		}

		// So now we've visited the contents of this Isochron feature.  Let's find out if
		// we were able to obtain all the information we need.
		if (d_accumulator->d_recon_plate_id == NULL) {
			// We couldn't obtain the reconstruction plate ID.
			d_accumulator.reset(NULL);
			return;
		}

		// If we got to here, we have all the information we need.

		std::vector<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>::iterator point_iter =
				d_accumulator->d_not_yet_reconstructed_points.begin();
		std::vector<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>::iterator point_end =
				d_accumulator->d_not_yet_reconstructed_points.end();
		for ( ; point_iter != point_end; ++point_iter) {
			boost::intrusive_ptr<GPlatesMaths::PointOnSphere> reconstructed_point =
					d_recon_tree_ptr->reconstruct_point(*point_iter,
					d_accumulator->d_recon_plate_id->value(), d_root_plate_id);
			if (reconstructed_point == NULL) {
				// No match for the reconstruction plate ID.
				continue;
			} else {
				// It will be valid to dereference 'reconstructed_point'.
				GPlatesMaths::PointOnSphere::non_null_ptr_type p(*reconstructed_point);
				ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> rfg(p);
				d_reconstructed_points_ptr->push_back(rfg);
			}
		}

		std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>::iterator polyline_iter =
				d_accumulator->d_not_yet_reconstructed_polylines.begin();
		std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>::iterator polyline_end =
				d_accumulator->d_not_yet_reconstructed_polylines.end();
		for ( ; polyline_iter != polyline_end; ++polyline_iter) {
			boost::intrusive_ptr<GPlatesMaths::PolylineOnSphere> reconstructed_polyline =
					d_recon_tree_ptr->reconstruct_polyline(*polyline_iter,
					d_accumulator->d_recon_plate_id->value(), d_root_plate_id);
			if (reconstructed_polyline == NULL) {
				// No match for the reconstruction plate ID.
				continue;
			} else {
				// It will be valid to dereference 'reconstructed_polyline'.
				GPlatesMaths::PolylineOnSphere::non_null_ptr_type p(*reconstructed_polyline);
				ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> rfg(p);
				d_reconstructed_polylines_ptr->push_back(rfg);
			}
		}

		d_accumulator.reset(NULL);
	}
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_line_string(
		GmlLineString &gml_line_string)
{
	d_accumulator->d_not_yet_reconstructed_polylines.push_back(gml_line_string.polyline());
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_orientable_curve(
		GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gpml_constant_value(
		GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gpml_plate_id(
		GpmlPlateId &gpml_plate_id)
{
	boost::intrusive_ptr<GpmlPlateId> gpml_plate_id_ptr = &gpml_plate_id;
	d_accumulator->d_recon_plate_id = gpml_plate_id_ptr;
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_inline_property_container(
		InlinePropertyContainer &inline_property_container)
{
	InlinePropertyContainer::const_iterator iter = inline_property_container.begin(); 
	InlinePropertyContainer::const_iterator end = inline_property_container.end(); 
	for ( ; iter != end; ++iter) {
		(*iter)->accept_visitor(*this);
	}
}
