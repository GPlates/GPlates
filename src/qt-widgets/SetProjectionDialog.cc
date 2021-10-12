/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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


#include "SetProjectionDialog.h"
#include "MapCanvas.h"
#include "ViewportWindow.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


GPlatesQtWidgets::SetProjectionDialog::SetProjectionDialog(
	ViewportWindow &viewport_window,
	QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_viewport_window_ptr(&viewport_window)
{
	setupUi(this);

	// FIXME: Synchronise these with the definitions in the the MapProjection class. 
	combo_projection->addItem(tr("3D Globe"),GPlatesGui::ORTHOGRAPHIC);
	combo_projection->addItem(tr("Rectangular"),GPlatesGui::RECTANGULAR);
	combo_projection->addItem(tr("Mercator"),GPlatesGui::MERCATOR);
	combo_projection->addItem(tr("Mollweide"),GPlatesGui::MOLLWEIDE);
	combo_projection->addItem(tr("Robinson"),GPlatesGui::ROBINSON);

	// The central_meridian spinbox should be disabled if we're in Orthographic mode. 
	update_central_meridian_status();
	QObject::connect(combo_projection,SIGNAL(currentIndexChanged(int)),
			this,SLOT(update_central_meridian_status()));

}

void
GPlatesQtWidgets::SetProjectionDialog::set_projection(
	GPlatesGui::ProjectionType projection_type)
{
	// Now we can quickly select the appropriate line of the combobox
	// by finding our projection ID (and not worrying about the text
	// label)
	const int idx = combo_projection->findData(projection_type);
	if (idx != -1) {
		combo_projection->setCurrentIndex(idx);
	}
}

void
GPlatesQtWidgets::SetProjectionDialog::set_central_meridian(
	double central_meridian_)
{
	spin_central_meridian->setValue(central_meridian_);
}

void
GPlatesQtWidgets::SetProjectionDialog::setup()
{
	// Get the current projection. 
	const GPlatesGui::ProjectionType projection_type =
			d_viewport_window_ptr->reconstruction_view_widget().map_canvas().projection_type();

	set_projection(projection_type);
}

void
GPlatesQtWidgets::SetProjectionDialog::update_central_meridian_status()
{
	spin_central_meridian->setDisabled(
		combo_projection->currentIndex() == GPlatesGui::ORTHOGRAPHIC);
}

GPlatesGui::ProjectionType
GPlatesQtWidgets::SetProjectionDialog::get_projection_type() const
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant projection_qv = combo_projection->itemData(combo_projection->currentIndex());

	// Extract projection type from QVariant.
	const GPlatesGui::ProjectionType projection_type =
			static_cast<GPlatesGui::ProjectionType>(projection_qv.toInt());
	
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			projection_type >= 0 && projection_type < GPlatesGui::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	return projection_type;
}

double
GPlatesQtWidgets::SetProjectionDialog::central_meridian()
{
	return spin_central_meridian->value();
}
