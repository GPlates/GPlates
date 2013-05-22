/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_SETDEFORMATIONPARAMETERSDIALOG_H
#define GPLATES_QTWIDGETS_SETDEFORMATIONPARAMETERSDIALOG_H

#include <boost/weak_ptr.hpp>
#include <QWidget>

#include "SetDeformationParametersDialogUi.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	/**
	 * Dialog to view and modify parameters for deforming feature geometries.
	 */
	class SetDeformationParametersDialog :
			public QDialog,
			protected Ui_SetDeformationParametersDialog
	{
		Q_OBJECT

	public:

		explicit
		SetDeformationParametersDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				QWidget *parent_ = NULL);

		/**
		 * Causes the dialog to be populated with values from the given @a visual_layer.
		 * Returns true iff the dialog was successfully populated.
		 */
		bool
		populate(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer);

	private Q_SLOTS:

#if 0
		void
		handle_always_visible();
		
		void
		handle_time_window();
		
		void
		handle_delta_t();
		
		void
		handle_distant_past(
				bool state);
		
		void
		handle_distant_future(
				bool state);
#endif
		
		void
		handle_begin_time_spinbox_changed(
				double value);

		void
		handle_end_time_spinbox_changed(
				double value);

		void
		handle_time_increment_spinbox_changed(
				double value);

		void
		react_show_strain_accumulation_changed(
				int state);

		void
		handle_apply();

	private:
		
		void
		setup_connections();

		GPlatesAppLogic::ApplicationState &d_application_state;

		/**
		 * The visual layer for which we are currently displaying settings.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
	};
}

#endif  // GPLATES_QTWIDGETS_SETDEFORMATIONPARAMETERSDIALOG_H
