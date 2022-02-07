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

#include <QRegExp>
#include <QBrush>

#include "app-logic/LogModel.h"	// for LogEntry types and the special Roles we define.

#include "LogFilterModel.h"


GPlatesGui::LogFilterModel::LogFilterModel(
		QObject *_parent):
	QSortFilterProxyModel(_parent),
	d_show_debug_messages(true),
	d_show_warning_messages(true),
	d_show_critical_messages(true)
{
	setFilterCaseSensitivity(Qt::CaseInsensitive);
}


QVariant
GPlatesGui::LogFilterModel::data(
		const QModelIndex &idx,
		int role) const
{
	// Define special colours to use here; remember that whenever you use a special
	// foreground colour, you should always explicitly set the background colour as well,
	// since not everyone uses the same desktop colour theme you do.
	static const QBrush default_bg(Qt::white);
	static const QBrush default_fg(Qt::black);
	static const QBrush meta_fg(Qt::gray);
	static const QBrush warning_fg(QColor("#770000"));
	static const QBrush critical_fg(QColor("#AA0000"));

	// Override the foreground/background colours.
	if (role == Qt::ForegroundRole) {
		// Squeeze some extra info through the Qt Model/View system so we know what kind of log
		// entry we're dealing with.
		GPlatesAppLogic::LogModel::LogEntry::Severity entry_severity = 
				static_cast<GPlatesAppLogic::LogModel::LogEntry::Severity>(
						QSortFilterProxyModel::data(idx, GPlatesAppLogic::LogModel::SeverityRole).toInt());
		GPlatesAppLogic::LogModel::LogEntry::Type entry_type =
				static_cast<GPlatesAppLogic::LogModel::LogEntry::Type>(
						QSortFilterProxyModel::data(idx, GPlatesAppLogic::LogModel::TypeRole).toInt());
		
		// Apply some rules for colouring by type and severity:
		if (entry_type == GPlatesAppLogic::LogModel::LogEntry::META) {
			return meta_fg;
		} else {
			if (entry_severity == GPlatesAppLogic::LogModel::LogEntry::WARNING) {
				return warning_fg;
			} else if (entry_severity == GPlatesAppLogic::LogModel::LogEntry::CRITICAL
					|| entry_severity == GPlatesAppLogic::LogModel::LogEntry::FATAL) {
				return critical_fg;
			}
		}
		return default_fg;
		
	} else if (role == Qt::BackgroundRole) {
		return default_bg;
			
	} else {
		// Not related to colouring.
		// Just do whatever the superclass does.
		return QSortFilterProxyModel::data(idx, role);
	}
}


void
GPlatesGui::LogFilterModel::set_filter(
		const QString &filter_text,
		bool show_debug_messages,
		bool show_warning_messages,
		bool show_critical_messages)
{
	d_show_debug_messages = show_debug_messages;
	d_show_warning_messages = show_warning_messages;
	d_show_critical_messages = show_critical_messages;

	// Sets the base class's filterRegExp() property as though
	// QRegExp("text", Qt::CaseInsensitive, QRegExp::FixedString) were called.
	setFilterFixedString(filter_text);
}


bool
GPlatesGui::LogFilterModel::filterAcceptsRow(
		int source_row,
		const QModelIndex &source_parent) const
{
	// Assuming source_parent is an invalid index (should always be the case for a list model),
	QModelIndex row_idx = sourceModel()->index(source_row, 0, source_parent);
	QString row_text = sourceModel()->data(row_idx).toString();
	
	// Squeeze some extra info through the Qt Model/View system so we know what kind of log
	// entry we're dealing with.
	GPlatesAppLogic::LogModel::LogEntry::Severity entry_severity = 
			static_cast<GPlatesAppLogic::LogModel::LogEntry::Severity>(
					sourceModel()->data(row_idx, GPlatesAppLogic::LogModel::SeverityRole).toInt());
	
	return (matches_severity_filters(entry_severity) && matches_text_filter(row_text));
}


bool
GPlatesGui::LogFilterModel::matches_severity_filters(
		const GPlatesAppLogic::LogModel::LogEntry::Severity entry_severity) const
{
	// The default accepts all severities, since the set of checkboxes available is smaller than the number of severities.
	switch (entry_severity)
	{
	case GPlatesAppLogic::LogModel::LogEntry::DEBUG:
			return d_show_debug_messages;
			
	case GPlatesAppLogic::LogModel::LogEntry::WARNING:
			return d_show_warning_messages;
			
	case GPlatesAppLogic::LogModel::LogEntry::CRITICAL:
			return d_show_critical_messages;
	
	default:
			return true;
	}
}


bool
GPlatesGui::LogFilterModel::matches_text_filter(
		const QString &row_text) const
{
	if (filterRegExp().isEmpty()) {
		return true;
	} else {
		return row_text.contains(filterRegExp());
	}
}




