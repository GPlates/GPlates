/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_COREGISTRATIONRESULTTABLEDIALOG_H
#define GPLATES_QTWIDGETS_COREGISTRATIONRESULTTABLEDIALOG_H

#include <boost/weak_ptr.hpp>
#include <QWidget>
#include <QAbstractTableModel>
#include <QTableView>
#include <QLabel>
#include <QSpinBox>
#include <QMenu>
#include <QEvent>
#include <qevent.h>

#include "CoRegistrationResultTableDialogUi.h"
#include "SaveFileDialog.h"

#include "data-mining/DataTable.h"
#include "data-mining/OpaqueDataToQString.h"

#include "presentation/VisualLayer.h"

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileState;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;

	class ResultTableView : 
		public QTableView
	{
		Q_OBJECT

	public:

		explicit
		ResultTableView(
				QWidget *_parent) : 
			QTableView(_parent)  
	    {
			d_highlight_seed_action = new QAction(tr("highlight seed"), this);
			setContextMenuPolicy(Qt::DefaultContextMenu);
			
			QObject::connect(d_highlight_seed_action, SIGNAL(triggered()),
				_parent, SLOT(highlight_seed()));
		}  

		virtual
		void
		contextMenuEvent(
				QContextMenuEvent *_event)
		 {
		
			QMenu *menu = new QMenu(this);  
	        QModelIndex index = indexAt(_event->pos());  

			if(index.isValid()) 
			{
		       //menu->addAction(QString("Row %1 - Col %2 was clicked on").arg(index.row()).arg(index.column())); 
			   menu->addAction(d_highlight_seed_action);
			}
			else 
				menu->addAction("No item was clicked on");  

			menu->exec(QCursor::pos());  
		 }

	protected:
		QAction* d_highlight_seed_action;
	};

	class ResultTableModel :
		public QAbstractTableModel
	{
		Q_OBJECT
	public:
		
		explicit
		ResultTableModel(
				const GPlatesDataMining::DataTable& _data_table,
				QObject *parent_ = NULL) :
			QAbstractTableModel(parent_),
			d_table(_data_table)
		{;}

		/**
		 */
		int
		rowCount(
				const QModelIndex &parent_ = QModelIndex()) const
		{
			return d_table.size();
		}
		
		/**
		 */
		int
		columnCount(
				const QModelIndex &parent_ = QModelIndex()) const
		{
			//fix here
			return d_table.table_header().size();
		}
		
		/**
		 */
		Qt::ItemFlags
		flags(
				const QModelIndex &idx) const
		{
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		}
		
		/**
		 */
		QVariant
		headerData(
				int section,
				Qt::Orientation orientation,
				int role = Qt::DisplayRole) const
		{
			if(d_table.size() == 0)
			{
				return QVariant();
			}
			if (orientation == Qt::Horizontal) 
			{
				if (role == Qt::DisplayRole) 
				{
					if(d_table.table_header().size() - section > 0)
					{
						return d_table.table_header()[section];
					}
				} 
				else if (role == Qt::ToolTipRole) 
				{
					return QVariant();
				}
				else if (role == Qt::SizeHintRole) 
				{
					return QVariant();
				} 
				else 
				{
					return QVariant();
				}
			} 
			else 
			{
				if (role == Qt::DisplayRole) 
				{
					QString horiz_name = "Seed: " + QString::number(section);
					return QVariant(horiz_name);
				}
			}
			return  QVariant();
		}

		/**
		 */
		QVariant
		data(
				const QModelIndex &idx,
				int role) const;

		const GPlatesDataMining::DataTable&
		data_table()
		{
			return d_table;
		}

	public slots:
	protected:
		const GPlatesDataMining::DataTable d_table;
	};

	/**
	 * 
	 */
	class CoRegistrationResultTableDialog:
			public QDialog,
			protected Ui_CoRegistrationResultTableDialog
	{
		Q_OBJECT
	public:

		explicit
		CoRegistrationResultTableDialog(
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer,
				QWidget *parent_ = NULL);

		~CoRegistrationResultTableDialog()
		{ }

		void
		pop_up();

		void
		set_visual_layer(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
		{
			d_visual_layer = visual_layer;
		}

	public slots:

		/**
		 * Retrieves co-registration results from the associated co-registration layer proxy.
		 *
		 * Internally this is signal/slot connected such that it gets called whenever a new
		 * reconstruction happens (which in turn happens when the reconstruction time changes or
		 * any layers/connections/inputs have been changed/modified).
		 */
		void
		update();

		void
		reject();

		void
		highlight_seed();

	signals:
		
	private:

		void	
		connect_application_state_signals(
				GPlatesAppLogic::ApplicationState &application_state);

		void
		update_co_registration_data(
				const GPlatesDataMining::DataTable &co_registration_data_table);

		GPlatesModel::FeatureHandle::weak_ref
		find_feature_by_id(
				GPlatesAppLogic::FeatureCollectionFileState& state,
				const QString& id);

		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;

		boost::scoped_ptr< ResultTableModel > d_table_model_prt;
		QTableView* table_view;
	};
}

#endif  // GPLATES_QTWIDGETS_COREGISTRATIONRESULTTABLEDIALOG_H
