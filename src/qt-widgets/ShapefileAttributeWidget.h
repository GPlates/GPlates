/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEWIDGET_H
#define GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEWIDGET_H

#include <QWidget>

#include "ShapefileAttributeWidgetUi.h"

namespace GPlatesQtWidgets
{

	class ShapefileAttributeWidget:
			public QWidget,
			protected Ui_ShapefileAttributeWidget
	{
		Q_OBJECT

	public:

		ShapefileAttributeWidget(
				QWidget *parent_,
				QString &filename,
				QStringList &field_names,
				QMap<QString,QString> &model_to_attribute_map,
				bool remapping = false);

		/**
		 * Set up the combo boxes with fields from the shapefile.
		 */ 
		void
		setup();

		/**
		 * Reset the combo boxes to the state they were in when the
		 * dialog was created.
		 */
		void
		reset_fields();

		/**
		 * Use the current state of the combo boxes to build up the 
		 * shapefile-attribute-to-model-property map.
		 */ 
		void
		accept_fields();
			
	private:
		QString d_filename;

		// The attribute field names obtained from the ShapefileReader. 
		const QStringList &d_field_names;

		// A map of the model property to the shapefile attribute.
		QMap<QString,QString> &d_model_to_attribute_map;

		// The default names for the model fields. 
		QStringList d_default_fields;

		// The combo box settings. 
		std::vector<int> d_combo_reset_map;

	};

}
#endif  // GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEWIDGET_H

