/* $Id$ */

/**
 * \file 
 * This file contains a collection of data types which will be used
 * by the PLATES-format parser.  They will probably _not_ be used
 * by any other parser.  These types are really only intended to be
 * temporary place-holders, providing data-types for the parsing
 * before the geometry engine takes over.  These types are currently
 * all structs due to their primitive and temporary nature;
 * if necessary, they may become classes (with a correspondingly more
 * complicated interface of accessors and modifiers).
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_LINEREADER_H
#define GPLATES_FILEIO_LINEREADER_H

#include <iosfwd>
#include <string>
#include <boost/noncopyable.hpp>

#include "utils/SafeBool.h"


namespace GPlatesFileIO
{
	/**
	 * Reads lines in a text file allowing client to peek ahead one line.
	 *
	 * Also handles newline conventions for different platforms:
	 *  - Window:  CR/LF
	 *  - Unix:    LF
	 *  - MacOS X: CR
	 * ...and a single text file can contain a mixture of newline conventions.
	 */
	class LineReader :
			public GPlatesUtils::SafeBool<LineReader>,
			private boost::noncopyable
	{
	public:
		explicit
		LineReader(
				std::istream &stream);

		
		LineReader &
		getline(
				std::string &line);

					
		LineReader &
		peekline(
				std::string &line);


		/**
		 * SafeBool base class provides operator bool().
		 *
		 * Returns true if a line can be read by @a getline.
		 */
		bool
		boolean_test() const
		{
			return d_have_buffered_line || *d_stream_ptr;
		}

		
		unsigned int 
		line_number() const
		{
			return d_line_number;
		}
	
	private:
		std::istream *d_stream_ptr;
		unsigned int d_line_number;
		std::string d_buffered_line;
		bool d_have_buffered_line;


		bool
		readline(
				std::string &line);
	};
}

#endif  // GPLATES_FILEIO_LINEREADER_H
