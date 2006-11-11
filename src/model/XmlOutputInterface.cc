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
#include "XmlOutputInterface.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


GPlatesFileIO::XmlOutputInterface::ElementPairStackFrame::ElementPairStackFrame(
		XmlOutputInterface &interface,
		const UnicodeString &elem_name):
	d_interface_ptr(&interface),
	d_elem_name(elem_name) {
	d_interface_ptr->write_opening_element(d_elem_name);
}


GPlatesFileIO::XmlOutputInterface::ElementPairStackFrame::~ElementPairStackFrame() {
	// Do not allow any exceptions to leave this destructor.
	try {
		d_interface_ptr->write_closing_element(d_elem_name);
	} catch (...) {
		// There's not really anything useful we can do here.
	}
}


const GPlatesFileIO::XmlOutputInterface
GPlatesFileIO::XmlOutputInterface::create_for_stdout(
		const UnicodeString &indentation_unit) {
	return XmlOutputInterface(std::cout, indentation_unit);
}


void
GPlatesFileIO::XmlOutputInterface::write_opening_element(
		const UnicodeString &elem_name) {
	write_indentation();
	write_unicode_string("<");
	write_unicode_string(elem_name);
	write_unicode_string(">\n");

	++d_indentation_level;
}


void
GPlatesFileIO::XmlOutputInterface::write_closing_element(
		const UnicodeString &elem_name) {
	--d_indentation_level;

	write_indentation();
	write_unicode_string("</");
	write_unicode_string(elem_name);
	write_unicode_string(">\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_empty_element(
		const UnicodeString &elem_name) {
	write_indentation();
	write_unicode_string("<");
	write_unicode_string(elem_name);
	write_unicode_string(" />\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_line_of_string_content(
		const UnicodeString &content) {
	write_indentation();
	write_unicode_string(content);
	write_unicode_string("\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_line_of_integer_content(
		const long &content) {
	write_indentation();
	// CHECKME:  Do we need to worry about ensuring the locale is appropriate?
	*d_os_ptr << content;
	write_unicode_string("\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_line_of_decimal_content(
		const double &content) {
	write_indentation();
	// CHECKME:  Do we need to worry about ensuring the locale is appropriate?
	*d_os_ptr << content;
	write_unicode_string("\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_indentation() {
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
GPlatesFileIO::XmlOutputInterface::write_unicode_string(
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


void
GPlatesFileIO::XmlOutputInterface::write_attribute_name(
		const GPlatesModel::XmlAttributeName &xan) {

	// FIXME:  This function should filter out any characters which are not suitable for
	// attribute names.

	if (status() != NO_ERROR) {
		// Some error has previously occurred.
		return;
	}
	*d_os_ptr << xan.get();
	if ( ! *d_os_ptr) {
		// There was an error during writing.
		set_status(WRITE_ERROR);
	}
}


void
GPlatesFileIO::XmlOutputInterface::write_attribute_value(
		const GPlatesModel::XmlAttributeValue &xav) {

	// FIXME:  This function should filter out (or transform?) any characters which are not
	// suitable for attribute values.

	if (status() != NO_ERROR) {
		// Some error has previously occurred.
		return;
	}
	*d_os_ptr << xav.get();
	if ( ! *d_os_ptr) {
		// There was an error during writing.
		set_status(WRITE_ERROR);
	}
}


void
GPlatesFileIO::XmlOutputInterface::write_decimal_content(
		const double &content) {
	// CHECKME:  Do we need to worry about ensuring the locale is appropriate?
	*d_os_ptr << content;
}
