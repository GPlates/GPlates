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
 
#ifndef GPLATES_QTWIDGETS_GEOMETRYDESTINATIONSWIDGET_H
#define GPLATES_QTWIDGETS_GEOMETRYDESTINATIONSWIDGET_H

#include <utility>
#include <boost/optional.hpp>
#include <QWidget>

#include "SelectionWidget.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"


namespace GPlatesQtWidgets
{
	/**
	 * GeometryDestinationsWidget encapsulates a widget that offers the user a
	 * selection of geometry property names that can be used with a particular
	 * feature type.
	 *
	 * It is used, for example, by the CreateFeatureDialog.
	 */
	class GeometryDestinationsWidget :
			public QWidget
	{
		Q_OBJECT

	public:

		explicit
		GeometryDestinationsWidget(
				SelectionWidget::DisplayWidget display_widget,
				QWidget *parent_ = NULL);

		/**
		 * Returns the currently selected PropertyName, together with a boolean value
		 * indicating whether a time dependent wrapper is expected.
		 */
		boost::optional<std::pair<GPlatesModel::PropertyName, bool> >
		get_current_property_name() const;

		/**
		 * Causes this widget to show geometry properties appropriate for
		 * @a target_feature_type.
		 */
		void
		populate(
				const GPlatesModel::FeatureType &target_feature_type);

	signals:

		void
		item_activated();

	private slots:

		void
		handle_item_activated(
				int index);

	private:

		SelectionWidget *d_selection_widget;
	};
}


#endif	// GPLATES_QTWIDGETS_GEOMETRYDESTINATIONSWIDGET_H
