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

#ifndef GPLATES_SCRIBE_SCRIBEBINARYARCHIVEWRITER_H
#define GPLATES_SCRIBE_SCRIBEBINARYARCHIVEWRITER_H

#include <QDataStream>
#include <QtGlobal>

#include "ScribeArchiveWriter.h"


namespace GPlatesScribe
{
	/**
	 * Binary scribe archiver writer.
	 */
	class BinaryArchiveWriter :
			public ArchiveWriter
	{
	public:

		// Convenience typedefs for a shared pointer to a @a BinaryArchiveWriter.
		typedef GPlatesUtils::non_null_intrusive_ptr<BinaryArchiveWriter> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const BinaryArchiveWriter> non_null_ptr_to_const_type;


		/**
		 * Create an archive writer that writes to the specified output stream.
		 */
		static
		non_null_ptr_type
		create(
				QDataStream &output_stream)
		{
			return non_null_ptr_type(new BinaryArchiveWriter(output_stream));
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
		BinaryArchiveWriter(
				QDataStream &output_stream);

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
				const qint32 object);

		void
		write(
				const quint32 object);

		void
		write(
				const float object);

		void
		write(
				const double &object);

		void
		write(
				const std::string &object);


		QDataStream &d_output_stream;

	};
}

#endif // GPLATES_SCRIBE_SCRIBEBINARYARCHIVEWRITER_H
