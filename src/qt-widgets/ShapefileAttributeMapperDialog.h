/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEMAPPERDIALOG_H
#define GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEMAPPERDIALOG_H

#include <QDialog>

#include "model/FeatureCollectionHandle.h"

#include "ShapefileAttributeMapperDialogUi.h"
#include "ShapefileAttributeWidget.h"


// TODO: this dialog is used for re-mapping other OGR formats too. We should eventually update the UI, and
// file/class-names etc, to reflect this.

namespace GPlatesQtWidgets
{
	class ShapefileAttributeMapperDialog: 
			public QDialog,
			protected Ui_ShapefileAttributeMapper
	{
		Q_OBJECT
		
	public:
		
		explicit
		ShapefileAttributeMapperDialog(
				QWidget *parent_ = NULL);

		virtual
		~ShapefileAttributeMapperDialog()
		{  }

		void
		setup(
			QString &filename,
			QStringList &field_names,
			QMap<QString,QString> &model_to_attribute_map);

	public Q_SLOTS:

		/**
		 * Use the current state of the combo boxes to build up the 
		 * shapefile-attribute-to-model-property map.
		 */
		void
		accept();

		/**
		 * Reset the combo boxes to the state they were in when the 
		 * dialog was created. 
		 */
		void
		reset_fields();

	private Q_SLOTS:

		void
		handle_buttonbox_clicked(
				QAbstractButton *button);

	private:

		ShapefileAttributeWidget *d_shapefile_attribute_widget;
	};
}

#endif  // GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEMAPPERDIALOG_H
