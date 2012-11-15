/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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


#include "ExtractScalarField3DFeatureProperties.h"


#include <boost/foreach.hpp>
namespace
{
	/**
	 * Visits a feature collection and determines whether the feature collection
	 * contains any scalar field features.
	 */
	class CanResolveScalarField3DFeature :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		CanResolveScalarField3DFeature() :
			d_has_raster_feature(false)
		{  }

		bool
		has_scalar_field_3d_feature() const
		{
			return d_has_raster_feature;
		}

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			d_seen_gpml_scalar_field_3d_file = false;

			return true;
		}

		virtual
		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			if (d_seen_gpml_scalar_field_3d_file)
			{
				d_has_raster_feature = true;
			}
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			d_inside_constant_value = true;
			gpml_constant_value.value()->accept_visitor(*this);
			d_inside_constant_value = false;
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			d_inside_piecewise_aggregation = true;
			std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows =
				gpml_piecewise_aggregation.time_windows();
			BOOST_FOREACH(const GPlatesPropertyValues::GpmlTimeWindow &time_window, time_windows)
			{
				time_window.time_dependent_value()->accept_visitor(*this);
			}
			d_inside_piecewise_aggregation = false;
		}

		virtual
		void
		visit_gpml_scalar_field_3d_file(
				const GPlatesPropertyValues::GpmlScalarField3DFile &gpml_scalar_field_3d_file)
		{
			static const GPlatesModel::PropertyName SCALAR_FIELD_FILE =
					GPlatesModel::PropertyName::create_gpml("file");

			if (d_inside_constant_value || d_inside_piecewise_aggregation)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == SCALAR_FIELD_FILE)
				{
					d_seen_gpml_scalar_field_3d_file = true;
				}
			}
		}

	private:

		bool d_seen_gpml_scalar_field_3d_file;

		bool d_inside_constant_value;
		bool d_inside_piecewise_aggregation;

		bool d_has_raster_feature;
	};
}


bool
GPlatesAppLogic::is_scalar_field_3d_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	CanResolveScalarField3DFeature visitor;
	visitor.visit_feature(feature);
	return visitor.has_scalar_field_3d_feature();
}


bool
GPlatesAppLogic::contains_scalar_field_3d_feature(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	// Temporarily disable scalar fields on trunk until fully implemented...
#if 1
	return false;
#endif

	CanResolveScalarField3DFeature visitor;
	for (GPlatesModel::FeatureCollectionHandle::const_iterator iter = feature_collection->begin();
			iter != feature_collection->end(); ++iter)
	{
		visitor.visit_feature(iter);
	}
	return visitor.has_scalar_field_3d_feature();
}


GPlatesAppLogic::ExtractScalarField3DFeatureProperties::ExtractScalarField3DFeatureProperties(
			const double &reconstruction_time) :
	d_reconstruction_time(reconstruction_time),
	d_inside_constant_value(false),
	d_inside_piecewise_aggregation(false)
{
}


const boost::optional<GPlatesPropertyValues::TextContent> &
GPlatesAppLogic::ExtractScalarField3DFeatureProperties::get_scalar_field_filename() const
{
	return d_filename;
}


bool
GPlatesAppLogic::ExtractScalarField3DFeatureProperties::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	d_filename = boost::none;

	return true;
}


void
GPlatesAppLogic::ExtractScalarField3DFeatureProperties::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	d_inside_constant_value = true;
	gpml_constant_value.value()->accept_visitor(*this);
	d_inside_constant_value = false;
}


void
GPlatesAppLogic::ExtractScalarField3DFeatureProperties::visit_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	d_inside_piecewise_aggregation = true;
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows =
		gpml_piecewise_aggregation.time_windows();
	BOOST_FOREACH(const GPlatesPropertyValues::GpmlTimeWindow &time_window, time_windows)
	{
		const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
				time_window.valid_time();

		// If the time window period contains the current reconstruction time then visit.
		// The time periods should be mutually exclusive - if we happen to be it
		// two time periods then we're probably right on the boundary between the two
		// and then it doesn't really matter which one we choose.
		if (time_period->contains(d_reconstruction_time))
		{
			time_window.time_dependent_value()->accept_visitor(*this);
		}
	}
	d_inside_piecewise_aggregation = false;
}


void
GPlatesAppLogic::ExtractScalarField3DFeatureProperties::visit_gpml_scalar_field_3d_file(
		const GPlatesPropertyValues::GpmlScalarField3DFile &gpml_scalar_field_3d_file)
{
	static const GPlatesModel::PropertyName SCALAR_FIELD_FILE =
			GPlatesModel::PropertyName::create_gpml("file");

	if (d_inside_constant_value || d_inside_piecewise_aggregation)
	{
		const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
		if (propname && *propname == SCALAR_FIELD_FILE)
		{
			d_filename = gpml_scalar_field_3d_file.file_name()->value();
		}
	}
}
