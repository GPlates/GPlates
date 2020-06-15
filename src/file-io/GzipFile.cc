/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include <QtGlobal>
#if defined(Q_OS_WIN)
	// Note that, on Windows, ZLIB_WINAPI should be defined before including "zlib.h".
	// This is because, on Windows, we compiled our own "zlibwapi.dll" (version 1.2.11) which also defines it.
	// According to the zlib docs (specifically "win32/DLL_FAQ.txt" in zlib source code)...
	//
	//   The ZLIB_WINAPI macro will switch on the WINAPI (STDCALL) convention.
	//   The name of this DLL must be different than the official ZLIB1.DLL.
	//
	// Apparently the official zlib1.dll has a '1' in it because the old zlib.dll, built from zlib-1.1.4
	// or earlier, required compilation settings that were incompatible to those used by a static build.
	// And the only solution was to make a binary-incompatible change in the DLL interface and to change the name.
	//
	// We could have just used the official pre-compiled zlib1.dll (and not defined ZLIB_WINAPI) but zlib1.dll
	// is really designed for "C functionality" using the CDECL calling convention, and we're using C++
	// in "Windows functionality" where the STDCALL convention is more suitable.
	//
	#define ZLIB_WINAPI
#endif
#include <zlib.h>


#include "GzipFile.h"

#include <boost/numeric/conversion/cast.hpp>

#include <QBuffer>
#include <QByteArray>
#include <QDataStream>

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesFileIO
{
	/**
	 * Wrapper around zlib's z_stream.
	 *
	 * It's defined in cc file because we only want to include <zlib.h> in cc file.
	 */
	class GzipFile::ZStream
	{
	public:
		ZStream() :
			status(Z_OK)
		{  }

		z_stream stream;
		int status;
	};
}


//
// The 'windowBits' parameter of zlib's 'inflateInit2()' and 'deflateInit2()' functions.
//
// Specifying this correctly enables gzip (instead of zlib) decompression/compression.
// According to the zlib docs:
//
//  The default value is 15 if inflateInit is used instead. windowBits must be greater than or
//  equal to the windowBits value provided to deflateInit2() while compressing, or it must be
//  equal to 15 if deflateInit2() was not used.
//
//   windowBits can also be greater than 15 for optional gzip decoding.  Add
//   32 to windowBits to enable zlib and gzip decoding with automatic header
//   detection, or add 16 to decode only the gzip format(the zlib format will
//   return a Z_DATA_ERROR).
//
// ...so we use MAX_WBITS (15) for the first paragraph above (to ensure we can decode files created
// by any application), and 16 for the second paragraph (we're encoding/decoding only gzip).
//
const int GPlatesFileIO::GzipFile::GZIP_WINDOW_BITS = 16 + MAX_WBITS;


GPlatesFileIO::GzipFile::GzipFile(
		QIODevice* device,
		int compression_level,
		QObject *parent_) :
	QIODevice(parent_),
	d_device(device),
	d_zstream(new ZStream()),
	d_compression_level(compression_level)
{
	// Compression level should be 9 or less.
	// Note: compression level only applies to write mode (it's ignored in read mode).
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_compression_level <= 9,
			GPLATES_ASSERTION_SOURCE);
	if (d_compression_level < 0)
	{
		d_compression_level = Z_DEFAULT_COMPRESSION;
	}
}


GPlatesFileIO::GzipFile::~GzipFile()
{
	// Destructor of 'd_zstream' boost::scoped_ptr requires ZStream class definition.

	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		// Also call 'close()' noting that the base class QIODevice destructor does not call it.
		close();
	}
	catch (...)
	{
	}
}


