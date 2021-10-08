/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 Geological Survey of Norway
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
#include "ShapefileAttributeMapperDialogUi.h"

#include "model/FeatureCollectionHandle.h"

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
		setup(std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>&);

		public slots:
		void
		shapefileChanged(int);

		void
		accept();

	signals:
	private:
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_collections;
		
	};
}

#endif  // GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEMAPPERDIALOG_H
