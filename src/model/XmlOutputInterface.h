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

#ifndef GPLATES_FILEIO_XMLOUTPUTINTERFACE_H
#define GPLATES_FILEIO_XMLOUTPUTINTERFACE_H

#include <iosfwd>
#include <unicode/unistr.h>


namespace GPlatesFileIO {

	/**
	 * This class provides a convenient interface for XML output.
	 *
	 * Client code performs output using XML-oriented functions such as
	 * @a write_opening_element and @a write_string_content_line.
	 */
	class XmlOutputInterface {

	public:

		/**
		 * Elements of this enumeration represent the possible status of the interface.
		 */
		enum Status {
			NO_ERROR,
			WRITE_ERROR
		};

		/**
		 * This class provides a convenient means to automate the closing of opened
		 * elements (and maintain the correct nesting of elements) using a mechanism
		 * similar to RAII.
		 */
		class ElementPairStackFrame {

		public:
			ElementPairStackFrame(
					XmlOutputInterface &interface,
					const UnicodeString &elem_name);

			~ElementPairStackFrame();

		private:

			XmlOutputInterface *d_interface_ptr;
			UnicodeString d_elem_name;

			// This constructor should never be defined, because we don't want to allow
			// copy-construction.
			//
			// (If we prevent copy-construction, an ElementPairStackFrame instance
			// can't outlive its XmlOutputInterface, unless you start doing
			// gratuitously stupid things like allocating the ElementPairStackFrame on
			// the heap, which, incidentally, would defeat the whole purpose of
			// ElementPairStackFrame.)
			ElementPairStackFrame(
					const ElementPairStackFrame &);

			// This operator should never be defined, because we don't want to allow
			// copy-assignment.
			ElementPairStackFrame &
			operator=(
					const ElementPairStackFrame &);
		};

		/**
		 * Create a new interface instance which will write to the standard output stream.
		 *
		 * The parameter @a indentation_unit is the string which is output for indentation,
		 * once per level of indentation.
		 */
		static
		const XmlOutputInterface
		create_for_stdout(
				const UnicodeString &indentation_unit = " ");

		/**
		 * Return the status of this instance.
		 */
		Status
		status() const {
			return d_status;
		}

		/**
		 * Set the status of this instance.
		 */
		void
		set_status(
				Status new_status) {
			d_status = new_status;
		}

		/**
		 * Write an opening element.
		 *
		 * The element will be named @a elem_name.
		 *
		 * The function will indent the element and append a newline.
		 */
		void
		write_opening_element(
				const UnicodeString &elem_name);

		/**
		 * Write an opening XML element which contains attributes.
		 *
		 * The element will be named @a elem_name.
		 *
		 * The attributes will be accessed through the parameters @a attrs_pair_begin and
		 * @a attrs_pair_end.  These parameters are assumed to be forward iterators, one
		 * being the "begin" iterator of an iterable range of attributes, the other being
		 * the "end" iterator.
		 *
		 * The following requirements must be satisfied by the template type @a F for the
		 * template instantiation of this function to succeed during compilation:
		 *  - The iterator type must possess the operators:
		 *    - equality-comparison operator;
		 *    - pre-increment operator;
		 *    - member-access-through-pointer operator.
		 *  - The referenced type must possess the members:
		 *    - @c first (of type reference-to-const-UnicodeString)
		 *    - @c second (of type reference-to-const-UnicodeString)
		 *
		 * The following requirements must be satisfied by the types and values of
		 * @a attrs_pair_begin and @a attrs_pair_end for this function to execute in a
		 * meaningful fashion:
		 *  - @a attrs_pair_begin must be either equal to @a attrs_pair_end (using the
		 *    type's equality-comparison operator) or incrementable (using the type's
		 *    pre-increment operator) to be equal to @a attrs_pair_end.
		 *  - The member-access-through-pointer operator must allow the same element to be
		 *    accessed more than once.
		 *
		 * For example, the iterators of @c std::map<UnicodeString, UnicodeString> would
		 * fulfil these requirements, as would the iterators of
		 * @c std::vector<std::pair<UnicodeString, UnicodeString> @c > and
		 * @c std::list<std::pair<UnicodeString, UnicodeString> @c >.
		 *
		 * The function will indent the element and append a newline.
		 */
		template<typename F>
		void
		write_opening_element_with_attributes(
				const UnicodeString &elem_name,
				F attrs_pair_begin,
				F attrs_pair_end);

		/**
		 * Write a closing element.
		 *
		 * The element will be named @a elem_name.
		 *
		 * The function will indent the element and append a newline.
		 */
		void
		write_closing_element(
				const UnicodeString &elem_name);

		/**
		 * Write an empty element.
		 *
		 * The element will be named @a elem_name.
		 *
		 * The function will indent the element and append a newline.
		 */
		void
		write_empty_element(
				const UnicodeString &elem_name);

		/**
		 * Write a line of string content.
		 *
		 * The function will indent the line and append a newline.
		 */
		void
		write_string_content_line(
				const UnicodeString &line);

	protected:

		XmlOutputInterface(
				std::ostream &os,
				const UnicodeString &indentation_unit):
			d_os_ptr(&os),
			d_indentation_unit(indentation_unit),
			d_indentation_level(0),
			d_status(NO_ERROR)
		{  }

		void
		write_indentation();

		void
		write_unicode_string(
				const UnicodeString &s);

	private:

		/*
		 * This pointer (rather than its target) can (and should) be copied in
		 * copy-constructors and copy-assignment operators.
		 *
		 * When the object is destructing, the pointer should simply be allowed to expire:
		 * Do not attempt to deallocate the target of the pointer, since the target is a
		 * pre-existing standard stream.
		 */
		std::ostream *d_os_ptr;

		/*
		 * This is the string which is output for indentation of the XML output, once per
		 * level of indentation.
		 */
		UnicodeString d_indentation_unit;

		/*
		 * This is the current indentation level of the XML output.
		 */
		unsigned d_indentation_level;

		/*
		 * This is the current status of the interface.
		 */
		Status d_status;

	};


	template<typename F>
	inline
	void
	XmlOutputInterface::write_opening_element_with_attributes(
			const UnicodeString &elem_name,
			F attrs_pair_begin,
			F attrs_pair_end) {
		write_indentation();
		write_unicode_string("<");
		write_unicode_string(elem_name);

		for ( ; attrs_pair_begin != attrs_pair_end; ++attrs_pair_begin) {
			write_unicode_string(" ");
			write_attribute_name(attrs_pair_begin->first);
			write_unicode_string("=\"");
			write_attribute_value(attrs_pair_begin->second);
			write_unicode_string("\"");
		}
		write_unicode_string(">\n");

		++d_indentation_level;
	}

}

#endif  // GPLATES_FILEIO_XMLOUTPUTINTERFACE_H
