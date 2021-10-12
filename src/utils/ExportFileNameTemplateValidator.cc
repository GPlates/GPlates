/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include "utils/ExportFileNameTemplateValidator.h"

const std::string 
GPlatesUtils::ExportFileNameTemplateValidator::INVALID_CHARACTERS = "/\\|*?\"><:";

boost::shared_ptr<
		GPlatesUtils::ExportFileNameTemplateValidator >
GPlatesUtils::ExportFileNameTemplateValidatorFactory::create_validator(
		GPlatesUtils::Exporter_ID id)
{
	if(	id == GPlatesUtils::RECONSTRUCTED_GEOMETRIES_SHAPEFILE ||
		id == GPlatesUtils::RECONSTRUCTED_GEOMETRIES_GMT)
	{
		return boost::shared_ptr<
				GPlatesUtils::ExportFileNameTemplateValidator >(
						new ExportReconstructedGeometryFileNameTemplateValidator());
	}

	if(id == GPlatesUtils::PROJECTED_GEOMETRIES_SVG)
	{
		return boost::shared_ptr<
				GPlatesUtils::ExportFileNameTemplateValidator >(
						new ExportSvgFileNameTemplateValidator());
	}
	
	if(id == GPlatesUtils::MESH_VELOCITIES_GPML)
	{
		return boost::shared_ptr<
				GPlatesUtils::ExportFileNameTemplateValidator >(
						new ExportVelocityFileNameTemplateValidator());
	}
	
	if(id == GPlatesUtils::RESOLVED_TOPOLOGIES_GMT)
	{
		return boost::shared_ptr<
				GPlatesUtils::ExportFileNameTemplateValidator >(
						new ExportResolvedTopologyFileNameTemplateValidator());
	}
	
	if( id == GPlatesUtils::RELATIVE_ROTATION_CSV_COMMA ||
		id == GPlatesUtils::RELATIVE_ROTATION_CSV_SEMICOLON ||
		id == GPlatesUtils::RELATIVE_ROTATION_CSV_TAB ||
		id == GPlatesUtils::EQUIVALENT_ROTATION_CSV_COMMA ||
		id == GPlatesUtils::EQUIVALENT_ROTATION_CSV_SEMICOLON ||
		id == GPlatesUtils::EQUIVALENT_ROTATION_CSV_TAB)
	{
		return boost::shared_ptr<
				GPlatesUtils::ExportFileNameTemplateValidator >(
						new ExportRotationFileNameTemplateValidator());
	}

	if( id == GPlatesUtils::ROTATION_PARAMS_CSV_SEMICOLON ||
		id == GPlatesUtils::ROTATION_PARAMS_CSV_TAB ||
		id == GPlatesUtils::ROTATION_PARAMS_CSV_COMMA)
	{
		return boost::shared_ptr<
				GPlatesUtils::ExportFileNameTemplateValidator >(
						new ExportRotationParamsFileNameTemplateValidator());
	}
	
	if( id == GPlatesUtils::RASTER_BMP ||
		id == GPlatesUtils::RASTER_JPG ||
		id == GPlatesUtils::RASTER_JPEG ||
		id == GPlatesUtils::RASTER_PNG ||
		id == GPlatesUtils::RASTER_PPM ||
		id == GPlatesUtils::RASTER_TIFF ||
		id == GPlatesUtils::RASTER_XBM ||
		id == GPlatesUtils::RASTER_XPM)
	{
		return boost::shared_ptr<
				GPlatesUtils::ExportFileNameTemplateValidator >(
						new ExportRasterFileNameTemplateValidator());
	}

	return boost::shared_ptr<GPlatesUtils::ExportFileNameTemplateValidator>(
		new DummyExportFileNameTemplateValidator());
}

bool
GPlatesUtils::ExportRotationFileNameTemplateValidator::is_valid(
		const QString &filename)
{
	if(has_invalid_characters(filename) || has_percent_P(filename))
	{
		return false;
	}
	return file_sequence_validate(filename);
}

bool
GPlatesUtils::ExportSvgFileNameTemplateValidator::is_valid(
		const QString &filename)
{
	if(has_invalid_characters(filename) || has_percent_P(filename))
	{
		return false;
	}
	return file_sequence_validate(filename);
}

bool
GPlatesUtils::ExportVelocityFileNameTemplateValidator::is_valid(
		const QString &filename)
{
	if(has_invalid_characters(filename) || !has_percent_P(filename))
	{
		return false;
	}
	return file_sequence_validate(filename);
}

bool
GPlatesUtils::ExportReconstructedGeometryFileNameTemplateValidator::is_valid(
		const QString &filename)
{
	if(has_invalid_characters(filename) || has_percent_P(filename))
	{
		return false;
	}
	return file_sequence_validate(filename);
}

bool
GPlatesUtils::ExportResolvedTopologyFileNameTemplateValidator::is_valid(
		const QString &filename)
{
	if(has_invalid_characters(filename) || !has_percent_P(filename))
	{
		return false;
	}
	return file_sequence_validate(filename);
}

bool
GPlatesUtils::ExportRasterFileNameTemplateValidator::is_valid(
	const QString &filename)
{
	if(has_invalid_characters(filename) || has_percent_P(filename))
	{
		return false;
	}
	return file_sequence_validate(filename);
}

bool
GPlatesUtils::ExportRotationParamsFileNameTemplateValidator::is_valid(
		const QString &filename)
{
	if(has_invalid_characters(filename) || has_percent_P(filename))
	{
		return false;
	}
	return file_sequence_validate(filename);
}

bool
GPlatesUtils::DummyExportFileNameTemplateValidator::is_valid(
		const QString &filename)
{
	return true;
}

bool
GPlatesUtils::ExportFileNameTemplateValidator::file_sequence_validate(
		const QString &filename)
{
	std::ostringstream os;
	try
	{
		GPlatesUtils::ExportTemplateFilename::validate_filename_template(filename);
		return true;
	}
	catch (
			GPlatesUtils::ExportTemplateFilename::UnrecognisedFormatString &exc)
	{
		exc.write(os);
		set_result_message(os.str().c_str());
		return false;
	}
	catch (	
			GPlatesUtils::ExportTemplateFilename::NoFilenameVariation &exc)
	{
		exc.write(os);
		os << "Cannot find necessary file name variations. ";
		set_result_message(os.str().c_str());
		return false;
	}
	catch (...)
	{
		set_result_message("Unexpected exception happened in the validation of file name template.");
		throw;
	}
}

bool
GPlatesUtils::ExportFileNameTemplateValidator::has_invalid_characters(
		const QString &filename)
{
	if(filename.toStdString().find_first_of(INVALID_CHARACTERS) != std::string::npos)
	{
		set_result_message(
				QString(
						"File name contains illegal characters -- ").append(
								INVALID_CHARACTERS.c_str()));
		return true;
	}
	else
	{
		return false;
	}
}

bool
GPlatesUtils::ExportFileNameTemplateValidator::has_percent_P(
		const QString &filename)
{
	if(filename.toStdString().find("%P") != std::string::npos)
	{
		set_result_message(
					"Parameter(%P) has been found in the file name template.");
		return true;
	}
	else
	{
		set_result_message(
					"Parameter(%P) has not been found in the file name template.");
		return false;
	}
}






