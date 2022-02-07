/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include <boost/none.hpp>
#include <QHeaderView>
#include <QGridLayout>
#include <QVBoxLayout>

#include "EditFeaturePropertiesWidget.h"

#include "EditTimePeriodWidget.h"

#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "model/ModelUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "presentation/ViewState.h"


GPlatesQtWidgets::EditFeaturePropertiesWidget::EditFeaturePropertiesWidget(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_focus_ptr(&view_state_.get_feature_focus()),
	d_property_model_ptr(
			new GPlatesGui::FeaturePropertyTableModel(
					view_state_.get_feature_focus())),
	d_edit_widget_group_box_ptr(
			new GPlatesQtWidgets::EditWidgetGroupBox(
					view_state_, this)),
	d_add_property_dialog_ptr(
			new GPlatesQtWidgets::AddPropertyDialog(
					view_state_.get_feature_focus(), view_state_, this))
{
	setupUi(this);
	
	set_up_edit_widgets();
	
	property_table->setModel(d_property_model_ptr);
	property_table->verticalHeader()->hide();
	property_table->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	property_table->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
	property_table->horizontalHeader()->setHighlightSections(false);
	QObject::connect(property_table->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			this,
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));

	// Open the add property dialog.
	QObject::connect(button_add_property, SIGNAL(clicked()),
			d_add_property_dialog_ptr, SLOT(pop_up()));
	// Delete selected property.
	QObject::connect(button_delete_property, SIGNAL(clicked()),
			this, SLOT(delete_selected_property()));
	
	// Handle things without error if the feature we are looking at is deleted.
	QObject::connect(d_feature_focus_ptr, 
			SIGNAL(focused_feature_deleted(
					GPlatesGui::FeatureFocus &)),
			this,
			SLOT(handle_feature_deletion()));

	// Handle things gracefully if the feature we are looking at is modified.
#if 0
	// property table model should no longer be emitting this signal.
	// I think I'm going to take a leap of faith here and temporarily disable the call
	// to handle_model_change() entirely - I suspect it might not be necessary anymore,
	// now that EditFeaturePropertiesWidget has control of things.
	// Yes... we shouldn't need handle_model_change() at the moment as the only way of
	// changing a PropertyValue in the model is the edit widgets. Previously we added
	// this function to cope with a situation where we had an edit widget open with old
	// data and the user modifies things directly in the table cell. Since we can't
	// modify things via table cells right now, this doesn't matter,... for now.
	QObject::connect(d_property_model_ptr, SIGNAL(feature_modified(GPlatesModel::FeatureHandle::weak_ref)),
			this, SLOT(handle_model_change()));
#endif
}


void
GPlatesQtWidgets::EditFeaturePropertiesWidget::set_up_edit_widgets()
{
	// Add the EditWidgetGroupBox. Ugly, but this is the price to pay if you want
	// to mix Qt Designer UIs with coded-by-hand UIs.
	QVBoxLayout *edit_layout = new QVBoxLayout;
	edit_layout->setSpacing(0);
	edit_layout->setMargin(0);
	edit_layout->addWidget(d_edit_widget_group_box_ptr);
	placeholder_edit_widget->setLayout(edit_layout);
	
	QObject::connect(d_edit_widget_group_box_ptr, SIGNAL(commit_me()),
			this, SLOT(commit_edit_widget_data()));
	
	// A special case for the EditTimePeriodWidget: we need to change a conflicting
	// accelerator on the &End label.
	d_edit_widget_group_box_ptr->time_period_widget().label_end()->setText(
			tr("E&nd (time of disappearance):"));
}


void
GPlatesQtWidgets::EditFeaturePropertiesWidget::edit_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	if (feature_ref != d_feature_ref) {
		// Brand new feature to look at!
		// Clean up.
		commit_edit_widget_data();
		clean_up();
		
		// Load new data.
		d_property_model_ptr->set_feature_reference(feature_ref);
		d_feature_ref = feature_ref;

	} else {
		// A redisplay of the current feature!
		d_property_model_ptr->refresh_data();
	}

	// Update the Add Property Dialog.
	// NOTE: We do this regardless of whether the feature reference or type has changed or not
	// because the feature's existing properties may have changed and this affects the listing
	// of properties that can be added to the feature (due to allowed GPGIM property multiplicity).
	d_add_property_dialog_ptr->set_feature(feature_ref);
}


void
GPlatesQtWidgets::EditFeaturePropertiesWidget::handle_feature_deletion()
{
	// Clean up immediately without committing anything back to the model.
	d_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
	d_property_model_ptr->set_feature_reference(d_feature_ref);
	clean_up();
}



