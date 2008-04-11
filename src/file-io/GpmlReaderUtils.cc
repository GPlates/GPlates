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


#include "GpmlReaderUtils.h"

namespace 
{
	bool
	append_error_if(
			bool condition,
			const GPlatesModel::XmlNode::non_null_ptr_type &current_elem,
			GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
			const GPlatesFileIO::GpmlReaderUtils::ReaderParams &params,
			GPlatesFileIO::ReadErrors::Description desc,
			GPlatesFileIO::ReadErrors::Result res)
	{
		if (condition) {
			boost::shared_ptr<GPlatesFileIO::LocationInDataSource> loc(
				new GPlatesFileIO::LineNumberInFile(current_elem->line_number()));
			errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						params.source, loc, desc, res));
		}
		return condition;
	}


	bool
	append_error_if(
			bool condition,
			GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
			const GPlatesFileIO::GpmlReaderUtils::ReaderParams &params,
			GPlatesFileIO::ReadErrors::Description desc,
			GPlatesFileIO::ReadErrors::Result res)
	{
		if (condition) {
			boost::shared_ptr<GPlatesFileIO::LocationInDataSource> loc(
				new GPlatesFileIO::LineNumberInFile(params.reader.lineNumber()));
			errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						params.source, loc, desc, res));
		}
		return condition;
	}
}


bool
GPlatesFileIO::GpmlReaderUtils::append_warning_if(
		bool condition,
		const GPlatesModel::XmlNode::non_null_ptr_type &current_elem,
		ReaderParams &params,
		const ReadErrors::Description &desc,
		const ReadErrors::Result &res)
{
	return append_error_if(condition, current_elem,
			params.errors.d_warnings, params, desc, res);
}


bool
GPlatesFileIO::GpmlReaderUtils::append_warning_if(
		bool condition,
		ReaderParams &params,
		const ReadErrors::Description &desc,
		const ReadErrors::Result &res)
{
	return append_error_if(condition,
			params.errors.d_warnings, params, desc, res);
}


bool
GPlatesFileIO::GpmlReaderUtils::append_recoverable_error_if(
		bool condition,
		const GPlatesModel::XmlNode::non_null_ptr_type &current_elem,
		ReaderParams &params,
		const ReadErrors::Description &desc,
		const ReadErrors::Result &res)
{
	return append_error_if(condition, current_elem,
			params.errors.d_recoverable_errors, params, desc, res);
}


bool
GPlatesFileIO::GpmlReaderUtils::append_terminating_error_if(
		bool condition,
		const GPlatesModel::XmlNode::non_null_ptr_type &current_elem,
		ReaderParams &params,
		const ReadErrors::Description &desc,
		const ReadErrors::Result &res)
{
	return append_error_if(condition, current_elem,
			params.errors.d_terminating_errors, params, desc, res);
}


bool
GPlatesFileIO::GpmlReaderUtils::append_failure_to_begin_if(
		bool condition,
		ReaderParams &params,
		const ReadErrors::Description &desc,
		const ReadErrors::Result &res)
{
	return append_error_if(condition,
			params.errors.d_recoverable_errors, params, desc, res);
}
