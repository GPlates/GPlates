/* $Id: CreateVGPDialog.cc 11436 2011-05-04 11:04:00Z rwatson $ */

/**
 * \file 
 * $Revision: 11436 $
 * $Date: 2011-05-04 13:04:00 +0200 (on, 04 mai 2011) $ 
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

#include <boost/foreach.hpp>
#include <QListWidget>
#include <QMessageBox>

#include "CreateSmallCircleFeatureDialog.h"

#include "ChooseFeatureCollectionWidget.h"
#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/PointOnSphere.h"
#include "maths/SmallCircle.h"

#include "model/Model.h"
#include "model/PropertyName.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"

#include "presentation/ViewState.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

#include "qt-widgets/EditTimePeriodWidget.h"

namespace
{

	void
	append_name_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const QString &name)
	{
		
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("name"),
					GPlatesPropertyValues::XsString::create(
							GPlatesUtils::UnicodeString(name.toStdString().c_str()))));
	}

	void
	append_description_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const QString &description)
	{

		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
			GPlatesModel::PropertyName::create_gml("description"),
			GPlatesPropertyValues::XsString::create(
			GPlatesUtils::UnicodeString(description.toStdString().c_str()))));
	}

}

GPlatesQtWidgets::CreateSmallCircleFeatureDialog::CreateSmallCircleFeatureDialog(
		GPlatesAppLogic::ApplicationState *app_state_ptr,
		const small_circle_collection_type &small_circles,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_model_ptr(app_state_ptr->get_model_interface()),
	d_file_state(app_state_ptr->get_feature_collection_file_state()),
	d_file_io(app_state_ptr->get_feature_collection_file_io()),
	d_application_state_ptr(app_state_ptr),
	d_choose_feature_collection_widget(
			new ChooseFeatureCollectionWidget(
				d_application_state_ptr->get_reconstruct_method_registry(),
				d_file_state,
				d_file_io,
				this)),
	d_edit_time_period_widget(
			new EditTimePeriodWidget(this)),
	d_small_circles(small_circles)
{
	setupUi(this);

	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_collection_widget,
			widget_choose_feature_collection_placeholder);

	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_edit_time_period_widget,
			widget_time_period_placeholder);
	
	reset();

	setup_connections();
}

void
GPlatesQtWidgets::CreateSmallCircleFeatureDialog::setup_connections()
{
	QObject::connect(button_previous,SIGNAL(clicked()),this,SLOT(handle_previous()));
	QObject::connect(button_next,SIGNAL(clicked()),this,SLOT(handle_next()));
	QObject::connect(button_create,SIGNAL(clicked()),this,SLOT(handle_create()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(handle_cancel()));			

	// Pushing Enter or double-clicking should cause the create button to focus.
	QObject::connect(d_choose_feature_collection_widget, SIGNAL(item_activated()),
		button_create, SLOT(setFocus()));

	QObject::connect(this,SIGNAL(feature_created()),
			d_application_state_ptr,SLOT(reconstruct()));
}

void
GPlatesQtWidgets::CreateSmallCircleFeatureDialog::handle_previous()
{
	// If we're on the "add_feature" page, go back.
	if (stacked_widget->currentIndex() == COLLECTION_PAGE)
	{
		setup_properties_page();
	}
}

void
GPlatesQtWidgets::CreateSmallCircleFeatureDialog::handle_next()
{
	if (stacked_widget->currentIndex() == PROPERTIES_PAGE)
	{
		setup_collection_page();
	}
}

void
GPlatesQtWidgets::CreateSmallCircleFeatureDialog::handle_create()
{

	try
	{
		// Get the FeatureCollection the user has selected.
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> collection_file_iter =
			d_choose_feature_collection_widget->get_file_reference();
		GPlatesModel::FeatureCollectionHandle::weak_ref collection =
			(collection_file_iter.first).get_file().get_feature_collection();
	

		static const GPlatesModel::FeatureType feature_type = 
			GPlatesModel::FeatureType::create_gpml("SmallCircle");	

		small_circle_collection_type::const_iterator 
			it = d_small_circles.begin(),
			end = d_small_circles.end();

		for (; it != end ; ++it)
		{
			// Actually create the Feature!
			GPlatesModel::FeatureHandle::weak_ref feature =
				GPlatesModel::FeatureHandle::create(collection, feature_type);

			append_name_to_feature(
				feature,
				lineedit_name->text());

			// Create and add small circle centre.
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type point =
				GPlatesPropertyValues::GmlPoint::create(GPlatesMaths::PointOnSphere(it->axis_vector()));
			feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("centre"),
				point));

			// Create and add small circle radius.

            //FIXME: Should we use GpmlMeasure or GpmlAngle here? The EditAngleWidget uses
            // GpmlMeasure. We don't yet have a GpmlAngle property value.

			// uom- (unit-of-measure) related stuff copied from EditAngleWidget
			GPlatesModel::XmlAttributeName attr_name = GPlatesModel::XmlAttributeName::create_gml("uom");
			GPlatesModel::XmlAttributeValue attr_value("urn:ogc:def:uom:OGC:1.0:degree");
			std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> uom;
			uom.insert(std::make_pair(attr_name, attr_value));

			GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type radius =
				GPlatesPropertyValues::GpmlMeasure::create(GPlatesMaths::convert_rad_to_deg(it->colatitude()).dval(),uom);

			feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("radius"),
				radius));

			GPlatesModel::PropertyValue::non_null_ptr_type time_period = 
				d_edit_time_period_widget->create_property_value_from_widget();

			feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gml("validTime"),
				time_period));

			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
				GPlatesPropertyValues::GpmlPlateId::create(spinbox_plate_id->value());

			feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
				GPlatesModel::ModelUtils::create_gpml_constant_value(
					gpml_plate_id,
					GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId"))));	
		}



		// To trigger a reconstruction.
		emit feature_created();

#if 0
		// Create a new layer if necessary.
		d_application_state_ptr->update_layers(collection_file_iter.first);
#endif

		accept();
	}
	catch (const ChooseFeatureCollectionWidget::NoFeatureCollectionSelectedException &)
	{
		QMessageBox::critical(this, tr("No feature collection selected"),
				tr("Please select a feature collection to add the new feature to."));
		return;
	}

}

void
GPlatesQtWidgets::CreateSmallCircleFeatureDialog::handle_cancel()
{
	close();
}



void
GPlatesQtWidgets::CreateSmallCircleFeatureDialog::setup_properties_page()
{
	stacked_widget->setCurrentIndex(PROPERTIES_PAGE);
	button_previous->setEnabled(false);
	button_next->setEnabled(true);
	button_create->setEnabled(false);
}

void
GPlatesQtWidgets::CreateSmallCircleFeatureDialog::setup_collection_page()
{
	stacked_widget->setCurrentIndex(COLLECTION_PAGE);
	button_previous->setEnabled(true);
	button_next->setEnabled(false);
	button_create->setEnabled(true);

	d_choose_feature_collection_widget->initialise();
	d_choose_feature_collection_widget->setFocus();
}

void
GPlatesQtWidgets::CreateSmallCircleFeatureDialog::reset()
{
	setup_properties_page();
}

