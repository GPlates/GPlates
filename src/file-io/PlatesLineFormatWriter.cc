/* $Id$ */

/**
 * \file 
 * Implementation of the Plates4 data writer.
 *
 * Most recent change:
 *   $Date$
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
#include <numeric>
#include <unicode/ustream.h>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "PlatesLineFormatWriter.h"
#include "PlatesLineFormatHeaderVisitor.h"
#include "PlatesLineFormatGeometryExporter.h"
#include "ErrorOpeningFileForWritingException.h"
#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"

#include "maths/Real.h"
#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/InvalidPolylineContainsZeroPointsException.h"
#include "maths/InvalidPolylineContainsOnlyOnePointException.h"

#include "utils/StringFormattingUtils.h"
#include "global/GPlatesAssert.h"


namespace
{
	/**
	 * Visitor determines number of points in a derived GeometryOnSphere object.
	 */
	class NumberOfGeometryPoints :
		private GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		NumberOfGeometryPoints() :
			d_number_of_points(0)
		{  }

		unsigned int
		get_number_of_points(
				const GPlatesMaths::GeometryOnSphere *geometry)
		{
			geometry->accept_visitor(*this);
			return d_number_of_points;
		}

	private:
		int d_number_of_points;

		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_number_of_points = multi_point_on_sphere->number_of_points();
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
		{
			d_number_of_points = 1;
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_number_of_points = polygon_on_sphere->number_of_vertices() + 1;
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_number_of_points = polyline_on_sphere->number_of_vertices();
		}
	};

	/**
	 * Returns number of points in geometry.
	 */
	unsigned int
	get_number_of_points_in_geometry(
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry)
	{
		NumberOfGeometryPoints num_geom_points;
		return num_geom_points.get_number_of_points(geometry.get());
	}
}


GPlatesFileIO::PlatesLineFormatWriter::PlatesLineFormatWriter(
		const FileInfo &file_info)
{
	// Open the file.
	d_output_file.reset( new QFile(file_info.get_qfileinfo().filePath()) );
	if ( ! d_output_file->open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.get_qfileinfo().filePath());
	}

	d_output_stream.reset( new QTextStream(d_output_file.get()) );
}


void
GPlatesFileIO::PlatesLineFormatWriter::write_feature(
	const GPlatesModel::FeatureHandle& feature_handle)
{
	// Clear accumulator before visiting feature.
	d_feature_accumulator.clear();

	// Collect any geometries in the current feature.
	feature_handle.accept_visitor(*this);

	OldPlatesHeader old_plates_header;

	// Delegate formating of feature header.
	const bool valid_header = d_feature_header.get_old_plates_header(
		feature_handle, old_plates_header);

	// If we have a valid header and at least one geometry then we can output for the current feature.
	if (valid_header && d_feature_accumulator.have_geometry())
	{
		using boost::lambda::_1;
		using boost::lambda::_2;
		using boost::lambda::bind;

		// Calculate total number of geometry points in the current feature.
		const unsigned int number_points_in_feature = std::accumulate(
				d_feature_accumulator.geometries_begin(),
				d_feature_accumulator.geometries_end(),
				0 /*initial_value*/,
				_1 + bind(&get_number_of_points_in_geometry, _2));

		// Store the total number of geometry points in old plates header.
		old_plates_header.number_of_points = number_points_in_feature;

		// Write out the header.
		print_header_lines(old_plates_header);

		// For each geometry of the current feature write out the geometry data.
		PlatesLineFormatGeometryExporter geometry_exporter(*d_output_stream);
		geometry_exporter.export_feature_geometries(
				d_feature_accumulator.geometries_begin(),
				d_feature_accumulator.geometries_end());
	}
}

void
GPlatesFileIO::PlatesLineFormatWriter::print_header_lines(
	const OldPlatesHeader &old_plates_header)
{
	using namespace GPlatesUtils;

	/*
	 * The magic numbers that appear below are taken from p38 of the PLATES4
	 * User's Manual.
	 */

	// First line of the PLATES4 header.
	*d_output_stream << formatted_int_to_string(old_plates_header.region_number, 2).c_str()
		<< formatted_int_to_string(old_plates_header.reference_number, 2).c_str()
		<< " "
		<< formatted_int_to_string(old_plates_header.string_number, 4).c_str()
		<< " "
		<< GPlatesUtils::make_qstring_from_icu_string(old_plates_header.geographic_description)
		<< endl;

	// Second line of the PLATES4 header.
	*d_output_stream << " "
		<< formatted_int_to_string(old_plates_header.plate_id_number, 3).c_str()
		<< " "
		<< formatted_double_to_string(old_plates_header.age_of_appearance, 6, 1).c_str()
		<< " "
		<< formatted_double_to_string(old_plates_header.age_of_disappearance, 6, 1).c_str()
		<< " "
		<< GPlatesUtils::make_qstring_from_icu_string(old_plates_header.data_type_code)
		<< formatted_int_to_string(old_plates_header.data_type_code_number, 4).c_str()
		<< " "
		<< formatted_int_to_string(old_plates_header.conjugate_plate_id_number, 3).c_str()
		<< " "
		<< formatted_int_to_string(old_plates_header.colour_code, 3).c_str()
		<< " "
		<< formatted_int_to_string(old_plates_header.number_of_points, 5).c_str()
		<< endl;
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_feature_handle(
	const GPlatesModel::FeatureHandle &feature_handle)
{
	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_top_level_property_inline(
	const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	visit_property_values(top_level_property_inline);
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_line_string(
	const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_feature_accumulator.add_geometry(gml_line_string.polyline());
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_multi_point(
	const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_feature_accumulator.add_geometry(gml_multi_point.multipoint());
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_orientable_curve(
	const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_point(
	const GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_feature_accumulator.add_geometry(gml_point.point());
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_polygon(
	const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// FIXME: Handle interior rings. Requires a bit of restructuring.
	d_feature_accumulator.add_geometry(gml_polygon.exterior());
}

void
GPlatesFileIO::PlatesLineFormatWriter::visit_gpml_constant_value(
	const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}
