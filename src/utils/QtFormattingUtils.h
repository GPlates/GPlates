/* $Id$ */

/**
 * \file
 * Here are various formatting utilities which have Qt dependencies,
 * which should probably be kept separate to other utilities that the
 * core of GPlates uses.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_UTILS_QTFORMATTINGUTILS_H
#define GPLATES_UTILS_QTFORMATTINGUTILS_H

#include <QDateTime>

namespace GPlatesUtils
{
	/**
	 * Format a QDateTime @a from (e.g. Feature creation time) as a short, rough summary
	 * of the elapsed duration to a second time @a to, which defaults to 'now'.
	 * If the duration is more than a week, it just returns the default stringification
	 * of the date part of @a from.
	 *
	 * This is used by the Clicked Feature Table to indicate how long ago a feature
	 * was created.
	 */
	QString
 	qdatetime_to_elapsed_duration(
			const QDateTime &from)
	{
		// Format the creation time display based on how long ago it was.
		QDateTime now = QDateTime::currentDateTime();
		int seconds_ago = from.secsTo(now);
		int days_ago = from.daysTo(now);
		if (seconds_ago < 2) {
			return QObject::tr("right now");
		} else if (seconds_ago < 60) {
			return QObject::tr("%1 seconds ago").arg(seconds_ago);
		} else if (seconds_ago < 120) {
			return QObject::tr("%1 minute ago").arg(seconds_ago / 60);
		} else if (seconds_ago < 3600) {
			return QObject::tr("%1 minutes ago").arg(seconds_ago / 60);
		} else if (seconds_ago < 7200) {
			return QObject::tr("%1 hour ago").arg(seconds_ago / 3600);
		} else if (seconds_ago < 86400) {
			return QObject::tr("%1 hours ago").arg(seconds_ago / 3600);
		} else if (days_ago < 2) {
			return QObject::tr("%1 day ago").arg(days_ago);
		} else if (days_ago < 8) {
			return QObject::tr("%1 days ago").arg(days_ago);
		} else {
			return from.date().toString();
		}
	}
}

#endif  // GPLATES_UTILS_QTFORMATTINGUTILS_H

