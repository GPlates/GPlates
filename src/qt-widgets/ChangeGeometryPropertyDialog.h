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
 
#ifndef GPLATES_QTWIDGETS_CHANGEGEOMETRYPROPERTYDIALOG_H
#define GPLATES_QTWIDGETS_CHANGEGEOMETRYPROPERTYDIALOG_H

#include "ChangeGeometryPropertyDialogUi.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"


namespace GPlatesQtWidgets
{
	class GeometryDestinationsListWidget;
	class PropertyNameItem;
	
	class ChangeGeometryPropertyDialog: 
			public QDialog,
			protected Ui_ChangeGeometryPropertyDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		ChangeGeometryPropertyDialog(
				QWidget *parent_ = NULL);

		PropertyNameItem *
		get_property_name_item() const;

		void
		populate(
				const GPlatesModel::FeatureType &feature_type,
				const GPlatesModel::PropertyName &old_property_name);

	private slots:

		void
		handle_checkbox_state_changed(
				int state);

	private:

		GeometryDestinationsListWidget *d_geometry_destinations_listwidget;
	};
}

#endif  // GPLATES_QTWIDGETS_CHANGEGEOMETRYPROPERTYDIALOG_H
