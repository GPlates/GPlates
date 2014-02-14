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

#include <string>
#include <iostream>
#include <sstream>

#include "ExportFileNameTemplateValidationUtils.h"

#include "file-io/ExportTemplateFilenameSequence.h"


namespace
{
	const std::string INVALID_CHARACTERS = "/\\|*?\"><:";
}


bool
GPlatesGui::ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence(
		const QString &filename_template,
		QString &filename_template_validation_message,
		bool check_filename_variation)
{
	try
	{
		GPlatesFileIO::ExportTemplateFilename::validate_filename_template(
				filename_template,
				check_filename_variation);
	}
	catch (GPlatesFileIO::ExportTemplateFilename::UnrecognisedFormatString &exc)
	{
		std::ostringstream os;
		exc.write(os);
		filename_template_validation_message = os.str().c_str();
		return false;
	}
	catch (GPlatesFileIO::ExportTemplateFilename::NoFilenameVariation &exc)
	{
		std::ostringstream os;
		exc.write(os);
		os << "Cannot find necessary file name variations. ";
		filename_template_validation_message = os.str().c_str();
		return false;
	}
	catch (std::exception &exc)
	{
		filename_template_validation_message =
				QString("Error validating file name template: %1)").arg(exc.what());
		throw;
	}
	catch (...)
	{
		filename_template_validation_message =
				"Unexpected exception happened in the validation of file name template.";
		throw;
	}

	return true;
}


bool
GPlatesGui::ExportFileNameTemplateValidationUtils::does_template_filename_have_invalid_characters(
		const QString &filename_template,
		QString &filename_template_validation_message)
{
	if (filename_template.toStdString().find_first_of(INVALID_CHARACTERS) != std::string::npos)
	{
		filename_template_validation_message =
				QString("File name contains illegal characters -- ").append(INVALID_CHARACTERS.c_str());

		return true;
	}

	return false;
}


bool
GPlatesGui::ExportFileNameTemplateValidationUtils::does_template_filename_have_percent_P(
		const QString &filename_template,
		QString &filename_template_validation_message)
{
	if(filename_template.toStdString().find("%P") != std::string::npos)
	{
		filename_template_validation_message =
					"Parameter(%P) has been found in the file name template.";

		return true;
	}

	filename_template_validation_message =
				"Parameter(%P) has not been found in the file name template.";
	
	return false;
}


bool
GPlatesGui::ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_without_percent_P(
		const QString &filename_template,
		QString &filename_template_validation_message,
		bool check_filename_variation)
{
	if (does_template_filename_have_invalid_characters(
			filename_template, filename_template_validation_message))
	{
		return false;
	}
	
	if (does_template_filename_have_percent_P(
			filename_template, filename_template_validation_message))
	{
		return false;
	}

	return is_valid_template_filename_sequence(
			filename_template,
			filename_template_validation_message,
			check_filename_variation);
}


bool
GPlatesGui::ExportFileNameTemplateValidationUtils::is_valid_template_filename_sequence_with_percent_P(
		const QString &filename_template,
		QString &filename_template_validation_message,
		bool check_filename_variation)
{
	if (does_template_filename_have_invalid_characters(
			filename_template, filename_template_validation_message))
	{
		return false;
	}
	
	if (!does_template_filename_have_percent_P(
			filename_template, filename_template_validation_message))
	{
		return false;
	}

	return is_valid_template_filename_sequence(
			filename_template,
			filename_template_validation_message,
			check_filename_variation);
}
