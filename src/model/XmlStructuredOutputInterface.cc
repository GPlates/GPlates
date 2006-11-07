/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <iostream>
#include <unicode/ustream.h>
#include "XmlStructuredOutputInterface.h"


const GPlatesFileIO::XmlStructuredOutputInterface
GPlatesFileIO::XmlStructuredOutputInterface::create_for_stdout(
		const UnicodeString &indentation_unit) {
	return XmlStructuredOutputInterface(std::cout, indentation_unit);
}


void
GPlatesFileIO::XmlStructuredOutputInterface::write_opening_element(
		const UnicodeString &elem_name) {
	write_indentation();
	write_unicode_string("<");
	write_unicode_string(elem_name);
	write_unicode_string(">\n");

	++d_indentation_level;
}


void
GPlatesFileIO::XmlStructuredOutputInterface::write_closing_element(
		const UnicodeString &elem_name) {
	--d_indentation_level;

	write_indentation();
	write_unicode_string("</");
	write_unicode_string(elem_name);
	write_unicode_string(">\n");
}


void
GPlatesFileIO::XmlStructuredOutputInterface::write_empty_element(
		const UnicodeString &elem_name) {
	write_indentation();
	write_unicode_string("<");
	write_unicode_string(elem_name);
	write_unicode_string(" />\n");
}


void
GPlatesFileIO::XmlStructuredOutputInterface::write_string_content_line(
		const UnicodeString &line) {
	write_indentation();
	write_unicode_string(line);
	write_unicode_string("\n");
}


void
GPlatesFileIO::XmlStructuredOutputInterface::write_indentation() {
	if (status() != NO_ERROR) {
		// Some error has previously occurred.
		return;
	}
	for (unsigned i = 0; i < d_indentation_level; ++i) {
		*d_os_ptr << d_indentation_unit;
		if ( ! *d_os_ptr) {
			// There was an error during writing.
			set_status(WRITE_ERROR);
		}
	}
}


void
GPlatesFileIO::XmlStructuredOutputInterface::write_unicode_string(
		const UnicodeString &s) {

	// FIXME:  This function should escape any occurrences of '<' or '&' (to "&lt;" and
	// "&amp;", respectively).

	// FIXME:  This function should increment a line-counter each time it encounters a newline
	// in the unicode string.

	// FIXME:  This function should check for any cases in which there is a newline character
	// followed by one or more non-newline characters.  In such cases, every newline character
	// which is followed by non-newline characters should have the current indentation inserted
	// after it.

	if (status() != NO_ERROR) {
		// Some error has previously occurred.
		return;
	}
	*d_os_ptr << s;
	if ( ! *d_os_ptr) {
		// There was an error during writing.
		set_status(WRITE_ERROR);
	}
}