bool
GPlatesFileIO::GzipFile::open(
		OpenMode mode)
{
	// Mode should be read or write only, with optional text flag.
	if (mode != QIODevice::ReadOnly &&
		mode != (QIODevice::ReadOnly | QIODevice::Text) &&
		mode != QIODevice::WriteOnly &&
		mode != (QIODevice::WriteOnly | QIODevice::Text))
	{
		return false;
	}

	if (d_device->isOpen())
	{
		// Device is already open, it should have been opened in read or write binary mode (ie, no text flag).
		QIODevice::OpenMode device_mode = d_device->openMode();
		if (mode.testFlag(QIODevice::ReadOnly))
		{
			if (device_mode != QIODevice::ReadOnly)
			{
				return false;
			}
		}
		else // write mode ...
		{
			if (device_mode != QIODevice::WriteOnly)
			{
				return false;
			}
		}
	}
	else // device not yet opened ...
	{
		// Open device in binary mode (we are reading/writing compressed data).
		const QIODevice::OpenMode device_mode = mode.testFlag(QIODevice::ReadOnly)
				? QIODevice::ReadOnly
				: QIODevice::WriteOnly;
		if (!d_device->open(device_mode))
		{
			return false;
		}
	}

	//
	// Initialise the zlib stream.
	//

	d_zstream->stream.zalloc = Z_NULL;
	d_zstream->stream.zfree = Z_NULL;
	d_zstream->stream.opaque = Z_NULL;

	if (mode.testFlag(QIODevice::ReadOnly))
	{
		// These z_stream fields must be initialised before calling 'inflateInit2()'.
		d_zstream->stream.avail_in = 0;
		d_zstream->stream.next_in = Z_NULL;

		d_zstream->status = inflateInit2(
				&d_zstream->stream,
				GZIP_WINDOW_BITS);
		if (d_zstream->status != Z_OK)
		{
			return false;
		}

		// Input (compressed) buffer is constant in size.
		d_stream_input_buffer.resize(STREAM_BUFFER_SIZE);
		// Output (decompressed) buffer starts out empty and then varies in size.
		d_stream_output_buffer.clear();

		// Set 'avail_out' to a non-zero value so that 'readData()' will start out by filling input buffer.
		d_zstream->stream.avail_out = STREAM_BUFFER_SIZE;
		d_zstream->stream.next_out = Z_NULL;
	}
	else // write mode ...
	{
		// No extra z_stream fields need to be initialised before calling 'deflateInit2()'.

		d_zstream->status = deflateInit2(
				&d_zstream->stream,
				d_compression_level,
				Z_DEFLATED/*method*/,
				GZIP_WINDOW_BITS/*windowBits*/,
				8/*memLevel*/,  // Default value of 8 uses around 256KB of memory.
				Z_DEFAULT_STRATEGY/*strategy*/);
		if (d_zstream->status != Z_OK)
		{
			return false;
		}

		// Input (uncompressed) buffer starts out empty and then varies in size.
		d_stream_input_buffer.clear();
		// Output (compressed) buffer is constant in size.
		d_stream_output_buffer.resize(STREAM_BUFFER_SIZE);

		// Set 'avail_out' to a non-zero value so that 'writeData()' will start out by filling input buffer.
		d_zstream->stream.avail_out = STREAM_BUFFER_SIZE;
		d_zstream->stream.next_out = Z_NULL;
	}

	setOpenMode(mode);

	return true;
}


void
GPlatesFileIO::GzipFile::close()
{
	const OpenMode mode = openMode();

	// If already closed then nothing to do.
	if (mode.testFlag(NotOpen))
	{
		return;
	}

	if (mode.testFlag(QIODevice::ReadOnly))
	{
		inflateEnd(&d_zstream->stream);  // clean up
	}
	else // write mode ...
	{
		flush_write();  // Flush any unwritten data still inside zlib.
		deflateEnd(&d_zstream->stream);  // clean up
	}

	d_device->close();

	setOpenMode(NotOpen);
}


