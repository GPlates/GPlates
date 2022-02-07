/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_PROJECTSESSION_H
#define GPLATES_PRESENTATION_PROJECTSESSION_H

#include <boost/optional.hpp>
#include <QString>
#include <QStringList>

#include "Session.h"


namespace GPlatesPresentation
{
	/**
	 * A project file session of GPlates (saved to an archive file).
	 */
	class ProjectSession :
			public Session
	{
	public:

		// Convenience typedefs for a shared pointer to a @a ProjectSession.
		typedef GPlatesUtils::non_null_intrusive_ptr<ProjectSession> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ProjectSession> non_null_ptr_to_const_type;


		/**
		 * Create a @a ProjectSession object, from the specified project file, that can be used to restore a session.
		 *
		 * NOTE: This doesn't actually restore the session. For that you need to call @a restore_session.
		 *
		 * The session state is obtained from the project file.
		 */
		static
		non_null_ptr_to_const_type
		create_restore_session(
				const QString &project_filename);


		/**
		 * Saves the current session to the specified project file and
		 * returns the session in a @a ProjectSession object.
		 *
		 * The singleton @a Application is used to obtain the session state since it contains
		 * the entire state of GPlates.
		 *
		 * NOTE: Throws an exception derived from GPlatesScribe::Exceptions::ExceptionBase
		 * if there was an error during serialization of the session state.
		 */
		static
		non_null_ptr_to_const_type
		save_session(
				const QString &project_filename);


		/**
		 * Restores the session state, contained within, to GPlates.
		 *
		 * Throws @a UnsupportedVersion exception if session was created from a
		 * version of GPlates that is either too old or too new.
		 *
		 * NOTE: Throws an exception derived from GPlatesScribe::Exceptions::ExceptionBase
		 * if there was an error during serialization of the session state.
		 *
		 * Returns a list of feature collection files that were not loaded
		 * (either they don't exist or the load failed).
		 */
		virtual
		QStringList
		restore_session() const;

	private:

		/**
		 * The name of the project file containing the serialized session state.
		 *
		 * This is the file being saved to or loaded from.
		 */
		QString d_project_filename;


		/**
		 * Construct a new @a ProjectSession object.
		 */
		ProjectSession(
				const QString &project_filename_,
				const QDateTime &time_,
				const QStringList &filenames_);
	};
}

#endif // GPLATES_PRESENTATION_PROJECTSESSION_H
