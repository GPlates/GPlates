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

#include <QAction>
#include <QVariant>

#include "ProjectionControlWidget.h"
#include "MapCanvas.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/Projection.h"


GPlatesQtWidgets::ProjectionControlWidget::ProjectionControlWidget(
		GPlatesGui::Projection &projection,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_projection(projection)
{
	setupUi(this);

	// First add the globe projections.
	for (unsigned int globe_projection_index = 0;
		globe_projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS;
		++globe_projection_index)
	{
		const GPlatesGui::GlobeProjection::Type globe_projection =
				static_cast<GPlatesGui::GlobeProjection::Type>(globe_projection_index);

		add_globe_map_projection(
				tr(GPlatesGui::GlobeProjection::get_display_name(globe_projection)),
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

		add_globe_map_projection(
				tr(GPlatesGui::MapProjection::get_display_name(map_projection_type)),
				// Add map projection indices *after* the globe projection indices...
				GPlatesGui::GlobeProjection::NUM_PROJECTIONS + map_projection_index);
	}

	// Now add the viewport projections.
	for (unsigned int viewport_projection_index = 0;
		viewport_projection_index < GPlatesGui::ViewportProjection::NUM_PROJECTIONS;
		++viewport_projection_index)
	{
		const GPlatesGui::ViewportProjection::Type viewport_projection =
				static_cast<GPlatesGui::ViewportProjection::Type>(viewport_projection_index);

		// Add to the combo box.
		combo_viewport_projection->addItem(
				tr(GPlatesGui::ViewportProjection::get_display_name(viewport_projection)),
				viewport_projection_index);
	}
	
	// Handle events from the user changing the combobox.
	QObject::connect(
			combo_globe_map_projection, SIGNAL(activated(int)),
			this, SLOT(handle_globe_map_projection_combo_changed(int)));
	QObject::connect(
			combo_viewport_projection, SIGNAL(activated(int)),
			this, SLOT(handle_viewport_projection_combo_changed(int)));
	
	// Listen for projection changes that may occur from some
	// other source, and update the combobox appropriately.
	QObject::connect(
			&d_projection, SIGNAL(globe_map_projection_changed(const GPlatesGui::Projection &)),
			this, SLOT(handle_globe_map_projection_changed(const GPlatesGui::Projection &)));
	QObject::connect(
			&d_projection, SIGNAL(viewport_projection_changed(const GPlatesGui::Projection &)),
			this, SLOT(handle_viewport_projection_changed(const GPlatesGui::Projection &)));
}


void
GPlatesQtWidgets::ProjectionControlWidget::add_globe_map_projection(
		const QString &projection_text,
		unsigned int projection_index)
{
	// Add to the combo box.
	combo_globe_map_projection->addItem(projection_text, projection_index);

	// Shortcut "Ctrl+1" for first projection, "Ctrl+2" for second, etc.
	const QString shortcut_key_sequence = QString("Ctrl+%1").arg(projection_index + 1);

	// Set the tooltip text for the current item.
	const QString combobox_item_tooltip_text = projection_text + " (" + shortcut_key_sequence + ")";
	combo_globe_map_projection->setItemData(
			combo_globe_map_projection->count() - 1,
			combobox_item_tooltip_text, Qt::ToolTipRole);

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
			this, SLOT(handle_globe_map_projection_shortcut_triggered()));
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_globe_map_projection_combo_changed(
		int idx)
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant selected_projection_qv = combo_globe_map_projection->itemData(idx);

	// Extract projection index from QVariant.
	const unsigned int selected_projection_index = selected_projection_qv.toUInt();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			selected_projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS + GPlatesGui::MapProjection::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	// Get current globe/map projection.
	GPlatesGui::Projection::globe_map_projection_type globe_map_projection = d_projection.get_globe_map_projection();

	// Set the projection type - it will also notify us of the change with its signal.
	if (selected_projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS)
	{
		// It's a globe projection.
		globe_map_projection.set_globe_projection_type(
				static_cast<GPlatesGui::GlobeProjection::Type>(selected_projection_index));
	}
	else // map projection...
	{
		globe_map_projection.set_map_projection_type(
				// Map projection indices come *after* the globe projection indices...
				static_cast<GPlatesGui::MapProjection::Type>(selected_projection_index - GPlatesGui::GlobeProjection::NUM_PROJECTIONS));
	}

	// Set globe/map projection.
	d_projection.set_globe_map_projection(globe_map_projection);
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_viewport_projection_combo_changed(
		int idx)
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant selected_projection_qv = combo_viewport_projection->itemData(idx);

	// Extract projection index from QVariant.
	const unsigned int selected_projection_index = selected_projection_qv.toUInt();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			selected_projection_index < GPlatesGui::ViewportProjection::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	// Set current viewport projection.
	GPlatesGui::Projection::viewport_projection_type viewport_projection =
			static_cast<GPlatesGui::ViewportProjection::Type>(selected_projection_index);
	d_projection.set_viewport_projection(viewport_projection);
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_globe_map_projection_shortcut_triggered()
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

	// Get current globe/map projection.
	GPlatesGui::Projection::globe_map_projection_type globe_map_projection = d_projection.get_globe_map_projection();

	// Set the projection type - it will also notify us of the change with its signal.
	if (projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS)
	{
		// It's a globe projection.
		globe_map_projection.set_globe_projection_type(
				static_cast<GPlatesGui::GlobeProjection::Type>(projection_index));
	}
	else // map projection...
	{
		globe_map_projection.set_map_projection_type(
				// Map projection indices come *after* the globe projection indices...
				static_cast<GPlatesGui::MapProjection::Type>(projection_index - GPlatesGui::GlobeProjection::NUM_PROJECTIONS));
	}

	// Set globe/map projection.
	d_projection.set_globe_map_projection(globe_map_projection);
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_globe_map_projection_changed(
		const GPlatesGui::Projection &projection)
{
	const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection = projection.get_globe_map_projection();

	// Get the projection index of the viewport projection (it's either a globe or map projection).
	unsigned int projection_index;
	if (globe_map_projection.is_viewing_globe_projection())
	{
		projection_index = globe_map_projection.get_globe_projection_type();
	}
	else // map projection...
	{
		// Map projection indices come *after* the globe projection indices.
		projection_index = GPlatesGui::GlobeProjection::NUM_PROJECTIONS + globe_map_projection.get_map_projection_type();
	}

	// Now we can quickly select the appropriate line of the combobox
	// by finding our projection ID (and not worrying about the text
	// label)
	const int idx = combo_globe_map_projection->findData(projection_index);
	if (idx != -1)
	{
		// This will not trigger activate() causing a infinite cycle
		// because we're setting this programmatically.
		combo_globe_map_projection->setCurrentIndex(idx);
	}
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_viewport_projection_changed(
		const GPlatesGui::Projection &projection)
{
	// Get the projection index of the viewport projection.
	const unsigned int projection_index = projection.get_viewport_projection();

	// Now we can quickly select the appropriate line of the combobox
	// by finding our projection ID (and not worrying about the text
	// label)
	const int idx = combo_viewport_projection->findData(projection_index);
	if (idx != -1)
	{
		// This will not trigger activate() causing a infinite cycle
		// because we're setting this programmatically.
		combo_viewport_projection->setCurrentIndex(idx);
	}
}
