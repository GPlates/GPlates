/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBEXMLARCHIVEREADER_H
#define GPLATES_SCRIBE_SCRIBEXMLARCHIVEREADER_H

#include <QLocale>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>

#include "ScribeArchiveReader.h"


namespace GPlatesScribe
{
	/**
	 * XML scribe archiver reader.
	 */
	class XmlArchiveReader :
			public ArchiveReader
	{
	public:

		// Convenience typedefs for a shared pointer to a @a XmlArchiveReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<XmlArchiveReader> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const XmlArchiveReader> non_null_ptr_to_const_type;


		/**
		 * Create an archive reader that reads from the specified input stream.
		 *
		 * NOTE: @a xml_stream_reader must currently be at the start of the XML element
		 * containing the archived stream.
		 */
		static
		non_null_ptr_type
		create(
				QXmlStreamReader &xml_stream_reader)
		{
			return non_null_ptr_type(new XmlArchiveReader(xml_stream_reader));
		}


		/**
		 * Reads a @a Transcription from the archive.
		 */
		virtual
		Transcription::non_null_ptr_type
		read_transcription();


		/**
		 * Close the archive.
		 *
		 * NOTE: Closing before all transcriptions are read will throw an exception.
		 */
		virtual
		void
		close();

	protected:

		explicit
		XmlArchiveReader(
				QXmlStreamReader &xml_stream_reader);

		/**
		 * Read Transcription composite object.
		 */
		void
		read_composite(
				Transcription::CompositeObject &composite_object);

		//
		// Write Transcription primitives to the archive.
		//

		int
		read_signed();

		unsigned int
		read_unsigned();

		float
		read_float();

		double
		read_double();

		std::string
		read_string();


		/**
		 * Read the object id attribute of the current XML element.
		 */
		Transcription::object_id_type
		read_object_id_attribute();


		/**
		 * Read the start of an XML element named @a element_name.
		 */
		bool
		read_start_element(
				const QString &element_name,
				bool require = false);

		/**
		 * Read the start of an XML element named any names in @a element_names.
		 */
		bool
		read_start_element(
				const QStringList &element_names,
				bool require = false);

		/**
		 * Read the end of an XML element named @a element_name.
		 */
		bool
		read_end_element(
				const QString &element_name,
				bool require = false);
		
		/**
		 * Read the end of an XML element named any names in @a element_names.
		 */
		bool
		read_end_element(
				const QStringList &element_names,
				bool require = false);


		/**
		 * A wrapper around QXmlStreamReader::readNext() to detect errors.
		 */
		void
		read_next_token();


		/**
		 * Use the "C" locale to convert numbers to and from the archive.
		 *
		 * This ensures that writing to an XML file using one locale and reading it using another
		 * will not cause stream synchronization problems.
		 *
		 * Even though the QLocale documentation states...
		 *
		 *  "QString::toInt(), QString::toDouble(), etc., interpret the string according to the default locale."
		 *  "If this fails, it falls back on the "C" locale."
		 *
		 * ...but still just go straight to using the "C" locale.
		 */
		static const QLocale C_LOCALE;

		/**
		 * Reads the XML data.
		 */
		QXmlStreamReader &d_input_stream;

		/**
		 * Have we finished reading?
		 */
		bool d_closed;

	};
}

#endif // GPLATES_SCRIBE_SCRIBEXMLARCHIVEREADER_H
