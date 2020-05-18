/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <QVariant>

#include "ProjectionControlWidget.h"
#include "MapCanvas.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/ViewportProjection.h"


GPlatesQtWidgets::ProjectionControlWidget::ProjectionControlWidget(
		GPlatesGui::ViewportProjection &viewport_projection,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_viewport_projection(viewport_projection)
{
	setupUi(this);
	show_label(false);

	// First add the globe projections.
	for (unsigned int globe_projection_index = 0;
		globe_projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS;
		++globe_projection_index)
	{
		const GPlatesGui::GlobeProjection::Type globe_projection_type =
				static_cast<GPlatesGui::GlobeProjection::Type>(globe_projection_index);

		add_projection(
				tr(GPlatesGui::GlobeProjection::get_display_name(globe_projection_type)),
				globe_projection_index);
	}

	// Then add the map projections.
	// NOTE: The map projections will have combo box indices after the globe projections.
	for (unsigned int map_projection_index = 0;
		map_projection_index < GPlatesGui::MapProjection::NUM_PROJECTIONS;
		++map_projection_index)
	{
		const GPlatesGui::MapProjection::Type map_projection_type =
				static_cast<GPlatesGui::MapProjection::Type>(map_projection_index);

		add_projection(
				tr(GPlatesGui::MapProjection::get_display_name(map_projection_type)),
				// Add map projection indices *after* the globe projection indices...
				GPlatesGui::GlobeProjection::NUM_PROJECTIONS + map_projection_index);
	}
	
	// Handle events from the user changing the combobox.
	QObject::connect(combo_projections, SIGNAL(activated(int)),
			this, SLOT(handle_combobox_changed(int)));
	
	// Listen for projection changes that may occur from some
	// other source, and update the combobox appropriately.
	QObject::connect(
			&d_viewport_projection, SIGNAL(projection_type_changed(const GPlatesGui::ViewportProjection &)),
			this, SLOT(handle_projection_type_changed(const GPlatesGui::ViewportProjection &)));
}


void
GPlatesQtWidgets::ProjectionControlWidget::add_projection(
		const QString &projection_text,
		unsigned int projection_index)
{
	// Add to the combo box.
	combo_projections->addItem(projection_text, projection_index);

	// Shortcut "Ctrl+1" for first projection, "Ctrl+2" for second, etc.
	const QString shortcut_key_sequence = QString("Ctrl+%1").arg(projection_index + 1);

	// Set the tooltip text for the current item.
	const QString combobox_item_tooltip_text = projection_text + " (" + shortcut_key_sequence + ")";
	combo_projections->setItemData(combo_projections->count() - 1, combobox_item_tooltip_text, Qt::ToolTipRole);

	// Create a QAction purely so we have a global shortcut.
	// The QAction.

	// Create a QAction around the specified shortcut key sequence.
	QAction *shortcut_action = new QAction(this);
	shortcut_action->setShortcut(shortcut_key_sequence);
	// Set the shortcut to be active when any applications windows are active.
	// This makes it easier to change the projection when the main window is not currently in focus.
	shortcut_action->setShortcutContext(Qt::ApplicationShortcut);

	// Add the shortcut QAction to this projection widget so it becomes active
	// (since this projection widget is always visible).
	// NOTE: There's no way for the user to select these actions other than through shortcuts.
	addAction(shortcut_action);

	// Set some data on the QAction so we know which projection it corresponds to when triggered.
	shortcut_action->setData(projection_index);

	// Call handler when action is triggered.
	QObject::connect(
			shortcut_action, SIGNAL(triggered()),
			this, SLOT(handle_shortcut_triggered()));
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_combobox_changed(
		int idx)
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant selected_projection_qv = combo_projections->itemData(idx);

	// Extract projection index from QVariant.
	const unsigned int selected_projection_index = selected_projection_qv.toUInt();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			selected_projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS + GPlatesGui::MapProjection::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	// Set the projection type - it will also notify us of the change with its signal.
	if (selected_projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS)
	{
		// It's a globe projection.
		d_viewport_projection.set_globe_projection_type(
				static_cast<GPlatesGui::GlobeProjection::Type>(selected_projection_index));
	}
	else // map projection...
	{
		d_viewport_projection.set_map_projection_type(
			// Map projection indices come *after* the globe projection indices...
			static_cast<GPlatesGui::MapProjection::Type>(selected_projection_index - GPlatesGui::GlobeProjection::NUM_PROJECTIONS));
	}
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_shortcut_triggered()
{
	// Get the QObject that triggered this slot.
	QObject *signal_sender = sender();
	// Return early in case this slot not activated by a signal - shouldn't happen.
	if (!signal_sender)
	{
		return;
	}

	// Cast to a QAction since only QAction objects trigger this slot.
	QAction *shortcut_action = qobject_cast<QAction *>(signal_sender);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			shortcut_action,
			GPLATES_ASSERTION_SOURCE);

	// Determine the projection to activate.
	const unsigned int projection_index = shortcut_action->data().toInt();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS + GPlatesGui::MapProjection::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	// Set the projection type - it will also notify us of the change with its signal.
	if (projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS)
	{
		// It's a globe projection.
		d_viewport_projection.set_globe_projection_type(
				static_cast<GPlatesGui::GlobeProjection::Type>(projection_index));
	}
	else // map projection...
	{
		d_viewport_projection.set_map_projection_type(
				// Map projection indices come *after* the globe projection indices...
				static_cast<GPlatesGui::MapProjection::Type>(projection_index - GPlatesGui::GlobeProjection::NUM_PROJECTIONS));
	}
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_projection_type_changed(
		const GPlatesGui::ViewportProjection &viewport_projection)
{
	// Get the projection index of the viewport projection (it's either a globe or map projection).
	unsigned int projection_index;
	if (boost::optional<GPlatesGui::GlobeProjection::Type> globe_projection_type =
		viewport_projection.get_globe_projection_type())
	{
		projection_index = globe_projection_type.get();
	}
	else // map projection...
	{
		boost::optional<GPlatesGui::MapProjection::Type> map_projection_type =
				viewport_projection.get_map_projection_type();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				map_projection_type,
				GPLATES_ASSERTION_SOURCE);

		// Map projection indices come *after* the globe projection indices.
		projection_index = GPlatesGui::GlobeProjection::NUM_PROJECTIONS + map_projection_type.get();
	}

	// Now we can quickly select the appropriate line of the combobox
	// by finding our projection ID (and not worrying about the text
	// label)
	const int idx = combo_projections->findData(projection_index);
	if (idx != -1) {
		// This will not trigger activate() causing a infinite cycle
		// because we're setting this programmatically.
		combo_projections->setCurrentIndex(idx);
	}
}

void
GPlatesQtWidgets::ProjectionControlWidget::show_label(
	bool show_)
{
	label_projections->setVisible(show_);
}
