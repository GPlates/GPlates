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
 
#ifndef GPLATES_QTWIDGETS_VIEWPORTWINDOW_H
#define GPLATES_QTWIDGETS_VIEWPORTWINDOW_H

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
#include "ReconstructionViewWidget.h"
#include "SpecifyFixedPlateDialog.h"
#include "AnimateDialog.h"
#include "AboutDialog.h"
#include "LicenseDialog.h"
#include "QueryFeaturePropertiesDialog.h"
#include "ReadErrorAccumulationDialog.h"

#include "model/ModelInterface.h"


namespace GPlatesGui
{
	class CanvasToolAdapter;
	class CanvasToolChoice;
}

namespace GPlatesQtWidgets
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

		GPlatesModel::Reconstruction &
		reconstruction() const
		{
			return *d_reconstruction_ptr;
		}

		const double &
		reconstruction_time() const
		{
			return d_recon_time;
		}

		unsigned long
		reconstruction_root() const
		{
			return d_recon_root;
		}

	public slots:
		void
		reconstruct();

		void
		reconstruct_to_time(
				double recon_time);

		void
		reconstruct_with_root(
				unsigned long recon_root);

		void
		pop_up_license_dialog();

		void
		choose_drag_globe_tool();

		void
		choose_query_feature_tool();
		
		void
		pop_up_read_errors_dialog();
		

	private:
		GPlatesModel::ModelInterface *d_model_ptr;
		GPlatesModel::Reconstruction::non_null_ptr_type d_reconstruction_ptr;
		GPlatesModel::FeatureCollectionHandle::weak_ref d_isochrons;
		GPlatesModel::FeatureCollectionHandle::weak_ref d_total_recon_seqs;
		double d_recon_time;
		GPlatesModel::integer_plate_id_type d_recon_root;
		ReconstructionViewWidget d_reconstruction_view_widget;
		SpecifyFixedPlateDialog d_specify_fixed_plate_dialog;
		AnimateDialog d_animate_dialog;
		AboutDialog d_about_dialog;
		LicenseDialog d_license_dialog;
		QueryFeaturePropertiesDialog d_query_feature_properties_dialog;
		ReadErrorAccumulationDialog d_read_errors_dialog;
		bool d_animate_dialog_has_been_shown;
		GlobeCanvas *d_canvas_ptr;
		GPlatesGui::CanvasToolAdapter *d_canvas_tool_adapter_ptr;
		GPlatesGui::CanvasToolChoice *d_canvas_tool_choice_ptr;

		void
		uncheck_all_tools();
	private slots:
		void
		pop_up_specify_fixed_plate_dialog();

		void
		pop_up_animate_dialog();

		void
		pop_up_about_dialog();
	};
}

#endif  // GPLATES_QTWIDGETS_VIEWPORTWINDOW_H
