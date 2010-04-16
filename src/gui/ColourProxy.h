/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourProxy class.
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

#ifndef GPLATES_GUI_COLOURPROXY_H
#define GPLATES_GUI_COLOURPROXY_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "Colour.h"
#include "ColourFilter.h"
#include "ColourScheme.h"
#include "model/ReconstructionGeometry.h"

namespace GPlatesGui
{
	class ColourProxyImpl;
	class DeferredColourProxyImpl;
	class FixedColourProxyImpl;

	/**
	 * This class allows the colour of a ReconstructionGeometry to be determined
	 * at a later time.
	 *
	 * Use the first constructor (with ReconstructionGeometry argument) if you
	 * would like deferred colour assignment behaviour. Typically, you would use
	 * this if you need to assign a colour to a ReconstructionGeometry.
	 *
	 * If you want to use a ColourScheme but you want to modify the output from
	 * the ColourScheme, use a ColourFilter. For example, if a RenderedGeometry
	 * is coloured red by the ColourScheme, you may wish to colour velocity arrows
	 * associated with that RenderedGeometry a different shade of red.
	 *
	 * Use the second constructor (with Colour argument) if you would not like
	 * deferred colour assignment. Typically, you would use this if you are adding
	 * RenderedGeometry that are not constructed from ReconstructionGeometry, e.g.
	 * user interface elements. There is an implicit conversion from Colour to
	 * ColourProxy.
	 */
	class ColourProxy
	{
	public:

		/**
		 * Constructs a ColourProxy with deferred colour assignment.
		 * @param reconstruction_geometry_ptr A pointer to the ReconstructionGeometry
		 * object for which the colour will be determined at a later time.
		 * @param colour_filter_ptr An optional pointer to a ColourFilter that will
		 * modify the output from the ColourScheme.
		 */
		explicit
		ColourProxy(
				GPlatesModel::ReconstructionGeometry::non_null_ptr_type reconstruction_geometry_ptr,
				boost::shared_ptr<ColourFilter> colour_filter_ptr =
					boost::shared_ptr<ColourFilter>());

		/**
		 * Constructs a ColourProxy without deferred colour assignment.
		 * This constructor is intentionally not explicit to allow for conversion.
		 * @param colour The fixed colour that will be returned by get_colour()
		 */
		ColourProxy(
				const Colour &colour);

		/**
		 * Constructs a ColourProxy without deferred colour assignment.
		 * This constructor is intentionally not explicit to allow for conversion.
		 * @param colour The fixed colour that will be returned by get_colour()
		 */
		ColourProxy(
				boost::optional<Colour> colour);

		/**
		 * Get the colour of the ReconstructionGeometry using a particular ColourScheme.
		 * Always check whether the result is boost::none before dereferencing.
		 *
		 * If a colour scheme did not give a colour to the ReconstructionGeometry
		 * (i.e. returned boost::none), this method will correct the result to being
		 * Colour::get_olive(). This is because we assume that if you created a
		 * RenderedGeometry out of a ReconstructionGeometry, you want it shown on
		 * screen. On the other hand, if this ColourProxy was manually constructed
		 * with boost::none, boost::none is returned and is not corrected to
		 * Colour::get_olive().
		 */
		boost::optional<Colour>
		get_colour(
				ColourScheme::non_null_ptr_type colour_scheme) const;

	private:
		
		boost::shared_ptr<ColourProxyImpl> d_impl_ptr;
		
	};

	/**
	 * Pimpl idiom.
	 */
	class ColourProxyImpl
	{
	public:
		
		virtual
		~ColourProxyImpl()
		{
		}

		virtual
		boost::optional<Colour>
		get_colour(
				ColourScheme::non_null_ptr_type colour_scheme) const = 0;
	};

	/**
	 * The version of ColourProxy where we want deferred colour assignment.
	 */
	class DeferredColourProxyImpl :
		public ColourProxyImpl
	{
	public:

		explicit
		DeferredColourProxyImpl(
				GPlatesModel::ReconstructionGeometry::non_null_ptr_type reconstruction_geometry_ptr,
				boost::shared_ptr<ColourFilter> colour_filter_ptr);

		virtual
		boost::optional<Colour>
		get_colour(
				ColourScheme::non_null_ptr_type colour_scheme) const;

	private:

		GPlatesModel::ReconstructionGeometry::non_null_ptr_type d_reconstruction_geometry_ptr;
		boost::shared_ptr<ColourFilter> d_colour_filter_ptr;

	};

	/**
	 * The version of ColourProxy where we don't want deferred colour assignment.
	 */
	class FixedColourProxyImpl :
		public ColourProxyImpl
	{
	public:

		explicit
		FixedColourProxyImpl(
				boost::optional<Colour> colour);

		virtual
		boost::optional<Colour>
		get_colour(
				ColourScheme::non_null_ptr_type colour_scheme) const;

	private:

		boost::optional<Colour> d_colour;

	};
}

#endif  // GPLATES_GUI_COLOURPROXY_H
