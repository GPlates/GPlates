/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <QObject>

#include "ReadErrorUtils.h"


void
GPlatesFileIO::ReadErrorUtils::group_read_errors_by_file(
		errors_by_file_map_type &errors_by_file,
		const ReadErrorAccumulation::read_error_collection_type &errors)
{
	ReadErrorAccumulation::read_error_collection_const_iterator it = errors.begin();
	ReadErrorAccumulation::read_error_collection_const_iterator end = errors.end();
	for (; it != end; ++it) {
		// Add the error to the map based on filename.
		std::ostringstream source;
		it->d_data_source->write_full_name(source);
		
		errors_by_file[source.str()].push_back(*it);
	}
}


void
GPlatesFileIO::ReadErrorUtils::group_read_errors_by_type(
		errors_by_type_map_type &errors_by_type,
		const ReadErrorAccumulation::read_error_collection_type &errors)
{
	ReadErrorAccumulation::read_error_collection_const_iterator it = errors.begin();
	ReadErrorAccumulation::read_error_collection_const_iterator end = errors.end();
	for (; it != end; ++it) {
		// Add the error to the map based on error code.
		errors_by_type[it->d_description].push_back(*it);
	}
}


const QString
GPlatesFileIO::ReadErrorUtils::build_summary_string(
		const ReadErrorAccumulation &read_errors)
{
	size_t num_failures = read_errors.d_failures_to_begin.size() +
			read_errors.d_terminating_errors.size();
	size_t num_recoverable_errors = read_errors.d_recoverable_errors.size();
	size_t num_warnings = read_errors.d_warnings.size();
	
	/*
	 * Firstly, work out what errors need to be summarised.
	 * The prefix of the sentence is affected by the quantity of the first error listed.
	 */
	QString prefix(QObject::tr("There were"));
	QString errors("");
	// Build sentence fragment for failures.
	if (num_failures > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_failures <= 1) {
				prefix = QObject::tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_failures > 1) {
			errors.append(QObject::tr("%1 failures").arg(num_failures));
		} else {
			errors.append(QObject::tr("%1 failure").arg(num_failures));
		}
	}
	// Build sentence fragment for errors.
	if (num_recoverable_errors > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_recoverable_errors <= 1) {
				prefix = QObject::tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_recoverable_errors > 1) {
			errors.append(QObject::tr("%1 errors").arg(num_recoverable_errors));
		} else {
			errors.append(QObject::tr("%1 error").arg(num_recoverable_errors));
		}
	}
	// Build sentence fragment for warnings.
	if (num_warnings > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_warnings <= 1) {
				prefix = QObject::tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_warnings > 1) {
			errors.append(QObject::tr("%1 warnings").arg(num_warnings));
		} else {
			errors.append(QObject::tr("%1 warning").arg(num_warnings));
		}
	}
	// Build sentence fragment for no problems.
	if (errors.isEmpty()) {
		errors.append(QObject::tr(" no problems"));
	}
	
	// Finally, build the whole summary string.
	QString summary("");
	summary.append(prefix);
	summary.append(errors);
	summary.append(".");
	
	return summary;
}
