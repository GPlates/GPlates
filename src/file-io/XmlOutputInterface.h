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

#ifndef GPLATES_FILEIO_XMLOUTPUTINTERFACE_H
#define GPLATES_FILEIO_XMLOUTPUTINTERFACE_H

#include <iosfwd>

#include "global/unicode.h"

// Even though we only pass XmlAttributeName and XmlAttributeValue by reference in this header, we
// can't just forward-declare the class, since XmlAttributeName and XmlAttributeValue are typedefs
// for template type-instantiations.
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


namespace GPlatesFileIO
{
	/**
	 * This class provides a convenient interface for XML output.
	 *
	 * Client code performs output using XML-oriented functions such as
	 * @a write_opening_element and @a write_line_of_string_content.
	 */
	class XmlOutputInterface
	{
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
			/**
			 * Open an element pair.
			 *
			 * The element will be named @a elem_name.
			 */
			ElementPairStackFrame(
					XmlOutputInterface &interface,
					const GPlatesUtils::UnicodeString &elem_name);

			/**
			 * Open an element pair whose opening element contains attributes.
			 *
			 * The element will be named @a elem_name.
			 *
			 * The attributes will be accessed through the parameters
			 * @a attrs_pair_begin and @a attrs_pair_end.  These parameters are assumed
			 * to be forward iterators, one being the "begin" iterator of an iterable
			 * range of attributes, the other being the "end" iterator.
			 *
			 * The following requirements must be satisfied by the template type @a F
			 * for the template instantiation of this function to succeed during
			 * compilation:
			 *  - The iterator type must possess the operators:
			 *    - equality-comparison operator;
			 *    - pre-increment operator;
			 *    - member-access-through-pointer operator.
			 *  - The referenced type must possess the members:
			 *    - @c first (of type reference-to-const-UnicodeString)
			 *    - @c second (of type reference-to-const-UnicodeString)
			 *
			 * The following requirements must be satisfied by the types and values of
			 * @a attrs_pair_begin and @a attrs_pair_end for this function to execute
			 * in a meaningful fashion:
			 *  - @a attrs_pair_begin must be either equal to @a attrs_pair_end (using
			 *    the type's equality-comparison operator) or incrementable (using the
			 *    type's pre-increment operator) to be equal to @a attrs_pair_end.
			 *  - The member-access-through-pointer operator must allow the same
			 *    element to be accessed more than once.
			 *
			 * For example, the iterators of @c std::map<UnicodeString, UnicodeString>
			 * would fulfil these requirements, as would the iterators of
			 * @c std::vector<std::pair<UnicodeString, UnicodeString> @c > and
			 * @c std::list<std::pair<UnicodeString, UnicodeString> @c >.
			 */
			template<typename F>
			ElementPairStackFrame(
					XmlOutputInterface &interface,
					const GPlatesUtils::UnicodeString &elem_name,
					F attrs_pair_begin,
					F attrs_pair_end);

			~ElementPairStackFrame();

		private:

			XmlOutputInterface *d_interface_ptr;
			GPlatesUtils::UnicodeString d_elem_name;

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
				const GPlatesUtils::UnicodeString &indentation_unit = "\t");
		
