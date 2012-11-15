/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2010, 2011 The University of Sydney, Australia
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

#include <boost/optional.hpp>
#include <QLocale>
#include <QDebug>

#include "FeatureSummaryWidget.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "gui/FeatureFocus.h"

#include "model/FeatureHandle.h"

#include "utils/UnicodeStringUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/FileInfo.h"

#include "global/InvalidFeatureCollectionException.h"
#include "global/InvalidParametersException.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/XsString.h"
#include "presentation/ViewState.h"

#include <boost/foreach.hpp>

namespace
{
	/**
	 * Borrowed from FeatureTableModel.cc.
	 */
	QString
	format_time_instant(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant)
	{
		QLocale locale;
		if (time_instant.time_position().is_real()) {
			return locale.toString(time_instant.time_position().value());
		} else if (time_instant.time_position().is_distant_past()) {
			return QObject::tr("past");
		} else if (time_instant.time_position().is_distant_future()) {
			return QObject::tr("future");
		} else {
			return QObject::tr("<invalid>");
		}
	}


	/**
	 * We now have four of these plate ID fields.
	 */
	void
	fill_plate_id_field(
			QLineEdit *field,
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			const GPlatesModel::PropertyName &property_name)
	{
		const GPlatesPropertyValues::GpmlPlateId *plate_id;
		if (GPlatesFeatureVisitors::get_property_value(
				feature_ref, property_name, plate_id))
		{
			// The feature has a plate ID of the desired kind.
			
			field->setText(QString::number(plate_id->value()));
		}
	}
	
	
	/**
	 * The slow way to test membership of a FeatureHandle in a FeatureCollection.
	 * It is ONLY okay to use here because we only select new features in response
	 * to a mouse-click.
	 */
	bool
	feature_collection_contains_feature(
			GPlatesModel::FeatureCollectionHandle::const_weak_ref collection_ref,
			GPlatesModel::FeatureHandle::const_weak_ref feature_ref)
	{
		// Weak refs. Check them.
		if ( ! collection_ref.is_valid()) {
			throw GPlatesGlobal::InvalidFeatureCollectionException(GPLATES_EXCEPTION_SOURCE,
					"Attempted to test for a feature inside an invalid feature collection.");
		}
		if ( ! feature_ref.is_valid()) {
			throw GPlatesGlobal::InvalidParametersException(GPLATES_EXCEPTION_SOURCE,
					"Attempted to test for an invalid feature inside a feature collection.");
		}
		
		// Search through FeatureCollection, comparing weakrefs until we find one that points
		// to the same FeatureHandle. I was going to use STL find but it bitched to me about
		// operator== not matching up quite right, presumably due to const weakref fun.
		GPlatesModel::FeatureCollectionHandle::const_iterator it = collection_ref->begin();
		GPlatesModel::FeatureCollectionHandle::const_iterator end = collection_ref->end();
		for (; it != end; ++it) {
			// 'it' is a revision aware iterator (intrusive ptr to FeatureHandle),
			const GPlatesModel::FeatureHandle &it_handle = **it;
			// Whereas we have a weakref to the FeatureHandle. We'll compare addresses
			// to determine if this is the feature handle we are looking for.
			if (feature_ref.handle_ptr() == &it_handle) {
				return true;
			}
		}
		// These aren't the feature handles we are looking for. Move along.
		return false;
	}
	
	/**
	 * The slow way to ascertain what File a particular Feature belongs to.
	 * Only checks FeatureCollections with loaded files, which is appropriate for the
	 * needs of FeatureSummaryWidget.
	 *
	 * If no match is found, the 'end' iterator is returned!
	 */
	boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
	get_file_reference_for_feature(
			GPlatesAppLogic::FeatureCollectionFileState &state,
			GPlatesModel::FeatureHandle::const_weak_ref feature_ref)
	{
		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
				state.get_loaded_files();
		BOOST_FOREACH(
				const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
				loaded_files)
		{
			GPlatesModel::FeatureCollectionHandle::const_weak_ref collection_ref =
					file_ref.get_file().get_feature_collection();
			
			if(!collection_ref.is_valid())
			{
				continue;
			}

			if (feature_collection_contains_feature(collection_ref, feature_ref)) {
				return file_ref;
			}
		}
		return boost::none;
	}
	
