/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#include "ReconstructToTimeDialog.h"


GPlatesGui::ReconstructToTimeDialog::ReconstructToTimeDialog(
		const double &init_value,
		QWidget *parent_):
	QDialog(parent_),
	d_value(init_value)
{
	setupUi(this);

	doubleSpinBox->setRange(0.0, 1000000.0);
	doubleSpinBox->setValue(d_value);
	QObject::connect(doubleSpinBox, SIGNAL(valueChanged(double)),
			this, SLOT(change_value(double)));
	QObject::connect(this, SIGNAL(accepted()),
			this, SLOT(propagate_value()));
}
