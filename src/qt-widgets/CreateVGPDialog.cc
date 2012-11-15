/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "CreateVGPDialog.h"

#include "ChooseFeatureCollectionWidget.h"
#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/Model.h"
#include "model/PropertyName.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"

#include "presentation/ViewState.h"

// FIXME: The following prop-vals can be removed 
// when the append... functionality is somewhere more appropriate.
#include "property-values/GmlPoint.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

namespace
{

	// FIXME: The following append.... functions are a duplicate of those in GmapReader's anonymous namespace
	// These should be put somewhere accessible by both the GmapReader and CreateVGPFeature.
	void
	append_name_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const QString &description)
	{
		
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("name"),
					GPlatesPropertyValues::XsString::create(
							GPlatesUtils::UnicodeString(description.toStdString().c_str()))));
	}	

	void
	append_site_geometry_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &latitude,
		const float &longitude)
	{
		GPlatesMaths::LatLonPoint llp(latitude,longitude);
		GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(llp);

		const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
			GPlatesPropertyValues::GmlPoint::create(point);

		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(gml_point);

		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition"),
				property_value)
			);
	}

	void
	append_inclination_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &inclination)	
	{
		
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("averageInclination"),
				GPlatesPropertyValues::XsDouble::create(inclination)));		
			
	}	

	void
	append_declination_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &declination)	
	{

		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
			GPlatesModel::PropertyName::create_gpml("averageDeclination"),
			GPlatesPropertyValues::XsDouble::create(declination)));					
	}	

	void
	append_a95_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &a95)	
	{
		
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("poleA95"),
				GPlatesPropertyValues::XsDouble::create(a95)));				
	}

	void
	append_age_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &age)
	{
		
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("averageAge"),
				GPlatesPropertyValues::XsDouble::create(age)));			

	}

	void
	append_vgp_position_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &vgp_latitude,
		const float &vgp_longitude)	
	{

		GPlatesMaths::LatLonPoint llp(vgp_latitude,vgp_longitude);
		GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(llp);

		const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
			GPlatesPropertyValues::GmlPoint::create(point);	

		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(gml_point);			


		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("polePosition"),
				property_value));					
					
	}

	void
	append_plate_id_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const GPlatesModel::integer_plate_id_type &plate_id)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(plate_id);

		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
				GPlatesModel::ModelUtils::create_gpml_constant_value(gpml_plate_id)));			
			
	}

	void
	append_dm_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &dm)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_dm = 
			GPlatesPropertyValues::XsDouble::create(dm);

		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("poleDm"),
				gpml_dm));			
			
	}

	void
	append_dp_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &dp)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_dp = 
			GPlatesPropertyValues::XsDouble::create(dp);

		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
			GPlatesModel::PropertyName::create_gpml("poleDp"),
			gpml_dp));	
	}	
}

GPlatesQtWidgets::CreateVGPDialog::CreateVGPDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	GPlatesDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_model_ptr(view_state_.get_application_state().get_model_interface()),
	d_file_state(view_state_.get_application_state().get_feature_collection_file_state()),
	d_file_io(view_state_.get_application_state().get_feature_collection_file_io()),
	d_application_state_ptr(&view_state_.get_application_state()),
	d_choose_feature_collection_widget(
			new ChooseFeatureCollectionWidget(
				view_state_.get_application_state().get_reconstruct_method_registry(),
				d_file_state,
				d_file_io,
				this))
{
	setupUi(this);

	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_collection_widget,
			widget_choose_feature_collection_placeholder);
	
	reset();

	checkbox_site->setEnabled(true);
	checkbox_site->setChecked(false);
	handle_site_checked(0);

	setup_connections();
}

