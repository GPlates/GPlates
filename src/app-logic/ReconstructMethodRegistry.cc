/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "ReconstructMethodRegistry.h"

#include "ReconstructMethodByPlateId.h"
#include "ReconstructMethodFlowline.h"
#include "ReconstructMethodHalfStageRotation.h"
#include "ReconstructMethodMotionPath.h"
#include "ReconstructMethodSmallCircle.h"
#include "ReconstructMethodVirtualGeomagneticPole.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


void
GPlatesAppLogic::ReconstructMethodRegistry::register_reconstruct_method(
		ReconstructMethod::Type reconstruct_method_type,
		const can_reconstruct_feature_function_type &can_reconstruct_feature_function_,
		const create_reconstruct_method_function_type &create_reconstruct_method_function_)
{
	d_reconstruct_method_info_map.insert(
			std::make_pair(
				reconstruct_method_type,
				ReconstructMethodInfo(
					can_reconstruct_feature_function_,
					create_reconstruct_method_function_)));
}


void
GPlatesAppLogic::ReconstructMethodRegistry::unregister_reconstruct_method(
		ReconstructMethod::Type reconstruct_method_type)
{
	d_reconstruct_method_info_map.erase(reconstruct_method_type);
}


std::vector<GPlatesAppLogic::ReconstructMethod::Type>
GPlatesAppLogic::ReconstructMethodRegistry::get_registered_reconstruct_methods() const
{
	std::vector<ReconstructMethod::Type> reconstruct_method_types;

	BOOST_FOREACH(
			const reconstruct_method_info_map_type::value_type &reconstruct_method_entry,
			d_reconstruct_method_info_map)
	{
		reconstruct_method_types.push_back(reconstruct_method_entry.first);
	}

	return reconstruct_method_types;
}


bool
GPlatesAppLogic::ReconstructMethodRegistry::can_reconstruct_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const
{
	// Iterate over the registered reconstruct methods.
	BOOST_FOREACH(
			const reconstruct_method_info_map_type::value_type &reconstruct_method_entry,
			d_reconstruct_method_info_map)
	{
		if (reconstruct_method_entry.second.can_reconstruct_feature_function(feature_ref))
		{
			// We've found a reconstruct method that can process the specified feature.
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::ReconstructMethodRegistry::can_reconstruct_feature(
		ReconstructMethod::Type reconstruct_method_type,
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const
{
	reconstruct_method_info_map_type::const_iterator iter =
			d_reconstruct_method_info_map.find(reconstruct_method_type);

	// Throw exception if reconstruct method type has not been registered.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			iter != d_reconstruct_method_info_map.end(),
			GPLATES_ASSERTION_SOURCE);

	const ReconstructMethodInfo &reconstruct_method_info = iter->second;

	return reconstruct_method_info.can_reconstruct_feature_function(feature_ref);
}


boost::optional<GPlatesAppLogic::ReconstructMethod::Type>
GPlatesAppLogic::ReconstructMethodRegistry::get_reconstruct_method_type(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref) const
{
	// Iterate over the registered layer tasks.
	// NOTE: We are iterating in reverse order so that we query the reconstruct methods
	// with the larger enumeration values for ReconstructMethod::Type before smaller values.
	// This has the effect of querying more specialised methods before more generalised methods.
	reconstruct_method_info_map_type::const_reverse_iterator riter = d_reconstruct_method_info_map.rbegin();
	const reconstruct_method_info_map_type::const_reverse_iterator rend = d_reconstruct_method_info_map.rend();
	for ( ; riter != rend; ++riter)
	{
		const ReconstructMethodInfo &reconstruct_method_info = riter->second;

		if (reconstruct_method_info.can_reconstruct_feature_function(feature_ref))
		{
			const ReconstructMethod::Type reconstruct_method_type = riter->first;

			return reconstruct_method_type;
		}
	}

	// No suitable reconstruct methods were found.
	return boost::none;
}


boost::optional<GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type>
GPlatesAppLogic::ReconstructMethodRegistry::create_reconstruct_method(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const ReconstructMethodInterface::Context &reconstruct_method_context) const
{
	// Iterate over the registered layer tasks.
	// NOTE: We are iterating in reverse order so that we query the reconstruct methods
	// with the larger enumeration values for ReconstructMethod::Type before smaller values.
	// This has the effect of querying more specialised methods before more generalised methods.
	reconstruct_method_info_map_type::const_reverse_iterator riter = d_reconstruct_method_info_map.rbegin();
	const reconstruct_method_info_map_type::const_reverse_iterator rend = d_reconstruct_method_info_map.rend();
	for ( ; riter != rend; ++riter)
	{
		const ReconstructMethodInfo &reconstruct_method_info = riter->second;

		if (reconstruct_method_info.can_reconstruct_feature_function(feature_ref))
		{
			return reconstruct_method_info.create_reconstruct_method_function(
					feature_ref,
					reconstruct_method_context);
		}
	}

	// No suitable reconstruct methods were found.
	return boost::none;
}


GPlatesAppLogic::ReconstructMethod::Type
GPlatesAppLogic::ReconstructMethodRegistry::get_reconstruct_method_or_default_type(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref) const
{
	const boost::optional<ReconstructMethod::Type> reconstruct_method_type =
			get_reconstruct_method_type(feature_ref);
	if (reconstruct_method_type)
	{
		return reconstruct_method_type.get();
	}

	return ReconstructMethod::BY_PLATE_ID;
}


GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type
GPlatesAppLogic::ReconstructMethodRegistry::create_reconstruct_method_or_default(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const ReconstructMethodInterface::Context &reconstruct_method_context) const
{
	const boost::optional<ReconstructMethodInterface::non_null_ptr_type> reconstruct_method =
			create_reconstruct_method(feature_ref, reconstruct_method_context);
	if (reconstruct_method)
	{
		return reconstruct_method.get();
	}

	// Find the 'ReconstructMethod::BY_PLATE_ID' reconstruct method entry.
	reconstruct_method_info_map_type::const_iterator by_plate_id_iter =
			d_reconstruct_method_info_map.find(ReconstructMethod::BY_PLATE_ID);

	// Throw exception if not registered.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			by_plate_id_iter != d_reconstruct_method_info_map.end(),
			GPLATES_ASSERTION_SOURCE);

	const ReconstructMethodInfo &by_plate_id_reconstruct_method_info = by_plate_id_iter->second;

	return by_plate_id_reconstruct_method_info.create_reconstruct_method_function(
			feature_ref,
			reconstruct_method_context);
}


GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type
GPlatesAppLogic::ReconstructMethodRegistry::create_reconstruct_method(
		const ReconstructMethodInterface &reconstruct_method,
		const ReconstructMethodInterface::Context &reconstruct_method_context) const
{
	reconstruct_method_info_map_type::const_iterator reconstruct_method_info_iter =
			d_reconstruct_method_info_map.find(reconstruct_method.get_reconstruction_method_type());

	// Throw exception if reconstruct method type has not been registered.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reconstruct_method_info_iter != d_reconstruct_method_info_map.end(),
			GPLATES_ASSERTION_SOURCE);

	const ReconstructMethodInfo &reconstruct_method_info = reconstruct_method_info_iter->second;

	return reconstruct_method_info.create_reconstruct_method_function(
			reconstruct_method.get_feature_ref(),
			reconstruct_method_context);
}


GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type
GPlatesAppLogic::ReconstructMethodRegistry::create_reconstruct_method(
		ReconstructMethod::Type reconstruct_method_type,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const ReconstructMethodInterface::Context &reconstruct_method_context) const
{
	reconstruct_method_info_map_type::const_iterator reconstruct_method_info_iter =
			d_reconstruct_method_info_map.find(reconstruct_method_type);

	// Throw exception if reconstruct method type has not been registered.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reconstruct_method_info_iter != d_reconstruct_method_info_map.end(),
			GPLATES_ASSERTION_SOURCE);

	const ReconstructMethodInfo &reconstruct_method_info = reconstruct_method_info_iter->second;

	return reconstruct_method_info.create_reconstruct_method_function(
			feature_ref,
			reconstruct_method_context);
}


