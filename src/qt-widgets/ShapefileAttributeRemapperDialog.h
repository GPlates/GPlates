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
 
#ifndef GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEREMAPPERDIALOG_H
#define GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEREMAPPERDIALOG_H

#include <QDialog>

#include "model/FeatureCollectionHandle.h"

#include "ui_ShapefileAttributeRemapperDialogUi.h"
#include "ShapefileAttributeWidget.h"


namespace GPlatesQtWidgets
{
	class ShapefileAttributeRemapperDialog: 
			public QDialog,
			protected Ui_ShapefileAttributeRemapper
	{
		Q_OBJECT
		
	public:
		
		explicit
		ShapefileAttributeRemapperDialog(
				QWidget *parent_ = NULL);

		virtual
		~ShapefileAttributeRemapperDialog()
		{  }

		void
		setup(
			const QString &filename,
			const QStringList &field_names,
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

#endif  // GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEREMAPPERDIALOG_H

