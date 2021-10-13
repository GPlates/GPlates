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
 
#ifndef GPLATES_QTWIDGETS_LAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_LAYEROPTIONSWIDGET_H

#include <boost/weak_ptr.hpp>
#include <QWidget>
#include <QString>


namespace GPlatesPresentation
{
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	/**
	 * This is the abstract base class of widgets used to display options
	 * particular to different visual layer types.
	 */
	class LayerOptionsWidget :
			public QWidget
	{
	public:

		LayerOptionsWidget(
				QWidget *parent_) :
			QWidget(parent_)
		{  }

		virtual
		~LayerOptionsWidget()
		{  }

		/**
		 * Requests that the widget display options for the given @a visual_layer.
		 */
		virtual
		void
		set_data(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer) = 0;

		virtual
		const QString &
		get_title() = 0;

	};
}


#endif	// GPLATES_QTWIDGETS_LAYEROPTIONSWIDGET_H
