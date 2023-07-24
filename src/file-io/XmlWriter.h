/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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


#ifndef GPLATES_FILEIO_XMLWRITER_H
#define GPLATES_FILEIO_XMLWRITER_H

#include <vector>
#include <utility>
#include <QXmlStreamWriter>
#include <QTextStream>
#include "model/StringContentTypeGenerator.h"
#include "model/QualifiedXmlName.h"
#include "utils/XmlNamespaces.h"
#include "utils/UnicodeStringUtils.h"

namespace GPlatesFileIO
{
	/**
	 * XmlWriter is a wrapper around a QXmlStreamWriter that takes care of
	 * ensuring that the namespace aliases emitted in the output are as close
	 * as possible to any that were declared in the originating document.
	 */
	class XmlWriter
	{
	public:

		// pair::first is the namespace uri, pair::second is the alias.
		typedef std::pair<
			GPlatesUtils::StringSet::SharedIterator,
			GPlatesUtils::StringSet::SharedIterator>
				NamespaceDeclaration;

		typedef std::vector<NamespaceDeclaration> NamespaceStack;

		
		/**
		 * Constructs the XmlWriter without specifying a QIODevice.
		 * This is provided as a convenience function for GpmlOutputVisitor's
		 * constructor to use. Do not attempt to write to this XmlWriter without
		 * first specifying a QIODevice with setDevice()!
		 */
		XmlWriter();
		
		/**
		 * Sets the output device, using QXmlStreamWriter::setDevice().
		 */
		void
		setDevice(
				QIODevice *target)
		{
			d_writer.setDevice(target);
		}

		/**
		 * Gets the output device, using QXmlStreamWriter::device().
		 */
		QIODevice*
		device()
		{
			return d_writer.device();
		}


		/**
		 * Constructs the XmlWriter with a QIODevice target.
		 * This is the preferred method of constructing an XmlWriter.
		 *
		 * Note that the caller is responsible for managing the QIODevice!
		 * In the case of QFile, this means closing it and ensuring the object
		 * will be deleted.
		 */
		XmlWriter(
				QIODevice *target);
		

		void
		writeNamespace(
				const QString &namespace_uri,
				const QString &namespace_alias);


		const GPlatesUtils::UnicodeString
		getAliasForNamespace(
				const GPlatesUtils::StringSet::SharedIterator namespace_uri) const;


		void
		writeStartDocument() {
			static const QString XML_VERSION("1.0");
			d_writer.writeStartDocument(XML_VERSION);
		}

		void
		writeEndDocument() {
			d_writer.writeEndDocument();
		}

		
		template<typename SingletonType>
		void
		writeEmptyElement(
				const GPlatesModel::QualifiedXmlName<SingletonType> &elem_name);

		
		void
		writeEmptyGpmlElement(
				const QString &name) {
			d_writer.writeEmptyElement(GPlatesUtils::XmlNamespaces::get_gpml_namespace_qstring(), name);
		}


		void
		writeEmptyGmlElement(
				const QString &name) {
			d_writer.writeEmptyElement(GPlatesUtils::XmlNamespaces::get_gml_namespace_qstring(), name);
		}


		/**
		 * Start a new element with name @a name.  If a new namespace declaration
		 * was added to the document (because elem_name.get_namespace() didn't
		 * match anything in the namespace stack), then this method returns true,
		 * otherwise it returns false.  The return value should be passed to
		 * the method writeEndElement() when it's called to signal the end of this
		 * element.
		 */
		template<typename SingletonType>
		bool
		writeStartElement(
				const GPlatesModel::QualifiedXmlName<SingletonType> &elem_name);


		void
		writeStartGpmlElement(
				const QString &elem_name) {
			d_writer.writeStartElement(GPlatesUtils::XmlNamespaces::get_gpml_namespace_qstring(), elem_name);
		}


		void
		writeStartGmlElement(
				const QString &elem_name) {
			d_writer.writeStartElement(GPlatesUtils::XmlNamespaces::get_gml_namespace_qstring(), elem_name);
		}


		void
		writeEndElement(
				bool pop_ns_stack = false);


		void
		writeText(
				const QString &text) {
			d_writer.writeCharacters(text);
		}


		void
		writeText(
				const GPlatesUtils::UnicodeString &text) {
			writeText(GPlatesUtils::make_qstring_from_icu_string(text));
		}

		
		template <typename T>
		void
		writeText(
				const GPlatesModel::StringContentTypeGenerator<T> &text) {
			writeText(GPlatesUtils::make_qstring_from_icu_string(text.get()));
		}
		

		void
		writeDecimal(
				double val) {
			d_writer.writeCharacters(QString::number(val, 'g', 17));
		}


		void
		writeDecimalPair(
				double val1,
				double val2);


		void
		writeCommaSeparatedDecimalPair(
				double val1,
				double val2);