void
GPlatesQtWidgets::CreateVGPDialog::setup_connections()
{
	QObject::connect(button_previous,SIGNAL(clicked()),this,SLOT(handle_previous()));
	QObject::connect(button_next,SIGNAL(clicked()),this,SLOT(handle_next()));
	QObject::connect(button_create,SIGNAL(clicked()),this,SLOT(handle_create()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(handle_cancel()));
	QObject::connect(checkbox_site,SIGNAL(stateChanged(int)),this,SLOT(handle_site_checked(int)));				

	// Pushing Enter or double-clicking should cause the create button to focus.
	QObject::connect(d_choose_feature_collection_widget, SIGNAL(item_activated()),
		button_create, SLOT(setFocus()));

	QObject::connect(this,SIGNAL(feature_created()),
			d_application_state_ptr,SLOT(reconstruct()));
}

void
GPlatesQtWidgets::CreateVGPDialog::handle_previous()
{
	// If we're on the "add_feature" page, go back.
	if (stacked_widget->currentIndex() == COLLECTION_PAGE)
	{
		setup_properties_page();
	}
}

void
GPlatesQtWidgets::CreateVGPDialog::handle_next()
{
	if (stacked_widget->currentIndex() == PROPERTIES_PAGE)
	{
		setup_collection_page();
	}
}

void
GPlatesQtWidgets::CreateVGPDialog::handle_create()
{
	try
	{
		// We want to merge model events across this scope so that only one model event
		// is generated instead of many as we incrementally modify the feature below.
		GPlatesModel::NotificationGuard model_notification_guard(
				d_application_state_ptr->get_model_interface().access_model());

		// Get the FeatureCollection the user has selected.
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> collection_file_iter =
			d_choose_feature_collection_widget->get_file_reference();
		GPlatesModel::FeatureCollectionHandle::weak_ref collection =
			(collection_file_iter.first).get_file().get_feature_collection();

		double vgp_lat = spinbox_pole_lat->value();
		double vgp_lon = spinbox_pole_lon->value();
			
		GPlatesModel::FeatureType feature_type = 
			GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");		

		// Actually create the Feature!
		GPlatesModel::FeatureHandle::weak_ref feature =
			GPlatesModel::FeatureHandle::create(collection, feature_type);
			
		append_name_to_feature(
				feature,
				line_description->text());
		append_vgp_position_to_feature(
				feature,
				vgp_lat,
				vgp_lon);
		append_plate_id_to_feature(
				feature,
				GPlatesModel::integer_plate_id_type(spinbox_plate_id->value()));
		append_age_to_feature(
				feature,
				spinbox_age->value());
		
		if (checkbox_site->isChecked())
		{
			append_site_geometry_to_feature(
					feature,
					spinbox_site_lat->value(),
					spinbox_site_lon->value());
		}
		
		append_a95_to_feature(feature,spinbox_A95->value());
		
		// Release the model notification guard now that we've finished modifying the feature.
		// Provided there are no nested guards this should notify model observers.
		// We want any observers to see the changes before we emit signals because we don't
		// know whose listening on those signals and they may be expecting model observers to
		// be up-to-date with the modified model.
		// Also this should be done before getting the application state reconstructs which
		// happens when the guard is released (because we modified the model).
		model_notification_guard.release_guard();

		// To trigger a reconstruction.
		emit feature_created();
	
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
GPlatesQtWidgets::CreateVGPDialog::handle_cancel()
{
	close();
}

void
GPlatesQtWidgets::CreateVGPDialog::handle_site_checked(int state)
{
	label_site_lat->setEnabled(state);
	spinbox_site_lat->setEnabled(state);
	label_site_lon->setEnabled(state);
	spinbox_site_lon->setEnabled(state);
}

void
GPlatesQtWidgets::CreateVGPDialog::setup_properties_page()
{
	stacked_widget->setCurrentIndex(PROPERTIES_PAGE);
	button_previous->setEnabled(false);
	button_next->setEnabled(true);
	button_create->setEnabled(false);
}

void
GPlatesQtWidgets::CreateVGPDialog::setup_collection_page()
{
	stacked_widget->setCurrentIndex(COLLECTION_PAGE);
	button_previous->setEnabled(true);
	button_next->setEnabled(false);
	button_create->setEnabled(true);

	d_choose_feature_collection_widget->initialise();
	d_choose_feature_collection_widget->setFocus();
}

void
GPlatesQtWidgets::CreateVGPDialog::reset()
{
	setup_properties_page();
}

