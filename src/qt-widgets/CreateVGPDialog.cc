/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#include <QListWidget>
#include <QMessageBox>

#include "CreateVGPDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruct.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "model/Model.h"
#include "model/PropertyName.h"
#include "model/FeatureType.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "presentation/ViewState.h"

// FIXME: The following prop-vals can be removed 
// when the append... functionality is somewhere more appropriate.
#include "property-values/GmlPoint.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

namespace
{

	// FIXME: The following append.... functions are a duplicate of those in GmapReader's anonymouse namespace
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
							UnicodeString(description.toStdString().c_str()))));
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
			GPlatesModel::ModelUtils::create_gpml_constant_value(
			gml_point, 
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));

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
			GPlatesModel::ModelUtils::create_gpml_constant_value(
			gml_point, 
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));			


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
				GPlatesModel::ModelUtils::create_gpml_constant_value(
					gpml_plate_id,
					GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId"))));			
			
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


	/**
	 * Subclass of QListWidgetItem so that we can display a list of FeatureCollection in
	 * the list widget using the filename as the label, while keeping track of which
	 * list item corresponds to which FeatureCollection.
	 */
	class FeatureCollectionItem :
			public QListWidgetItem
	{
	public:
		// Standard constructor for creating FeatureCollection entry.
		FeatureCollectionItem(
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_iter,
				const QString &label):
			QListWidgetItem(label),
			d_file_iter(file_iter)
		{  }

		// Constructor for creating fake "Make a new Feature Collection" entry.
		FeatureCollectionItem(
				const QString &label):
			QListWidgetItem(label)
		{  }
			
		bool
		is_create_new_collection_item()
		{
			return !d_file_iter;
		}

		/**
		 * NOTE: Check with @a is_create_new_collection_item first and set a valid file
		 * iterator if necessary before calling this method.
		 */
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator
		get_file_iterator()
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_file_iter, GPLATES_ASSERTION_SOURCE);

			return *d_file_iter;
		}

		void
		set_file_iterator(
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_iter)
		{
			d_file_iter = file_iter;
		}
	
	private:
		boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_iterator> d_file_iter;
	};

	/**
	 * Fill the list with currently loaded FeatureCollections we can add the feature to.
	 */
	void
	populate_feature_collections_list(
			QListWidget &list_widget,
			GPlatesAppLogic::FeatureCollectionFileState &state)
	{
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range it_range =
				state.get_loaded_files();
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator it = it_range.begin;
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator end = it_range.end;
		
		list_widget.clear();
		for (; it != end; ++it) {
			// Get the FeatureCollectionHandle for this file.
			GPlatesModel::FeatureCollectionHandle::weak_ref collection_opt =
					it->get_feature_collection();

			// Some files might not actually exist yet if the user created a new
			// feature collection internally and hasn't saved it to file yet.
			QString label;
			if (GPlatesFileIO::file_exists(it->get_file_info()))
			{
				// Get a suitable label; we will prefer the full filename.
				label = it->get_file_info().get_display_name(true);
			}
			else
			{
				// The file doesn't exist so give it a filename to indicate this.
				label = "New Feature Collection";
			}
			
			// We are only interested in loaded files which have valid FeatureCollections.
			if (collection_opt.is_valid()) {
				list_widget.addItem(new FeatureCollectionItem(it, label));
			}
		}
		// Add a final option for creating a brand new FeatureCollection.
		list_widget.addItem(new FeatureCollectionItem(QObject::tr(" < Create a new Feature Collection > ")));
		// Default to first entry.
		list_widget.setCurrentRow(0);
	}
}

GPlatesQtWidgets::CreateVGPDialog::CreateVGPDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_model_ptr(view_state_.get_application_state().get_model_interface()),
	d_file_state(view_state_.get_application_state().get_feature_collection_file_state()),
	d_file_io(view_state_.get_application_state().get_feature_collection_file_io()),
	d_reconstruct_ptr(&view_state_.get_reconstruct())
{
	setupUi(this);
	
	reset();

	checkbox_site->setEnabled(true);
	checkbox_site->setChecked(false);
	frame_site_lat_lon->setEnabled(false);

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
	
	// Get the FeatureCollection the user has selected.
	FeatureCollectionItem *collection_item = dynamic_cast<FeatureCollectionItem *>(
		listwidget_feature_collections->currentItem());
	if (collection_item == NULL) {
		QMessageBox::critical(this, tr("No feature collection selected"),
			tr("Please select a feature collection to add the new feature to."));
		return;
	}
	GPlatesModel::FeatureCollectionHandle::weak_ref collection;

	boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_iterator> new_file;
	if (collection_item->is_create_new_collection_item()) {
		new_file.reset(d_file_io.create_empty_file());
		collection_item->set_file_iterator(*new_file); 
		collection = (*new_file)->get_feature_collection();
	} else {
		collection = collection_item->get_file_iterator()->get_feature_collection(); 
	} 		
	
	

	double vgp_lat = spinbox_pole_lat->value();
	double vgp_lon = spinbox_pole_lon->value();

		
	GPlatesModel::FeatureType feature_type = 
		GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");		

	// Actually create the Feature!
	GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(collection, feature_type);
		
	append_name_to_feature(feature,line_description->text());
	append_vgp_position_to_feature(feature,vgp_lat,vgp_lon);
	append_plate_id_to_feature(feature,GPlatesModel::integer_plate_id_type(spinbox_plate_id->value()));
	append_age_to_feature(feature,spinbox_age->value());
	
	if (checkbox_site->isChecked())
	{
		append_site_geometry_to_feature(feature,spinbox_site_lat->value(),spinbox_site_lon->value());
	}
	
	append_a95_to_feature(feature,spinbox_A95->value());

	// We've just modified the feature collection so let the feature collection file state
	// know this so it can reclassify it.
	// TODO: This is not ideal since we have to manually call this whenever a feature in
	// the feature collection is modified - remove this call when feature/feature-collection
	// callbacks have been implemented and then utilised inside FeatureCollectionFileState to
	// listen on feature collection changes.
	d_file_state.reclassify_feature_collection(collection_item->get_file_iterator());

	emit feature_created(feature);

	// If we've created a new feature collection, we need to tell the ViewportWindow's flowlines
	// class to update its feature collections. Other app-logic code may also require this. 
	// FIXME: the new fileIO/Workflow code may not need the same information....
	if (collection_item->is_create_new_collection_item() && new_file)
	{
		emit feature_collection_created(collection,*new_file);
	}

	accept();	
}

void
GPlatesQtWidgets::CreateVGPDialog::handle_cancel()
{
	close();
}

void
GPlatesQtWidgets::CreateVGPDialog::handle_site_checked(int state)
{
	frame_site_lat_lon->setEnabled(state);
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
	
	// Populate list of feature collections.
	// Note that this should also be done any time the user opens the dialog with
	// fresh geometry they wish to create a Feature with.
	populate_feature_collections_list(*listwidget_feature_collections, d_file_state);

	// Pushing Enter or double-clicking should cause the buttonbox to focus.
	QObject::connect(listwidget_feature_collections, SIGNAL(itemActivated(QListWidgetItem *)),
		button_create, SLOT(setFocus()));	
	
}

void
GPlatesQtWidgets::CreateVGPDialog::reset()
{
	setup_properties_page();
}

