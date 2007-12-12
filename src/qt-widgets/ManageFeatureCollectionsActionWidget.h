/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSACTIONWIDGET_H
#define GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSACTIONWIDGET_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QWidget>
#include "ApplicationState.h"
#include "ManageFeatureCollectionsActionWidgetUi.h"

namespace GPlatesQtWidgets
{
	class ManageFeatureCollectionsDialog;
	
	
	class ManageFeatureCollectionsActionWidget:
			public QWidget, 
			protected Ui_ManageFeatureCollectionsActionWidget
	{
		Q_OBJECT
		
	public:
		explicit
		ManageFeatureCollectionsActionWidget(
				ManageFeatureCollectionsDialog &feature_collections_dialog,
				GPlatesAppState::ApplicationState::file_info_iterator file_it,
				QWidget *parent_ = NULL);
		
		GPlatesAppState::ApplicationState::file_info_iterator
		get_file_info_iterator() const
		{
			return d_file_info_iterator;
		}
	
	
	public slots:
		
		void
		save()
		{  }
		
		void
		save_as()
		{  }
		
		void
		save_copy()
		{  }
		
		void
		unload();
	
	private:
	
		ManageFeatureCollectionsDialog &d_feature_collections_dialog;
		GPlatesAppState::ApplicationState::file_info_iterator d_file_info_iterator;
		
	};
}

#endif  // GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSACTIONWIDGET_H
