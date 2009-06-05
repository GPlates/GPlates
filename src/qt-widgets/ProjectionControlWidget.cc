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

#include "ProjectionControlWidget.h"

#include "MapCanvas.h"
#include "gui/MapProjection.h"

#include <QVariant>

GPlatesQtWidgets::ProjectionControlWidget::ProjectionControlWidget(
		MapCanvas *map_canvas_ptr,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_map_canvas_ptr(map_canvas_ptr)
{
	setupUi(this);
	show_label(false);
	
	// TODO: pass a second param to addItem to ID each projection type.
	// As the text on the combobox is translated, we probably shouldn't react
	// to the text changing directly; instead, we can embed some data for
	// each combobox choice (perhaps an enumeration) via this form of
	// addItem() : http://doc.trolltech.com/4.3/qcombobox.html#addItem
	combo_projections->addItem(tr("3D Globe"),GPlatesGui::ORTHOGRAPHIC);
	combo_projections->addItem(tr("Rectangular"),GPlatesGui::RECTANGULAR);
	combo_projections->addItem(tr("Mercator"),GPlatesGui::MERCATOR);
	combo_projections->addItem(tr("Mollweide"),GPlatesGui::MOLLWEIDE);
	combo_projections->addItem(tr("Robinson"),GPlatesGui::ROBINSON);
	
	// Handle events from the user changing the combobox.
	QObject::connect(combo_projections, SIGNAL(activated(int)),
			this, SLOT(handle_combobox_changed(int)));
	
	// TODO: Listen for projection changes that may occur from some
	// other source, and update the combobox appropriately.
	// connect to handle_projection_changed(some data type)
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_combobox_changed(
		int idx)
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant selected_projection_qv = combo_projections->itemData(idx);
	
	// TODO: Extract selected projection from the QVariant and do something
	// to the canvas with it.
	d_map_canvas_ptr->set_projection_type(idx);

	// The reconstruction widget listens for this, and switches from map<-->globe if necessary.
	emit projection_changed(idx);
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_projection_changed(
		int projection_type
		)
{
	// TODO: Wrap the projection ID in a QVariant.
	QVariant selected_projection_qv(projection_type);
	
	// Now we can quickly select the appropriate line of the combobox
	// by finding our projection ID (and not worrying about the text
	// label)
	int idx = combo_projections->findData(selected_projection_qv);
	if (idx != -1) {
		combo_projections->setCurrentIndex(idx);
	}
}

void
GPlatesQtWidgets::ProjectionControlWidget::show_label(
	bool show_)
{
	label_projections->setVisible(show_);
}
