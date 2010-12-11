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
#include "EditTimeSequenceWidget.h"
#include "CreateFeatureDialog.h"

#include "FlowlinePropertiesWidget.h"

namespace
{
	/**
	 * Copied from ReconstructionViewWidget.cc
	 */
	 
	/**
	* This function is a bit of a hack, but we need this hack in enough places
	* in our hybrid Designer/C++ laid-out ReconstructionViewWidget that it's worthwhile
	* compressing it into an anonymous namespace function.
	*
	* The problem: We want to replace a 'placeholder' widget that we set up in the
	* designer with a widget we created in code via new.
	*
	* The solution: make an 'invisible' layout inside the placeholder (@a outer_widget),
	* then add the real widget (@a inner_widget) to that layout.
	*/
	void
	cram_widget_into_widget(
		QWidget *inner_widget,
		QWidget *outer_widget)
	{
		QHBoxLayout *invisible_layout = new QHBoxLayout(outer_widget);
		invisible_layout->setSpacing(0);
		invisible_layout->setContentsMargins(0, 0, 0, 0);
		invisible_layout->addWidget(inner_widget);
	}
}

GPlatesQtWidgets::FlowlinePropertiesWidget::FlowlinePropertiesWidget(
	GPlatesAppLogic::ApplicationState *application_state_ptr,
        CreateFeatureDialog *create_feature_dialog_ptr,
	QWidget *parent_):
	AbstractCustomPropertiesWidget(parent_),
	d_application_state_ptr(application_state_ptr),
        d_time_sequence_widget(new GPlatesQtWidgets::EditTimeSequenceWidget(
                *d_application_state_ptr,
                this)),
        d_create_feature_dialog_ptr(create_feature_dialog_ptr)
{
	setupUi(this);
	
        cram_widget_into_widget(d_time_sequence_widget,groupbox_time_sequence);

        radio_centre->setChecked(true);

}

void
GPlatesQtWidgets::FlowlinePropertiesWidget::add_properties_to_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_handle)
{
		
	
	GPlatesModel::PropertyValue::non_null_ptr_type time_sequence_value = 
			d_time_sequence_widget->create_property_value_from_widget();
	GPlatesModel::PropertyName time_sequence_name = GPlatesModel::PropertyName::create_gpml("times");
		
	feature_handle->add(
		GPlatesModel::TopLevelPropertyInline::create(
			time_sequence_name,
			time_sequence_value));

}

void
GPlatesQtWidgets::FlowlinePropertiesWidget::add_geometry_properties_to_feature(
                GPlatesModel::FeatureHandle::weak_ref feature_handle)
{

}


void
GPlatesQtWidgets::FlowlinePropertiesWidget::update()
{

}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesQtWidgets::FlowlinePropertiesWidget::do_geometry_tasks(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry_,
	const GPlatesModel::FeatureHandle::weak_ref &feature_handle)
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
        return geometry_;
    }



    GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder finder(
		d_application_state_ptr->get_current_reconstruction_time());
    finder.visit_feature(feature_handle);

    if (!finder.can_correct_seed_point())
    {
	return geometry_;
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


    return GPlatesAppLogic::FlowlineUtils::correct_end_point_to_centre(
		geometry_,
		plate_1,
		plate_2,
		finder.get_times(),
		d_application_state_ptr->get_current_reconstruction().get_default_reconstruction_tree(),
		d_application_state_ptr->get_current_reconstruction_time());

}
