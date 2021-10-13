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

#ifndef GPLATES_QTWIDGETS_RESULTTABLEDIALOG_H
#define GPLATES_QTWIDGETS_RESULTTABLEDIALOG_H

#include <QWidget>
#include <QAbstractTableModel>
#include <QTableView>
#include <QLabel>
#include <QSpinBox>
#include <QMenu>
#include <QEvent>
#include <qevent.h>

#include "ResultTableDialogUi.h"
#include "SaveFileDialog.h"

#include "data-mining/DataTable.h"
#include "data-mining/OpaqueDataToQString.h"

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	using namespace GPlatesDataMining;
	class ResultTableView : 
		public QTableView
	{
	public:

		explicit
		ResultTableView(
				QWidget *_parent) : 
			QTableView(_parent)  
	    {
			//QAction* newAct = new QAction(tr("&New"), this);
			//addAction(newAct);
			setContextMenuPolicy(Qt::DefaultContextMenu);
			}  

		virtual
		void
		contextMenuEvent(
				QContextMenuEvent *_event)
		 {
		
			QMenu *menu = new QMenu(this);  
	        QModelIndex index = indexAt(_event->pos());  

			if(index.isValid())  
		       menu->addAction(QString("Row %1 - Col %2 was clicked on").arg(index.row()).arg(index.column()));  
			else 
				menu->addAction("No item was clicked on");  

			menu->exec(QCursor::pos());  
		 }
	};

	class ResultTableModel :
		public QAbstractTableModel
	{
		Q_OBJECT
	public:
		
		explicit
		ResultTableModel(
				const DataTable& _data_table,
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
			return d_table.get_table_desc().size();
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
					return d_table.get_table_desc()[section];
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
				int role) const
		{
			if ( ! idx.isValid()) 
			{
				return QVariant();
			}

			if (idx.row() < 0 || idx.row() >= rowCount())	
			{
				return QVariant();
			}
	
			if (role == Qt::DisplayRole) 
			{
				OpaqueData o_data;
				d_table.at( idx.row() )->get_cell( idx.column(), o_data );

				return 
					boost::apply_visitor(
							GPlatesDataMining::ConvertOpaqueDataToString(),
							o_data);

			} 
			else if (role == Qt::TextAlignmentRole) 
			{
				return QVariant();
			}
			return QVariant();
		}

		const DataTable&
			data_table()
		{
			return d_table;
		}

	public slots:
	protected:
		const DataTable& d_table;
	};

	/**
	 * 
	 */
	class ResultTableDialog:
			public QDialog,
			protected Ui_ResultTableDialog
	{
		Q_OBJECT
	public:

		static const QString filter_csv;
		static const QString filter_csv_ext;
		static const QString page_label_format;

		explicit
		ResultTableDialog(
				const std::vector< DataTable > data_tables,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		~ResultTableDialog()
		{

		}

		void
		update_page_label();

		void
		update_time_label();

	public slots:
	
		void
		accept();

		void
		reject();
		
		void
		handle_next_page();

		void
		handle_previous_page();

		void 
		handle_goto_page();

		void
		handle_save_all();
	
	signals:
		
	private:

		void
		update();

		std::vector< DataTable > d_data_tables;
		GPlatesPresentation::ViewState &d_view_state;
		boost::scoped_ptr< ResultTableModel > d_table_model_prt;
		
		QTableView* table_view;
		QLabel* page_label;
		QLabel* time_label;
		QSpinBox* spinBox_page;
		QPushButton* pushButton_next;
		QPushButton* pushButton_previous;

		unsigned d_page_index;
		unsigned d_page_num;
	};
}

#endif  // GPLATES_QTWIDGETS_RESULTTABLEDIALOG_H
