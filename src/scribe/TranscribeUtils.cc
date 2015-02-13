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

#include "TranscribeUtils.h"

#include "ScribeInternalAccess.h"


void
GPlatesScribe::TranscribeUtils::FilePath::set_file_path(
		const QString &file_path)
{
	d_split_paths = file_path.split('/');
}


QString
GPlatesScribe::TranscribeUtils::FilePath::get_file_path() const
{
	return d_split_paths.join("/");
}


GPlatesScribe::TranscribeResult
GPlatesScribe::TranscribeUtils::FilePath::transcribe(
		Scribe &scribe,
		bool transcribed_construct_data)
{
	//
	// UPDATE/FIXME: Due to tracking issues, where an untracked sequence (eg, QStringList)
	// still has tracked objects (but we don't want to track them), we will explicitly
	// use the sequence protocol instead of transcribing the QStringList sequence.
	//

	if (scribe.is_saving())
	{
		unsigned int split_paths_size = d_split_paths.size();
		scribe.transcribe(TRANSCRIBE_SOURCE, split_paths_size, "size", DONT_TRACK);

		for (unsigned int p = 0; p < split_paths_size; ++p)
		{
			scribe.transcribe(TRANSCRIBE_SOURCE, d_split_paths[p], "item", DONT_TRACK);
		}
	}
	else // loading...
	{
		d_split_paths.clear();

		unsigned int split_paths_size;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, split_paths_size, "size", DONT_TRACK))
		{
			return scribe.get_transcribe_result();
		}

		for (unsigned int p = 0; p < split_paths_size; ++p)
		{
			QString split_path;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, split_path, "item", DONT_TRACK))
			{
				return scribe.get_transcribe_result();
			}

			d_split_paths.append(split_path);
		}
	}

	return TRANSCRIBE_SUCCESS;
}
