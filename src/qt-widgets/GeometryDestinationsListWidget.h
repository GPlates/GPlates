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
 
#ifndef GPLATES_QTWIDGETS_GEOMETRYDESTINATIONSLISTWIDGET_H
#define GPLATES_QTWIDGETS_GEOMETRYDESTINATIONSLISTWIDGET_H

#include <QString>
#include <QListWidget>
#include <QListWidgetItem>

#include "model/FeatureType.h"
#include "model/PropertyName.h"


namespace GPlatesQtWidgets
{
	// Forward declaration.
	class PropertyNameItem;

	/**
	 * GeometryDestinationsList encapsulates a list widget that offers the user a
	 * selection of geometry property names that can be used with a particular
	 * feature type.
	 *
	 * It is used, for example, by the CreateFeatureDialog.
	 */
	class GeometryDestinationsListWidget :
			public QListWidget
	{
	public:

		GeometryDestinationsListWidget(
				QWidget *parent_ = NULL);

		PropertyNameItem *
		get_current_property_name_item() const;

		void
		populate(
				const GPlatesModel::FeatureType &target_feature_type);
	};


	/**
	 * Subclass of QListWidgetItem so that we can display QualifiedXmlNames in the QListWidget
	 * without converting them to a QString (and thus forgetting that we had a QualifiedXmlName
	 * in the first place).
	 */
	class PropertyNameItem :
			public QListWidgetItem
	{
	public:
		PropertyNameItem(
				const GPlatesModel::PropertyName name,
				const QString &display_name,
				bool expects_time_dependent_wrapper_):
			QListWidgetItem(display_name),
			d_name(name),
			d_expects_time_dependent_wrapper(expects_time_dependent_wrapper_)
		{  }
	
		const GPlatesModel::PropertyName
		get_name()
		{
			return d_name;
		}

		bool
		expects_time_dependent_wrapper()
		{
			return d_expects_time_dependent_wrapper;
		}

	private:
		const GPlatesModel::PropertyName d_name;
		bool d_expects_time_dependent_wrapper;
	};
}


#endif	// GPLATES_QTWIDGETS_GEOMETRYDESTINATIONSLISTWIDGET_H
