/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_SHAPEFILEFILEFORMATCONFIGURATIONDIALOG_H
#define GPLATES_QTWIDGETS_SHAPEFILEFILEFORMATCONFIGURATIONDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>

#include "model/FeatureCollectionHandle.h"

#include "DatelineWrapOptionsWidget.h"
#include "ShapefileFileFormatConfigurationDialogUi.h"
#include "ShapefileAttributeWidget.h"


namespace GPlatesQtWidgets
{
	class ShapefileFileFormatConfigurationDialog: 
			public QDialog,
			protected Ui_ShapefileFileFormatConfiguration
	{
		Q_OBJECT
		
	public:
		
		explicit
		ShapefileFileFormatConfigurationDialog(
				QWidget *parent_ = NULL);

		virtual
		~ShapefileFileFormatConfigurationDialog()
		{  }

		void
		setup(
			bool dateline_wrap,
			const QString &filename,
			const QStringList &field_names,
			QMap<QString,QString> &model_to_attribute_map);

		/**
		 * Get the wrap-to-dateline option.
		 */ 
		bool
		get_wrap_to_dateline() const;

	public slots:

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
		reset();

	private slots:

		void
		handle_buttonbox_clicked(
				QAbstractButton *button);

	private:
		bool *d_dateline_wrap;

		DatelineWrapOptionsWidget *d_dateline_wrap_options_widget;
		ShapefileAttributeWidget *d_shapefile_attribute_widget;
	};
}

#endif  // GPLATES_QTWIDGETS_SHAPEFILEFILEFORMATCONFIGURATIONDIALOG_H

