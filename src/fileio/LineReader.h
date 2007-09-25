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

namespace GPlatesFileIO {

	class LineReader 
	{
	public:
		explicit
		LineReader(
				std::istream &stream) :
			d_stream_ptr(&stream),
			d_line_number(0)
		{ }
		
		LineReader &
		getline(
				std::string &line)
		{
			if (std::getline(*d_stream_ptr, line)) {
				d_line_number++;
			}
			
			return *this;
		}
		
		operator bool() const
		{
			return *d_stream_ptr;
		}
		
		unsigned int 
		line_number() const
		{
			return d_line_number;
		}
	
	private:
		// LineReader instances should not be copy-assigned or copy-constructed
		LineReader &
		operator=(
				const LineReader &rhs);

		LineReader(
				const LineReader &rhs);

		std::istream *d_stream_ptr;
		unsigned int d_line_number;
	};

}

#endif /* GPLATES_FILEIO_LINEREADER_H */
