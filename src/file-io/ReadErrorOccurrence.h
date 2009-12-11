/* $Id$ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_READERROROCCURRENCE_H
#define GPLATES_FILEIO_READERROROCCURRENCE_H

#include <string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <QFileInfo>
#include "ReadErrors.h"

namespace GPlatesFileIO
{
	struct DataSource
	{
		virtual
		~DataSource()
		{  }

		virtual
		void
		write_short_name(
				std::ostream &target) const = 0;

		virtual
		void
		write_full_name(
				std::ostream &target) const = 0;

		virtual
		void
		write_format(
				std::ostream &target) const = 0;
	};


	namespace DataFormats
	{
		enum DataFormat
		{
			GpmlOnePointSix,
			PlatesRotation,
			PlatesLine,
			Shapefile,
			Gmap,
			Unspecified
		};

		const char *
		data_format_to_str(
				DataFormat data_format);
	}


	struct LocalFileDataSource: public DataSource
	{
		LocalFileDataSource(
				const QString &filename,
				DataFormats::DataFormat data_format):
			d_filename(filename),
			d_fileinfo(filename),
			d_data_format(data_format)
		{  }

		virtual
		void
		write_short_name(
				std::ostream &target) const
		{
			target << d_fileinfo.fileName().toStdString();
		}

		virtual
		void
		write_full_name(
				std::ostream &target) const
		{
			target << d_filename.toStdString();
		}

		virtual
		void
		write_format(
				std::ostream &target) const
		{
			target << DataFormats::data_format_to_str(d_data_format) << " format";
		}

	private:
		QString d_filename;
		QFileInfo d_fileinfo;
		DataFormats::DataFormat d_data_format;
	};


	struct LocationInDataSource
	{
		virtual
		~LocationInDataSource()
		{  }

		virtual
		void
		write(
				std::ostream &target) const = 0;
	};


	struct LineNumberInFile: public LocationInDataSource
	{
		explicit
		LineNumberInFile(
				unsigned long line_num):
			d_line_num(line_num)
		{  }

		virtual
		void
		write(
				std::ostream &target) const
		{
			target << d_line_num;
		}
	private:
		unsigned long d_line_num;
	};


	struct ReadErrorOccurrence
	{
		/**
		 * Create a new ReadErrorOccurrence instance.
		 *
		 * Neither @a data_source or @a location should be NULL.
		 */
		ReadErrorOccurrence(
				boost::shared_ptr<DataSource> data_source,
				boost::shared_ptr<LocationInDataSource> location,
				ReadErrors::Description description,
				ReadErrors::Result result):
			d_data_source(data_source),
			d_location(location),
			d_description(description),
			d_result(result)
		{  }

		void
		write_short_name(
				std::ostream &target) const
		{
			d_data_source->write_short_name(target);
			target << " (";
			d_data_source->write_format(target);
			target << ")";
		}

		void
		write_full_name(
				std::ostream &target) const
		{
			d_data_source->write_full_name(target);
			target << ":";
			d_location->write(target);
			target << " (";
			d_data_source->write_format(target);
			target << ")";
		}

		boost::shared_ptr<DataSource> d_data_source;
		boost::shared_ptr<LocationInDataSource> d_location;
		ReadErrors::Description d_description;
		ReadErrors::Result d_result;
	};
}

#endif  // GPLATES_FILEIO_READERROROCCURRENCE_H
