/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
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
 
#include "VisualLayersDialog.h"

#include "QtWidgetUtils.h"
#include "VisualLayersWidget.h"


GPlatesQtWidgets::VisualLayersDialog::VisualLayersDialog(
		GPlatesPresentation::VisualLayers &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	QDialog(parent_, Qt::Window)
{
	setWindowTitle(tr("Layers"));
	resize(375, 675);

	VisualLayersWidget *widget = new VisualLayersWidget(
			visual_layers,
			application_state,
			view_state,
			viewport_window,
			this);
	QtWidgetUtils::add_widget_to_placeholder(widget, this);
}