		/**
		 * Create a new interface instance which will write to an output stream.
		 *
		 * The parameter @a output_stream is the stream object to use for output.
		 *
		 * The parameter @a indentation_unit is the string which is output for indentation,
		 * once per level of indentation.
		 */		
		static
		const XmlOutputInterface
		create_for_stream(
				std::ostream &output_stream,
				const GPlatesUtils::UnicodeString &indentation_unit = "\t");

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
				const GPlatesUtils::UnicodeString &elem_name);

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
				const GPlatesUtils::UnicodeString &elem_name,
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
				const GPlatesUtils::UnicodeString &elem_name);

		/**
		 * Write an empty element.
		 *
		 * The element will be named @a elem_name.
		 *
		 * The function will indent the element and append a newline.
		 */
		void
		write_empty_element(
				const GPlatesUtils::UnicodeString &elem_name);

		/**
		 * Write a line of string content.
		 *
		 * The function will indent the line and append a newline.
		 */
		void
		write_line_of_string_content(
				const GPlatesUtils::UnicodeString &content);

		/**
		 * Write a line of content consisting of a single integer.
		 *
		 * The function will indent the line and append a newline.
		 */
		void
		write_line_of_single_integer_content(
				const long &content);

		/**
		 * Write a line of content consisting of a single decimal.
		 *
		 * The function will indent the line and append a newline.
		 */
		void
		write_line_of_single_decimal_content(
				const double &content);

		/**
		 * Write a line of content consisting of a duple of decimals.
		 *
		 * The function will indent the line and append a newline.
		 */
		void
		write_line_of_decimal_duple_content(
				const double &first,
				const double &second);

		/**
		 * Write a line of content consisting of multiple decimals.
		 *
		 * The values will be accessed through the parameters @a content_begin and
		 * @a content_end.  These parameters are assumed to be forward iterators, one being
		 * the "begin" iterator of an iterable range of doubles, the other being the "end"
		 * iterator.
		 *
		 * The following requirements must be satisfied by the template type @a F for the
		 * template instantiation of this function to succeed during compilation:
		 *  - The iterator type must possess the operators:
		 *    - equality-comparison operator;
		 *    - pre-increment operator;
		 *    - dereference operator.
		 *  - The referenced type must be a 'double'.
		 *
		 * The following requirements must be satisfied by the types and values of
		 * @a content_begin and @a content_end for this function to execute in a meaningful
		 * fashion:
		 *  - @a content_begin must be either equal to @a content_end (using the type's
		 *    equality-comparison operator) or incrementable (using the type's
		 *    pre-increment operator) to be equal to @a content_end.
		 *
		 * If the range is non-empty, the function will indent the line, write the content,
		 * and append a newline.  If the range is empty, the function will do nothing.
		 */
		template<typename F>
		void
		write_line_of_multi_decimal_content(
				F content_begin,
				F content_end);

		/**
		 * Write a line of content which is the string version of the boolean value
		 * given.
		 *
		 * The function will indent the line and append a newline.
		 */
		void
		write_line_of_boolean_content(
				const bool &content);

		/**
		 * Flush the underlying stream.  If @c status() returns @c WRITE_ERROR,
		 * then this method is a no-op.
		 */
		void
		flush_underlying_stream();


		/**
		 * The destructor of XmlOutputInterface flushes the underlying stream
		 * but does not close it (since XmlOutputInterface is not responsible
		 * for the output stream as a resource).
		 */
		virtual
		~XmlOutputInterface()
		{
			flush_underlying_stream();
		}


	protected:

		XmlOutputInterface(
				std::ostream &os,
				const GPlatesUtils::UnicodeString &indentation_unit);

		void
		write_indentation();

		void
		write_unicode_string(
				const GPlatesUtils::UnicodeString &s);

		void
		write_attribute_name(
				const GPlatesModel::XmlAttributeName &xan);

		void
		write_attribute_value(
				const GPlatesModel::XmlAttributeValue &xav);

		void
		write_decimal_content(
				const double &content);

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
		GPlatesUtils::UnicodeString d_indentation_unit;

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
	XmlOutputInterface::ElementPairStackFrame::ElementPairStackFrame(
			XmlOutputInterface &interface,
			const GPlatesUtils::UnicodeString &elem_name,
			F attrs_pair_begin,
			F attrs_pair_end):
		d_interface_ptr(&interface),
		d_elem_name(elem_name) {
		d_interface_ptr->write_opening_element_with_attributes(elem_name, attrs_pair_begin, attrs_pair_end);
	}


	template<typename F>
	inline
	void
	XmlOutputInterface::write_opening_element_with_attributes(
			const GPlatesUtils::UnicodeString &elem_name,
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


	template<typename F>
	inline
	void
	XmlOutputInterface::write_line_of_multi_decimal_content(
			F content_begin,
			F content_end) {
		if (content_begin == content_end) {
			// There's nothing to write.
			return;
		}
		write_indentation();
		write_decimal_content(*content_begin);
		for (++content_begin; content_begin != content_end; ++content_begin) {
			write_unicode_string(" ");
			write_decimal_content(*content_begin);
		}
		write_unicode_string("\n");
	}

}

#endif  // GPLATES_FILEIO_XMLOUTPUTINTERFACE_H
