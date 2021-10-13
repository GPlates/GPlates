/* $Id$ */

/**
 * \file Qt Model/View proxy class for filtering the list of log entries.
 *       Does depend on QtGui modules so that we can add a bit of colouring to the log.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_LOGFILTERMODEL_H
#define GPLATES_GUI_LOGFILTERMODEL_H

#include <QSortFilterProxyModel>
#include <QString>
#include <boost/noncopyable.hpp>

#include "app-logic/LogModel.h"	// for LogEntry types and the special Roles we define.


namespace GPlatesGui
{
	/**
	 * Qt Model/View filter model - this sits between the app-logic LogModel and
	 * the LogDialog and provides filtering of log entries.
	 * Since this class is in GPlatesGui, we can also take the opportunity to add
	 * a splash of colour here.
	 */
	class LogFilterModel :
			public QSortFilterProxyModel,
			private boost::noncopyable
	{
		Q_OBJECT
	public:
		
		explicit
		LogFilterModel(
				QObject *_parent);
		
		virtual
		~LogFilterModel()
		{  }
		
		/**
		 * Reimplementation of QSortFilterProxyModel::data().
		 * This lets us recolour certain rows based on data from the source model.
		 */
		virtual
		QVariant
		data(
				const QModelIndex &idx,
				int role = Qt::DisplayRole) const;
				
		
	public slots:
	
		void
		set_filter(
				const QString &filter_text,
				bool show_debug_messages,
				bool show_warning_messages,
				bool show_critical_messages);
	
	protected:
	
		/**
		 * Reimplementation of QSortFilterProxyModel::filterAcceptsRow().
		 * This lets us fine-tune exactly which rows should match our filter.
		 */
		bool
		filterAcceptsRow(
				int source_row,
				const QModelIndex &source_parent) const;
	
		/**
		 * Used by filterAcceptsRow().
		 */
		bool
		matches_severity_filters(
				const GPlatesAppLogic::LogModel::LogEntry::Severity entry_severity) const;

		/**
		 * Used by filterAcceptsRow().
		 */
		bool
		matches_text_filter(
				const QString &row_text) const;
				
	private:
	
		bool d_show_debug_messages;
		bool d_show_warning_messages;
		bool d_show_critical_messages;
	};
}


#endif // GPLATES_GUI_LOGFILTERMODEL_H

