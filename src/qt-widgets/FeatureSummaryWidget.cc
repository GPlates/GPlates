/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <QLocale>
#include <QDebug>

#include "FeatureSummaryWidget.h"

#include "gui/FeatureFocus.h"
#include "model/FeatureHandle.h"
#include "utils/UnicodeStringUtils.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/XsString.h"


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
				*feature_ref, property_name, plate_id))
		{
			// The feature has a plate ID of the desired kind.
			
			field->setText(QString::number(plate_id->value()));
		}
	}
}


GPlatesQtWidgets::FeatureSummaryWidget::FeatureSummaryWidget(
		GPlatesGui::FeatureFocus &feature_focus,
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);
	clear();
	setDisabled(true);
	
	// Subscribe to focus events. We can discard the FeatureFocus reference afterwards.
	QObject::connect(&feature_focus,
			SIGNAL(focus_changed(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this,
			SLOT(display_feature(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));
	QObject::connect(&feature_focus,
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this,
			SLOT(display_feature(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));
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

	// Show/Hide some of the Plate ID fields depending on if they have anything
	// useful to report.
	hide_plate_id_fields_as_appropriate();
}


void
GPlatesQtWidgets::FeatureSummaryWidget::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
	// Clear the fields first, then fill in those that we have data for.
	clear();
	// Always check your weak_refs!
	if ( ! feature_ref.is_valid()) {
		setDisabled(true);
		return;
	} else {
		setDisabled(false);
	}
	
	// Populate the widget from the FeatureHandle:
	
	// Feature Type.
	lineedit_type->setText(GPlatesUtils::make_qstring_from_icu_string(
			feature_ref->feature_type().build_aliased_name()));
	
	// Feature Name.
	// FIXME: Need to adapt according to user's current codeSpace setting.
	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *name;
	if (GPlatesFeatureVisitors::get_property_value(*feature_ref, name_property_name, name))
	{
		// The feature has one or more name properties. Use the first one for now.
		lineedit_name->setText(GPlatesUtils::make_qstring(name->value()));
		lineedit_name->setCursorPosition(0);
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
			*feature_ref, valid_time_property_name, time_period))
	{
		// The feature has a gml:validTime property.
		
		lineedit_time_of_appearance->setText(format_time_instant(*(time_period->begin())));
		lineedit_time_of_disappearance->setText(format_time_instant(*(time_period->end())));
	}

	if (associated_rfg) {
		// There was an associated Reconstructed Feature Geometry (RFG), which means there
		// was a clicked geometry.
		GPlatesModel::FeatureHandle::properties_iterator geometry_property =
				associated_rfg->property();
		if (*geometry_property != NULL) {
			lineedit_clicked_geometry->setText(
					GPlatesUtils::make_qstring_from_icu_string(
						(*geometry_property)->property_name().build_aliased_name()));
		} else {
			lineedit_clicked_geometry->setText(tr("<No longer valid>"));
		}
	}
	
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

