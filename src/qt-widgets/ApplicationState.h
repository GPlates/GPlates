/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APPSTATE_APPLICATIONSTATE_H
#define GPLATES_APPSTATE_APPLICATIONSTATE_H

#include <list>
#include "file-io/FileInfo.h"


namespace GPlatesAppState
{
	/**
	 * Holds information associated with the currently loaded data files.
	 *
	 * ApplicationState is an implementation of the Singleton design pattern.
	 *
	 * @todo Implement scratch buffers.
	 */
	class ApplicationState 
	{
	public:
		typedef std::list<GPlatesFileIO::FileInfo>::iterator file_info_iterator;
		typedef std::list<GPlatesFileIO::FileInfo>::const_iterator file_info_const_iterator;

		file_info_iterator
		files_begin() 
		{ 
			return d_loaded_files.begin();
		}

		file_info_iterator
		files_end() 
		{ 
			return d_loaded_files.end(); 
		}

		file_info_iterator
		push_back_loaded_file(
				const GPlatesFileIO::FileInfo &file_info) 
		{
			return d_loaded_files.insert(files_end(), file_info);
		}

		/**
		 * Remove @a loaded_file from the collection of loaded files.
		 *
		 * This function should be invoked when a feature collection is unloaded by the
		 * user.
		 */
		file_info_iterator
		remove_loaded_file(
				file_info_iterator loaded_file)
		{
			// Unload the feature-collection *before* the iterator becomes invalid.
			loaded_file->unload_feature_collection();
			return d_loaded_files.erase(loaded_file);
		}

		~ApplicationState() 
		{
			delete d_instance;
		}

		static ApplicationState *
		instance()
		{
			if (d_instance == NULL)
			{
				d_instance = new ApplicationState();
			}
			return d_instance;
		}

	private:
		ApplicationState() 
		{ }

		std::list<GPlatesFileIO::FileInfo> d_loaded_files;
		static ApplicationState *d_instance;
	};
}

#endif // GPLATES_APPSTATE_APPLICATIONSTATE_H
