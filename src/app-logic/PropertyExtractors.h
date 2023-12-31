/* $Id$ */

/**
 * \file 
 * Contains a collection of functors that extract properties from ReconstructionGeometry.
 *
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

#ifndef GPLATES_APP_LOGIC_PROPERTYEXTRACTORS_H
#define GPLATES_APP_LOGIC_PROPERTYEXTRACTORS_H

#include <functional>
#include <boost/optional.hpp>

#include "ApplicationState.h"
#include "ReconstructionGeometry.h"
#include "ReconstructionGeometryUtils.h"

#include "maths/Real.h"

#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/FeatureUtils.h"


namespace GPlatesAppLogic
{
	template<typename Adaptee, typename ReturnType>
	class PropertyExtractorAdapter
	{
	public:

		typedef ReturnType return_type;

		explicit
		PropertyExtractorAdapter(
				const Adaptee &adaptee = Adaptee()) :
			d_adaptee(adaptee)
		{
		}

		template<typename ArguType>
		const boost::optional<return_type>
		operator()(const ArguType& argu) const
		{
			typedef typename Adaptee::return_type adaptee_return_type;
			boost::optional<adaptee_return_type> result = d_adaptee(argu);
			if (result)
			{
				return static_cast<return_type>(*result);
			}
			else
			{
				return boost::none;
			}
		}

	private:

		Adaptee d_adaptee;
	};

	/**
	 * Extracts the plate ID for use by GenericColourScheme.
	 */
	class PlateIdPropertyExtractor
	{
	public:
		
		typedef GPlatesModel::integer_plate_id_type return_type;

		const boost::optional<return_type>
		operator()(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const;
	
		const boost::optional<return_type>
		operator()(
				const GPlatesModel::FeatureHandle& feature) const;
	};

	/**
	 * Extracts the age for use by GenericColourScheme.
	 */
	class AgePropertyExtractor
	{
	public:

		typedef GPlatesMaths::Real return_type;

		explicit
		AgePropertyExtractor(
				ApplicationState &application_state) :
			d_application_state(application_state)
		{  }

		const boost::optional<return_type>
		operator()(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const;

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::FeatureHandle& feature) const
		{
			return GPlatesUtils::get_age(
					&feature, 
					d_application_state.get_current_reconstruction_time());
		}
	
	private:
		ApplicationState &d_application_state;
	};

	/**
	 * Extracts the feature type for use by GenericColourScheme.
	 */
	class FeatureTypePropertyExtractor
	{
	public:

		typedef GPlatesModel::FeatureType return_type;

		const boost::optional<return_type>
		operator()(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const;

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::FeatureHandle& feature) const
		{
			return feature.feature_type();
		}
	};
}

#endif // GPLATES_APP_LOGIC_PROPERTYEXTRACTORS_H
