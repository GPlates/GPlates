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

#ifndef GPLATES_MODEL_RECONSTRUCTIONGEOMETRY_H
#define GPLATES_MODEL_RECONSTRUCTIONGEOMETRY_H

#include <boost/optional.hpp>
#include "maths/GeometryOnSphere.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"


namespace GPlatesModel
{
	// Forward declaration to avoid circularity of headers.
	class Reconstruction;
	class ReconstructionGeometryVisitor;


	/**
	 * The abstract base class which stores geometries in a reconstruction.
	 */
	class ReconstructionGeometry
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const ReconstructionGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ReconstructionGeometry>.
		 */
		typedef boost::intrusive_ptr<ReconstructionGeometry> maybe_null_ptr_type;

		/**
		 * A convenience typedef for the geometry of this RFG.
		 */
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		virtual
		~ReconstructionGeometry()
		{  }

		/**
		 * Access the geometry.
		 */
		const geometry_ptr_type
		geometry() const
		{
			return d_geometry_ptr;
		}

		/**
		 * Access the Reconstruction instance which contains this RFG.
		 *
		 * This could be used, for instance, to access the ReconstructionTree which was
		 * used to reconstruct this RFG.
		 *
		 * Note that this pointer may be NULL.
		 */
		const Reconstruction *
		reconstruction() const
		{
			return d_reconstruction_ptr;
		}

		/**
		 * Access the Reconstruction instance which contains this RFG.
		 *
		 * This could be used, for instance, to access the ReconstructionTree which was
		 * used to reconstruct this RFG.
		 *
		 * Note that this pointer may be NULL.
		 */
		Reconstruction *
		reconstruction()
		{
			return d_reconstruction_ptr;
		}

		/**
		 * Set the reconstruction pointer.
		 *
		 * This function is intended to be invoked @em only when the RFG is sitting in the
		 * vector inside the Reconstruction instance, since even a copy-construction will
		 * reset the value of the reconstruction pointer back to NULL.
		 *
		 * WARNING:  This function should only be invoked by the code which is actually
		 * assigning an RFG instance into (the vector inside) a Reconstruction instance.
		 */
		void
		set_reconstruction_ptr(
				Reconstruction *reconstruction_ptr)
		{
			d_reconstruction_ptr = reconstruction_ptr;
		}

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor) = 0;

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

		/**
		 * Access the reference-count of this instance.
		 *
		 * This function is intended for use by the @a get_non_null_pointer function in
		 * derived classes.
		 */
		ref_count_type
		ref_count() const
		{
			return d_ref_count;
		}

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
				geometry_ptr_type geometry_ptr):
			d_ref_count(0),
			d_geometry_ptr(geometry_ptr),
			d_reconstruction_ptr(NULL)
		{  }

	private:

		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

		/**
		 * The geometry.
		 */
		geometry_ptr_type d_geometry_ptr;

		/**
		 * This is the Reconstruction instance which contains this RFG.
		 *
		 * Note that we do NOT want this to be any sort of ref-counting pointer, since the
		 * Reconstruction instance which contains this RFG, does so using a ref-counting
		 * pointer; circularity of ref-counting pointers would lead to memory leaks.
		 *
		 * Note that this pointer may be NULL.
		 *
		 * This pointer should only @em ever point to a Reconstruction instance which
		 * @em does contain this RFG inside its vector.  (This is the only way we can
		 * guarantee that the Reconstruction instance actually exists, ie that the pointer
		 * is not a dangling pointer.)
		 */
		Reconstruction *d_reconstruction_ptr;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		ReconstructionGeometry(
				const ReconstructionGeometry &other);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive-pointer to another.
		ReconstructionGeometry &
		operator=(
				const ReconstructionGeometry &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const ReconstructionGeometry *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const ReconstructionGeometry *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_RECONSTRUCTIONGEOMETRY_H
