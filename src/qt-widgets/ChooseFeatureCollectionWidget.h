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
 
#ifndef GPLATES_QTWIDGETS_CHOOSEFEATURECOLLECTIONWIDGET_H
#define GPLATES_QTWIDGETS_CHOOSEFEATURECOLLECTIONWIDGET_H

#include <utility>

#include <QGroupBox>

#include "ChooseFeatureCollectionWidgetUi.h"

#include "app-logic/FeatureCollectionFileState.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
}

namespace GPlatesQtWidgets
{
	class ChooseFeatureCollectionWidget :
			public QGroupBox,
			protected Ui_ChooseFeatureCollectionWidget
	{
		Q_OBJECT
		
	public:

		struct NoFeatureCollectionSelectedException {  };

		explicit
		ChooseFeatureCollectionWidget(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileIO &file_io,
				QWidget *parent_ = NULL);

		/**
		 * Initialises the ChooseFeatureCollectionWidget with the currently loaded
		 * feature collections.
		 */
		void
		initialise();

		/**
		 * Returns an iterator to the file selected by the user, and a boolean value
		 * indicating whether the iterator points to a file that was newly created.
		 *
		 * If the user chose to create a new feature collection, a new feature
		 * collection is created and an iterator to that new feature collection is
		 * returned.
		 *
		 * Throws NoFeatureCollectionSelectedException if no feature collection was
		 * selected by the user.
		 */
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_iterator, bool>
		get_file_iterator() const;

	signals:

		void
		item_activated();

	private slots:

		void
		handle_listwidget_item_activated(
				QListWidgetItem *);

	private:

		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;
		GPlatesAppLogic::FeatureCollectionFileIO &d_file_io;
	};
}

#endif  // GPLATES_QTWIDGETS_CHOOSEFEATURECOLLECTIONWIDGET_H
