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
#include <boost/optional.hpp>
#include <QFileInfo>

#include "ReadErrors.h"


namespace GPlatesFileIO
{
	namespace DataFormats
	{
		enum DataFormat
		{
			Gpml,
			PlatesRotation,
			PlatesLine,
			Shapefile,
			Gmap,
			RasterImage,
			ScalarField3D,
			Cpt,
			HellingerPick,
			Unspecified
		};

		const char *
		data_format_to_str(
				DataFormat data_format);
	}


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


	/**
	 * Use this DataSource derivation if the data source that triggered the read
	 * error is a local file.
	 */
	struct LocalFileDataSource :
			public DataSource
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


	/**
	 * This is a DataSource derivation that could be used for data sources other
	 * than local files.
	 */
	struct GenericDataSource :
			public DataSource
	{
		/**
		 * GenericDataSource constructor.
		 *
		 * If @a full_name is boost::none (the default value), the full name is set
		 * to be the same as the short name.
		 */
		GenericDataSource(
				DataFormats::DataFormat data_format,
				const std::string &short_name,
				const boost::optional<const std::string> &full_name = boost::none) :
			d_data_format(data_format),
			d_short_name(short_name),
			d_full_name(full_name ? *full_name : short_name)
		{ }

		virtual
		void
		write_short_name(
				std::ostream &target) const
		{
			target << d_short_name;
		}

		virtual
		void
		write_full_name(
				std::ostream &target) const
		{
			target << d_full_name;
		}

		virtual
		void
		write_format(
				std::ostream &target) const
		{
			target << DataFormats::data_format_to_str(d_data_format) << " format";
		}

	private:
		DataFormats::DataFormat d_data_format;
		std::string d_short_name;
		std::string d_full_name;
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


	/**
	 * Use this LocationInDataSource derivation if the data souurce that triggered
	 * the read error has a notion of line numbers.
	 */
	struct LineNumber :
			public LocationInDataSource
	{
		explicit
		LineNumber(
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


	/**
	 * A convenience function to create a ReadErrorOccurrence for file read errors.
	 */
	ReadErrorOccurrence
	make_read_error_occurrence(
			const QString &filename,
			DataFormats::DataFormat data_format,
			unsigned long line_num,
			ReadErrors::Description description,
			ReadErrors::Result result);

	/**
	 * A convenience function to create a ReadErrorOccurrence for read errors from
	 * data sources that have line numbers.
	 */
	ReadErrorOccurrence
	make_read_error_occurrence(
			boost::shared_ptr<DataSource> data_source,
			unsigned long line_num,
			ReadErrors::Description description,
			ReadErrors::Result result);
}

#endif  // GPLATES_FILEIO_READERROROCCURRENCE_H
