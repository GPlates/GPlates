/* $Id$ */

/**
 * \file
 * Contains the definition of the PaletteSelectionWidget class.
 *
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
 
#ifndef GPLATES_QTWIDGETS_PALETTESELECTIONWIDGET_H
#define GPLATES_QTWIDGETS_PALETTESELECTIONWIDGET_H

#include <QScrollArea>
#include <QString>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <utility>

#include "ColourSchemeInfo.h"


namespace GPlatesGui
{
	class ColourScheme;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapWidget;

	/**
	 * This widget is responsible for displaying the icons that allow the user to
	 * pick between different colour schemes.
	 */
	class PaletteSelectionWidget :
		public QScrollArea
	{
	
		Q_OBJECT
	
	public:

		PaletteSelectionWidget(
				GPlatesPresentation::ViewState &view_state,
				GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
				QWidget *parent_ = NULL);

		void
		set_colour_schemes(
				const std::vector<ColourSchemeInfo> &colour_schemes,
				int selected);

		virtual
		void
		resizeEvent(
				QResizeEvent *resize_event);

	signals:

		void
		selection_changed(
				int index);

	private:

		void
		create_layout();

		GPlatesPresentation::ViewState &d_view_state;

		GlobeAndMapWidget *d_existing_globe_and_map_widget_ptr;

		/**
		 * The widget that contains the contents of the scroll area.
		 */
		QWidget *d_widget;
		
	};
}

#endif  // GPLATES_QTWIDGETS_PALETTESELECTIONWIDGET_H
