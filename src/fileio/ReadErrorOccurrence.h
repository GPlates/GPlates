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
		write(
				std::ostringstream &target) const = 0;
	};


	namespace DataFormats
	{
		enum DataFormat
		{
			PlatesRotation,
			PlatesLine
		};

		const char *
		data_format_to_str(
				DataFormat data_format);
	}


	struct LocalFileDataSource: public DataSource
	{
		LocalFileDataSource(
				const std::string &filename,
				DataFormats::DataFormat data_format):
			d_filename(filename),
			d_data_format(data_format)
		{  }

		virtual
		void
		write(
				std::ostringstream &target) const
		{
			target << d_filename << " (" << d_data_format << " format)";
		}
	private:
		std::string d_filename;
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
				std::ostringstream &target) const = 0;
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
				std::ostringstream &target) const
		{
			target << "line " << d_line_num;
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

		boost::shared_ptr<DataSource> d_data_source;
		boost::shared_ptr<LocationInDataSource> d_location;
		ReadErrors::Description d_description;
		ReadErrors::Result d_result;
	};
}

#endif  // GPLATES_FILEIO_READERROROCCURRENCE_H
