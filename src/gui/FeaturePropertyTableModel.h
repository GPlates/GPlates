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

#ifndef GPLATES_GUI_FEATUREPROPERTYTABLEMODEL_H
#define GPLATES_GUI_FEATUREPROPERTYTABLEMODEL_H

#include <QAbstractTableModel>
#include <QItemSelection>
#include <QHeaderView>
#include <vector>

#include "gui/FeatureFocus.h"
#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "model/PropertyValue.h"
#include "model/WeakReference.h"


namespace GPlatesGui
{
	/**
	 * Struct used by FeaturePropertyTableModel to keep track of the properties being
	 * presented by the model and their state (i.e. editability).
	 */
	struct FeaturePropertyTableInfo
	{
		GPlatesModel::PropertyName property_name;
		GPlatesModel::FeatureHandle::iterator property_iterator;
		bool editable_inline;
	};
	
	/**
	 * This class is used by Qt to map a FeatureHandle::weak_ref to a QTableView.
	 * It displays the top-level properties of the feature and their values.
	 * 
	 * It uses Qt's model/view framework, not to be confused with GPlates' own model/view,
	 * to provide multi-column data to a view. See the following urls for more information:
	 * http://trac.gplates.org/wiki/QtModelView
	 * http://doc.trolltech.com/4.3/model-view-programming.html
	 *
	 * To link a FeaturePropertyTableModel to the GUI, simply create a QTableView (either
	 * in the designer or in code) and call table_view->setModel(FeaturePropertyTableModel *);
	 *
	 * The model was editable, but this has been disabled for now due to Complicated Things
	 * happening with time-dependent PropertyValues. In short: a properties_iterator is not
	 * always enough. Perhaps this model could be re-written sometime to display (and edit)
	 * more complicated feature-property trees. Until then, editing is disabled.
	 * See Bug #77 for some details.
	 */
	class FeaturePropertyTableModel:
			public QAbstractTableModel
	{
		Q_OBJECT
		
	public:
	
		typedef std::vector<FeaturePropertyTableInfo> property_info_container_type;
		typedef property_info_container_type::iterator property_info_container_iterator;
		typedef property_info_container_type::const_iterator property_info_container_const_iterator;

		explicit
		FeaturePropertyTableModel(
				GPlatesGui::FeatureFocus &feature_focus,
				QObject *parent_ = NULL);
		
		/**
		 * Qt Model/View function used to access row count, which will depend on the number
		 * of top-level properties of the feature.
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
				const QModelIndex &parent_ = QModelIndex()) const
		{
			// FIXME: magic number.
			return 2;
		}
		
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
		 * Qt Model/View function used to set individual cells of data.
		 */
		bool
		setData(
				const QModelIndex &idx,
				const QVariant &value,
				int role = Qt::EditRole);


		const GPlatesModel::PropertyName
		get_property_name(
				int row) const;

		/**
		 * Given a row of the table model, returns the corresponding property iterator.
		 * Throws an STL out of bounds exception if you ask for something stupid.
		 */
		GPlatesModel::FeatureHandle::iterator
		get_property_iterator_for_row(
				int row) const;

		/**
		 * Given a property iterator, returns the corresponding row of the table model.
		 * Returns -1 if no matching row is found.
		 */
		int
		get_row_for_property_iterator(
				GPlatesModel::FeatureHandle::iterator property_iterator) const;

		bool
		is_property_editable_inline(
				int row) const;


	public slots:
		
		/**
		 * Use this slot to clear the table and set it to a new feature reference.
		 * This is called at the appropriate time from EditFeaturePropertiesWidget.
		 */
		void
		set_feature_reference(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);

		/**
		 * Use this slot to simply rebuild the table from the current feature reference.
		 * This is called at the appropriate time from EditFeaturePropertiesWidget.
		 */
		void
		refresh_data();

	signals:
		
		/**
		 * Emitted when changes have been made to a feature.
		 * This can be used to e.g. update the ViewportWindow.
		 */
		void
		feature_modified(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);
		
	private:
		
		void
		clear_table();
		
		QVariant
		get_property_name_as_qvariant(
				int row) const;

		QVariant
		get_property_value_as_qvariant(
				int row,
				int role) const;
		
		GPlatesGui::FeatureFocus *d_feature_focus;
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;
		
		property_info_container_type d_property_info_cache;
	};
}

#endif // GPLATES_GUI_FEATUREPROPERTYTABLEMODEL_H
