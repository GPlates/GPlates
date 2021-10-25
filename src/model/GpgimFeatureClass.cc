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
#include <boost/foreach.hpp>

#include "GpgimFeatureClass.h"

void
GPlatesModel::GpgimFeatureClass::get_feature_properties(
		gpgim_property_seq_type &feature_properties) const
{
	// Recursively add the ancestor class feature properties first.
	if (d_parent_feature_class)
	{
		d_parent_feature_class.get()->get_feature_properties(feature_properties);
	}

	// Then add the feature properties from this (super)class.
	feature_properties.insert(
			feature_properties.end(),
			d_feature_properties.begin(),
			d_feature_properties.end());
}


bool
GPlatesModel::GpgimFeatureClass::get_feature_properties(
		const GPlatesPropertyValues::StructuralType &property_type,
		boost::optional<gpgim_property_seq_type &> feature_properties) const
{
	bool found_property_type = false;

	// First, recursively search our ancestor feature classes.
	if (d_parent_feature_class)
	{
		found_property_type = d_parent_feature_class.get()->get_feature_properties(
				property_type,
				feature_properties);
	}

	// Next, search all feature properties in this feature class.
	BOOST_FOREACH(
			const GpgimProperty::non_null_ptr_to_const_type &feature_property,
			d_feature_properties)
	{
		// See if the current feature property has a matching structural type.
		boost::optional<GpgimStructuralType::non_null_ptr_to_const_type> structural_type =
				feature_property->get_structural_type(property_type);
		if (structural_type)
		{
			if (feature_properties)
			{
				feature_properties->push_back(feature_property);
			}

			found_property_type = true;
		}
	}

	return found_property_type;
}


boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type>
GPlatesModel::GpgimFeatureClass::get_feature_property(
		const GPlatesModel::PropertyName &property_name) const
{
	// First, test all feature properties in this feature class.
	BOOST_FOREACH(
			const GpgimProperty::non_null_ptr_to_const_type &feature_property,
			d_feature_properties)
	{
		if (property_name == feature_property->get_property_name())
		{
			return feature_property;
		}
	}

	// Next, recursively test our ancestor feature classes.
	if (d_parent_feature_class)
	{
		return d_parent_feature_class.get()->get_feature_property(property_name);
	}

	// Not found.
	return boost::none;
}


bool
GPlatesModel::GpgimFeatureClass::get_geometry_feature_properties(
		gpgim_property_seq_type &geometry_feature_properties) const
{
	bool has_geometry_feature_properties = false;

	// Recursively add the ancestor class geometry feature properties first.
	if (d_parent_feature_class)
	{
		if (d_parent_feature_class.get()->get_geometry_feature_properties(geometry_feature_properties))
		{
			has_geometry_feature_properties = true;
		}
	}

	// Then add the geometry feature properties from this (super)class.
	BOOST_FOREACH(
			const GpgimProperty::non_null_ptr_to_const_type &feature_property,
			d_feature_properties)
	{
		if (feature_property->has_geometry_structural_type())
		{
			geometry_feature_properties.push_back(feature_property);
			has_geometry_feature_properties = true;
		}
	}

	return has_geometry_feature_properties;
}


boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type>
GPlatesModel::GpgimFeatureClass::get_default_geometry_feature_property() const
{
	// First, see if this feature class has a default geometry property. If it does then we don't
	// need to search ancestor classes because derived classes override ancestor classes.
	if (d_default_geometry_property)
	{
		return d_default_geometry_property.get();
	}

	// Next, recursively search our ancestor feature classes.
	if (d_parent_feature_class)
	{
		return d_parent_feature_class.get()->get_default_geometry_feature_property();
	}

	// Not found.
	return boost::none;
}


bool
GPlatesModel::GpgimFeatureClass::does_inherit_from(
		const GPlatesModel::FeatureType &feature_type) const
{
	boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type> feature_class(
			get_non_null_pointer(this));

	// Iterate over the ancestor feature classes.
	while (feature_class)
	{
		if (feature_class.get()->get_feature_type() == feature_type)
		{
			return true;
		}

		feature_class = feature_class.get()->get_parent_feature_class();
	}

	return false;
}
