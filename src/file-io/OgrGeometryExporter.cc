/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#include <boost/foreach.hpp>
#include <QDebug>
#include <QFile>

#include "model/FeatureVisitor.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PointOnSphere.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"

#include "OgrWriter.h"
#include "OgrGeometryExporter.h"

GPlatesFileIO::OgrGeometryExporter::OgrGeometryExporter(
	QString &filename,
	bool multiple_geometry_types,
	bool wrap_to_dateline):
	d_filename(filename),
	d_ogr_writer(new OgrWriter(d_filename, multiple_geometry_types, wrap_to_dateline))
{
}

GPlatesFileIO::OgrGeometryExporter::~OgrGeometryExporter()
{
	delete d_ogr_writer;
}

void
GPlatesFileIO::OgrGeometryExporter::visit_multi_point_on_sphere(
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
{
	d_multi_point_geometries.push_back(multi_point_on_sphere);
}

void
GPlatesFileIO::OgrGeometryExporter::visit_point_on_sphere(
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{
	d_point_geometries.push_back(*point_on_sphere);
}

void
GPlatesFileIO::OgrGeometryExporter::visit_polygon_on_sphere(
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	d_polygon_geometries.push_back(polygon_on_sphere);
}


void
GPlatesFileIO::OgrGeometryExporter::visit_polyline_on_sphere(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	d_polyline_geometries.push_back(polyline_on_sphere);
}

void
GPlatesFileIO::OgrGeometryExporter::export_geometry(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr)
{
	d_key_value_dictionary = boost::none;
	clear_geometries();

	geometry_ptr->accept_visitor(*this);

	write_geometries();
}

void
GPlatesFileIO::OgrGeometryExporter::export_geometry(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type key_value_dictionary)
{
	d_key_value_dictionary = key_value_dictionary;
	clear_geometries();

	geometry_ptr->accept_visitor(*this);

	write_geometries();
}

void
GPlatesFileIO::OgrGeometryExporter::clear_geometries()
{
	d_point_geometries.clear();
	d_multi_point_geometries.clear();
	d_polyline_geometries.clear();
	d_polygon_geometries.clear();
}

void
GPlatesFileIO::OgrGeometryExporter::write_geometries()
{
	if (!d_ogr_writer)
	{
		return;
	}

	// If a feature contains different geometry types, the geometries will be exported to
	// the appropriate file of the shapefile set.
	// This means that we're potentially splitting up a feature across different files.

	// Write the point geometries.
	if (!d_point_geometries.empty())
	{
		if (d_point_geometries.size() == 1)
		{
			d_ogr_writer->write_point_feature(d_point_geometries.front(), d_key_value_dictionary);
		}
		else
		{
			// We have more than one point in the feature, so we should handle this as a multi-point.
			d_ogr_writer->write_multi_point_feature(
					GPlatesMaths::MultiPointOnSphere::create_on_heap(
							d_point_geometries.begin(),
							d_point_geometries.end()),
					d_key_value_dictionary);
		}
	}

	// Write the multi-point geometries.
	BOOST_FOREACH(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point,
			d_multi_point_geometries)
	{
		d_ogr_writer->write_multi_point_feature(multi_point, d_key_value_dictionary);
	}

	// Write the polyline geometries.
	if (!d_polyline_geometries.empty())
	{
		if (d_polyline_geometries.size() == 1)
		{
			d_ogr_writer->write_polyline_feature(d_polyline_geometries.front(), d_key_value_dictionary);
		}
		else
		{
			d_ogr_writer->write_multi_polyline_feature(d_polyline_geometries, d_key_value_dictionary);
		}
	}

	// Write the polygon geometries.
	if (!d_polygon_geometries.empty())
	{
		if (d_polygon_geometries.size() == 1)
		{
			d_ogr_writer->write_polygon_feature(d_polygon_geometries.front(), d_key_value_dictionary);
		}
		else
		{
			d_ogr_writer->write_multi_polygon_feature(d_polygon_geometries, d_key_value_dictionary);
		}
	}
}
