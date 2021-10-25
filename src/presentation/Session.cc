/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
#include <QFileInfo>
#include <QList>
#include <QStringList>
#include <QVariant>

#include "Session.h"


namespace
{
	QString
	common_base_dir(
			const QString &a,
			const QString &b)
	{
		QStringList alist = a.split('/', QString::SkipEmptyParts);
		QStringList blist = b.split('/', QString::SkipEmptyParts);
		QStringList rlist;
		for (int i = 0; i < alist.size() && i < blist.size(); ++i) {
			if (alist.at(i) != blist.at(i)) {
				break;
			}

			rlist << alist.at(i);
		}
		return rlist.join("/");
	}


	QString
	common_base_dir(
			const QSet<QString> &filenames)
	{
		if (filenames.isEmpty()) {
			return QString();
		}
		
		QString common = QFileInfo(*filenames.begin()).path();
		Q_FOREACH(QString filename, filenames) {
			QString dir = QFileInfo(filename).path();
			common = common_base_dir(common, dir);
		}
		return QFileInfo(common).fileName();
	}


	/**
	 * Removes any "" entries from a QStringList, to avoid potential bugs with incorrectly saved Sessions.
	 */
	QStringList
	strip_empty_entries(
			QStringList list)
	{
		list.removeAll("");
		return list;
	}
}


GPlatesPresentation::Session::Session(
		const QDateTime &time_,
		const QStringList &files_):
	d_time(time_),
	d_loaded_files(QSet<QString>::fromList(strip_empty_entries(files_)))
{  }


QString
GPlatesPresentation::Session::get_description() const
{
	// Please note: In theory, these sort of pluralisation issues can be taken care of
	// with Qt Linguist and the appropriate translation, however we don't have any yet,
	// not even a translation from C -> English. The ternary below will suffice for now.
	QString files_str = d_loaded_files.size() == 1 ? tr("file") : tr("files");
	QString location = common_base_dir(d_loaded_files);
	QString desc;
	if (location.isEmpty())
	{
		desc = tr("%1 %2 on %3")
				.arg(d_loaded_files.size())
				.arg(files_str)
				.arg(d_time.toString(Qt::SystemLocaleLongDate));
	}
	else
	{
		desc = tr("%1 %2 in \"%3\" on %4")
				.arg(d_loaded_files.size())
				.arg(files_str)
				.arg(location)
				.arg(d_time.toString(Qt::SystemLocaleLongDate));
	}

	return desc;
}


const QDateTime &
GPlatesPresentation::Session::get_time() const
{
	return d_time;
}

QList<QString>
GPlatesPresentation::Session::get_loaded_files() const
{
	return QStringList::fromSet(d_loaded_files);
}


bool
GPlatesPresentation::Session::is_empty() const
{
	return d_loaded_files.isEmpty();
}


bool
GPlatesPresentation::Session::has_same_loaded_files_as(
		const Session &other) const
{
	return d_loaded_files == other.d_loaded_files;
}
