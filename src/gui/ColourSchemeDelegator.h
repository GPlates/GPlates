/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#ifndef GPLATES_GUI_COLOURSCHEMEDELEGATOR_H
#define GPLATES_GUI_COLOURSCHEMEDELEGATOR_H

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include "ColourScheme.h"

namespace GPlatesModel
{
	class ReconstructionGeometry;
}

namespace GPlatesGui
{
	/**
	 * Keeps track of changing target colour schemes - allows switching
	 * of the actual colour scheme implementation without having to change
	 * reference(s) to it (just refer to @a ColourTableDelegator instead).
	 */
	class ColourSchemeDelegator :
		public ColourScheme
	{
	public:

		explicit
		ColourSchemeDelegator(
				boost::shared_ptr<ColourScheme> colour_scheme) :
			d_colour_scheme(colour_scheme)
		{
		}

		/**
		 * Change the target colour scheme.
		 * @param colour_scheme A pointer to the new colour scheme.
		 */
		void
		set_colour_scheme(
				boost::shared_ptr<ColourScheme> colour_scheme)
		{
			d_colour_scheme = colour_scheme;
		}

		/**
		 * Gets the target colour scheme.
		 */
		boost::shared_ptr<ColourScheme>
		get_colour_scheme() const
		{
			return d_colour_scheme;
		}

		/**
		 * Retrieves a colour for a @a reconstruction_geometry from the current
		 * colour scheme.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const;

	private:

		boost::shared_ptr<ColourScheme> d_colour_scheme;
	};
}

#endif // GPLATES_GUI_COLOURSCHEMEDELEGATOR_H
