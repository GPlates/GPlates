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

#ifndef GPLATES_PRESENTATION_SESSION_H
#define GPLATES_PRESENTATION_SESSION_H

#include <QCoreApplication>
#include <QDateTime>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#include "utils/ReferenceCount.h"


namespace GPlatesPresentation
{
	class Application;

	/**
	 * Base class encapsulates a session of GPlates including files loaded and the layers state.
	 *
	 * A derived class @a InternalSession or @a ProjectSession should be instantiated depending on whether the
	 * session state is stored in a text archive (in UserPreferences) or a binary archive (project file).
	 */
	class Session :
			public GPlatesUtils::ReferenceCount<Session>
	{
		Q_DECLARE_TR_FUNCTIONS(Session)

	public:

		// Convenience typedefs for a shared pointer to a @a Session.
		typedef GPlatesUtils::non_null_intrusive_ptr<Session> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Session> non_null_ptr_to_const_type;


		virtual
		~Session()
		{  }


		/**
		 * Textual description suitable for menus, e.g.
		 * "5 files on Mon Nov 1, 5:57 PM"
		 */
		QString
		get_description() const;


		/**
		 * The time when the session was saved; usually the time GPlates
		 * last quit while these files were active.
		 */
		const QDateTime &
		get_time() const;


		/**
		 * Which files were active when the session was saved.
		 */
		QList<QString>
		get_loaded_files() const;


		/**
		 * It is possible to have an 'empty' session without any files.
		 *
		 * The definition of an empty session could change.
		 * For example, if the view position/orientation is saved as session state then is it still
		 * possible to have an empty session? Currently the answer is "yes" because not all
		 * session state is considered in the definition of 'empty'.
		 */
		bool
		is_empty() const;


		/**
		 * Comparing two Session together should ignore the datestamp and
		 * focus on whether the list of files match; this is so that
		 * GPlates can be a bit smarter about how the Recent Sessions menu
		 * operates w.r.t. people loading/saving prior sessions.
		 *
		 * Changes in Layer configuration should also not affect equality.
		 * This is because we are only testing for equality to see which
		 * Session Menu labels should be added, or merely refreshed and
		 * "bumped" to the top.
		 */
		bool
		has_same_loaded_files_as(
				const Session &other) const;


		/**
		 * Restores the session state, contained within, to GPlates.
		 *
		 * Throws @a UnsupportedVersion exception if session was created from a
		 * version of GPlates that is either too old or too new.
		 *
		 * NOTE: Throws an exception derived from GPlatesScribe::Exceptions::ExceptionBase
		 * if there was an error during serialization of the session state.
		 *
		 * Any files that were not loaded (either they don't exist or the load failed) get reported
		 * in the read errors dialog.
		 */
		virtual
		void
		restore_session() const = 0;

	protected:


		/**
		 * Construct a new Session object to represent a specific collection
		 * of files that were loaded in GPlates at some time.
		 *
		 * @a files_ is a collection of absolute path names, obtained via QFileInfo::absoluteFilePath().
		 */
		Session(
				const QDateTime &time_,
				const QStringList &files_);

	private:

		/**
		 * The time when the session was saved; usually the time GPlates
		 * last quit while these files were active.
		 */
		QDateTime d_time;

		/**
		 * Which files were active when the session was saved.
		 */
		QSet<QString> d_loaded_files;
	};
}

#endif // GPLATES_PRESENTATION_SESSION_H
