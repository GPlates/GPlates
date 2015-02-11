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

#ifndef GPLATES_SCRIBE_SCRIBETEXTARCHIVEWRITER_H
#define GPLATES_SCRIBE_SCRIBETEXTARCHIVEWRITER_H

#include <ostream>
#include <boost/io/ios_state.hpp>

#include "ScribeArchiveWriter.h"


namespace GPlatesScribe
{
	/**
	 * Text scribe archiver writer.
	 */
	class TextArchiveWriter :
			public ArchiveWriter
	{
	public:

		// Convenience typedefs for a shared pointer to a @a TextArchiveWriter.
		typedef GPlatesUtils::non_null_intrusive_ptr<TextArchiveWriter> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TextArchiveWriter> non_null_ptr_to_const_type;


		/**
		 * Create an archive writer that writes to the specified output stream.
		 */
		static
		non_null_ptr_type
		create(
				std::ostream &output_stream)
		{
			return non_null_ptr_type(new TextArchiveWriter(output_stream));
		}


		/**
		 * Writes a @a Transcription to the archive.
		 */
		virtual
		void
		write_transcription(
				const Transcription &transcription);


		/**
		 * Close the archive.
		 */
		virtual
		void
		close()
		{  }

	protected:

		explicit
		TextArchiveWriter(
				std::ostream &output_stream);

		void
		write_object_group(
				const Transcription &transcription,
				Transcription::object_id_type start_object_id_in_group,
				unsigned int num_object_ids_in_group);

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


		std::ostream &d_output_stream;

		//
		// Stream IO state savers to restore the stream state when finished.
		//
		boost::io::ios_flags_saver d_output_stream_flags_saver;
		boost::io::ios_precision_saver d_output_stream_precision_saver;
		boost::io::basic_ios_locale_saver<std::ostream::char_type, std::ostream::traits_type>
				d_output_stream_locale_saver;

	};
}

#endif // GPLATES_SCRIBE_SCRIBETEXTARCHIVEWRITER_H
