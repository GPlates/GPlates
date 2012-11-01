/* $Id: FlowlinePropertiesWidget.cc 8461 2010-05-20 14:18:01Z rwatson $ */

/**
* \file 
* $Revision: 8461 $
* $Date: 2010-05-20 16:18:01 +0200 (to, 20 mai 2010) $ 
* 
* Copyright (C) 2009, 2010 Geological Survey of Norway
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

#include <iostream>

#include "app-logic/ApplicationState.h"
#include "app-logic/FlowlineUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructUtils.h"

#include "model/ModelUtils.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlPlateId.h"

#include "FlowlinePropertiesWidget.h"


GPlatesQtWidgets::FlowlinePropertiesWidget::FlowlinePropertiesWidget(
		const GPlatesAppLogic::ApplicationState &application_state_,
		QWidget *parent_):
	AbstractCustomPropertiesWidget(parent_),
	d_application_state(application_state_)
{
	setupUi(this);

    radio_centre->setChecked(true);

}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesQtWidgets::FlowlinePropertiesWidget::do_geometry_tasks(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstruction_time_geometry_,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
// Here we correct the geometry depending on the desired role of the selected point.
//
// If the user wants the point to be the flowline spreading centre, we do nothing; the point will be
// reverse half-stage reconstructed in the main CreateFeatureDialog code.
//
// If the user wants the point to be either of the end-points, we want to find the central point (at current reconstruction
// time) which would give us the desired end point.  This point will then be reverse half-stage reconstructed in the
// main CreateFeatureDialog code.

    if (radio_centre->isChecked())
    {
        return reconstruction_time_geometry_;
    }



    GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder finder(
		d_application_state.get_current_reconstruction_time());
    finder.visit_feature(feature_ref);

    if (!finder.can_correct_seed_point())
    {
		return reconstruction_time_geometry_;
    }


    // Call the plates plate_1 and plate_2 so we can use the same stage-pole calculation further below.
    GPlatesModel::integer_plate_id_type plate_1;
    GPlatesModel::integer_plate_id_type plate_2;


    if (radio_left->isChecked())
    {
		plate_1 = finder.get_left_plate().get();
		plate_2 = finder.get_right_plate().get();
    }
    else if (radio_right->isChecked())
    {
		plate_1 = finder.get_right_plate().get();
		plate_2 = finder.get_left_plate().get();
    }

	// The default reconstruction tree.
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type default_reconstruction_tree =
			d_application_state.get_current_reconstruction()
					.get_default_reconstruction_layer_output()->get_reconstruction_tree();
	// A function to get reconstruction trees with.
	GPlatesAppLogic::ReconstructionTreeCreator
			reconstruction_tree_creator =
					GPlatesAppLogic::get_cached_reconstruction_tree_creator(
							default_reconstruction_tree->get_reconstruction_features(),
							default_reconstruction_tree->get_reconstruction_time(),
							default_reconstruction_tree->get_anchor_plate_id());


    return GPlatesAppLogic::FlowlineUtils::correct_end_point_to_centre(
		reconstruction_time_geometry_,
		plate_1,
		plate_2,
		finder.get_times(),
		reconstruction_tree_creator,
		d_application_state.get_current_reconstruction_time());

}
