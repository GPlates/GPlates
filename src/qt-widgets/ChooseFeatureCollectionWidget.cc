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

#include <QListWidget>
#include <QListWidgetItem>
#include <QString>
#include <QDir>

#include "ChooseFeatureCollectionWidget.h"


#include "app-logic/FeatureCollectionFileIO.h"

#include "file-io/FeatureCollectionFileFormatClassify.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/FeatureCollectionHandle.h"

#include <boost/foreach.hpp>

namespace
{
	/**
	 * Subclass of QListWidgetItem so that we can display a list of FeatureCollection in
	 * the list widget using the filename as the label, while keeping track of which
	 * list item corresponds to which FeatureCollection.
	 */
	class FeatureCollectionItem :
			public QListWidgetItem
	{
	public:
		// Standard constructor for creating FeatureCollection entry.
		FeatureCollectionItem(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref,
				const QString &label):
			QListWidgetItem(QDir::toNativeSeparators(label)),
			d_file_ref(file_ref)
		{  }

		// Constructor for creating fake "Make a new Feature Collection" entry.
		FeatureCollectionItem(
				const QString &label):
			QListWidgetItem(label)
		{  }
			
		bool
		is_create_new_collection_item()
		{
			return !d_file_ref;
		}

		/**
		 * NOTE: Check with @a is_create_new_collection_item first and set a valid file
		 * iterator if necessary before calling this method.
		 */
		GPlatesAppLogic::FeatureCollectionFileState::file_reference
		get_file_reference()
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_file_ref, GPLATES_ASSERTION_SOURCE);

			return *d_file_ref;
		}

		void
		set_file_reference(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref)
		{
			d_file_ref = file_ref;
		}
	
	private:
		boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference> d_file_ref;
	};

	bool
	collection_is_of_allowed_type(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
		const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
		const boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::classifications_type> &allowed_types)
	{
		if (allowed_types)
		{
			GPlatesFileIO::FeatureCollectionFileFormat::classifications_type type_of_collection =
				GPlatesFileIO::FeatureCollectionFileFormat::classify(
					feature_collection_ref,reconstruct_method_registry);

			return GPlatesFileIO::FeatureCollectionFileFormat::intersect(type_of_collection,*allowed_types);
		}
		else
		{
			return true;
		}
	}

	/**
	 * Fill the list with currently loaded FeatureCollections we can add the feature to.
	 */
	void
	populate_feature_collections_list(
			QListWidget &list_widget,
			GPlatesAppLogic::FeatureCollectionFileState &state,
			const boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::classifications_type> &allowed_collection_types,
			const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry)
	{
		list_widget.clear();

		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
				state.get_loaded_files();
		BOOST_FOREACH(
				const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
				loaded_files)
		{
			// Get the FeatureCollectionHandle for this file.
			GPlatesModel::FeatureCollectionHandle::weak_ref collection_opt =
					file_ref.get_file().get_feature_collection();

			// Some files might not actually exist yet if the user created a new
			// feature collection internally and hasn't saved it to file yet.
			QString label;
			if (GPlatesFileIO::file_exists(file_ref.get_file().get_file_info()))
			{

				// Get a suitable label; we will prefer the full filename.
				label = file_ref.get_file().get_file_info().get_display_name(true);
			}
			else
			{
				// The file doesn't exist so give it a filename to indicate this.
				label = GPlatesQtWidgets::ChooseFeatureCollectionWidget::tr("New Feature Collection");
			}
			
			// We are only interested in loaded files which have valid FeatureCollections.
			if (collection_opt.is_valid() && 
				collection_is_of_allowed_type(collection_opt,reconstruct_method_registry,allowed_collection_types)) {
				list_widget.addItem(new FeatureCollectionItem(file_ref, label));
			}
		}

		// Add a final option for creating a brand new FeatureCollection.
		list_widget.addItem(
				new FeatureCollectionItem(
					GPlatesQtWidgets::ChooseFeatureCollectionWidget::tr(" < Create a new feature collection > ")));

		// Default to first entry.
		list_widget.setCurrentRow(0);
	}
}


GPlatesQtWidgets::ChooseFeatureCollectionWidget::ChooseFeatureCollectionWidget(
		const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileIO &file_io,
		QWidget *parent_,
		const boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::classifications_type> &collection_types) :
	QGroupBox(parent_),
	d_file_state(file_state),
	d_file_io(file_io),
	d_allowed_collection_types(collection_types),
	d_reconstruct_method_registry(reconstruct_method_registry)
{
	setupUi(this);

	// Emit signal if the user pushes Enter or double-clicks on the list.
	QObject::connect(
			listwidget_feature_collections,
			SIGNAL(itemActivated(QListWidgetItem *)),
			this,
			SLOT(handle_listwidget_item_activated(QListWidgetItem *)));
}


void
GPlatesQtWidgets::ChooseFeatureCollectionWidget::initialise()
{
	populate_feature_collections_list(
		*listwidget_feature_collections, 
		d_file_state, 
		d_allowed_collection_types,
		d_reconstruct_method_registry);
}


void
GPlatesQtWidgets::ChooseFeatureCollectionWidget::set_help_text(
		const QString &text)
{
	label_help_text->setText(text);
}


void
GPlatesQtWidgets::ChooseFeatureCollectionWidget::handle_listwidget_item_activated(
		QListWidgetItem *)
{
	Q_EMIT item_activated();
}


std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool>
GPlatesQtWidgets::ChooseFeatureCollectionWidget::get_file_reference() const
{
	FeatureCollectionItem *collection_item = dynamic_cast<FeatureCollectionItem *>(
			listwidget_feature_collections->currentItem());
	if (collection_item)
	{
		if (collection_item->is_create_new_collection_item())
		{
			return std::make_pair(d_file_io.create_empty_file(), true);
		}
		else
		{
			return std::make_pair(collection_item->get_file_reference(), false);
		}
	}
	else
	{
		throw NoFeatureCollectionSelectedException();
	}
}


void
GPlatesQtWidgets::ChooseFeatureCollectionWidget::select_file_reference(
		const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_reference)
{
	select_feature_collection(file_reference.get_file().get_feature_collection());
}


void
GPlatesQtWidgets::ChooseFeatureCollectionWidget::select_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	for (int i = 0; i != listwidget_feature_collections->count(); ++i)
	{
		FeatureCollectionItem *collection_item = dynamic_cast<FeatureCollectionItem *>(
				listwidget_feature_collections->item(i));
		if (collection_item)
		{
			if (!collection_item->is_create_new_collection_item() &&
				collection_item->get_file_reference().get_file().get_feature_collection() ==
					feature_collection)
			{
				listwidget_feature_collections->setCurrentRow(i);
				return;
			}
		}
	}
}


void
GPlatesQtWidgets::ChooseFeatureCollectionWidget::focusInEvent(
		QFocusEvent *ev)
{
	listwidget_feature_collections->setFocus();
}
