/**
* \file 
* $Revision: 8461 $
* $Date: 2010-05-20 16:18:01 +0200 (to, 20 mai 2010) $ 
* 
* Copyright (C) 2010 Geological Survey of Norway
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

#include "app-logic/ApplicationState.h"

#include "model/ModelUtils.h"

#include "property-values/XsDouble.h"

#include "EditTimeSequenceWidget.h"

#include "TopologyNetworkPropertiesWidget.h"

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

GPlatesQtWidgets::TopologyNetworkPropertiesWidget::TopologyNetworkPropertiesWidget(
	GPlatesAppLogic::ApplicationState *application_state_ptr,
	QWidget *parent_):
	AbstractCustomPropertiesWidget(parent_),
	d_application_state_ptr(application_state_ptr)
{
	setupUi(this);
}

void
GPlatesQtWidgets::TopologyNetworkPropertiesWidget::add_properties_to_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_handle)
{
	GPlatesPropertyValues::XsDouble::non_null_ptr_type max_edge = 
		GPlatesPropertyValues::XsDouble::create( spinbox_max_edge->value() );

	feature_handle->add(
		GPlatesModel::TopLevelPropertyInline::create(
			GPlatesModel::PropertyName::create_gpml("maxEdge"),
			max_edge) );	
	
	GPlatesPropertyValues::XsDouble::non_null_ptr_type shape_factor = 
		GPlatesPropertyValues::XsDouble::create( spinbox_shape_factor->value() );

	feature_handle->add(
		GPlatesModel::TopLevelPropertyInline::create(
			GPlatesModel::PropertyName::create_gpml("shapeFactor"),
			shape_factor) );	
	
}

void
GPlatesQtWidgets::TopologyNetworkPropertiesWidget::add_geometry_properties_to_feature(
        GPlatesModel::FeatureHandle::weak_ref feature_handle)
{

}


void
GPlatesQtWidgets::TopologyNetworkPropertiesWidget::update()
{

}
