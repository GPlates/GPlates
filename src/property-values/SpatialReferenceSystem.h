/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTY_VALUES_SPATIALREFERENCESYSTEM_H
#define GPLATES_PROPERTY_VALUES_SPATIALREFERENCESYSTEM_H

#include <memory>
#include <boost/shared_ptr.hpp>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


// Forward declarations.
class OGRSpatialReference;


namespace GPlatesPropertyValues
{
	/**
	 * A spatial reference system.
	 *
	 * This class wraps OGRSpatialReference.
	 */
	class SpatialReferenceSystem :
			public GPlatesUtils::ReferenceCount<SpatialReferenceSystem>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<SpatialReferenceSystem> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const SpatialReferenceSystem> non_null_ptr_to_const_type;


		/**
		 * Spatial reference system for standard "WGS84".
		 */
		static
		non_null_ptr_to_const_type
		get_WGS84();


		/**
		 * Creates a spatial reference system.
		 *
		 * The specified OGRSpatialReference is copied/cloned internally.
		 */
		static
		non_null_ptr_type
		create(
				const OGRSpatialReference &ogr_srs)
		{
			return non_null_ptr_type(new SpatialReferenceSystem(ogr_srs));
		}


		~SpatialReferenceSystem();


		/**
		 * Returns true if this spatial reference system is a geographic coordinate system.
		 */
		bool
		is_geographic() const;

		/**
		 * Returns true if this spatial reference system is a projected coordinate system.
		 */
		bool
		is_projected() const;

		/**
		 * Returns true if the spatial reference system is WGS84.
		 */
		bool is_wgs84() const;

		/**
		 * Return the internal OGR spatial reference system.
		 */
		const OGRSpatialReference &
		get_ogr_srs() const
		{
			return *d_ogr_srs;
		}

		/**
		 * Return the internal OGR spatial reference system.
		 */
		OGRSpatialReference &
		get_ogr_srs()
		{
			return *d_ogr_srs;
		}

	private:

		//! Delete OGRSpatialReference objects using 'OSRDestroySpatialReference()'.
		struct OGRSpatialReferenceDeleter
		{
			void
			operator()(
					OGRSpatialReference *ogr_srs);
		};

		boost::shared_ptr<OGRSpatialReference> d_ogr_srs;
		

		explicit
		SpatialReferenceSystem(
				const OGRSpatialReference &ogr_srs);

	};
}

#endif // GPLATES_PROPERTY_VALUES_SPATIALREFERENCESYSTEM_H
