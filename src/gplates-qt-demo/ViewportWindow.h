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
 
#ifndef GPLATES_GUI_VIEWPORTWINDOW_H
#define GPLATES_GUI_VIEWPORTWINDOW_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <string>
#include <QtCore/QTimer>
#include "ViewportWindowUi.h"
#include "GlobeCanvas.h"
#include "ReconstructToTimeDialog.h"
#include "SpecifyFixedPlateDialog.h"
#include "AnimateDialog.h"
#include "model/Model.h"

namespace GPlatesGui
{
	class ViewportWindow:
			public QMainWindow, 
			protected Ui_ViewportWindow
	{
		Q_OBJECT
		
	public:
		ViewportWindow(
				const std::string &plates_line_fname,
				const std::string &plates_rot_fname);

		const double &
		reconstruction_time() const
		{
			return d_recon_time;
		}
	public slots:
		void
		set_reconstruction_time_and_reconstruct(
				double recon_time);

		void
		set_reconstruction_root_and_reconstruct(
				unsigned long recon_root);

		void
		increment_reconstruction_time_and_reconstruct();

		void
		decrement_reconstruction_time_and_reconstruct();

	private:
		GlobeCanvas *d_canvas_ptr;
		GPlatesModel::Model *d_model_ptr;
		GPlatesModel::FeatureCollectionHandle::weak_ref d_isochrons;
		GPlatesModel::FeatureCollectionHandle::weak_ref d_total_recon_seqs;
		double d_recon_time;
		GPlatesModel::GpmlPlateId::integer_plate_id_type d_recon_root;
		ReconstructToTimeDialog d_reconstruct_to_time_dialog;
		SpecifyFixedPlateDialog d_specify_fixed_plate_dialog;
		AnimateDialog d_animate_dialog;
		bool d_animate_dialog_has_been_shown;
		
	private slots:
		void
		pop_up_reconstruct_to_time_dialog();

		void
		pop_up_specify_fixed_plate_dialog();

		void
		pop_up_animate_dialog();
	};
}

#endif  // GPLATES_GUI_VIEWPORTWINDOW_H
