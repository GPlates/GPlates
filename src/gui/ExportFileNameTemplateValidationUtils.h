/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_EXPORTFILENAMETEMPLATEVALIDATIONUTILS_H
#define GPLATES_GUI_EXPORTFILENAMETEMPLATEVALIDATIONUTILS_H

#include <QString>


namespace GPlatesGui
{
	namespace ExportFileNameTemplateValidationUtils
	{
		/**
		 * Returns true if filename template is a valid filename sequence.
		 *
		 * If @a check_filename_variation is true then also checks that there is filename variation
		 * (varies with reconstruction time).
		 * This should normally be true except when a exporting for a single time instant.
		 *
		 * Internally this validates using the @a ExportTemplateFilename class.
		 */
		bool
		is_valid_template_filename_sequence(
				const QString &filename_template,
				QString &filename_template_validation_message,
				bool check_filename_variation = true);


		/**
		 * Returns true if filename template has invalid characters.
		 */
		bool
		does_template_filename_have_invalid_characters(
				const QString &filename_template,
				QString &filename_template_validation_message);


		/**
		 * Returns true if filename template contains "%P".
		 */
		bool
		does_template_filename_have_percent_P(
				const QString &filename_template,
				QString &filename_template_validation_message);


		/**
		 * A common usage of the above functions.
		 *
		 * Returns true if filename template:
		 * - has no invalid characters, and
		 * - does *not* contain "%P", and
		 * - has a valid filename template sequence.
		 *
		 * If @a check_filename_variation is true then also checks that there is filename variation
		 * (varies with reconstruction time).
		 * This should normally be true except when a exporting for a single time instant.
		 */
		bool
		is_valid_template_filename_sequence_without_percent_P(
				const QString &filename_template,
				QString &filename_template_validation_message,
				bool check_filename_variation = true);


		/**
		 * A common usage of the above functions.
		 *
		 * Returns true if filename template:
		 * - has no invalid characters, and
		 * - does contain "%P", and
		 * - has a valid filename template sequence.
		 *
		 * If @a check_filename_variation is true then also checks that there is filename variation
		 * (varies with reconstruction time).
		 * This should normally be true except when a exporting for a single time instant.
		 */
		bool
		is_valid_template_filename_sequence_with_percent_P(
				const QString &filename_template,
				QString &filename_template_validation_message,
				bool check_filename_variation = true);
	}
}

#endif // GPLATES_GUI_EXPORTFILENAMETEMPLATEVALIDATIONUTILS_H
