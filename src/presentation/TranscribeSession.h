/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_TRANSCRIBESESSION_H
#define GPLATES_PRESENTATION_TRANSCRIBESESSION_H

#include <vector>
#include <boost/optional.hpp>
#include <QString>

#include "scribe/ScribeExceptions.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	class Scribe;
}

namespace GPlatesPresentation
{
	namespace TranscribeSession
	{
		/**
		 * Transcribe the session using the specified Scribe.
		 *
		 * This will either save or load depending on @a scribe.
		 *
		 * If @a project_filename is specified then the session state is being transcribed for
		 * a project file, otherwise for an internal session (saved in user preferences state).
		 *
		 * Throws @a UnsupportedVersion exception, on loading, if the transcription is incompatible
		 * (ie, if was generated by a version of GPlates that is either too new or too old).
		 *
		 * Can also throw other exceptions derived in namespace 'GPlatesScribe::Exceptions' if there were
		 * any hard errors (such as incorrectly saving the session, eg, due to not registering a
		 * the polymorphic object type, etc).
		 *
		 * Returns a list of feature collection files that were not loaded
		 * (either they don't exist or the load failed).
		 * This does not apply when saving (in which case an empty list is returned).
		 */
		QList<QString>
		transcribe(
				GPlatesScribe::Scribe &scribe,
				const QList<QString> &feature_collection_files,
				boost::optional<QString> project_filename = boost::none);


		/**
		 * Return a list of filenames of currently loaded files in the application.
		 *
		 * This is used when saving a session.
		 *
		 * Does not return entries for files with no filename
		 * (i.e. "New Feature Collection"s that only exist in memory).
		 */
		QList<QString>
		get_save_session_files();


		/**
		 * Exception that's thrown if a session's archive stream (being read) was written using a
		 * version of GPlates that is either too old (no longer supported due to breaking changes
		 * in the way some of GPlates objects are currently transcribed) or too new
		 * (was written using a future version of GPlates that stopped providing backwards
		 * compatibility for the current version).
		 */
		class UnsupportedVersion :
				public GPlatesScribe::Exceptions::BaseException
		{
		public:

			/**
			 * If @a project_filename is not specified then the session is assumed to be recent
			 * session (ie, not stored in a project file but instead stored in user preferences).
			 */
			explicit
			UnsupportedVersion(
					const GPlatesUtils::CallStack::Trace &exception_source,
					boost::optional<QString> project_filename = boost::none,
					boost::optional< std::vector<GPlatesUtils::CallStack::Trace> >
							transcribe_incompatible_call_stack = boost::none) :
				GPlatesScribe::Exceptions::BaseException(exception_source),
				d_project_filename(project_filename),
				d_transcribe_incompatible_call_stack(transcribe_incompatible_call_stack)
			{  }

			~UnsupportedVersion() throw() { }

			//! Returns the project filename if any.
			boost::optional<QString>
			get_project_filename() const
			{
				return d_project_filename;
			}

			//! Returns the transcribe-incompatible call stack trace, if any.
			boost::optional< std::vector<GPlatesUtils::CallStack::Trace> >
			get_transcribe_incompatible_trace() const
			{
				return d_transcribe_incompatible_call_stack;
			}

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "TranscribeSession::UnsupportedVersion";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			boost::optional<QString> d_project_filename;
			boost::optional< std::vector<GPlatesUtils::CallStack::Trace> > d_transcribe_incompatible_call_stack;

		};
	}
}

#endif // GPLATES_PRESENTATION_TRANSCRIBESESSION_H
