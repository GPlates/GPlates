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

#include <algorithm>
#include <boost/foreach.hpp>
#include <QDebug>

#include "ScalarField3DLayerProxy.h"

#include "ExtractScalarField3DFeatureProperties.h"


const boost::optional<GPlatesPropertyValues::TextContent> &
GPlatesAppLogic::ScalarField3DLayerProxy::get_scalar_field_filename(
		const double &reconstruction_time)
{
	// If the reconstruction time has changed then we need to re-resolve the scalar field feature properties.
	if (d_cached_resolved_scalar_field_feature_properties.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time) ||
		!d_cached_resolved_scalar_field_feature_properties.cached_scalar_field_filename)
	{
		// Attempt to resolve the scalar field feature.
		if (!resolve_scalar_field_feature(reconstruction_time))
		{
			invalidate_scalar_field();
		}

		d_cached_resolved_scalar_field_feature_properties.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	return d_cached_resolved_scalar_field_feature_properties.cached_scalar_field_filename;
}


boost::optional<GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_type>
GPlatesAppLogic::ScalarField3DLayerProxy::get_resolved_scalar_field_3d(
		const double &reconstruction_time)
{
	// If we have no input scalar field feature then there's nothing we can do.
	if (!d_current_scalar_field_feature)
	{
		return boost::none;
	}

	if (!get_scalar_field_filename(reconstruction_time))
	{
		// We need a valid scalar field for the specified reconstruction time.
		return boost::none;
	}

	// Create a resolved scalar field reconstruction geometry.
	return ResolvedScalarField3D::create(
			*d_current_scalar_field_feature.get().handle_ptr(),
			reconstruction_time,
			// FIXME: Dodgy - need to remove requirement of providing a reconstruction tree...
			create_reconstruction_tree(reconstruction_time, 0),
			GPlatesUtils::get_non_null_pointer(this));
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::ScalarField3DLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.

	return d_subject_token;
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::ScalarField3DLayerProxy::get_scalar_field_subject_token(
		const double &reconstruction_time)
{
	// We need to check if the new reconstruction time will resolve to a different scalar field.
	// Because if it will then we need to let the caller know.
	//
	// Get the scalar field for the specified time - this will invalidate the scalar field subject token
	// if the scalar field itself has changed (or if the scalar field could not be obtained).
	get_scalar_field_filename(reconstruction_time);

	return d_scalar_field_subject_token;
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::ScalarField3DLayerProxy::get_scalar_field_feature_subject_token()
{
	return d_scalar_field_feature_subject_token;
}


void
GPlatesAppLogic::ScalarField3DLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't invalidate our caches because we only do that when the client
	// requests a reconstruction time that differs from the cached reconstruction time.
}


void
GPlatesAppLogic::ScalarField3DLayerProxy::set_current_scalar_field_feature(
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> scalar_field_feature,
		const ScalarField3DLayerTask::Params &scalar_field_params)
{
	d_current_scalar_field_feature = scalar_field_feature;

	set_scalar_field_params(scalar_field_params);

	// The scalar field feature has changed.
	invalidate_scalar_field_feature();
}


void
GPlatesAppLogic::ScalarField3DLayerProxy::modified_scalar_field_feature(
		const ScalarField3DLayerTask::Params &scalar_field_params)
{
	set_scalar_field_params(scalar_field_params);

	// The scalar field feature has changed.
	invalidate_scalar_field_feature();
}


void
GPlatesAppLogic::ScalarField3DLayerProxy::set_scalar_field_params(
		const ScalarField3DLayerTask::Params &raster_params)
{
}


void
GPlatesAppLogic::ScalarField3DLayerProxy::invalidate_scalar_field_feature()
{
	// The scalar field feature has changed.
	d_scalar_field_feature_subject_token.invalidate();

	// Also means the scalar field itself might have changed so invalidate it.
	invalidate_scalar_field();
}


void
GPlatesAppLogic::ScalarField3DLayerProxy::invalidate_scalar_field()
{
	d_cached_resolved_scalar_field_feature_properties.invalidate();

	// The scalar field.
	// Either it's a time-dependent scalar field and a new time was requested, or
	// the scalar field feature changed.
	d_scalar_field_subject_token.invalidate();

	// Also means this scalar field layer proxy has changed.
	invalidate();
}


void
GPlatesAppLogic::ScalarField3DLayerProxy::invalidate()
{
	// This raster layer proxy has changed in some way.
	d_subject_token.invalidate();
}


bool
GPlatesAppLogic::ScalarField3DLayerProxy::resolve_scalar_field_feature(
		const double &reconstruction_time)
{
	if (!d_current_scalar_field_feature)
	{
		// We must have a scalar field feature.
		return false;
	}

	// Extract the scalar field feature properties at the specified reconstruction time.
	GPlatesAppLogic::ExtractScalarField3DFeatureProperties visitor(reconstruction_time);
	visitor.visit_feature(d_current_scalar_field_feature.get());

	if (!visitor.get_scalar_field_filename())
	{
		return false;
	}

	// If the scalar field filename has changed then let clients know.
	// This happens for time-dependent scalar fields as the reconstruction time is changed far enough
	// away from the last cached time that a new scalar field is encountered.
	if (visitor.get_scalar_field_filename() != d_cached_resolved_scalar_field_feature_properties.cached_scalar_field_filename)
	{
		invalidate_scalar_field();
	}

	// Get the scalar field filename.
	d_cached_resolved_scalar_field_feature_properties.cached_scalar_field_filename = visitor.get_scalar_field_filename();

	return true;
}
