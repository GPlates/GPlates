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

#include <utility>
#include <boost/optional.hpp>
#include <QMap>
#include <QString>
#include <QStringList>

#include "Session.h"

#include "scribe/Transcription.h"


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
		non_null_ptr_type
		create_restore_session(
				QString project_filename);


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
		non_null_ptr_type
		save_session(
				QString project_filename);


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
		restore_session() const;


		/**
		 * Returns the project filename (passed into @a create_restore_session or @a save_session).
		 *
		 * This is the filename currently being saved to or loaded from.
		 */
		QString
		get_project_filename() const
		{
			return d_project_filename;
		}


		/**
		 * Returns the number of file paths transcribed via the TranscribeUtils::FilePath API.
		 */
		int
		get_num_file_paths() const
		{
			return d_all_file_paths_when_saved.size();
		}


		/**
		 * Returns true if the project file being loaded has moved from where it was saved.
		 *
		 * This can happen when loading a project. When saving a project this always returns true.
		 */
		bool
		has_project_file_moved() const;


		/**
		 * Returns unique sorted lists of all absolute file paths of transcribed files that
		 * currently exist and are currently missing.
		 *
		 * These are file paths transcribed via the TranscribeUtils::FilePath API when the project
		 * file was saved (but with Windows drive letters or share names added/removed to suit
		 * the local/runtime system).
		 *
		 * Note: These are the file paths transcribed into the project file when it was saved, and the
		 * files might no longer exist or might have been incorrectly moved to another location or machine.
		 */
		void
		get_absolute_file_paths(
				QStringList &existing_absolute_file_paths,
				QStringList &missing_absolute_file_paths) const;


		/**
		 * Returns unique sorted lists of all relative file paths of transcribed files that
		 * currently exist and are currently missing.
		 *
		 * These file paths are relative to the location of the project file being loaded
		 * (except when a relative path cannot be formed - eg, a different drive letter - in which case
		 * the originally saved absolute path, converted to the local system, is used instead).
		 * This can be different to the absolute file paths, transcribed when the project file was saved,
		 * if the project file (being loaded) has moved location since it was saved.
		 */
		void
		get_relative_file_paths(
				QStringList &existing_relative_file_paths,
				QStringList &missing_relative_file_paths) const;


		/**
		 * Specify whether to use file paths that are relative to the project file when loading data files
		 * (when @a restore_session is called) - see @a get_relative_file_paths.
		 *
		 * By default uses the absolute file paths transcribed into the project -
		 * see @a get_absolute_file_paths.
		 *
		 * This is useful when the project file has moved and the data files have also moved such
		 * that their locations relative to the project file are unchanged (for example, when zipping
		 * the project and data files, and unzipping in another location or on another machine).
		 */
		void
		set_load_relative_file_paths(
				bool load_relative_file_paths = true);


		/**
		 * Specify whether to remap missing file paths to existing file paths.
		 *
		 * This is used to rename missing files to existing files when a loaded project references
		 * files that no longer exist (see @a get_absolute_file_paths and @a get_relative_file_paths).
		 *
		 * In the case of relative file paths (see @a set_load_relative_file_paths)
		 * the remapping is from file paths relative to the loaded project file location.
		 */
		void
		set_remapped_file_paths(
				boost::optional< QMap<QString/*missing*/, QString/*existing*/> > file_path_remapping);


		/**
		 * Compare the current session state with the last saved or restored session state to see
		 * if the session state has changed.
		 *
		 * This method saves a temporary copy of the current session state and then compares it with the
		 * session state when @a save_session or @a restore_session was last called on this project session.
		 *
		 * Note: This does not detect unsaved changes to feature collection files.
		 * That's handled by class UnsavedChangesTracker.
		 */
		bool
		has_session_state_changed() const;

	private:

		/**
		 * The name of the project file containing the session state.
		 *
		 * This is the file currently being saved to or loaded from.
		 */
		QString d_project_filename;

		/**
		 * The project filename when the project was saved.
		 *
		 * This can be different than @a d_project_filename when loading a project that has moved.
		 *
		 * Note that we haven't added/removed Windows drive letter or share name because
		 * we want to compare the project filename on the system the project was saved on
		 * with the data filenames on the same save system.
		 */
		QString d_project_filename_when_saved;

		/**
		 * A unique sorted list of all transcribed filenames (transcribed via the TranscribeUtils::FilePath API)
		 * when the project was saved.
		 *
		 * Note that we haven't added/removed Windows drive letter or share name because
		 * we want to compare the project filename on the system the project was saved on
		 * with the data filenames on the same save system.
		 */
		QStringList d_all_file_paths_when_saved;

		/**
		 * Whether to use file paths that are relative to the loaded project file location when
		 * loading data files (rather than relative to the location the project file was saved).
		 */
		boost::optional< std::pair<
				QString/*project_file_path_when_saved*/,
				QString/*project_file_path_when_loaded*/> > d_load_files_relative_to_project;

		/**
		 * Whether to remap missing file paths to existing file paths.
		 */
		boost::optional< QMap<QString/*missing*/, QString/*existing*/> > d_file_path_remapping;

		/**
		 * Record the last session state saved or restored by this project file.
		 *
		 * Note that this is none if a project session has not yet been restored - ie, a project session
		 * created with @a create_restore_session that has not yet called @a restore_session.
		 */
		mutable boost::optional<GPlatesScribe::Transcription::non_null_ptr_to_const_type> d_last_saved_or_restored_session_state;


		/**
		 * Construct a new @a ProjectSession object.
		 */
		ProjectSession(
				const QString &project_filename_,
				const QString &project_filename_when_saved_,
				const QDateTime &time_,
				const QStringList &filenames_,
				const QStringList &all_file_paths_when_saved_,
				boost::optional<GPlatesScribe::Transcription::non_null_ptr_to_const_type> last_saved_or_restored_session_state = boost::none);
	};
}

#endif // GPLATES_PRESENTATION_PROJECTSESSION_H
