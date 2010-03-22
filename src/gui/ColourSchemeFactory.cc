/* $Id$ */

/**
 * @file 
 * Contains the implementation of the ColourSchemeFactory class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "AgeColourPalette.h"
#include "ColourSchemeFactory.h"
#include "FeatureColourPalette.h"
#include "GenericColourScheme.h"
#include "PlateIdColourPalettes.h"
#include "SingleColourScheme.h"

#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "maths/Real.h"
#include "model/types.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "property-values/GeoTimeInstant.h"

#include <boost/optional.hpp>

namespace
{
	/**
	 * Extracts the plate ID for use by GenericColourScheme.
	 */
	class PlateIdPropertyExtractor
	{
	public:
		
		typedef GPlatesModel::integer_plate_id_type return_type;

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
		{
			return GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id(
						&reconstruction_geometry);
		}
	};

	/**
	 * Extracts the age for use by GenericColourScheme.
	 */
	class AgePropertyExtractor
	{
	public:

		typedef GPlatesMaths::Real return_type;

		AgePropertyExtractor(
				const GPlatesAppLogic::Reconstruct &reconstruct) :
			d_reconstruct(reconstruct)
		{  }

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
		{
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> geo_time =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_time_of_formation(
					&reconstruction_geometry);
			if (!geo_time)
			{
				return boost::none;
			}

			if (geo_time->is_distant_past())
			{
				// Distant past.
				// Cannot calculate 'age' from the point of view of the current reconstruction time.
				return GPlatesMaths::Real::negative_infinity();
			}
			else if (geo_time->is_distant_future())
			{
				// Distant future.
				return GPlatesMaths::Real::positive_infinity();
			}
			else
			{
				// Has a real time of formation.
				return GPlatesMaths::Real(
						geo_time->value() - d_reconstruct.get_current_reconstruction_time());
			}
		}
	
	private:

		const GPlatesAppLogic::Reconstruct &d_reconstruct;
	};

	/**
	 * Extracts the feature type for use by GenericColourScheme.
	 */
	class FeaturePropertyExtractor
	{
	public:

		typedef GPlatesModel::FeatureType return_type;

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
		{
			GPlatesModel::FeatureHandle::weak_ref feature_ref;
			if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
						&reconstruction_geometry, feature_ref))
			{
				return boost::none;
			}
			else
			{
				return feature_ref->handle_data().feature_type();
			}
		}
	};
}

boost::shared_ptr<GPlatesGui::ColourScheme>
GPlatesGui::ColourSchemeFactory::create_single_colour_scheme(
		const QColor &qcolor)
{
	return boost::shared_ptr<ColourScheme>(
			new SingleColourScheme(qcolor));
}

boost::shared_ptr<GPlatesGui::ColourScheme>
GPlatesGui::ColourSchemeFactory::create_default_plate_id_colour_scheme()
{
	return boost::shared_ptr<ColourScheme>(
			new GenericColourScheme<PlateIdPropertyExtractor>(
				new DefaultPlateIdColourPalette()));
}

boost::shared_ptr<GPlatesGui::ColourScheme>
GPlatesGui::ColourSchemeFactory::create_regional_plate_id_colour_scheme()
{
	return boost::shared_ptr<ColourScheme>(
			new GenericColourScheme<PlateIdPropertyExtractor>(
				new RegionalPlateIdColourPalette()));
}

boost::shared_ptr<GPlatesGui::ColourScheme>
GPlatesGui::ColourSchemeFactory::create_default_age_colour_scheme(
		const GPlatesAppLogic::Reconstruct &reconstruct)
{
	return boost::shared_ptr<ColourScheme>(
			new GenericColourScheme<AgePropertyExtractor>(
				new AgeColourPalette(),
				AgePropertyExtractor(reconstruct)));
}

boost::shared_ptr<GPlatesGui::ColourScheme>
GPlatesGui::ColourSchemeFactory::create_default_feature_colour_scheme()
{
	return boost::shared_ptr<ColourScheme>(
			new GenericColourScheme<FeaturePropertyExtractor>(
				new FeatureColourPalette()));
}

