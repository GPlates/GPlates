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

#include "model/FeatureType.h"
#include "model/FeatureId.h"
#include "model/FeatureRevision.h"
#include "utils/UnicodeStringUtils.h"
#include "presentation/ViewState.h"


GPlatesQtWidgets::FeaturePropertiesDialog::FeaturePropertiesDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_query_feature_properties_widget(new GPlatesQtWidgets::QueryFeaturePropertiesWidget(
			view_state_, this)),
	d_edit_feature_properties_widget(new GPlatesQtWidgets::EditFeaturePropertiesWidget(
			view_state_, this)),
	d_view_feature_geometries_widget(new GPlatesQtWidgets::ViewFeatureGeometriesWidget(
			view_state_, this))
{
	setupUi(this);
	
	// Set up the tab widget. Note we have to delete the 'dummy' tab set up by the Designer.
	tabwidget_query_edit->clear();
	tabwidget_query_edit->addTab(d_query_feature_properties_widget,
			QIcon(":/gnome_edit_find_16.png"), tr("&Query Properties"));
	tabwidget_query_edit->addTab(d_edit_feature_properties_widget,
			QIcon(":/gnome_gtk_edit_16.png"), tr("&Edit Properties"));
	tabwidget_query_edit->addTab(d_view_feature_geometries_widget,
			QIcon(":/gnome_stock_edit_points_16.png"), tr("View &Coordinates"));
	tabwidget_query_edit->setCurrentIndex(0);

	// Handle tab changes.
	QObject::connect(tabwidget_query_edit, SIGNAL(currentChanged(int)),
			this, SLOT(handle_tab_change(int)));
	
	// Handle focus changes.
	QObject::connect(&view_state_.get_feature_focus(), 
			SIGNAL(focus_changed(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			this,
			SLOT(display_feature(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)));
	QObject::connect(&view_state_.get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			this,
			SLOT(display_feature(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)));
	
	// Refresh display - since the feature ref is invalid at this point,
	// the dialog should lock everything down that might otherwise cause problems.
	refresh_display();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type focused_rg)
{
	d_feature_ref = feature_ref;
	d_focused_rg = focused_rg;

	refresh_display();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::refresh_display()
{
	if ( ! d_feature_ref.is_valid()) {
		// Disable everything except the Close button.
		lineedit_feature_type->setEnabled(false);
		tabwidget_query_edit->setEnabled(false);
		lineedit_feature_type->clear();
		return;
	}

	//PROFILE_FUNC();

	// Ensure everything is enabled.
	lineedit_feature_type->setEnabled(true);
	tabwidget_query_edit->setEnabled(true);
	
	// Update our text fields at the top.
	lineedit_feature_type->setText(
			GPlatesUtils::make_qstring_from_icu_string(d_feature_ref->feature_type().build_aliased_name()));
	
	// Update our tabbed sub-widgets.
	d_query_feature_properties_widget->display_feature(d_feature_ref, d_focused_rg);
	d_edit_feature_properties_widget->edit_feature(d_feature_ref);
	d_view_feature_geometries_widget->edit_feature(d_feature_ref, d_focused_rg);
}

		
void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_query_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_query_feature_properties_widget);
	pop_up();
}

void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_edit_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_edit_feature_properties_widget);
	pop_up();
}

void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_geometries_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_view_feature_geometries_widget);
	pop_up();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::pop_up()
{
	show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	raise();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::setVisible(bool visible)
{
	if ( ! visible) {
		// We are closing. Ensure things are left tidy.
		d_edit_feature_properties_widget->commit_edit_widget_data();
		d_edit_feature_properties_widget->clean_up();
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

