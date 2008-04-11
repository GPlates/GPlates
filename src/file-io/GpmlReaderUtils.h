/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GPMLREADERUTILS_H
#define GPLATES_FILEIO_GPMLREADERUTILS_H


#include <utility>
#include <QString>
#include <QtXml/QXmlStreamReader>
#include "ReadErrorOccurrence.h"
#include "ReadErrorAccumulation.h"
#include "ReadErrors.h"

#include "utils/StringSet.h"
#include "utils/UnicodeStringUtils.h"
#include "model/StringSetSingletons.h"
#include "model/XmlNode.h"


namespace GPlatesFileIO
{
	namespace GpmlReaderUtils
	{
		struct ReaderParams
		{
			QXmlStreamReader &reader;
			boost::shared_ptr<GPlatesFileIO::DataSource> source;
			GPlatesFileIO::ReadErrorAccumulation &errors;

			ReaderParams(
					QXmlStreamReader &reader_,
					boost::shared_ptr<GPlatesFileIO::DataSource> &source_,
					GPlatesFileIO::ReadErrorAccumulation &errors_) :
				reader(reader_), source(source_), errors(errors_) {  }
		};


		/**
		 * Warning and error logging helper functions.
		 */
		bool
		append_warning_if(
				bool condition,
				const GPlatesModel::XmlNode::non_null_ptr_type &current_elem,
				ReaderParams &params,
				const ReadErrors::Description &desc,
				const ReadErrors::Result &res);

		bool
		append_warning_if(
				bool condition,
				ReaderParams &params,
				const ReadErrors::Description &desc,
				const ReadErrors::Result &res);

		bool
		append_recoverable_error_if(
				bool condition,
				const GPlatesModel::XmlNode::non_null_ptr_type &current_elem,
				ReaderParams &params,
				const ReadErrors::Description &desc,
				const ReadErrors::Result &res);

		bool
		append_terminating_error_if(
				bool condition,
				const GPlatesModel::XmlNode::non_null_ptr_type &current_elem,
				ReaderParams &params,
				const ReadErrors::Description &desc,
				const ReadErrors::Result &res);

		bool
		append_failure_to_begin_if(
				bool condition,
				ReaderParams &params,
				const ReadErrors::Description &desc,
				const ReadErrors::Result &res);
	}
}

#endif // GPLATES_FILEIO_GPMLREADERUTILS_H
