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

#ifndef GPLATES_SCRIBE_SCRIBEXMLARCHIVEWRITER_H
#define GPLATES_SCRIBE_SCRIBEXMLARCHIVEWRITER_H

#include <QLocale>
#include <QString>
#include <QXmlStreamWriter>

#include "ScribeArchiveWriter.h"


namespace GPlatesScribe
{
	/**
	 * XML scribe archiver writer.
	 */
	class XmlArchiveWriter :
			public ArchiveWriter
	{
	public:

		// Convenience typedefs for a shared pointer to a @a XmlArchiveWriter.
		typedef GPlatesUtils::non_null_intrusive_ptr<XmlArchiveWriter> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const XmlArchiveWriter> non_null_ptr_to_const_type;


		/**
		 * Create an archive writer that writes to the specified output.
		 */
		static
		non_null_ptr_type
		create(
				QXmlStreamWriter &xml_stream_writer)
		{
			return non_null_ptr_type(new XmlArchiveWriter(xml_stream_writer));
		}


		virtual
		~XmlArchiveWriter();


		/**
		 * Writes a @a Transcription to the archive.
		 */
		virtual
		void
		write_transcription(
				const Transcription &transcription);


		/**
		 * Close the archive.
		 *
		 * Any final writes to the archive are done here.
		 *
		 * If this is not called then the archive writer's destructor should call this.
		 */
		virtual
		void
		close();

	protected:

		explicit
		XmlArchiveWriter(
				QXmlStreamWriter &xml_stream_writer);

		/**
		 * Write Transcription composite object.
		 */
		void
		write(
				const Transcription::CompositeObject &composite_object);

		//
		// Write Transcription primitives to the archive.
		//

		void
		write(
				const Transcription::int32_type object);

		void
		write(
				const Transcription::uint32_type object);

		void
		write(
				const float object);

		void
		write(
				const double &object);

		void
		write(
				const std::string &object);


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
		 * Writes the XML data.
		 */
		QXmlStreamWriter &d_output_stream;

		/**
		 * Have we finished writing?
		 */
		bool d_closed;

	};
}

#endif // GPLATES_SCRIBE_SCRIBEXMLARCHIVEWRITER_H
