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
	bool multiple_geometries):
	d_filename(filename),
	d_multiple_geometries(multiple_geometries),
	d_ogr_writer(0)
{
	try{
		d_ogr_writer = new OgrWriter(d_filename,multiple_geometries);
	}
	catch(...)
	{
		qDebug() << "Exception caught creating OgrWriter.";
	}
}

GPlatesFileIO::OgrGeometryExporter::~OgrGeometryExporter()
{
	if (d_ogr_writer)
	{
		delete d_ogr_writer;
	};
}

void
GPlatesFileIO::OgrGeometryExporter::visit_multi_point_on_sphere(
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
{
	if (!d_ogr_writer)
	{
		return;
	}
	try
	{
		d_ogr_writer->write_multi_point_feature(multi_point_on_sphere,d_key_value_dictionary);

	}
	catch(...)
	{
		qDebug() << "Exception caught writing multi-point to shapefile.";
	}
}

void
GPlatesFileIO::OgrGeometryExporter::visit_point_on_sphere(
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{
	if (!d_ogr_writer)
	{
		return;
	}
	try
	{
		d_ogr_writer->write_point_feature(point_on_sphere,d_key_value_dictionary);

	}
	catch(...)
	{
		qDebug() << "Exception caught writing point to shapefile.";
	}
}

void
GPlatesFileIO::OgrGeometryExporter::visit_polygon_on_sphere(
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	if (!d_ogr_writer)
	{
		return;
	}
	try
	{
		d_ogr_writer->write_polygon_feature(polygon_on_sphere,d_key_value_dictionary);

	}
	catch(...)
	{
		qDebug() << "Exception caught writing polygon to shapefile.";
	}
}


void
GPlatesFileIO::OgrGeometryExporter::visit_polyline_on_sphere(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	if (!d_ogr_writer)
	{
		return;
	}
	try
	{
		d_ogr_writer->write_polyline_feature(polyline_on_sphere,d_key_value_dictionary);
	}
	catch(...)
	{
		qDebug() << "Exception caught writing polyline to shapefile.";
	}
}

void
GPlatesFileIO::OgrGeometryExporter::export_geometry(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr)
{
	d_key_value_dictionary.reset();
	geometry_ptr->accept_visitor(*this);
}

void
GPlatesFileIO::OgrGeometryExporter::export_geometry(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type key_value_dictionary)
{
	d_key_value_dictionary.reset(key_value_dictionary);	
	geometry_ptr->accept_visitor(*this);
}
