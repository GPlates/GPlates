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
	// Assume input text file is UTF8 encoded (which includes the ASCII character set).
	// If we don't specify this then UTF8 characters will not decoded correctly on reading.
	d_text_stream.setCodec("UTF-8");
}


bool
GPlatesFileIO::LineReader::getline(
		QString &line)
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
		QString &line)
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
		QString &line)
{
	if (d_text_stream.atEnd())
	{
		return false;
	}

	// Note that QTextStream::readLine() recognises both "\n" (used by Unix and Mac OS X) and
	// "\r\n" (used by Windows). It doesn't recognise "\r" used by Macs prior to Mac OS X, but
	// those are very old systems and hopefully we don't have any (or many) files around these
	// days with just "\r".
	line = d_text_stream.readLine();

	return true;
}
