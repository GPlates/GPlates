/* $Id$ */

/**
 * @file 
 * Contains the implementation of the ColourProxy class.
 * 
 * Most recent change:
 *   $Date$
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

#include "ColourProxy.h"
#include "ColourScheme.h"

// ColourProxy ////////////////////////////////////////////////////////////////

GPlatesGui::ColourProxy::ColourProxy(
		GPlatesModel::ReconstructionGeometry::non_null_ptr_type reconstruction_geometry_ptr,
		boost::shared_ptr<ColourFilter> colour_filter_ptr) :
	d_impl_ptr(
			new DeferredColourProxyImpl(
				reconstruction_geometry_ptr,
				colour_filter_ptr))
{
}


GPlatesGui::ColourProxy::ColourProxy(
		const Colour &colour) :
	d_impl_ptr(
			new FixedColourProxyImpl(
				boost::optional<Colour>(colour)))
{
}


GPlatesGui::ColourProxy::ColourProxy(
		boost::optional<Colour> colour) :
	d_impl_ptr(
			new FixedColourProxyImpl(
				colour))
{
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::ColourProxy::get_colour(
		ColourScheme::non_null_ptr_type colour_scheme) const
{
	return d_impl_ptr->get_colour(colour_scheme);
}


// DeferredColourProxyImpl ////////////////////////////////////////////////////

GPlatesGui::DeferredColourProxyImpl::DeferredColourProxyImpl(
		GPlatesModel::ReconstructionGeometry::non_null_ptr_type reconstruction_geometry_ptr,
		boost::shared_ptr<ColourFilter> colour_filter_ptr) :
	d_reconstruction_geometry_ptr(reconstruction_geometry_ptr),
	d_colour_filter_ptr(colour_filter_ptr)
{
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::DeferredColourProxyImpl::get_colour(
		ColourScheme::non_null_ptr_type colour_scheme) const
{
	boost::optional<Colour> colour =
		colour_scheme->get_colour(*d_reconstruction_geometry_ptr);
	if (!colour)
	{
		return boost::none;
	}

	// Run it through the colour filter, if one is installed.
	if (d_colour_filter_ptr)
	{
		*colour = d_colour_filter_ptr->change_colour(*colour);
	}

	return colour;
}


// FixedColourProxyImpl ///////////////////////////////////////////////////////

GPlatesGui::FixedColourProxyImpl::FixedColourProxyImpl(
		boost::optional<Colour> colour) :
	d_colour(colour)
{
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::FixedColourProxyImpl::get_colour(
		ColourScheme::non_null_ptr_type colour_scheme) const
{
	return d_colour;
}

