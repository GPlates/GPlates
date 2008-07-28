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

#include <QString>
#include "FeaturePropertiesDialog.h"

#include "utils/UnicodeStringUtils.h"
#include "model/FeatureType.h"
#include "model/FeatureId.h"
#include "model/FeatureRevision.h"


GPlatesQtWidgets::FeaturePropertiesDialog::FeaturePropertiesDialog(
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesGui::FeatureFocus &feature_focus,
		QWidget *parent_):
	QDialog(parent_),
	d_query_feature_properties_widget(new GPlatesQtWidgets::QueryFeaturePropertiesWidget(
			view_state_, feature_focus, this)),
	d_edit_feature_properties_widget(new GPlatesQtWidgets::EditFeaturePropertiesWidget(
			view_state_, feature_focus, this)),
	d_edit_feature_geometries_widget(new GPlatesQtWidgets::EditFeatureGeometriesWidget(
			view_state_, feature_focus, this))
{
	setupUi(this);
	
	// Set up the tab widget. Note we have to delete the 'dummy' tab set up by the Designer.
	tabwidget_query_edit->clear();
	tabwidget_query_edit->addTab(d_query_feature_properties_widget,
			QIcon(":/gnome_edit_find_16.png"), tr("&Query Properties"));
	tabwidget_query_edit->addTab(d_edit_feature_properties_widget,
			QIcon(":/gnome_gtk_edit_16.png"), tr("&Edit Properties"));
	tabwidget_query_edit->addTab(d_edit_feature_geometries_widget,
			QIcon(":/gnome_stock_edit_points_16.png"), tr("View &Coordinates"));
	tabwidget_query_edit->setCurrentIndex(0);

	// Handle tab changes.
	QObject::connect(tabwidget_query_edit, SIGNAL(currentChanged(int)),
			this, SLOT(handle_tab_change(int)));
	
	// Handle focus changes.
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
	// Note: In the future, we may have a "feature was deleted" event we should listen to.
	// This should be connected to refresh_display(), possibly indirectly via some
	// when_feature_deleted_refresh_if_necessary(weakref) slot.
	
	// Refresh display - since the feature ref is invalid at this point,
	// the dialog should lock everything down that might otherwise cause problems.
	refresh_display();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)
{
	d_feature_ref = feature_ref;
	refresh_display();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::refresh_display()
{
	if ( ! d_feature_ref.is_valid()) {
		// Disable everything except the Close button.
		lineedit_feature_type->setEnabled(false);
		tabwidget_query_edit->setEnabled(false);
		return;
	}
	
	// Ensure everything is enabled.
	lineedit_feature_type->setEnabled(true);
	tabwidget_query_edit->setEnabled(true);
	
	// Update our text fields at the top.
	lineedit_feature_type->setText(
			GPlatesUtils::make_qstring_from_icu_string(d_feature_ref->feature_type().build_aliased_name()));
	
	// Update our tabbed sub-widgets.
	d_query_feature_properties_widget->display_feature(d_feature_ref);
	d_edit_feature_properties_widget->edit_feature(d_feature_ref);
	d_edit_feature_geometries_widget->edit_feature(d_feature_ref);
}

		
void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_query_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_query_feature_properties_widget);
	setVisible(true);
}

void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_edit_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_edit_feature_properties_widget);
	setVisible(true);
}

void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_geometries_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_edit_feature_geometries_widget);
	setVisible(true);
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::setVisible(bool visible)
{
	if ( ! visible) {
		// We are closing. Ensure things are left tidy.
		d_edit_feature_properties_widget->commit_and_clean_up();
	}
	QDialog::setVisible(visible);
}



void
GPlatesQtWidgets::FeaturePropertiesDialog::handle_tab_change(
		int index)
{
	const QIcon icon = tabwidget_query_edit->tabIcon(index);
	setWindowIcon(icon);
}