void
GPlatesQtWidgets::EditFeaturePropertiesWidget::handle_model_change()
{
	// If the focused feature has been modified, the QTableView probably already knows about it,
	// however the current EditWidget doesn't necessarily.
	// This can cause the edit widget to be out of sync with the table, causing hilarity
	// if you then click on another table row (causing the outdated data in the edit widget
	// to be re-committed over the top of the new data).
	// We need to update the edit widget to fix this before it becomes a problem.
	if (d_edit_widget_group_box_ptr->is_edit_widget_active()) {
		if (d_selected_property_iterator) {
			d_edit_widget_group_box_ptr->refresh_edit_widget(*d_selected_property_iterator);
		}
	}
}

// FIXME: This function does too many things and is too sensitive to the order in which
// things are done. Consider removing the d_selected_property_iterator thing entirely, for
// instance, and putting that logic into the edit widgets themselves.
void
GPlatesQtWidgets::EditFeaturePropertiesWidget::handle_selection_change(
		const QItemSelection &selected,
		const QItemSelection &deselected)
{
	// Disable things which depend on an item being selected.
	button_delete_property->setDisabled(true);

	// If an edit widget is currently displayed, we need to push its data into the
	// model before showing the next widget (if any).
	commit_edit_widget_data();

	d_selected_property_iterator = boost::none;
	if (selected.indexes().isEmpty()) {
		// No selection, exit early.
		return;
	}
	// We assume that the view has been constrained to allow only single-row selections,
	// so only concern ourselves with the first index in the list.
	QModelIndex idx = selected.indexes().first();
	if ( ! idx.isValid()) {
		return;
	}

	// We have a valid selection. Find out what it is!
	GPlatesModel::FeatureHandle::iterator it = 
			d_property_model_ptr->get_property_iterator_for_row(idx.row());
	
	// Enable things which depend on an item being selected.
	button_delete_property->setDisabled(false);
	
	d_edit_widget_group_box_ptr->activate_appropriate_edit_widget(it);
	d_selected_property_iterator = it;
}


void
GPlatesQtWidgets::EditFeaturePropertiesWidget::delete_selected_property()
{
	if ( ! property_table->selectionModel()->hasSelection()) {
		return;
	}
	// We assume that the view has been constrained to allow only single-row selections,
	// so only concern ourselves with the first index in the list.
	QModelIndex idx = property_table->selectionModel()->selection().indexes().first();
	if ( ! idx.isValid()) {
		return;
	}
	
	// We have a valid selection. Find out what it is!
	GPlatesModel::FeatureHandle::iterator it = 
			d_property_model_ptr->get_property_iterator_for_row(idx.row());

	// Clear the selection beforehand, or we could end up in trouble.
	d_edit_widget_group_box_ptr->deactivate_edit_widgets();
	property_table->selectionModel()->clear();

	if ( ! d_feature_ref.is_valid()) {
		// Nothing can be done.
		return;
	}

	// FIXME: UNDO
	// Delete the property container for the given iterator.
	d_feature_ref->remove(it);

	// We have just changed the model. Tell anyone who cares to know.
	// This will cause FeaturePropertyTableModel to refresh_data(), amongst other things.
	d_feature_focus_ptr->announce_modification_of_focused_feature();
}


void
GPlatesQtWidgets::EditFeaturePropertiesWidget::clean_up()
{
	// Get widgets ready for the next feature, if any.
	property_table->selectionModel()->clear();
	d_edit_widget_group_box_ptr->deactivate_edit_widgets();
	d_add_property_dialog_ptr->reject();
}


void
GPlatesQtWidgets::EditFeaturePropertiesWidget::commit_edit_widget_data()
{
	if ( ! d_feature_ref.is_valid()) {
		return;
	}
	if ( ! d_selected_property_iterator) {
		return;
	}
	
	if (d_edit_widget_group_box_ptr->is_edit_widget_active()) {
		if (d_edit_widget_group_box_ptr->is_dirty()) {
			// FIXME: UNDO
			// Edit PropertyValues in the model by modifying them in-place.
			bool modified = d_edit_widget_group_box_ptr->update_property_value_from_widget();
			// As we are no longer going through FeaturePropertyTableModel to make this change,
			// we should notify others of the modification.
			if (modified) {
				// As something was actually modified, we should let others know about it.
				d_feature_focus_ptr->announce_modification_of_focused_feature();
			} else {
				// No actual modification took place, and we SHOULD NOT TELL ANYONE ELSE OTHERWISE!
				// Otherwise we can hit a nasty Signal/Slot loop pretty easily.
			}
		}
	}
}




