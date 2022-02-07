/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include <iostream>

#include "XmlOutputInterface.h"


namespace
{
	void
	write_xml_header_line(std::ostream *os)
	{
		// This header is required in any XML document.  Note that we have fixed the
		// encoding at the moment.
		static const GPlatesUtils::UnicodeString XML_HEADER = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		*os << XML_HEADER;
	}
}


GPlatesFileIO::XmlOutputInterface::ElementPairStackFrame::ElementPairStackFrame(
		XmlOutputInterface &interface,
		const GPlatesUtils::UnicodeString &elem_name):
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
		const GPlatesUtils::UnicodeString &indentation_unit) {
	return XmlOutputInterface(std::cout, indentation_unit);
}


const GPlatesFileIO::XmlOutputInterface
GPlatesFileIO::XmlOutputInterface::create_for_stream(
		std::ostream &output_stream,
		const GPlatesUtils::UnicodeString &indentation_unit) {
	return XmlOutputInterface(std::cout, indentation_unit);
}


void
GPlatesFileIO::XmlOutputInterface::write_opening_element(
		const GPlatesUtils::UnicodeString &elem_name) {
	write_indentation();
	write_unicode_string("<");
	write_unicode_string(elem_name);
	write_unicode_string(">\n");

	++d_indentation_level;
}


void
GPlatesFileIO::XmlOutputInterface::write_closing_element(
		const GPlatesUtils::UnicodeString &elem_name) {
	--d_indentation_level;

	write_indentation();
	write_unicode_string("</");
	write_unicode_string(elem_name);
	write_unicode_string(">\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_empty_element(
		const GPlatesUtils::UnicodeString &elem_name) {
	write_indentation();
	write_unicode_string("<");
	write_unicode_string(elem_name);
	write_unicode_string(" />\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_line_of_string_content(
		const GPlatesUtils::UnicodeString &content) {
	write_indentation();
	write_unicode_string(content);
	write_unicode_string("\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_line_of_single_integer_content(
		const long &content) {
	write_indentation();
	// CHECKME:  Do we need to worry about ensuring the locale is appropriate?
	*d_os_ptr << content;
	write_unicode_string("\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_line_of_single_decimal_content(
		const double &content) {
	write_indentation();
	// CHECKME:  Do we need to worry about ensuring the locale is appropriate?
	*d_os_ptr << content;
	write_unicode_string("\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_line_of_decimal_duple_content(
		const double &first,
		const double &second) {
	write_indentation();
	// CHECKME:  Do we need to worry about ensuring the locale is appropriate?
	*d_os_ptr << first;
	write_unicode_string(" ");
	*d_os_ptr << second;
	write_unicode_string("\n");
}


void
GPlatesFileIO::XmlOutputInterface::write_line_of_boolean_content(
		const bool &content)
{
	write_indentation();
	// CHECKME:  Do we need to worry about ensuring the locale is appropriate?
	*d_os_ptr << std::boolalpha << content;
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
		const GPlatesUtils::UnicodeString &s) {

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
	*d_os_ptr << xan.build_aliased_name();
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


void
GPlatesFileIO::XmlOutputInterface::flush_underlying_stream()
{
	if (status() == NO_ERROR) {
		*d_os_ptr << std::flush;
	}
}


GPlatesFileIO::XmlOutputInterface::XmlOutputInterface(
		std::ostream &os,
		const GPlatesUtils::UnicodeString &indentation_unit) :
	d_os_ptr(&os),
	d_indentation_unit(indentation_unit),
	d_indentation_level(0),
	d_status(NO_ERROR)
{
	write_xml_header_line(d_os_ptr);
}
