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
 
#include "SpecifyTimeIncrementDialog.h"

#include "gui/AnimationController.h"


GPlatesQtWidgets::SpecifyTimeIncrementDialog::SpecifyTimeIncrementDialog(
		GPlatesGui::AnimationController &animation_controller,
		QWidget *parent_):
	QDialog(parent_)
{
	setupUi(this);

	spinbox_increment->setValue(animation_controller.time_increment());
	QObject::connect(&animation_controller, SIGNAL(time_increment_changed(double)),
			spinbox_increment, SLOT(setValue(double)));
	QObject::connect(spinbox_increment, SIGNAL(valueChanged(double)),
			&animation_controller, SLOT(set_time_increment(double)));
}

