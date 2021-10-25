/* $Id$ */


/**
 * \file
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

#ifndef GPLATES_PRESENTATION_DEPRECATEDSESSIONRESTORE_H
#define GPLATES_PRESENTATION_DEPRECATEDSESSIONRESTORE_H

#include <QDateTime>
#include <QString>
#include <QStringList>


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	namespace DeprecatedSessionRestore
	{
		/**
		 * Handles the old way of restoring sessions before the general scribe system was
		 * introduced in session version 4.
		 *
		 * Returns a list of feature collection files that don't exist (ones that exist
		 * but fail to load will result in an exception and a partial restore).
		 */
		QStringList
		restore_session(
				int version,
				const QDateTime &time,
				const QStringList &loaded_files,
				const QString &layers_state,
				GPlatesAppLogic::ApplicationState &app_state);
	}
}

#endif // GPLATES_PRESENTATION_DEPRECATEDSESSIONRESTORE_H
