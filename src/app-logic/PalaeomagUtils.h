/* $Id: ReconstructedVirtualGeomagneticPole.h 9024 2010-07-30 10:47:35Z elau $ */


/**
 * \file 
 * $Revision: 9024 $
 * $Date: 2010-07-30 12:47:35 +0200 (fr, 30 jul 2010) $
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
 
#ifndef GPLATES_APP_LOGIC_PALAEOMAGUTILS_H
#define GPLATES_APP_LOGIC_PALAEOMAGUTILS_H

#include "app-logic/ReconstructedVirtualGeomagneticPole.h"
#include "model/FeatureVisitor.h"

namespace GPlatesAppLogic
{
	namespace PalaeomagUtils
	{
		/**
		 *  Obtains pmag related properties from a vgp feature.                                                                    
		 */		
		class VirtualGeomagneticPolePropertyFinder :
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			VirtualGeomagneticPolePropertyFinder():
			  d_is_vgp_feature(false)
			{ }



			const boost::optional<GPlatesModel::integer_plate_id_type>
			get_plate_id()
			{
				return d_plate_id;
			}
		
			const boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
			get_site_point()
			{
				return d_site_point;
			}

			const boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
			get_vgp_point()
			{
				return d_vgp_point;
			}

			const boost::optional<double>
			get_age()
			{
				return d_age;
			}

			bool
			is_vgp_feature()
			{
				return d_is_vgp_feature;
			}

		private:

			virtual
			bool
			initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

			virtual
			void
			visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

			virtual
			void
			visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

			virtual
			void
			visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

			virtual
			void
			visit_xs_double(
				const GPlatesPropertyValues:: XsDouble &xs_double);

			// May use this later to fill up the ReconVGPParams structure in the ReconstructedFeatureGeometryPopulator. 
			//ReconstructedVirtualGeomagneticPoleParams &d_vgp_params;

			boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;
			boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> d_site_point;
			boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> d_vgp_point;
			boost::optional<double> d_age;

			bool d_is_vgp_feature;
		};


	}
}


#endif // GPLATES_APP_LOGIC_PALAEOMAGUTILS_H