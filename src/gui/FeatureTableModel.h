/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_FEATURETABLEMODEL_H
#define GPLATES_GUI_FEATURETABLEMODEL_H

#include <vector>
#include <boost/optional.hpp>
#include <QAbstractTableModel>
#include <QItemSelection>
#include <QHeaderView>

#include "app-logic/Layer.h"
#include "app-logic/ReconstructionGeometry.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class ReconstructGraph;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class FeatureFocus;

	/**
	 * This class is used by Qt to map a FeatureWeakRefSequence to a QTableView.
	 * 
	 * It uses Qt's model/view framework, not to be confused with GPlates' own model/view,
	 * to provide multi-column data to a view. See the following urls for more information:
	 * http://trac.gplates.org/wiki/QtModelView
	 * http://doc.trolltech.com/4.3/model-view-programming.html
	 *
	 * To link a FeatureTableModel to the GUI, simply create a QTableView (either in the
	 * designer or in code) and call table_view->setModel(FeatureTableModel *);
	 * It is best to follow this with a call to table_view->resizeColumnsToContents();
	 * as Qt seems very picky about size hints for header items.
	 *
	 * To constrain which columns you wish to display using your QTableView, use the
	 * table_view->horizontalHeader() function to obtain a pointer to a QHeaderView.
	 * This will allow you to show, hide, and rearrange columns.
	 * The vertical header supplied by this model does not contain any useful information,
	 * and can be safely hidden with table_view->verticalHeader()->hide().
	 *
	 * The model is currently non-editable.
	 */
	class FeatureTableModel:
			public QAbstractTableModel
	{
		Q_OBJECT
	public:
		//! A reconstruction geometry and information associated with it.
		struct ReconstructionGeometryRow
		{
			ReconstructionGeometryRow(
					GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type
							reconstruction_geometry_,
					const GPlatesAppLogic::ReconstructGraph &reconstruct_graph);

			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry;

			/**
			 * The reconstruction tree layer that generated the reconstruction tree that was
			 * used to reconstruct the ReconstructionGeometry.
			 * This is used to update the ReconstructionGeometry as the reconstruction time changes.
			 */
			boost::optional<GPlatesAppLogic::Layer> reconstruction_tree_layer;
		};

		//! Typedef for a sequence of reconstruction geometry rows.
		typedef std::vector<ReconstructionGeometryRow> geometry_sequence_type;
	
		explicit
		FeatureTableModel(
				GPlatesPresentation::ViewState &view_state,
				QObject *parent_ = NULL);
		
		/**
		 * Accessor for the underlying data structure.
		 * Be aware that if you use this function to add or remove features,
		 * you must call begin_insert_features(int first, int last) and end_insert_features()
		 * on this FeatureTableModel before and after the change.
		 *
		 * See http://doc.trolltech.com/4.3/qabstractitemmodel.html#beginInsertRows for
		 * more details.
		 */
		geometry_sequence_type &
		geometry_sequence()
		{
			return d_sequence;
		}
		
		
		/**
		 * Qt Model/View function used to access row count, which will depend on the number
		 * of features in the FeatureWeakRefSequence.
		 * For our table model, parent_ will always be a dummy QModelIndex().
		 */
		int
		rowCount(
				const QModelIndex &parent_ = QModelIndex()) const;
		
		/**
		 * Qt Model/View function used to access column count, which will be a fixed number.
		 * For our table model, parent_ will always be a dummy QModelIndex().
		 */
		int
		columnCount(
				const QModelIndex &parent_ = QModelIndex()) const;
		
		/**
		 * Qt Model/View function used to access editable/selectable/etc status of cells.
		 */
		Qt::ItemFlags
		flags(
				const QModelIndex &idx) const;
		
		/**
		 * Qt Model/View function used to access header data, both horizontal and vertical.
		 * Pay careful attention to the role specified; it is used to select between many
		 * different types of data that might be requested by the view.
		 */
		QVariant
		headerData(
				int section,
				Qt::Orientation orientation,
				int role = Qt::DisplayRole) const;

		/**
		 * Qt Model/View function used to access individual cells of data.
		 * Pay careful attention to the role specified; it is used to select between many
		 * different types of data that might be requested by the view.
		 */
		QVariant
		data(
				const QModelIndex &idx,
				int role) const;

		/**
		 * Convenience function which will clear() the FeatureWeakRefSequence and notify any
		 * QTableViews of the change in layout.
		 */
		void
		clear()
		{
			layoutAboutToBeChanged();
			d_sequence.clear();
			layoutChanged();
		}
		
		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function before any major changes to the table data happen.
		 */
		void
		sequence_about_to_be_changed()
		{
			layoutAboutToBeChanged();
		}

		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function after any major changes to the table data happen.
		 */
		void
		sequence_changed()
		{
			layoutChanged();
		}

		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function before features are inserted. [first, last] is an
		 * inclusive range, and correspond to the row numbers the new features will
		 * have after they have been inserted.
		 */
		void
		begin_insert_features(int first, int last)
		{
			beginInsertRows(QModelIndex(), first, last);
		}

		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function after features have been inserted.
		 */
		void
		end_insert_features()
		{
			endInsertRows();
		}

		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function before features are removed. [first, last] is an
		 * inclusive range, and correspond to the row numbers the features will
		 * be removed from.
		 */
		void
		begin_remove_features(int first, int last)
		{
			beginRemoveRows(QModelIndex(), first, last);
		}

		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function after features have been removed.
		 */
		void
		end_remove_features()
		{
			endRemoveRows();
		}




		/**
		 * Convenience function to initialise a QHeaderView with the suggested
		 * resize mode appropriate for each column.
		 */
		static
		void
		set_default_resize_modes(
				QHeaderView &header);

#if 0  // We don't need this function right now.
		/**
		 * Searches the table for the given FeatureHandle::weak_ref.
		 * If found, returns a QModelIndex that can be used by the
		 * ViewportWindow to highlight the appropriate row in the
		 * QTableView (for instance).
		 *
		 * There is no guarantee that the FeatureHandle::weak_ref will
		 * be in the FeatureTableModel of course; in these situations,
		 * an invalid QModelIndex is returned, which can be tested for
		 * via the @a .isValid() method.
		 */
		QModelIndex
		get_index_for_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);
#endif

	public slots:
		
		/**
		 * ViewportWindow connects the QTableView's selection model's change event to this
		 * slot, so that the model can use it to focus the corresponding geometry.
		 */
		void
		handle_selection_change(
				const QItemSelection &selected,
				const QItemSelection &deselected);

		/**
		 * Lets the model know that a feature has been modified.
		 * 
		 * The model will test to see if any of the rows it is currently keeping track of
		 * correspond to that feature, and emit update events appropriately.
		 *
		 * Internally, the model connects FeatureFocus::focused_feature_modified() to this
		 * slot, since the only changes to features will usually be changes to whatever is
		 * currently focused.
		 */
		void
		handle_feature_modified(
				GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Update the internal @a ReconstructionGeometries for the new reconstruction.
		 */
		void
		handle_reconstruction(
				GPlatesAppLogic::ApplicationState &application_state);

		QModelIndex
		current_index() {
			return d_current_index;
		}
			
	private:
		
		FeatureFocus *d_feature_focus_ptr;
		geometry_sequence_type d_sequence;
	
		QModelIndex d_current_index;
	};
}

#endif // GPLATES_GUI_FEATURETABLEMODEL_H
