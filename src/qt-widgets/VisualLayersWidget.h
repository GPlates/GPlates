/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_VISUALLAYERSWIDGET_H
#define GPLATES_QTWIDGETS_VISUALLAYERSWIDGET_H

#include <QWidget>

#include "VisualLayersWidgetUi.h"

#include "gui/VisualLayersProxy.h"


namespace GPlatesPresentation
{
	class VisualLayers;
}

namespace GPlatesQtWidgets
{
	/**
	 * VisualLayersWidget displays the contents of VisualLayers. It displays a
	 * widget for each visual layer, as well as tools to manipulate those layers.
	 */
	class VisualLayersWidget :
			public QWidget,
			protected Ui_VisualLayersWidget
	{
		Q_OBJECT
		
	public:

		explicit
		VisualLayersWidget(
				GPlatesPresentation::VisualLayers &visual_layers,
				QWidget *parent_ = NULL);

	private:

		/**
		 * A wrapper around VisualLayers to invert the ordering for the user interface.
		 */
		GPlatesGui::VisualLayersProxy d_visual_layers;
	};
}

#endif  // GPLATES_QTWIDGETS_VISUALLAYERSWIDGET_H
