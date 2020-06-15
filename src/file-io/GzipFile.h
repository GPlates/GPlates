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

#ifndef GPLATES_FILE_IO_GZIPFILE_H
#define GPLATES_FILE_IO_GZIPFILE_H

#include <boost/scoped_ptr.hpp>

#include <QByteArray>
#include <QIODevice>
#include <QObject>


namespace GPlatesFileIO
{
	/**
	 * A QIODevice that can read (decompress) or write (compress) a Gzip data stream.
	 *
	 * The implementation is adapted from the zpipe.c example at https://www.zlib.net/zlib_how.html
	 */
	class GzipFile :
			public QIODevice
	{
	public:
		/**
		 * @param device The device (eg, file) to decompress the gzip data stream from or compress to.
		 * @param compression_level Only used when opened in write mode (compressing), it's ignored in read mode.
		 *                          0 is no compression, 1 is best speed and 9 is best compression.
		 *                          Use -1 to specify default compression which is compromise between
		 *                          speed and compression (currently equivalent to level 6).
		 */
		explicit
		GzipFile(
				QIODevice* device,
				int compression_level = -1,
				QObject *parent_ = NULL);

		~GzipFile();

		virtual
		bool
		open(
				OpenMode mode);

		virtual
		void
		close();

		virtual
		bool
		isSequential() const
		{
			// Sequential, no seeking.
			return true;
		}

	protected:
		virtual
		qint64
		readData(
				char *data,
				qint64 maxSize);

		virtual
		qint64
		writeData(
				const char *data,
				qint64 maxSize);

	private:

		/**
		 * Wrapper around zlib's z_stream.
		 *
		 * It's not defined in this header because we only want to include <zlib.h> in cc file.
		 */
		class ZStream;

		/**
		 * Size of stream buffers used for compressing/decompressing.
		 *
		 * zlib recommends a decent size if possible, like 128kb.
		 */
		static const int STREAM_BUFFER_SIZE = 128 * 1024;

		/**
		 * The 'windowBits' parameter of zlib's 'inflateInit2()' and 'deflateInit2()' functions.
		 *
		 * Specifying this correctly enables gzip (instead of zlib) decompression/compression.
		 */
		static const int GZIP_WINDOW_BITS;

		QIODevice *d_device;
		boost::scoped_ptr<ZStream> d_zstream;
		QByteArray d_stream_input_buffer;
		QByteArray d_stream_output_buffer;

		/**
		 * Compression level 0-9: 0 is no compression, 1 is best speed and 9 is best compression.
		 *
		 * Only applies when this device is opened in write mode.
		 */
		int d_compression_level;


		/**
		 * Flush any unwritten data still inside zlib.
		 *
		 * Returns false is unable to write compressed data to output device.
		 */
		bool
		flush_write();
	};
}

#endif // GPLATES_FILE_IO_GZIPFILE_H
