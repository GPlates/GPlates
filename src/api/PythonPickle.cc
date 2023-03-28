/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#include "PythonPickle.h"

#include "scribe/ScribeBinaryArchiveReader.h"
#include "scribe/ScribeBinaryArchiveWriter.h"


namespace GPlatesApi
{
	namespace PythonPickle
	{
		namespace Impl
		{
			/**
			 * Pickle suite for @a Bytes.
			 */
			struct BytesPickleSuite :
					boost::python::pickle_suite
			{
				// Pickle C++ 'Bytes' object into a Python 'bytes' object.
				static
				boost::python::tuple
				getinitargs(
						const Bytes &bytes)
				{
					// Create a Python 'bytes' object from the QByteArray containing the transcribed byte stream.
					boost::python::object bytes_object(boost::python::handle<>(
							// Returns a new reference so no need for 'boost::python::borrowed'...
							PyBytes_FromStringAndSize(bytes.bytes.constData(), bytes.bytes.size())));

					return boost::python::make_tuple(bytes_object);
				}
			};


			// Unpickle a Python 'bytes' object back into a C++ 'Bytes' object.
			Bytes::Bytes(
					boost::python::object bytes_object) :
				bytes(QByteArray(PyBytes_AsString(bytes_object.ptr()), PyBytes_Size(bytes_object.ptr())))
			{  }
		}
	}
}


GPlatesApi::PythonPickle::Impl::Bytes
GPlatesApi::PythonPickle::Impl::transcription_to_bytes(
		const GPlatesScribe::Transcription &object_transcription)
{
	// Stream into a QByteArray.
	QBuffer archive;
	archive.open(QBuffer::WriteOnly);
	QDataStream archive_stream(&archive);

	// Archive writer for the binary stream.
	GPlatesScribe::ArchiveWriter::non_null_ptr_type archive_writer =
			GPlatesScribe::BinaryArchiveWriter::create(archive_stream);

	// Write the transcription to the archive.
	archive_writer->write_transcription(object_transcription);

	// Close archive writer and archive.
	archive_writer->close();
	archive.close();

	// Extract the archive bytes.
	return Bytes(archive.data());
}


GPlatesScribe::Transcription::non_null_ptr_type
GPlatesApi::PythonPickle::Impl::bytes_to_transcription(
		const Bytes &object_bytes)
{
	// Stream from the requested QByteArray.
	QDataStream archive_stream(object_bytes.bytes);

	// Archive reader for the binary stream.
	GPlatesScribe::ArchiveReader::non_null_ptr_type archive_reader =
			GPlatesScribe::BinaryArchiveReader::create(archive_stream);

	// Read the transcription from the archive.
	const GPlatesScribe::Transcription::non_null_ptr_type object_transcription =
			archive_reader->read_transcription();

	// Close archive reader.
	// We have read the transcription and we want to check we've correctly reached the end of the archive.
	archive_reader->close();

	return object_transcription;
}


void
export_pickle()
{
	// Class 'Bytes' can be pickled to a Python 'bytes' object and unpickled back again.
	//
	// Note: Instead of wrapping a QByteArray (via class Impl::Bytes) using boost::python::class_
	//       we could have registered a to-python converter from QByteArray (or class Impl::Bytes)
	//       to Python 'bytes' and a from-python converter back again. However we also have a from-python
	//       converter from Python 'bytes' to QString (it can convert from Python 'bytes' or 'str').
	//       This means if any wrapped class (call it 'Type') happened to have an __init__ that accepted
	//       a QString (as well as the unpickle __init__ accepting Python 'bytes') then the former might
	//       incorrectly get called instead of the latter (when unpickling). This is avoided when we
	//       explicitly wrap using boost::python::class_ because Impl::Bytes itself is then pickled/unpickled,
	//       as the Python type "PickleBytes", and so now only the unpickle __init__(Impl::Bytes)
	//       of class 'Type' can get called (not __init__(QString)).
	boost::python::class_<GPlatesApi::PythonPickle::Impl::Bytes>(
			"PickleBytes",
			boost::python::init<boost::python::object>())                // unpickle
		.def_pickle(GPlatesApi::PythonPickle::Impl::BytesPickleSuite())  // pickle
	;
}
