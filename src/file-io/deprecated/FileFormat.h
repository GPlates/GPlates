/* $Id$ */

/**
 * @file 
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

#ifndef GPLATES_FILEIO_FILEFORMAT_H
#define GPLATES_FILEIO_FILEFORMAT_H

#include <QString>
#include <list>
#include <boost/optional.hpp>
#include "Reader.h"
#include "Writer.h"

namespace GPlatesFileIO {

	/**
	 * FileFormat contains information relevant to a particular file format.
	 *
	 * FileFormat includes the name of the format, a list of standard file suffixes, 
	 * and references to a Reader and a Writer that can be used to read and write 
	 * files in the format.
	 */
	class FileFormat 
	{
	public:
		typedef std::list<QString> SuffixList;
		typedef SuffixList::const_iterator suffix_const_iterator;

		/**
		 * A descriptive name for this file format.
		 */
		const QString &
		name() const { return d_name; }

		/**
		 * A Reader that will read in files in this format (if one exists).
		 */
		boost::optional<const Reader &>
		reader() const { return d_reader; }

		/**
		 * A Writer that will write in files in this format (if one exists).
		 */
		boost::optional<const Writer &>
		writer() const { return d_writer; }

		//@{
		/**
		 * Suffix access functions.
		 */
		suffix_const_iterator
		suffixes_begin() const { return d_suffixes.begin(); }

		suffix_const_iterator
		suffixes_end() const { return d_suffixes.end(); }
		//@}

	private:
	
		// XXX: This should be made private eventually.
		//FileFormat() { }

		QString d_name;
		SuffixList d_suffixes;
		boost::optional<const Reader &> d_reader;
		boost::optional<const Writer &> d_writer;
	};
}

#endif // GPLATES_FILEIO_FILEFORMAT_H
