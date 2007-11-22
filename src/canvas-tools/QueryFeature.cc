/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include <queue>
#include <QLocale>

#include "QueryFeature.h"

#include "gui/ProximityTests.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/QueryFeaturePropertiesDialog.h"
#include "feature-visitors/QueryFeaturePropertiesDialogPopulator.h"
#include "feature-visitors/PlateIdFinder.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/LatLonPointConversions.h"
#include "model/FeatureHandle.h"
#include "utils/UnicodeStringUtils.h"


void
GPlatesCanvasTools::QueryFeature::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	double proximity_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);

	std::priority_queue<GPlatesGui::ProximityTests::ProximityHit> sorted_hits;
	GPlatesGui::ProximityTests::find_close_rfgs(sorted_hits, view_state().reconstruction(),
			oriented_click_pos_on_globe, proximity_inclusion_threshold);
	if (sorted_hits.size() == 0) {
		emit no_hits_found();
		return;
	}
	GPlatesModel::FeatureHandle::weak_ref feature_ref(sorted_hits.top().d_feature->reference());
	if ( ! feature_ref.is_valid()) {
		// FIXME:  How did this happen?  Throw an exception!
		return;  // FIXME:  Should throw an exception instead.
	}

	qfp_dialog().set_feature_type(
			GPlatesUtils::make_qstring(feature_ref->feature_type()));

	// These next few fields only make sense if the feature is reconstructable, ie. if it has a
	// reconstruction plate ID.
	GPlatesModel::PropertyName plate_id_property_name(UnicodeString("gpml:reconstructionPlateId"));
	GPlatesFeatureVisitors::PlateIdFinder plate_id_finder(plate_id_property_name);
	plate_id_finder.visit_feature_handle(*feature_ref);
	if (plate_id_finder.found_plate_ids_begin() != plate_id_finder.found_plate_ids_end()) {
		// The feature has a reconstruction plate ID.
		GPlatesModel::integer_plate_id_type recon_plate_id =
				*plate_id_finder.found_plate_ids_begin();
		qfp_dialog().set_plate_id(recon_plate_id);

		GPlatesModel::integer_plate_id_type root_plate_id =
				view_state().reconstruction_root();
		qfp_dialog().set_root_plate_id(root_plate_id);
		qfp_dialog().set_reconstruction_time(
				view_state().reconstruction_time());

		// Now let's use the reconstruction plate ID of the feature to find the appropriate 
		// absolute rotation in the reconstruction tree.
		GPlatesModel::ReconstructionTree &recon_tree =
				view_state().reconstruction().reconstruction_tree();
		std::pair<GPlatesMaths::FiniteRotation,
				GPlatesModel::ReconstructionTree::ReconstructionCircumstance>
				absolute_rotation =
						recon_tree.get_composed_absolute_rotation(recon_plate_id);

		// FIXME:  Do we care about the reconstruction circumstance?
		// (For example, there may have been no match for the reconstruction plate ID.)
		GPlatesMaths::UnitQuaternion3D uq = absolute_rotation.first.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(uq)) {
			qfp_dialog().set_euler_pole(QObject::tr("indeterminate"));
			qfp_dialog().set_angle(0.0);
		} else {
			using namespace GPlatesMaths;

			UnitQuaternion3D::RotationParams params = uq.get_rotation_params();

			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);

			// Use the default locale for the floating-point-to-string conversion.
			// (We need the underscore at the end of the variable name, because
			// apparently there is already a member of QueryFeature named 'locale'.)
			QLocale locale_;
			QString euler_pole_lat = locale_.toString(llp.latitude());
			QString euler_pole_lon = locale_.toString(llp.longitude());
			QString euler_pole_as_string;
			euler_pole_as_string.append(euler_pole_lat);
			euler_pole_as_string.append(QObject::tr(" ; "));
			euler_pole_as_string.append(euler_pole_lon);

			qfp_dialog().set_euler_pole(euler_pole_as_string);

			const double &angle = radiansToDegrees(params.angle).dval();
			qfp_dialog().set_angle(angle);
		}
	}

	GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator populator(
			qfp_dialog().property_tree());
	populator.visit_feature_handle(*feature_ref);

	qfp_dialog().show();

	// FIXME:  We should re-enable this.
	emit sorted_hits_updated();
}
