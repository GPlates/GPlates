/* $Id$ */

/**
 * \file 
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
#ifndef GPLATESDATAMINING_REGIONOFINTERESTFILTER_H
#define GPLATESDATAMINING_REGIONOFINTERESTFILTER_H

#include <vector>
#include <QDebug>
#include <boost/foreach.hpp>

#include "CoRegFilter.h"
#include "IsCloseEnoughChecker.h"

#include "app-logic/ReconstructedFeatureGeometry.h"


namespace GPlatesDataMining
{
	class RegionOfInterestFilter : public CoRegFilter
	{
	public:
		RegionOfInterestFilter(
				const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature, 
				const double range):
			d_reconstructed_seed_feature(reconstructed_seed_feature),
			d_range(range)
			{	}

		class Config : public CoRegFilter::Config
		{
		public:
			explicit
			Config(double val) :
				d_range(val)
			{	}

			CoRegFilter*
			create_filter(
					const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature)
			{
				return new RegionOfInterestFilter(reconstructed_seed_feature, d_range);
			}

			bool
			is_same_type(const CoRegFilter::Config* other)
			{
				return dynamic_cast<const RegionOfInterestFilter::Config*>(other);
			}

			const QString
			to_string()
			{
				return QString("Region of Interest(%1)").arg(d_range);
			}

			std::vector<QString>
			get_parameters_as_strings() const
			{
				std::vector<QString> ret;
				ret.push_back(QString::number(d_range));
				return ret;
			}

			bool
			operator< (const CoRegFilter::Config& other)
			{
				if(!is_same_type(&other))
					throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"Try to compare different filter types.");
				return d_range < dynamic_cast<const RegionOfInterestFilter::Config&>(other).range();
			}

			bool
			operator==(const CoRegFilter::Config& other) 
			{
				if(!is_same_type(&other))
					throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"Try to compare different filter types.");
				return GPlatesMaths::are_slightly_more_strictly_equal(d_range,dynamic_cast<const RegionOfInterestFilter::Config&>(other).range());
			}

			const QString
			filter_name()
			{
				return "Region of Interest";
			}
			
			~Config(){ }

			const double &
			range() const
			{
				return d_range;
			}

		private:
			double d_range;
		};

		void
		process(
				CoRegFilter::reconstructed_feature_vector_type::const_iterator input_begin,
				CoRegFilter::reconstructed_feature_vector_type::const_iterator input_end,
				CoRegFilter::reconstructed_feature_vector_type& output) 
		{
			// Iterate over the reconstructed target features.
			for(; input_begin != input_end; input_begin++)
			{
				const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_target_feature =
						*input_begin;

				// For the current reconstructed target feature filter those geometries that are
				// within the region of interest of any of our reconstructed seed features.
				GPlatesAppLogic::ReconstructContext::ReconstructedFeature::reconstruction_seq_type
						filtered_reconstructed_target_geometries;
				region_of_interest_filter(
						filtered_reconstructed_target_geometries,
						reconstructed_target_feature);

				// If any within ROI then add a filtered reconstructed target feature to the results.
				if (!filtered_reconstructed_target_geometries.empty())
				{
					GPlatesAppLogic::ReconstructContext::ReconstructedFeature filtered_reconstructed_target_feature(
							reconstructed_target_feature.get_feature(),
							filtered_reconstructed_target_geometries);

					output.push_back(filtered_reconstructed_target_feature);
				}
			}
		}

		~RegionOfInterestFilter(){ }

	protected:

		void
		region_of_interest_filter(
				GPlatesAppLogic::ReconstructContext::ReconstructedFeature::reconstruction_seq_type &filtered_reconstructed_target_geometries,
				const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_target_feature)
		{
			const GPlatesAppLogic::ReconstructContext::ReconstructedFeature::reconstruction_seq_type &
					reconstructed_target_geometries = reconstructed_target_feature.get_reconstructions();

			// Iterate over the reconstructed target feature's geometries first.
			BOOST_FOREACH(
					const GPlatesAppLogic::ReconstructContext::Reconstruction &reconstructed_target_geom,
					reconstructed_target_geometries)
			{
				const GPlatesAppLogic::ReconstructContext::ReconstructedFeature::reconstruction_seq_type &
						reconstructed_seed_geometries = d_reconstructed_seed_feature.get_reconstructions();

				// Iterate over the reconstructed seed feature's geometries.
				// If the current target geometry is close enough to any of the seed geometries then add it.
				BOOST_FOREACH(
						const GPlatesAppLogic::ReconstructContext::Reconstruction &reconstructed_seed_geom,
						reconstructed_seed_geometries)
				{
					if (is_close_enough(
							*reconstructed_seed_geom.get_reconstructed_feature_geometry()->reconstructed_geometry(), 
							*reconstructed_target_geom.get_reconstructed_feature_geometry()->reconstructed_geometry(), 
							d_range))
					{
						filtered_reconstructed_target_geometries.push_back(
								GPlatesAppLogic::ReconstructContext::Reconstruction(
										reconstructed_target_geom.get_geometry_property_handle(),
										reconstructed_target_geom.get_reconstructed_feature_geometry()));

						// Only add the current reconstructed target geometry once.
						break;
					}
				}
			}
		}

		const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &d_reconstructed_seed_feature;
		double d_range;
	};
}
#endif





