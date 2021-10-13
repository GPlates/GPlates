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

#include "GMTFormatGeometryExporter.h"
#include "PlatesLineFormatHeaderVisitor.h"
#include "ErrorOpeningFileForWritingException.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "global/unicode.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"

#include "utils/StringFormattingUtils.h"


GPlatesFileIO::GMTFormatWriter::GMTFormatWriter(
	const FileInfo &file_info,
	HeaderFormat header_format)
{
	// Open the file.
	d_output_file.reset( new QFile(file_info.get_qfileinfo().filePath()) );
	if ( ! d_output_file->open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.get_qfileinfo().filePath());
	}

	d_output_stream.reset( new QTextStream(d_output_file.get()) );

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
	const bool valid_header = d_feature_header->get_feature_header_lines(
		feature_handle.reference(), header_lines);
	
	// If we have a valid header and at least one geometry then we can output for the current feature.
	if (valid_header && d_feature_accumulator.have_geometry())
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
