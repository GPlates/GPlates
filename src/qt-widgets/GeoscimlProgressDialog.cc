/* $Id$ */

/**
 * @file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2026 The University of Sydney, Australia
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

#include "GeoscimlProgressDialog.h"


GPlatesQtWidgets::GeoscimlProgressDialog::GeoscimlProgressDialog(
		QWidget *parent) :
	d_progress_dialog(new QProgressDialog("Translating features...", "Cancel", 0, 0, parent)),
	d_count(0)
{
	d_progress_dialog->setWindowModality(Qt::WindowModal);
}


void
GPlatesQtWidgets::GeoscimlProgressDialog::set_count(
		int count)
{
	d_count = count;

	d_progress_dialog->setRange(0, count);
	d_progress_dialog->setValue(0);
	d_progress_dialog->show();
}


bool
GPlatesQtWidgets::GeoscimlProgressDialog::update(
		int index)
{
	if (d_progress_dialog->wasCanceled())
	{
		return false;
	}

	d_progress_dialog->show();
	d_progress_dialog->setValue(index);
	d_progress_dialog->setLabelText(
			QString("Translating feature %1 of %2").arg(index).arg(d_count));

	return true;
}