		template <typename IntegerType>
		void
		writeInteger(
				const IntegerType &val) {
			d_writer.writeCharacters(QString::number(val));
		}


		void
		writeBoolean(
				bool val) {
			// FIXME: Work out how to do this with QBool and some kind of
			// boolalpha manipulator.
			d_writer.writeCharacters(val ? "true" : "false");
		}


		/**
		 * Dereferencing a DecimalIterator should return an int or double,
		 * or something that can be upcast to such.
		 */
		template<typename DecimalIterator>
		void
		writeNumericalSequence(
				const DecimalIterator &begin,
				const DecimalIterator &end);


		/**
		 * Dereferencing a StringIterator should return a QString,
		 * or something that can be upcast to such.
		 */
		template<typename StringIterator>
		void
		writeStringSequence(
				const StringIterator &begin,
				const StringIterator &end);


		void
		writeAttribute(
				const QString &namespace_uri,
				const QString &name,
				const QString &value) {
			d_writer.writeAttribute(namespace_uri, name, value);
		}


		template<typename SingletonType>
		void
		writeAttribute(
				const GPlatesModel::QualifiedXmlName<SingletonType> &name,
				const QString &value) {
			writeAttribute(
					GPlatesUtils::make_qstring_from_icu_string(name.get_namespace()),
					GPlatesUtils::make_qstring_from_icu_string(name.get_name()),
					value);
		}


		/**
		 * Dereferencing an AttributeIterator should return a pair consisting of
		 * a QualifiedXmlName and an XmlAttributeValue.
		 */
		template<typename AttributeIterator>
		void
		writeAttributes(
				const AttributeIterator &begin,
				const AttributeIterator &end);


		void
		writeGpmlAttribute(
				const QString &name,
				const QString &value) {
			writeAttribute(GPlatesUtils::XmlNamespaces::get_gpml_namespace_qstring(), name, value);
		}


		void
		writeGmlAttribute(
				const QString &name,
				const QString &value) {
			writeAttribute(GPlatesUtils::XmlNamespaces::get_gml_namespace_qstring(), name, value);
		}


		/**
		 * Writes the @a absolute_file_path as a path relative to the directory that
		 * contains the file that we are outputting XML to (if any).
		 *
		 * If we are not outputting to a file, it just writes @a absolute_file_path.
		 */
		void
		writeRelativeFilePath(
				const QString &absolute_file_path);


		void
		writeRelativeFilePath(
				const GPlatesUtils::UnicodeString &absolute_file_path)
		{
			writeRelativeFilePath(GPlatesUtils::make_qstring_from_icu_string(absolute_file_path));
		}


		QXmlStreamWriter &
		get_writer() {
			return d_writer;
		}

	private:

		NamespaceStack d_ns_stack;
		QXmlStreamWriter d_writer;


		bool
		declare_namespace_if_necessary(
				const NamespaceDeclaration &ns_decl);
	};



	template<typename SingletonType>
	void
	XmlWriter::writeEmptyElement(
			const GPlatesModel::QualifiedXmlName<SingletonType> &elem_name)
	{
		d_writer.writeEmptyElement(
				GPlatesUtils::make_qstring_from_icu_string(elem_name.get_namespace()), 
				GPlatesUtils::make_qstring_from_icu_string(elem_name.get_name()));
	}


	template<typename SingletonType>
	bool
	XmlWriter::writeStartElement(
			const GPlatesModel::QualifiedXmlName<SingletonType> &elem_name)
	{
		bool res = false;
		res = declare_namespace_if_necessary(
				std::make_pair(
					elem_name.get_namespace_iterator(),
					elem_name.get_namespace_alias_iterator()));

		d_writer.writeStartElement(
				GPlatesUtils::make_qstring_from_icu_string(elem_name.get_namespace()),
				GPlatesUtils::make_qstring_from_icu_string(elem_name.get_name()));
		return res;
	}


	template<typename DecimalIterator>
	void
	XmlWriter::writeNumericalSequence(
			const DecimalIterator &begin,
			const DecimalIterator &end) 
	{
		for (DecimalIterator iter = begin; iter != end; ++iter) 
		{
			writeDecimal(*iter);
			d_writer.writeCharacters(" ");
		}
	}


	template<typename StringIterator>
	void
	XmlWriter::writeStringSequence(
			const StringIterator &begin,
			const StringIterator &end)
	{
		for (StringIterator iter = begin; iter != end; ++iter)
		{
			writeText(*iter);
			d_writer.writeCharacters(" ");
		}
	}


	template<typename AttributeIterator>
	void
	XmlWriter::writeAttributes(
			const AttributeIterator &begin,
			const AttributeIterator &end) 
	{
		for (AttributeIterator iter = begin; iter != end; ++iter) 
		{
			writeAttribute(
					iter->first,
					GPlatesUtils::make_qstring_from_icu_string(iter->second.get()));
		}
	}

}

#endif  // GPLATES_FILEIO_XMLWRITER_H
