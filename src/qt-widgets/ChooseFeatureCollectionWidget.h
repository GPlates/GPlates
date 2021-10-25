/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 * Copyright (C) 2011 Geological Survey of Norway
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
#include "file-io/FeatureCollectionFileFormatClassify.h"

#include "model/FeatureCollectionHandle.h"


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
				const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileIO &file_io,
				QWidget *parent_ = NULL,
				const boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::classifications_type> &allowed_collection_types = boost::none);

		/**
		 * Initialises the ChooseFeatureCollectionWidget with the currently loaded feature collections.
		 *
		 * If the previously selected feature collection (if any) is in the new list of
		 * feature collections then the selection is retained.
		 */
		void
		initialise();

		/**
		 * Changes the help text in the widget to @a text.
		 */
		void
		set_help_text(
				const QString &text);

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
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool>
		get_file_reference() const;

		/**
		 * Selects the item in the list that corresponds to @a file_reference.
		 */
		void
		select_file_reference(
				const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_reference);

		/**
		 * Selects the item in the list that corresponds to @a feature_collection.
		 */
		void
		select_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

	Q_SIGNALS:

		void
		item_activated();

	private Q_SLOTS:

		void
		handle_listwidget_item_activated(
				QListWidgetItem *);

	protected:

		virtual
		void
		focusInEvent(
				QFocusEvent *ev);

	private:

		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;
		GPlatesAppLogic::FeatureCollectionFileIO &d_file_io;

		/**
		 * The collection types which we wish to display in the widget.
		 *
		 * To show only reconstruction types, for example, we would instantiate this class something like
		 * the following:
		 *
		 *		GPlatesFileIO::FeatureCollectionFileFormat::classifications_type reconstruction_collections;
		 *		reconstruction_collections.set(GPlatesFileIO::FeatureCollectionFileFormat::RECONSTRUCTION);
		 *
		 *		d_choose_feature_collection_widget = 
		 *			new ChooseFeatureCollectionWidget(registry,file_state,file_io,this,reconstruction_collections);
		 *
		 *
		 */
		boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::classifications_type> d_allowed_collection_types;

		const GPlatesAppLogic::ReconstructMethodRegistry &d_reconstruct_method_registry;
	};
}

#endif  // GPLATES_QTWIDGETS_CHOOSEFEATURECOLLECTIONWIDGET_H
