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

#include "gui/ViewportProjection.h"


GPlatesQtWidgets::ProjectionControlWidget::ProjectionControlWidget(
		GPlatesGui::ViewportProjection &viewport_projection,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_viewport_projection(viewport_projection)
{
	setupUi(this);
	show_label(false);
	
	add_projection(tr("3D Orthographic"), GPlatesGui::MapProjection::ORTHOGRAPHIC, tr("Ctrl+1"));
	add_projection(tr("Rectangular"), GPlatesGui::MapProjection::RECTANGULAR, tr("Ctrl+2"));
	add_projection(tr("Mercator"), GPlatesGui::MapProjection::MERCATOR, tr("Ctrl+3"));
	add_projection(tr("Mollweide"), GPlatesGui::MapProjection::MOLLWEIDE, tr("Ctrl+4"));
	add_projection(tr("Robinson"), GPlatesGui::MapProjection::ROBINSON, tr("Ctrl+5"));
	
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
		GPlatesGui::MapProjection::Type projection_type,
		const QString &shortcut_key_sequence)
{
	// Add to the combo box.
	combo_projections->addItem(projection_text, projection_type);

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
	shortcut_action->setData(static_cast<uint>(projection_type));

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

	// Extract projection type from QVariant.
	const GPlatesGui::MapProjection::Type projection_type =
			static_cast<GPlatesGui::MapProjection::Type>(selected_projection_qv.toInt());
	
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			projection_type >= 0 && projection_type < GPlatesGui::MapProjection::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	// Set the projection type - it will also notify us of the change with its signal.
	d_viewport_projection.set_projection_type(projection_type);
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
	const GPlatesGui::MapProjection::Type projection_type =
			static_cast<GPlatesGui::MapProjection::Type>(shortcut_action->data().toUInt());

	// Set the projection type - it will also notify us of the change with its signal.
	d_viewport_projection.set_projection_type(projection_type);
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_projection_type_changed(
		const GPlatesGui::ViewportProjection &viewport_projection)
{
	QVariant selected_projection_qv(viewport_projection.get_projection_type());
	
	// Now we can quickly select the appropriate line of the combobox
	// by finding our projection ID (and not worrying about the text
	// label)
	const int idx = combo_projections->findData(selected_projection_qv);
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
