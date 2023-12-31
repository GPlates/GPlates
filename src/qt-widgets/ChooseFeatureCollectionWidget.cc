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

#include <boost/foreach.hpp>
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
			d_file(file_ref)
		{  }

		// Constructor for creating fake "Make a new Feature Collection" entry.
		FeatureCollectionItem(
				const QString &label):
			QListWidgetItem(label)
		{  }
			
		bool
		is_create_new_collection_item()
		{
			return !d_file;
		}

		/**
		 * NOTE: Check with @a is_create_new_collection_item first and set a valid file
		 * iterator if necessary before calling this method.
		 */
		GPlatesAppLogic::FeatureCollectionFileState::file_reference
		get_file_reference()
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_file, GPLATES_ASSERTION_SOURCE);

			return d_file->file_ref;
		}

		void
		set_file_reference(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref)
		{
			d_file = File(file_ref);
		}

		/**
		 * Returns the referenced feature collection or an invalid weak reference if
		 * either not created with a file or file has since been unloaded.
		 *
		 * NOTE: This method was added purely to support retaining the previously selected
		 * file when initialising ChooseFeatureCollectionWidget. We can't use @a get_file_reference
		 * because it can crash if used to reference a file that has since been unloaded.
		 */
		GPlatesModel::FeatureCollectionHandle::weak_ref
		get_feature_collection_reference()
		{
			return d_file
					? d_file->feature_collection_ref
					: GPlatesModel::FeatureCollectionHandle::weak_ref();
		}

	private:

		struct File
		{
			explicit
			File(
					const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref_) :
				file_ref(file_ref_),
				feature_collection_ref(file_ref_.get_file().get_feature_collection())
			{  }

			GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref;

			/**
			 * This is here only so we can remember which feature collection was previously selected.
			 * We can't use @a d_file_ref because it can crash if used to reference a file that has
			 * since been unloaded (whereas feature collection has a weak reference).
			 */
			GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref;
		};

		boost::optional<File> d_file;
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
		// Remember the current selection so we can re-select if it exists after re-populating.
		boost::optional<int> selected_collection_row;
		boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref> previously_selected_collection;
		if (list_widget.currentRow() >= 0)
		{
			FeatureCollectionItem *collection_item =
					dynamic_cast<FeatureCollectionItem *>(list_widget.currentItem());
			if (collection_item)
			{
				// Referencing feature collection (even if it hasn't been saved to file yet).
				// Note that this would crash if we had used FeatureCollectionItem::get_file_reference()
				// and some files had been unloaded thus making their file references invalid.
				previously_selected_collection = collection_item->get_feature_collection_reference();
			}
		}

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
				collection_is_of_allowed_type(collection_opt,reconstruct_method_registry,allowed_collection_types))
			{
				list_widget.addItem(new FeatureCollectionItem(file_ref, label));

				// Set the newly selected file if it matches previous selection (if there was any) and
				// the previous selection exists in the new list.
				if (!selected_collection_row &&
					previously_selected_collection &&
					previously_selected_collection.get() == collection_opt)
				{
					selected_collection_row = list_widget.count() - 1;
				}
			}
		}

		// Add a final option for creating a brand new FeatureCollection.
		list_widget.addItem(
				new FeatureCollectionItem(
					GPlatesQtWidgets::ChooseFeatureCollectionWidget::tr(" < Create a new feature collection > ")));

		if (selected_collection_row)
		{
			list_widget.setCurrentRow(selected_collection_row.get());
		}
		else
		{
			// Default to the last entry (create a new feature collection). The previously selected
			// file does not exist anymore so allow the user to create a new feature collection.
			list_widget.setCurrentRow(list_widget.count() - 1);
		}
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
		const bool newly_created = collection_item->is_create_new_collection_item();
		if (newly_created)
		{
			collection_item->set_file_reference(d_file_io.create_empty_file());
		}

		return std::make_pair(collection_item->get_file_reference(), newly_created);
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
