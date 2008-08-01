/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "ReconstructionPoleWidget.h"

#include "ViewportWindow.h"


GPlatesQtWidgets::ReconstructionPoleWidget::ReconstructionPoleWidget(
		GPlatesQtWidgets::ViewportWindow &view_state,
		QWidget *parent_):
	QWidget(parent_),
	d_view_state_ptr(&view_state)
{
	setupUi(this);
	
	// Reset button to change the current adjustment to an identity rotation.
	QObject::connect(button_adjustment_reset, SIGNAL(clicked()),
			this, SLOT(handle_reset_adjustment()));
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::handle_reset_adjustment()
{  }


