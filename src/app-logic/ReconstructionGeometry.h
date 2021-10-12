/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <boost/optional.hpp>

#include "ReconstructionTree.h"
#include "ReconstructionGeometryVisitor.h"

#include "maths/GeometryOnSphere.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	// Forward declaration to avoid circularity of headers.
	class Reconstruction;
	class ReconstructionGeometryCollection;


	/**
	 * Classes derived from @a ReconstructionGeometry contain geometry that has been
	 * reconstructed to a particular geological time-instant.
	 */
	class ReconstructionGeometry :
			public GPlatesUtils::ReferenceCount<ReconstructionGeometry>
	{
	public:
		/**
		 * A convenience typedef for a shared pointer to a non-const @a ReconstructionGeometry.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometry> non_null_ptr_type;

		/**
		 * A convenience typedef for a shared pointer to a const @a ReconstructionGeometry.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionGeometry> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ReconstructionGeometry>.
		 */
		typedef boost::intrusive_ptr<ReconstructionGeometry> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const ReconstructionGeometry>.
		 */
		typedef boost::intrusive_ptr<const ReconstructionGeometry> maybe_null_ptr_to_const_type;


		virtual
		~ReconstructionGeometry()
		{  }

		/**
		 * Access the ReconstructionGeometryCollection instance which contains this ReconstructionGeometry.
		 *
		 * Note that this pointer will be NULL if this ReconstructionGeometry is not contained in
		 * a ReconstructionGeometryCollection.
		 */
		GPlatesAppLogic::ReconstructionGeometryCollection *
		reconstruction_geometry_collection() const
		{
			return d_reconstruction_geometry_collection_ptr;
		}

		/**
		 * Access the ReconstructionTree that was used to reconstruct this ReconstructionGeometry.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		reconstruction_tree() const
		{
			return d_reconstruction_tree;
		}

		/**
		 * Access the Reconstruction instance which indirectly (through a
		 * @a ReconstructionGeoemtryCollection) contains this ReconstructionGeometry.
		 *
		 * Note that this pointer will be NULL if this ReconstructionGeometry is not contained in
		 * a ReconstructionGeometryCollection AND that ReconstructionGeometryCollection is not
		 * contained in a Reconstruction.
		 */
		Reconstruction *
		reconstruction() const;

		/**
		 * Set the reconstruction geometry collection pointer.
		 *
		 * This function is intended to be invoked @em only when the ReconstructionGeometry is
		 * sitting in the vector inside the ReconstructionGeometryCollection instance, since even a
		 * copy-construction will reset the value of the reconstruction pointer back to NULL.
		 *
		 * WARNING:  This function should only be invoked by the code which is actually
		 * assigning an ReconstructionGeometry instance into (the vector inside) a
		 * ReconstructionGeometryCollection instance.
		 *
		 * NOTE: This method is const even though it modifies a data member.
		 * This is so this ReconstructionGeometry can be added to a ReconstructionGeometryCollection
		 * even if it's const.
		 *
		 * @throws PreconditionViolationError if this method has previously been called
		 * on this object.
		 */
		void
		set_collection_ptr(
				GPlatesAppLogic::ReconstructionGeometryCollection *collection_ptr) const;

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
			d_reconstruction_tree(reconstruction_tree_),
			d_reconstruction_geometry_collection_ptr(NULL)
		{  }

	private:
		/**
		 * The reconstruction tree used to reconstruct us.
		 */
		ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;

		/**
		 * This is the ReconstructionGeometryCollection instance which contains this ReconstructionGeometry.
		 *
		 * Note that we do NOT want this to be any sort of ref-counting pointer, since the
		 * ReconstructionGeometryCollection instance which contains this ReconstructionGeometry,
		 * does so using a ref-counting pointer; circularity of ref-counting pointers would
		 * lead to memory leaks.
		 *
		 * Note that this pointer may be NULL.
		 *
		 * This pointer should only @em ever point to a ReconstructionGeometryColection instance
		 * which @em does contain this ReconstructionGeometry inside its vector.  (This is the only
		 * way we can guarantee that the ReconstructionGeometryCollection instance actually exists,
		 * ie that the pointer is not a dangling pointer.)
		 */
		mutable GPlatesAppLogic::ReconstructionGeometryCollection *d_reconstruction_geometry_collection_ptr;
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRY_H
