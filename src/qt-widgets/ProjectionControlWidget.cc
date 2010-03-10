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

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "gui/ViewportProjection.h"

#include <QVariant>

GPlatesQtWidgets::ProjectionControlWidget::ProjectionControlWidget(
		GPlatesGui::ViewportProjection &viewport_projection,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_viewport_projection(viewport_projection)
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
	
	// Listen for projection changes that may occur from some
	// other source, and update the combobox appropriately.
	QObject::connect(
			&d_viewport_projection, SIGNAL(projection_type_changed(const GPlatesGui::ViewportProjection &)),
			this, SLOT(handle_projection_type_changed(const GPlatesGui::ViewportProjection &)));
}


void
GPlatesQtWidgets::ProjectionControlWidget::handle_combobox_changed(
		int idx)
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant selected_projection_qv = combo_projections->itemData(idx);

	// Extract projection type from QVariant.
	const GPlatesGui::ProjectionType projection_type =
			static_cast<GPlatesGui::ProjectionType>(selected_projection_qv.toInt());
	
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			projection_type >= 0 && projection_type < GPlatesGui::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

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