qint64
GPlatesFileIO::GzipFile::readData(
		char *decompressed_data,
		qint64 decompressed_data_size)
{
	//
	// Decompression.
	//
	// The algorithm is as follows:
	//
	//   Client requests a specified amount of decompressed data.
	//   If decompressed output buffer is not empty then move bytes to client's buffer (decreases output buffer size).
	//   While client's buffer not yet filled:
	//     If there's no compressed data in input buffer still being processed by zlib:
	//       If we're at the end of the compressed input file (we've read all of compressed input):
	//         Check that zlib has processed the entire compressed input file.
	//         Return (finished)
	//       Else:
	//         Check that zlib has 'not' processed the entire compressed input file.
	//       Read a block of data from compressed input file into input buffer.
	//     Create a new decompressed data output buffer (output buffer should be empty; drained by client).
	//     Get zlib to decompress input buffer into output buffer.
	//     If output buffer not completely filled by zlib then chop off unfilled end part.
	//     Move decompressed bytes from output buffer to client's buffer (decreases output buffer size).
	//

	qint64 decompressed_bytes_read = 0;

	QByteArray &compressed_buffer = d_stream_input_buffer;
	QByteArray &decompressed_buffer = d_stream_output_buffer;

	// First consume decompressed data from our output buffer if it's not empty.
	if (!decompressed_buffer.isEmpty())
	{
		int decompressed_bytes_to_copy = boost::numeric_cast<int>(decompressed_data_size);
		if (decompressed_bytes_to_copy > decompressed_buffer.size())
		{
			decompressed_bytes_to_copy = decompressed_buffer.size();
		}

		// Copy the decompressed bytes to the caller.
		for (int n = 0; n < decompressed_bytes_to_copy; ++n, ++decompressed_bytes_read)
		{
			decompressed_data[decompressed_bytes_read] = decompressed_buffer[n];
		}

		// Remove buffered data that we've consumed.
		decompressed_buffer.remove(0, decompressed_bytes_to_copy);
	}

	// While we still have bytes to decompress for the caller.
	while (decompressed_bytes_read < decompressed_data_size)
	{
		// If there's no uncompressed data in input buffer still being processed by zlib then read some more compressed input.
		if (d_zstream->stream.avail_out > 0)
		{
			if (d_device->atEnd())
			{
				if (d_zstream->status != Z_STREAM_END)
				{
					// EOF reached before the compressed data self-terminates,
					// so compressed data is incomplete and an error is returned.
					return -1;
				}
				return decompressed_bytes_read;
			}

			if (d_zstream->status == Z_STREAM_END)
			{
				// EOF not reached but the compressed data has self-terminated,
				// so the input continues past the zlib stream and an error is returned.
				return -1;
			}

			// Read some compressed data from compressed input file into our input buffer.
			const qint64 compressed_bytes_read = d_device->read(compressed_buffer.data(), compressed_buffer.size());
			if (compressed_bytes_read < 0)
			{
				// There was an error reading the compressed input.
				return -1;
			}

			// Let zlib know how much compressed input is currently available.
			d_zstream->stream.next_in = reinterpret_cast<unsigned char *>(compressed_buffer.data());
			d_zstream->stream.avail_in = compressed_bytes_read;
		}

		// Resize output buffer (it's currently empty).
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				decompressed_buffer.isEmpty(),
				GPLATES_ASSERTION_SOURCE);
		decompressed_buffer.resize(STREAM_BUFFER_SIZE);

		d_zstream->stream.next_out = reinterpret_cast<unsigned char *>(decompressed_buffer.data());
		d_zstream->stream.avail_out = decompressed_buffer.size();

		// Decompress.
		d_zstream->status = inflate(&d_zstream->stream, Z_NO_FLUSH);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_zstream->status != Z_STREAM_ERROR,  // should not get Z_STREAM_ERROR if initialised properly
				GPLATES_ASSERTION_SOURCE);
		// Apparently these are the only hard errors we can get at this point (see https://www.zlib.net/zlib_how.html).
		if (d_zstream->status == Z_NEED_DICT ||
			d_zstream->status == Z_DATA_ERROR ||
			d_zstream->status == Z_MEM_ERROR)
		{
			return -1;
		}

		// If zlib filled our output buffer (avail_out == 0) then it means it potentially still has more input to process.
		//
		// Although there's a chance the output buffer is filled and the input buffer is fully processed, in which
		// case the next call to 'inflate()' will return Z_BUF_ERROR and nothing will happen in that loop iteration
		// (note that Z_BUF_ERROR is not considered a hard error) and so a subsequent loop iteration will be needed
		// but then 'avail_out' will be non-zero (and so we'll return to normal processing).

		if (d_zstream->stream.avail_out > 0)
		{
			// Not all of the decompressed buffer was filled, so chop off the unused end part.
			decompressed_buffer.chop(d_zstream->stream.avail_out);
		}

		int decompressed_bytes_to_copy = boost::numeric_cast<int>(decompressed_data_size - decompressed_bytes_read);
		if (decompressed_bytes_to_copy > decompressed_buffer.size())
		{
			decompressed_bytes_to_copy = decompressed_buffer.size();
		}

		// Copy the decompressed bytes to the caller.
		for (int n = 0; n < decompressed_bytes_to_copy; ++n, ++decompressed_bytes_read)
		{
			decompressed_data[decompressed_bytes_read] = decompressed_buffer[n];
		}

		// Remove buffered data that we've consumed.
		decompressed_buffer.remove(0, decompressed_bytes_to_copy);
	}

	return decompressed_bytes_read;
}


