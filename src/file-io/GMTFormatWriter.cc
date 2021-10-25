/* $Id$ */

/**
 * \file 
 * Defines the interface for writing data in GMT xy format.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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


#include <ostream>
#include <fstream>
#include <vector>
#include <QTextStream>

#include "GMTFormatWriter.h"

#include "ErrorOpeningFileForWritingException.h"
#include "FeatureCollectionFileFormatConfigurations.h"
#include "FileInfo.h"
#include "GMTFormatGeometryExporter.h"
#include "PlatesLineFormatHeaderVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/unicode.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"

#include "utils/StringFormattingUtils.h"


GPlatesFileIO::GMTFormatWriter::GMTFormatWriter(
	File::Reference &file_ref,
	const FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type &default_gmt_file_configuration,
	const GPlatesModel::Gpgim &gpgim)
{
	const FileInfo &file_info = file_ref.get_file_info();

	// Open the file.
	d_output_file.reset( new QFile(file_info.get_qfileinfo().filePath()) );
	if ( ! d_output_file->open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.get_qfileinfo().filePath());
	}

	d_output_stream.reset( new QTextStream(d_output_file.get()) );

	// If there's a GMT file configuration then use it to determine the header format.
	boost::optional<FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type> gmt_file_configuration =
			FeatureCollectionFileFormat::dynamic_cast_configuration<
					const FeatureCollectionFileFormat::GMTConfiguration>(file_ref.get_file_configuration());
	// Otherwise use the default GMT configuration and attach it to the file reference.
	if (!gmt_file_configuration)
	{
		gmt_file_configuration = default_gmt_file_configuration;

		// Store the file configuration in the file reference.
		FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
				file_configuration = gmt_file_configuration.get();
		file_ref.set_file_info(file_info, file_configuration);
	}

	// The header format.
	HeaderFormat header_format = gmt_file_configuration.get()->get_header_format();

	switch (header_format)
	{
	case PLATES4_STYLE_HEADER:
		d_feature_header.reset(new GMTFormatPlates4StyleHeader());
		break;

	case VERBOSE_HEADER:
		d_feature_header.reset(new GMTFormatVerboseHeader());
		break;

	case PREFER_PLATES4_STYLE_HEADER:
		d_feature_header.reset(new GMTFormatPreferPlates4StyleHeader());
		break;

	default:
		throw GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE);
	}
}


GPlatesFileIO::GMTFormatWriter::~GMTFormatWriter()
{
	// Empty destructor defined here so that 'd_feature_header' destructor
	// will be instantiated here where 'GMTHeaderFormatter' is a complete type .
}


bool
GPlatesFileIO::GMTFormatWriter::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Clear accumulator before visiting feature.
	d_feature_accumulator.clear();

	// Next, visit the feature properties to collect any geometries in the feature.
	return true;
}


void
GPlatesFileIO::GMTFormatWriter::finalise_post_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	std::vector<QString> header_lines;

	// Delegate formating of feature header.
	d_feature_header->get_feature_header_lines(feature_handle.reference(), header_lines);
	
	// If we have at least one geometry then we can output for the current feature.
	// We write out even if there's no header lines (because not enough property information).
	// This is because the user might still like to output the feature to a file.
	if (d_feature_accumulator.have_geometry())
	{
		// For each GeometryOnSphere write out a header and the geometry data.
		for(
			FeatureAccumulator::geometries_const_iterator_type geometry_iter =
				d_feature_accumulator.geometries_begin();
			geometry_iter != d_feature_accumulator.geometries_end();
			++geometry_iter)
		{
			// Write out the header.
			d_header_printer.print_feature_header_lines(*d_output_stream, header_lines);

			// Write out the geometry.
			GMTFormatGeometryExporter geometry_exporter(*d_output_stream);
			geometry_exporter.export_geometry(*geometry_iter);
		}
	}
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_line_string(
	const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_feature_accumulator.add_geometry(gml_line_string.polyline());
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_multi_point(
	const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_feature_accumulator.add_geometry(gml_multi_point.multipoint());
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_orientable_curve(
	const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_point(
	const GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_feature_accumulator.add_geometry(gml_point.point());
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_polygon(
	const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// FIXME: Handle interior rings. Requires a bit of restructuring.
	d_feature_accumulator.add_geometry(gml_polygon.exterior());
}


void
GPlatesFileIO::GMTFormatWriter::visit_gpml_constant_value(
	const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


GPlatesScribe::TranscribeResult
GPlatesFileIO::transcribe(
		GPlatesScribe::Scribe &scribe,
		GMTFormatWriter::HeaderFormat &header_format,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("PLATES4_STYLE_HEADER", GMTFormatWriter::PLATES4_STYLE_HEADER),
		GPlatesScribe::EnumValue("VERBOSE_HEADER", GMTFormatWriter::VERBOSE_HEADER),
		GPlatesScribe::EnumValue("PREFER_PLATES4_STYLE_HEADER", GMTFormatWriter::PREFER_PLATES4_STYLE_HEADER),
		GPlatesScribe::EnumValue("NUM_FORMATS", GMTFormatWriter::NUM_FORMATS)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			header_format,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
