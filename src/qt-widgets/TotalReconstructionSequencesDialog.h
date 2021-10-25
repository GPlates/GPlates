/* $Id$ */

/**
 * \file 
 * Contains the definition of class TotalReconstructionSequencesDialog.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONSEQUENCESDIALOG_H
#define GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONSEQUENCESDIALOG_H

#include <QDialog>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "ui_TotalReconstructionSequencesDialogUi.h"

#include "GPlatesDialog.h"

#include "app-logic/TRSUtils.h"

#include "file-io/File.h"
#include "file-io/PlatesRotationFileProxy.h"
#include "model/FeatureHandle.h"
#include "model/types.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileState;
	class ReconstructionTree;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	/**
	 * A predicate to filter by plate ID.
	 *
	 * This is the abstract base class in an instance of the Interpreter pattern.
	 *
	 * The concrete derivations are defined in the source file.
	 */
	class PlateIdFilteringPredicate
	{
	public:
		virtual
		~PlateIdFilteringPredicate()
		{  }

		virtual
		bool
		allow_plate_id(
				GPlatesModel::integer_plate_id_type plate_id) const = 0;
	};

	typedef std::map<QTreeWidgetItem*,GPlatesModel::FeatureHandle::weak_ref> tree_item_to_feature_map_type;
	typedef std::map<QTreeWidgetItem*,GPlatesModel::FeatureCollectionHandle::weak_ref> tree_item_to_feature_collection_map_type;

    class CreateTotalReconstructionSequenceDialog;
    class EditTotalReconstructionSequenceDialog;
	class TotalReconstructionSequencesSearchIndex;
	class MetadataDialog;

	class TotalReconstructionSequencesDialog:
		public GPlatesDialog,
		protected Ui_TotalReconstructionSequencesDialog
	{
		Q_OBJECT

	public:
		
		TotalReconstructionSequencesDialog(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

                ~TotalReconstructionSequencesDialog();

	public Q_SLOTS:

		/**
		 * Update the dialog (after a file has been loaded or unloaded, for example).
		 */
		void
		update();

		/**
		 * Respond to the "Apply Filter" button.
		 */
		void
		apply_filter();

		/**
		 * Respond to the "Reset Filter" button.
		 */
		void
		reset_filter();

		/**
		 * React when the "current item" of the QTreeWidget has changed.
		 */
		void
		handle_current_item_changed(
				QTreeWidgetItem *current,
				QTreeWidgetItem *previous);

		/**
		 * Respond to the "Edit Sequence" button.                                                                    
		 */
		void
		edit_sequence();

		/**
		 * Respond to the "New Sequence" button.                                                                    
		 */
		void
		create_new_sequence();

		/**
		 * Respond to the "Delete Sequence" button.                                                                    
		 */
		void
		delete_sequence();

		void
		show_metadata();

		void
        disable_enable_pole();

		void
		disable_sequence();

		void
		enable_sequence();

        /**
		 * Update the tree after a TRS feature has been edited. 
		 *
		 * Calls update(), but also restores the state of the tree.
		 */
		void
		update_edited_feature();

		/**
		 *  Listen for changes in the file state so that we can update the tree.                                                                   
		 */
		void
		handle_feature_collection_file_state_changed();


		void
		handle_file_reloaded()
		{
			update();
		}


		/*
		* This function should only be used to update pole data from EditTotalReconstructionSequenceDialog.
		* The trs data from EditTotalReconstructionSequenceDialog does not contain metadata.
		* Preserve the original metadata as possible as we could.
		*/
		void
		update_current_sequence(
				GPlatesModel::TopLevelProperty::non_null_ptr_type moving_plate_id,
				GPlatesModel::TopLevelProperty::non_null_ptr_type fix_plate_id,
				GPlatesModel::TopLevelProperty::non_null_ptr_type trs);

		bool
		has_metadata(
				GPlatesModel::FeatureCollectionHandle::weak_ref);

		GPlatesModel::FeatureHandle::weak_ref
		get_current_feature();

		/*
		* Insert the feature into rotation file proxy.
		*/
		void
		insert_feature_to_proxy(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesFileIO::File::Reference&);

		bool
		is_seq_disabled(
				GPlatesModel::FeatureHandle::weak_ref);

		void
		set_seq_disabled(
				GPlatesModel::FeatureHandle::weak_ref,
				bool flag);

	protected:

		const boost::shared_ptr<PlateIdFilteringPredicate>
		parse_plate_id_filtering_text() const;

		GPlatesFileIO::File::Reference&
		get_current_file_ref();

		GPlatesFileIO::PlatesRotationFileProxy*
		get_current_rotation_file_proxy();

		GPlatesFileIO::PlatesRotationFileProxy*
		get_rotation_file_proxy(
				GPlatesFileIO::File::Reference&);

		/*
		* Remove the feature from rotation file proxy.
		* The @a keep_mprs_header is used to control if the mprs header should be kept.
		*/
		void
		remove_feature_from_proxy(
				GPlatesModel::FeatureHandle::weak_ref,
				bool keep_mprs_header = false);

		/*
		* Insert the feature into rotation file proxy.
		*/
		void
		insert_feature_to_proxy(
				GPlatesModel::FeatureHandle::weak_ref);

		void
		insert_feature_to_proxy(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesFileIO::PlatesRotationFileProxy&);

		/*
		*
		*/
		std::vector<GPlatesFileIO::RotationPoleData>
		get_pole_data_from_feature(
				GPlatesModel::FeatureHandle::weak_ref);

		/*
		*
		*/
		GPlatesModel::FeatureCollectionMetadata
		get_current_fc_metadata();


		GPlatesModel::FeatureHandle::iterator
		get_current_metadata_property();

		boost::tuple<
				bool, 
				GPlatesFileIO::File::Reference*>
		get_file_ref(
				GPlatesModel::FeatureCollectionHandle *) const;

	private:
		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState *d_file_state_ptr;

		/**
		 * The search index used to search by plate ID and text-in-comment.
		 */
		boost::shared_ptr<TotalReconstructionSequencesSearchIndex> d_search_index_ptr;


		/**
		 * A map of tree item to model property, so that we can edit the appropriate 
		 * part of the model when we select a TRS tree item.
		 */
		tree_item_to_feature_map_type d_tree_item_to_feature_map;
		/**
		 * The currently selected item in the tree.
		 *
		 * We store this before we edit a TRS so that we can restore the state of the
		 * tree afterwards. 
		 */
		QTreeWidgetItem *d_current_item;

		/**
		 * Whether or not the current item in the tree was expanded.
		 *
		 * Storing this lets us restore the state of the tree after an update.
		 */
		bool d_current_trs_was_expanded;

		/**
		 *                                                                     
		 */
		GPlatesAppLogic::ApplicationState &d_app_state;

		/**
		 * Initialise the signal-slot connections in the constructor.
		 */
		void
		make_signal_slot_connections();

		/**
		 * Connect to signals from a @a FeatureCollectionFileState object.
		 *
		 * FIXME:  Define this function.
		 */
		void
		connect_to_file_state_signals();

		/**
		* The create_trs dialog.
		*/
		boost::scoped_ptr<CreateTotalReconstructionSequenceDialog> d_create_trs_dialog_ptr;


		/**
		* The edit_trs dialog.
		*/
		boost::scoped_ptr<EditTotalReconstructionSequenceDialog> d_edit_trs_dialog_ptr;


		/*
		* No need to use boost::scoped_ptr. Set dialog parent properly, 
		* the dialog will be destructed by qt automatically when destructing its parent.
		*/
		MetadataDialog* d_metadata_dlg;
	};

}

#endif // GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONSEQUENCESDIALOG_H

