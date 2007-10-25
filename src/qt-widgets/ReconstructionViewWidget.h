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
 
#ifndef GPLATES_GUI_RECONSTRUCTIONVIEWWIDGET_H
#define GPLATES_GUI_RECONSTRUCTIONVIEWWIDGET_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include "ReconstructionViewWidgetUi.h"

#include "GlobeCanvas.h"

namespace GPlatesQtWidgets
{
	class ReconstructionViewWidget:
			public QWidget, 
			protected Ui_ReconstructionViewWidget
	{
		Q_OBJECT
		
	public:
		explicit
		ReconstructionViewWidget(
				ViewportWindow &view_state,
				QWidget *parent_ = 0);

		const double &
		reconstruction_time() const
		{
			return d_recon_time;
		}

		GlobeCanvas *
		get_globe_canvas() const
		{
			return d_canvas_ptr;
		}
		
	public slots:
		void
		set_reconstruction_time_and_reconstruct(
				double recon_time);

		void
		increment_reconstruction_time_and_reconstruct();

		void
		decrement_reconstruction_time_and_reconstruct();

	private:
		GlobeCanvas *d_canvas_ptr;
		double d_recon_time;
		
	};
}



#endif  // GPLATES_GUI_RECONSTRUCTIONVIEWWIDGET_H

