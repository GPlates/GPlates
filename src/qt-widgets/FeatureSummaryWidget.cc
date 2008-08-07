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

#include "FeatureSummaryWidget.h"

#include "gui/FeatureFocus.h"
#include "model/FeatureHandle.h"
#include "utils/UnicodeStringUtils.h"
#include "feature-visitors/PlateIdFinder.h"
#include "feature-visitors/GmlTimePeriodFinder.h"
#include "feature-visitors/XsStringFinder.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GeoTimeInstant.h"


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
	lineedit_time_of_appearance->clear();
	lineedit_time_of_disappearance->clear();
	lineedit_clicked_geometry->clear();
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
	GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
	string_finder.visit_feature_handle(*feature_ref);
	if (string_finder.found_strings_begin() != string_finder.found_strings_end()) {
		// The feature has one or more name properties. Use the first one for now.
		GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name = 
				*string_finder.found_strings_begin();
		
		lineedit_name->setText(GPlatesUtils::make_qstring(name->value()));
		lineedit_name->setCursorPosition(0);
	}

	// Plate ID.
	static const GPlatesModel::PropertyName plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	GPlatesFeatureVisitors::PlateIdFinder plate_id_finder(plate_id_property_name);
	plate_id_finder.visit_feature_handle(*feature_ref);
	if (plate_id_finder.found_plate_ids_begin() != plate_id_finder.found_plate_ids_end()) {
		// The feature has a reconstruction plate ID.
		GPlatesModel::integer_plate_id_type recon_plate_id =
				*plate_id_finder.found_plate_ids_begin();
		
		lineedit_plate_id->setText(QString::number(recon_plate_id));
	}
	
	// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");
	GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(valid_time_property_name);
	time_period_finder.visit_feature_handle(*feature_ref);
	if (time_period_finder.found_time_periods_begin() != time_period_finder.found_time_periods_end()) {
		// The feature has a gml:validTime property.
		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
				*time_period_finder.found_time_periods_begin();
		
		lineedit_time_of_appearance->setText(format_time_instant(*(time_period->begin())));
		lineedit_time_of_disappearance->setText(format_time_instant(*(time_period->end())));
	}

	if (associated_rfg) {
		// There was an associated Reconstructed Feature Geometry (RFG), which means there
		// was a clicked geometry.
		GPlatesModel::FeatureHandle::properties_iterator geometry_property =
				associated_rfg->property();
		lineedit_clicked_geometry->setText(
				GPlatesUtils::make_qstring_from_icu_string(
					(*geometry_property)->property_name().build_aliased_name()));
	}
}


