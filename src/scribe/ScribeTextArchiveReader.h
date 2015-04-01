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

#ifndef GPLATES_SCRIBE_SCRIBETEXTARCHIVEREADER_H
#define GPLATES_SCRIBE_SCRIBETEXTARCHIVEREADER_H

#include <istream>
#include <boost/io/ios_state.hpp>

#include "ScribeArchiveReader.h"


namespace GPlatesScribe
{
	/**
	 * Text scribe archiver reader.
	 */
	class TextArchiveReader :
			public ArchiveReader
	{
	public:

		// Convenience typedefs for a shared pointer to a @a TextArchiveReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<TextArchiveReader> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TextArchiveReader> non_null_ptr_to_const_type;


		/**
		 * Create an archive reader that reads from the specified input stream.
		 */
		static
		non_null_ptr_type
		create(
				std::istream &input_stream)
		{
			return non_null_ptr_type(new TextArchiveReader(input_stream));
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
		TextArchiveReader(
				std::istream &input_stream);

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
		// Read Transcription primitives from the archive.
		//

		template <typename ObjectType>
		ObjectType
		read();


		std::istream &d_input_stream;

		//
		// Stream IO state savers to restore the stream state when finished.
		//
		boost::io::ios_flags_saver d_input_stream_flags_saver;
		boost::io::ios_precision_saver d_input_stream_precision_saver;
		boost::io::basic_ios_locale_saver<std::istream::char_type, std::istream::traits_type>
				d_input_stream_locale_saver;

	};


	// Specialisation of protected member template function.
	template <>
	std::string
	TextArchiveReader::read<std::string>();
}

#endif // GPLATES_SCRIBE_SCRIBETEXTARCHIVEREADER_H
