/* $Id: ModifyGeometryWidget.cc 10789 2011-01-19 08:58:33Z elau $ */

/**
 * \file Displays lat/lon points of geometry being modified by a canvas tool.
 * 
 * $Revision: 10789 $
 * $Date: 2011-01-19 09:58:33 +0100 (on, 19 jan 2011) $
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#include <QDebug>
#include <QWidget>

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructUtils.h"
#include "presentation/ViewState.h"
#include "maths/MathsUtils.h"
#include "model/types.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlMeasure.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "CreateSmallCircleDialog.h"
#include "CreateSmallCircleFeatureDialog.h"
#include "QtWidgetUtils.h"
#include "SmallCircleWidget.h"



GPlatesQtWidgets::SmallCircleWidget::SmallCircleWidget(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	TaskPanelWidget(parent_),
	d_application_state_ptr(&view_state.get_application_state()),
        d_create_small_circle_dialog_ptr(new CreateSmallCircleDialog(this,view_state.get_application_state(),this)),
	d_small_circle_layer(view_state.get_rendered_geometry_collection().get_main_rendered_layer(
                GPlatesViewOperations::RenderedGeometryCollection::SMALL_CIRCLE_LAYER))
{
	setupUi(this);
	
	set_default_states();

	QObject::connect(this, SIGNAL(feature_created()),d_application_state_ptr,SLOT(reconstruct()));
        QObject::connect(button_specify,SIGNAL(clicked()),this,SLOT(handle_specify()));

	d_small_circle_layer->set_active();
}

void
GPlatesQtWidgets::SmallCircleWidget::set_default_states()
{
    button_create_feature->setEnabled(false);

}

void
GPlatesQtWidgets::SmallCircleWidget::handle_create_feature()
{
	CreateSmallCircleFeatureDialog *create_small_circle_feature_dialog_ptr =
		new CreateSmallCircleFeatureDialog(d_application_state_ptr,d_small_circles,this);

	if (create_small_circle_feature_dialog_ptr->exec() == QDialog::Accepted)
	{
		handle_clear();
	}

}


void
GPlatesQtWidgets::SmallCircleWidget::hideEvent(
	QHideEvent *hide_event)
{
	// If the small circle canvas tool gets deactivated, then we 
	// want to hide the stage pole dialog too.
        if (d_create_small_circle_dialog_ptr)
	{
                d_create_small_circle_dialog_ptr->close();
	}

	d_small_circle_layer->set_active(false);
}



void
GPlatesQtWidgets::SmallCircleWidget::update_small_circle_layer()
{
	using namespace GPlatesMaths;

	d_small_circle_layer->clear_rendered_geometries();
	small_circle_collection_type::const_iterator it = d_small_circles.begin(),
		end = d_small_circles.end();

	for ( ; it != end ; ++it)
	{
		GPlatesViewOperations::RenderedGeometry circle = GPlatesViewOperations::create_rendered_small_circle(
			GPlatesMaths::SmallCircle::create_colatitude(it->axis_vector(),it->colatitude()));

		d_small_circle_layer->add_rendered_geometry(circle);
	}

	button_create_feature->setEnabled(!d_small_circles.empty());
}

void
GPlatesQtWidgets::SmallCircleWidget::handle_activation()
{
	d_small_circle_layer->set_active();
}

void
GPlatesQtWidgets::SmallCircleWidget::handle_clear()
{
	d_small_circles.clear();
	lineedit_centre->clear();
	textedit_radii->clear();
	update_small_circle_layer();

	// The canvas tool will listen for this and reset any of its circles. 
	emit clear_geometries();
}


void
GPlatesQtWidgets::SmallCircleWidget::update_buttons()
{
	// We only want to allow feature creation when we've actually got some circles to use.
	button_create_feature->setEnabled(!d_small_circles.empty());
}

void
GPlatesQtWidgets::SmallCircleWidget::update_current_centre(
	const GPlatesMaths::PointOnSphere &current_centre)
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(current_centre);
	QString centre_string = QString("(%1°,%2°)").arg(llp.latitude(),0,'f',2).arg(llp.longitude(),0,'f',2);
	lineedit_centre->setText(centre_string);
}

void
GPlatesQtWidgets::SmallCircleWidget::update_current_circles()
{
	if (d_small_circles.empty())
	{
		return;
	}

	GPlatesMaths::PointOnSphere centre(d_small_circles.front().axis_vector());
	update_current_centre(centre);

	update_radii();
        update_buttons();
        update_small_circle_layer();
}

void
GPlatesQtWidgets::SmallCircleWidget::update_radii(
	boost::optional<double> current_radius)
{
	small_circle_collection_type::const_iterator
		it = d_small_circles.begin(),
		end = d_small_circles.end();

	std::vector<double> radii;
	for (; it != end ; ++it)
	{
		radii.push_back(it->colatitude().dval());
	}
	if (current_radius)
	{
		radii.push_back(*current_radius);
	}

	std::sort(radii.begin(),radii.end());

	std::vector<double>::const_iterator r_it = radii.begin(), r_end = radii.end();

	textedit_radii->clear();
	for (; r_it != r_end ; ++r_it)
	{
                if (current_radius && (GPlatesMaths::Real(*r_it) == GPlatesMaths::Real(*current_radius)))
		{
			textedit_radii->setTextBackgroundColor(Qt::yellow);
		}
		else
		{
			textedit_radii->setTextBackgroundColor(Qt::white);
		}
		textedit_radii->append(QString("%1°").arg(GPlatesMaths::convert_rad_to_deg(*r_it)));
	}
}

void
GPlatesQtWidgets::SmallCircleWidget::handle_specify()
{
    // The stage pole dialog is non-modal. Use the 'raise' etc tricks provided in QtWidgetUtils.
    QtWidgetUtils::pop_up_dialog(d_create_small_circle_dialog_ptr);
}

void
GPlatesQtWidgets::SmallCircleWidget::update_circles(
        small_circle_collection_type &small_circle_collection_)
{
    emit clear_geometries();
    d_small_circles = small_circle_collection_;
    update_current_circles();
}


