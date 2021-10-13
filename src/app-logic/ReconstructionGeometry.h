/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRY_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRY_H

#include "ReconstructionTree.h"
#include "ReconstructionGeometryVisitor.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Classes derived from @a ReconstructionGeometry contain geometry that has been
	 * reconstructed to a particular geological time-instant.
	 */
	class ReconstructionGeometry :
			public GPlatesUtils::ReferenceCount<ReconstructionGeometry>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ReconstructionGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometry> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a ReconstructionGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionGeometry> non_null_ptr_to_const_type;

		//! A convenience typedef for boost::intrusive_ptr<ReconstructionGeometry>.
		typedef boost::intrusive_ptr<ReconstructionGeometry> maybe_null_ptr_type;

		//! A convenience typedef for boost::intrusive_ptr<const ReconstructionGeometry>.
		typedef boost::intrusive_ptr<const ReconstructionGeometry> maybe_null_ptr_to_const_type;


		virtual
		~ReconstructionGeometry()
		{  }


		/**
		 * Access the ReconstructionTree that was used to reconstruct this ReconstructionGeometry.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		reconstruction_tree() const
		{
			return d_reconstruction_tree;
		}

		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const = 0;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor) = 0;

	protected:
		/**
		 * Construct a ReconstructionGeometry instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes.
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		explicit
		ReconstructionGeometry(
				ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree_) :
			d_reconstruction_tree(reconstruction_tree_)
		{  }

	private:
		/**
		 * The reconstruction tree used to reconstruct us.
		 */
		ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRY_H
