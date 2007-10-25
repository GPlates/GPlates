/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include "ReconstructionViewWidget.h"

GPlatesQtWidgets::ReconstructionViewWidget::ReconstructionViewWidget(
		ViewportWindow &view_state,
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);
	d_canvas_ptr = new GlobeCanvas(view_state, this);

	gridLayout->addWidget(d_canvas_ptr, 1, 0);

	// Make sure the globe is expanding as much as possible!
	QSizePolicy globe_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	d_canvas_ptr->setSizePolicy(globe_size_policy);
	d_canvas_ptr->updateGeometry();
}


void
GPlatesQtWidgets::ReconstructionViewWidget::set_reconstruction_time_and_reconstruct(
		double recon_time)
{  }

void
GPlatesQtWidgets::ReconstructionViewWidget::increment_reconstruction_time_and_reconstruct()
{  }

void
GPlatesQtWidgets::ReconstructionViewWidget::decrement_reconstruction_time_and_reconstruct()
{  }

