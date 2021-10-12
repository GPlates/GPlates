/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_OGRWRITER_H
#define GPLATES_FILEIO_OGRWRITER_H


#ifdef HAVE_CONFIG_H
// We're building on a UNIX-y system, and can thus expect "global/config.h".

// On some systems, it's <ogrsf_frmts.h>, on others, <gdal/ogrsf_frmts.h>.
// The "CMake" script should have determined which one to use.
#include "global/config.h"
#ifdef HAVE_GDAL_OGRSF_FRMTS_H
#include <gdal/ogrsf_frmts.h>
#else
#include <ogrsf_frmts.h>
#endif

#else  // We're not building on a UNIX-y system.  We'll have to assume it's <ogrsf_frmts.h>.
#include <ogrsf_frmts.h>
#endif  // HAVE_CONFIG_H

#include <QDebug>
#include <boost/optional.hpp>

#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "property-values/GpmlKeyValueDictionary.h"


namespace GPlatesFileIO
{

	class OgrWriter 
	{
	public:

		OgrWriter(
			QString filename,
			bool multiple_layers);

		~OgrWriter();

		/**
		 * Write a point feature to a point-type layer. If a point-type layer does not exist, it will
		 * be created if possible. 
		 */
		void
		write_point_feature(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);

		void
		write_multi_point_feature(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);		

		void
		write_polyline_feature(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);	

		void
		write_polyline_feature(
			const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polyline_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);	

		void
		write_polygon_feature(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);	
			
		void
		write_polygon_feature(
			const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygon_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);				

	private:

		/**
		 * Filename used by Ogr library to create a data source.
		 */
		QString d_filename;

		/**
		 * Filename stripped of any extension for use in naming layers. 
		 */
		QString d_root_filename;

		OGRDataSource *d_ogr_data_source_ptr;

		/**
		 * True if we know we need to create multiple layers, each layer holding different geometry types. 
		 * We need this to know what sort of string to pass during creation of the OGRDataSource.
		 */
		bool d_multiple_layers;

		/**
		 * Pointers to the geometry layers. Not all geometry layers will be required, hence they're
		 * optional.  These layer pointers don't have ownership of the layer. 
		 */
		boost::optional<OGRLayer*> d_ogr_point_layer;
		boost::optional<OGRLayer*> d_ogr_multi_point_layer;
		boost::optional<OGRLayer*> d_ogr_polyline_layer;
		boost::optional<OGRLayer*> d_ogr_polygon_layer;

	};

}

#endif // GPLATES_FILEIO_OGRWRITER_H