	/**
	 * Returns the name of the FeatureCollection that the given FeatureHandle is
	 * contained within.
	 *
	 * Needs FeatureCollectionFileState so we can scan through loaded files.
	 */
	QString
	get_feature_collection_name_for_feature(
			GPlatesAppLogic::FeatureCollectionFileState &file_state,
			GPlatesModel::FeatureHandle::const_weak_ref feature_ref)
	{
		boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference> file_ref = 
				get_file_reference_for_feature(file_state, feature_ref);
		if (!file_ref) {
			return QObject::tr("< Invalid Feature Collection >");
		}
		
		// Some files might not actually exist yet if the user created a new
		// feature collection internally and hasn't saved it to file yet.
		QString name;
		if (GPlatesFileIO::file_exists(file_ref->get_file().get_file_info()))
		{
			// Get a suitable label; we will prefer the short filename.
			name = file_ref->get_file().get_file_info().get_display_name(false);
		}
		else
		{
			// The file doesn't exist so give it a filename to indicate this.
			name = "New Feature Collection";
		}
		return name;
	}
}


GPlatesQtWidgets::FeatureSummaryWidget::FeatureSummaryWidget(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	TaskPanelWidget(parent_),
	d_file_state(view_state_.get_application_state().get_feature_collection_file_state()),
	d_feature_focus(view_state_.get_feature_focus())
{
	setupUi(this);
	clear();
	setDisabled(true);
	
	// Subscribe to focus events.
	QObject::connect(&d_feature_focus,
			SIGNAL(focus_changed(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(display_feature(GPlatesGui::FeatureFocus &)));
	QObject::connect(&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(display_feature(GPlatesGui::FeatureFocus &)));
}


void
GPlatesQtWidgets::FeatureSummaryWidget::clear()
{
	lineedit_type->clear();
	lineedit_name->clear();
	lineedit_plate_id->clear();
	lineedit_conjugate_plate_id->clear();
	lineedit_left_plate_id->clear();
	lineedit_right_plate_id->clear();
	lineedit_time_of_appearance->clear();
	lineedit_time_of_disappearance->clear();
	lineedit_clicked_geometry->clear();
	lineedit_feature_collection->clear();

	// Show/Hide some of the Plate ID fields depending on if they have anything
	// useful to report.
	hide_plate_id_fields_as_appropriate();
}


void
GPlatesQtWidgets::FeatureSummaryWidget::display_feature(
		GPlatesGui::FeatureFocus &feature_focus)
{
	const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature_focus.focused_feature();
	const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type associated_rg =
			feature_focus.associated_reconstruction_geometry();

	// Clear the fields first, then fill in those that we have data for.
	clear();

	emit_clear_action_enabled_changed(feature_ref.is_valid());

	// Always check your weak_refs!
	if ( ! feature_ref.is_valid()) {
		setDisabled(true);
		return;
	} else {
		setDisabled(false);
	}
	
	// Populate the widget from the FeatureHandle:
	
	// Feature Type.
	lineedit_type->setText(convert_qualified_xml_name_to_qstring(feature_ref->feature_type()));
	
	// Feature Name.
	// FIXME: Need to adapt according to user's current codeSpace setting.
	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *name;
	if (GPlatesFeatureVisitors::get_property_value(feature_ref, name_property_name, name))
	{
		// The feature has one or more name properties. Use the first one for now.
		lineedit_name->setText(GPlatesUtils::make_qstring(name->value()));
		lineedit_name->setCursorPosition(0);
		lineedit_name->setToolTip(GPlatesUtils::make_qstring(name->value()));
	}

	// Plate ID.
	static const GPlatesModel::PropertyName recon_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	fill_plate_id_field(lineedit_plate_id, feature_ref, recon_plate_id_property_name);

	// Conjugate Plate ID.
	// NOTE: Isochrons also have a 'conjugate' property, which is the proper
	// feature-centric reference to the twin of that Isochron, which no-one uses yet.
	// We also have a backwards-compatible PLATES4 header to think about.
	static const GPlatesModel::PropertyName conjugate_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("conjugatePlateId");
	fill_plate_id_field(lineedit_conjugate_plate_id, feature_ref, conjugate_plate_id_property_name);

	// Left Plate ID.
	static const GPlatesModel::PropertyName left_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("leftPlate");
	fill_plate_id_field(lineedit_left_plate_id, feature_ref, left_plate_id_property_name);

	// Right Plate ID.
	static const GPlatesModel::PropertyName right_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("rightPlate");
	fill_plate_id_field(lineedit_right_plate_id, feature_ref, right_plate_id_property_name);
	
	
	// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	const GPlatesPropertyValues::GmlTimePeriod *time_period;
	if (GPlatesFeatureVisitors::get_property_value(
			feature_ref, valid_time_property_name, time_period))
	{
		// The feature has a gml:validTime property.
		
		lineedit_time_of_appearance->setText(format_time_instant(*(time_period->begin())));
		lineedit_time_of_disappearance->setText(format_time_instant(*(time_period->end())));
	}

	if (associated_rg)
	{
		// There was an associated ReconstructionGeometry, which means there
		// was a clicked geometry.
		boost::optional<GPlatesModel::FeatureHandle::iterator> geometry_property =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
						associated_rg);
		if (geometry_property)
		{
			lineedit_clicked_geometry->setText(
					convert_qualified_xml_name_to_qstring((*geometry_property.get())->property_name()));
		}
		else
		{
			lineedit_clicked_geometry->setText(tr("<No longer valid>"));
		}
	}

	// Feature Collection's file name
	GPlatesModel::FeatureHandle::const_weak_ref const_feature_ref = feature_ref;
	QString feature_collection_name = get_feature_collection_name_for_feature(
			d_file_state, const_feature_ref);
	lineedit_feature_collection->setText(feature_collection_name);
	lineedit_feature_collection->setCursorPosition(0);
	lineedit_feature_collection->setToolTip(feature_collection_name);
	
	// Show/Hide some of the Plate ID fields depending on if they have anything
	// useful to report.
	hide_plate_id_fields_as_appropriate();
}


void
GPlatesQtWidgets::FeatureSummaryWidget::hide_plate_id_fields_as_appropriate()
{
	// Note that we'll always show the reconstuction "Plate ID" field, because it's
	// just so damn awesome.
	
	// Hide the Conjugate field if no data, show otherwise.
	bool has_conjugate_data = ! lineedit_conjugate_plate_id->text().isEmpty();
	lineedit_conjugate_plate_id->setVisible(has_conjugate_data);
	label_conjugate_plate_id->setVisible(has_conjugate_data);

	// Hide the Left Plate and Right Plate fields as a pair.
	bool has_left_data = ! lineedit_left_plate_id->text().isEmpty();
	bool has_right_data = ! lineedit_right_plate_id->text().isEmpty();

	lineedit_left_plate_id->setVisible(has_left_data || has_right_data);
	label_left_plate_id->setVisible(has_left_data || has_right_data);
	lineedit_right_plate_id->setVisible(has_left_data || has_right_data);
	label_right_plate_id->setVisible(has_left_data || has_right_data);
}


void
GPlatesQtWidgets::FeatureSummaryWidget::handle_activation()
{
}


QString
GPlatesQtWidgets::FeatureSummaryWidget::get_clear_action_text() const
{
	return tr("C&lear Selection");
}


bool
GPlatesQtWidgets::FeatureSummaryWidget::clear_action_enabled() const
{
	return d_feature_focus.is_valid();
}


void
GPlatesQtWidgets::FeatureSummaryWidget::handle_clear_action_triggered()
{
	d_feature_focus.unset_focus();
}

