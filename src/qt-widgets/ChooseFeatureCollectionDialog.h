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
 
#ifndef GPLATES_QTWIDGETS_CHOOSEFEATURECOLLECTIONDIALOG_H
#define GPLATES_QTWIDGETS_CHOOSEFEATURECOLLECTIONDIALOG_H

#include <boost/optional.hpp>
#include <QDialog>

#include "ChooseFeatureCollectionDialogUi.h"

#include "GPlatesDialog.h"

#include "app-logic/FeatureCollectionFileState.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
	class ReconstructMethodRegistry;
}

namespace GPlatesQtWidgets
{
	class ChooseFeatureCollectionWidget;

	class ChooseFeatureCollectionDialog : 
			public GPlatesDialog,
			protected Ui_ChooseFeatureCollectionDialog 
	{
	public:

		explicit
		ChooseFeatureCollectionDialog(
				const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileIO &file_io,
				QWidget *parent_ = NULL);

		/**
		 * Returns an iterator to the file selected by the user, and a boolean value
		 * indicating whether the iterator points to a file that was newly created.
		 *
		 * If the user chose to create a new feature collection, a new feature
		 * collection is created and an iterator to that new feature collection is
		 * returned.
		 */
		boost::optional<std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> >
		get_file_reference(
				const GPlatesAppLogic::FeatureCollectionFileState::file_reference &initial);

		/**
		 * Returns an iterator to the file selected by the user, and a boolean value
		 * indicating whether the iterator points to a file that was newly created.
		 *
		 * If the user chose to create a new feature collection, a new feature
		 * collection is created and an iterator to that new feature collection is
		 * returned.
		 */
		boost::optional<std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> >
		get_file_reference(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &initial);

		/**
		 * Overloaded version of get_file_reference which does not require an initial feature collection
		 * or file_reference.
		 *
		 * Returns an iterator to the file selected by the user, and a boolean value
		 * indicating whether the iterator points to a file that was newly created.
		 *
		 * If the user chose to create a new feature collection, a new feature
		 * collection is created and an iterator to that new feature collection is
		 * returned.
		 */
		boost::optional<std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> >
		get_file_reference();

	private:

		ChooseFeatureCollectionWidget *d_choose_widget;
	};
}

#endif  // GPLATES_QTWIDGETS_CHOOSEFEATURECOLLECTIONDIALOG_H