qint64
GPlatesFileIO::GzipFile::writeData(
		const char *uncompressed_data,
		qint64 uncompressed_data_size)
{
	//
	// Compression.
	//
	// The algorithm is as follows:
	//
	//   Client gives us a specified amount of uncompressed data.
	//   While have uncompressed data:
	//     Copy client's uncompressed data into input buffer.
	//     If input buffer not full:
	//         Return.
	//     Do:
	//       Using Z_NO_FLUSH, get zlib to compress input buffer into output buffer.
	//       Write output buffer to compressed output file.
	//     ..while there is uncompressed data in input buffer still being processed by zlib.
	//     Clear input buffer.
	//

	qint64 uncompressed_bytes_written = 0;

	QByteArray &uncompressed_buffer = d_stream_input_buffer;
	QByteArray &compressed_buffer = d_stream_output_buffer;

	// While we still have uncompressed bytes from the caller.
	while (uncompressed_bytes_written < uncompressed_data_size)
	{
		// There's no uncompressed data in input buffer still being processed by zlib, so copy client's uncompressed data into input buffer.
		const int unfilled_bytes_in_uncompressed_buffer = STREAM_BUFFER_SIZE - uncompressed_buffer.size();
		int uncompressed_bytes_to_copy = boost::numeric_cast<int>(uncompressed_data_size - uncompressed_bytes_written);
		if (uncompressed_bytes_to_copy > unfilled_bytes_in_uncompressed_buffer)
		{
			uncompressed_bytes_to_copy = unfilled_bytes_in_uncompressed_buffer;
		}

		// Copy the uncompressed bytes from the caller into input buffer.
		uncompressed_buffer += QByteArray::fromRawData(uncompressed_data + uncompressed_bytes_written, uncompressed_bytes_to_copy);
		uncompressed_bytes_written += uncompressed_bytes_to_copy;

		// If input buffer is not yet full then nothing left to do (for now).
		// It's more efficient to give zlib a full input buffer rather than bits and pieces.
		if (uncompressed_buffer.size() < STREAM_BUFFER_SIZE)
		{
			return uncompressed_bytes_written;
		}

		// Let zlib know how much uncompressed input is currently available.
		d_zstream->stream.next_in = reinterpret_cast<unsigned char *>(uncompressed_buffer.data());
		d_zstream->stream.avail_in = uncompressed_buffer.size();

		// Loop until all uncompressed data in input buffer has been processed.
		do
		{
			d_zstream->stream.next_out = reinterpret_cast<unsigned char *>(compressed_buffer.data());
			d_zstream->stream.avail_out = compressed_buffer.size();

			// Compress using Z_NO_FLUSH flush flag.
			d_zstream->status = deflate(&d_zstream->stream, Z_NO_FLUSH);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_zstream->status != Z_STREAM_ERROR,  // should not get Z_STREAM_ERROR if initialised properly
					GPLATES_ASSERTION_SOURCE);
			// Apparently there are no hard errors possible at this point (see https://www.zlib.net/zlib_how.html).

			// If zlib filled our output buffer (avail_out == 0) then it means it potentially still has more input to process.
			//
			// Although there's a chance the output buffer is filled and the input buffer is fully processed, in which
			// case the next call to 'inflate()' will return Z_BUF_ERROR and nothing will happen in that loop iteration
			// (note that Z_BUF_ERROR is not considered a hard error) and so a subsequent loop iteration will be needed
			// but then 'avail_out' will be non-zero (and so we'll return to normal processing).

			// Write compressed data from our output buffer to compressed output file.
			const int compressed_bytes_to_write = compressed_buffer.size() - d_zstream->stream.avail_out;
			qint64 compressed_bytes_written = 0;
			do 
			{
				// Maybe need to do multiple write calls if any call actually writes less than requested.
				const qint64 bytes_written = d_device->write(
						compressed_buffer.constData() + compressed_bytes_written,
						compressed_bytes_to_write - compressed_bytes_written);
				if (bytes_written < 0)
				{
					// There was an error writing the compressed output.
					return -1;
				}

				compressed_bytes_written += bytes_written;
			}
			while (compressed_bytes_written < compressed_bytes_to_write);
		}
		while (d_zstream->stream.avail_out == 0);

		// We've processed the compressed data in the input buffer.
		uncompressed_buffer.clear();
	}

	return uncompressed_bytes_written;
}


