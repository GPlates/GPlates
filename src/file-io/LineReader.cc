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
		QFile &input) :
	d_text_stream(&input),
	d_line_number(0),
	d_have_buffered_line(false)
{
}


bool
GPlatesFileIO::LineReader::getline(
		std::string &line)
{
	if (d_have_buffered_line)
	{
		line = d_buffered_line;
		d_have_buffered_line = false;
		d_line_number++;

		return true;
	}
	else if (readline(line))
	{
		d_line_number++;

		return true;
	}
	
	return false;
}


bool
GPlatesFileIO::LineReader::peekline(
		std::string &line)
{
	if (d_have_buffered_line)
	{
		line = d_buffered_line;

		return true;
	}
	else if (readline(d_buffered_line))
	{
		line = d_buffered_line;
		d_have_buffered_line = true;

		return true;
	}

	return false;
}


bool
GPlatesFileIO::LineReader::readline(
		std::string &line)
{
	if (d_text_stream.atEnd())
	{
		return false;
	}

	// Using QFile but still returning std::string to avoid introducing bugs into clients of this class.
	// TODO: Change this class and clients to use 'QString' instead of 'std::string'.
	line = d_text_stream.readLine().toStdString();

	return true;

	//
	// TODO: Handle mixtures of newline conventions.
	//
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
