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
 
#ifndef GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEVIEWERDIALOG_H
#define GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEVIEWERDIALOG_H

#include <QDialog>

#include "ui_ShapefileAttributeViewerDialogUi.h"

#include "GPlatesDialog.h"

#include "file-io/File.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileState;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;

	class ShapefileAttributeViewerDialog:
		public GPlatesDialog,
		protected Ui_ShapefileAttributeViewerDialog
	{
		Q_OBJECT

	public:
		explicit
		ShapefileAttributeViewerDialog(
			GPlatesAppLogic::FeatureCollectionFileState &file_state,
			QWidget *parent_= NULL);
		
		virtual
		~ShapefileAttributeViewerDialog()
		{  }
		
	public Q_SLOTS:
		/**
		 * Update the dialog to reflect the current Application State.
		 */
		void
		update(
				GPlatesAppLogic::FeatureCollectionFileState &file_state);

	private:
		
		void
		update_table();

		void	
		connect_feature_collection_file_state_signals(
				GPlatesAppLogic::FeatureCollectionFileState &file_state);

	private Q_SLOTS:
		/**
		 * Handle the feature-collection combo-box changing, which will require us
		 * to update the table contents. 
		 */
		void
		handle_feature_collection_changed(
			int index);

	private:

		/** 
		 * Files corresponding to shapefile feature collections.
		 * This will make its own copy of the files....
		 */
		std::vector<GPlatesFileIO::File::Reference *> d_file_vector;

	};

}

#endif // GPLATES_QTWIDGETS_SHAPEFILEATTRIBUTEVIEWERDIALOG_H
