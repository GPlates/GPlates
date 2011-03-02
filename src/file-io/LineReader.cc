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

#include "LineReader.h"

GPlatesFileIO::LineReader::LineReader(
		std::istream &stream) :
	d_stream_ptr(&stream),
	d_line_number(0),
	d_have_buffered_line(false)
{
}


GPlatesFileIO::LineReader &
GPlatesFileIO::LineReader::getline(
		std::string &line)
{
	if (d_have_buffered_line)
	{
		line = d_buffered_line;
		d_have_buffered_line = false;
		d_line_number++;
	}
	else if (readline(line))
	{
		d_line_number++;
	}
	
	return *this;
}


GPlatesFileIO::LineReader &
GPlatesFileIO::LineReader::peekline(
		std::string &line)
{
	if (d_have_buffered_line)
	{
		line = d_buffered_line;
	}
	else if (readline(d_buffered_line))
	{
		line = d_buffered_line;
		d_have_buffered_line = true;
	}

	return *this;
}


bool
GPlatesFileIO::LineReader::readline(
		std::string &line)
{
	return std::getline(*d_stream_ptr, line);
#if defined(__WINDOWS__)
	// On windows std::getline:
	// CR/LF -> NULL
	// LF    -> NULL
	// CR    -> CR
	//
#elif defined(__APPLE__)
	// On MacOS std::getline:
	// CR/LF -> LF
	// LF    -> LF
	// CR    -> NULL
#else
	/* Other platforms - assume unix */
	// On unix std::getline:
	// CR/LF -> CR
	// LF    -> NULL
	// CR    -> CR
#endif
}
