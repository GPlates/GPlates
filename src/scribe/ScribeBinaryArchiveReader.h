/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBEBINARYARCHIVEREADER_H
#define GPLATES_SCRIBE_SCRIBEBINARYARCHIVEREADER_H

#include <QDataStream>
#include <QtGlobal>

#include "ScribeArchiveReader.h"


namespace GPlatesScribe
{
	/**
	 * Binary scribe archiver reader.
	 */
	class BinaryArchiveReader :
			public ArchiveReader
	{
	public:

		// Convenience typedefs for a shared pointer to a @a BinaryArchiveReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<BinaryArchiveReader> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const BinaryArchiveReader> non_null_ptr_to_const_type;


		/**
		 * Create an archive reader that reads from the specified input stream.
		 */
		static
		non_null_ptr_type
		create(
				QDataStream &input_stream)
		{
			return non_null_ptr_type(new BinaryArchiveReader(input_stream));
		}


		/**
		 * Reads a @a Transcription from the archive.
		 */
		virtual
		Transcription::non_null_ptr_type
		read_transcription();


		/**
		 * Close the archive.
		 */
		virtual
		void
		close()
		{  }

	protected:

		explicit
		BinaryArchiveReader(
				QDataStream &input_stream);

		bool
		read_object_group(
				Transcription &transcription);

		/**
		 * Read Transcription composite object.
		 */
		void
		read(
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


		QDataStream &d_input_stream;

	};
}

#endif // GPLATES_SCRIBE_SCRIBEBINARYARCHIVEREADER_H