void
GPlatesAppLogic::register_default_reconstruct_method_types(
		ReconstructMethodRegistry &registry)
{
	//
	// NOTE: The order of registration does *not* matter.
	// It's the order of ReconstructMethod::Type enumerations that matters - higher value
	// enumerations are more specialised and get higher priority.
	//

	//
	// Reconstruct by plate ID.
	//
	registry.register_reconstruct_method(
			ReconstructMethod::BY_PLATE_ID,
			&ReconstructMethodByPlateId::can_reconstruct_feature,
			&ReconstructMethodByPlateId::create);

	//
	// Reconstruct using half-stage rotation.
	//
	registry.register_reconstruct_method(
			ReconstructMethod::HALF_STAGE_ROTATION,
			&ReconstructMethodHalfStageRotation::can_reconstruct_feature,
			&ReconstructMethodHalfStageRotation::create);

	//
	// Reconstruct Virtual Geomagnetic Poles.
	//
	registry.register_reconstruct_method(
			ReconstructMethod::VIRTUAL_GEOMAGNETIC_POLE,
			&ReconstructMethodVirtualGeomagneticPole::can_reconstruct_feature,
			&ReconstructMethodVirtualGeomagneticPole::create);

	//
	// Reconstruct flowlines.
	//
	registry.register_reconstruct_method(
			ReconstructMethod::FLOWLINE,
			&ReconstructMethodFlowline::can_reconstruct_feature,
			&ReconstructMethodFlowline::create);

	//
	// Reconstruct motion paths.
	//
	registry.register_reconstruct_method(
			ReconstructMethod::MOTION_PATH,
			&ReconstructMethodMotionPath::can_reconstruct_feature,
			&ReconstructMethodMotionPath::create);

	//
	// Reconstruct small circles.
	//
	registry.register_reconstruct_method(
			ReconstructMethod::SMALL_CIRCLE,
			&ReconstructMethodSmallCircle::can_reconstruct_feature,
			&ReconstructMethodSmallCircle::create);
}