bool
GPlatesFileIO::GzipFile::flush_write()
{
	//
	// Flush any unwritten data still inside zlib.
	//
	// The algorithm is as follows:
	//
	//   Input buffer might not be empty, and/or there might be internal state inside zlib not yet flushed.
	//   Using Z_FINISH, get zlib to continue compressing input buffer into output buffer.
	//   Write output buffer to compressed output file.
	//

	QByteArray &uncompressed_buffer = d_stream_input_buffer;
	QByteArray &compressed_buffer = d_stream_output_buffer;

	// Let zlib know how much uncompressed input is currently available.
	if (uncompressed_buffer.isEmpty())
	{
		// No input data left to process, but the Z_FINISH flush flag will complete the compressed stream
		// (eg, flush out any internal state inside zlib, and then write stream trailer).
		d_zstream->stream.next_in = Z_NULL;
		d_zstream->stream.avail_in = 0;
	}
	else
	{
		d_zstream->stream.next_in = reinterpret_cast<unsigned char *>(uncompressed_buffer.data());
		d_zstream->stream.avail_in = uncompressed_buffer.size();
	}

	// Continue until there is no uncompressed data in input buffer still being processed by zlib.
	do
	{
		d_zstream->stream.next_out = reinterpret_cast<unsigned char *>(compressed_buffer.data());
		d_zstream->stream.avail_out = compressed_buffer.size();

		// Compress using Z_FINISH flush flag.
		d_zstream->status = deflate(&d_zstream->stream, Z_FINISH);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_zstream->status != Z_STREAM_ERROR,  // should not get Z_STREAM_ERROR if initialised properly
				GPLATES_ASSERTION_SOURCE);
		// Apparently there are no hard errors possible at this point (see https://www.zlib.net/zlib_how.html).

		// Write compressed data from our output buffer to compressed output file.
		const int compressed_bytes_to_write = compressed_buffer.size() - d_zstream->stream.avail_out;
		qint64 compressed_bytes_written = 0;
		do 
		{
			// Maybe need to do multiple write calls if any call actually writes less than requested.
			const qint64 bytes_written = d_device->write(
					compressed_buffer.constData() + compressed_bytes_written,
					compressed_bytes_to_write - compressed_bytes_written);
			if (bytes_written < 0)
			{
				// There was an error writing the compressed output.
				return false;
			}

			compressed_bytes_written += bytes_written;
		}
		while (compressed_bytes_written < compressed_bytes_to_write);
	}
	while (d_zstream->stream.avail_out == 0);

	// Should be at end of zlib stream.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_zstream->status == Z_STREAM_END,
			GPLATES_ASSERTION_SOURCE);

	return true;
}
