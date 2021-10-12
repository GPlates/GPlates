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

#ifndef GPLATES_MODEL_TEMPORARYGEOMETRY_H
#define GPLATES_MODEL_TEMPORARYGEOMETRY_H

#include "ReconstructionGeometry.h"


namespace GPlatesModel
{
	class TemporaryGeometry:
			public ReconstructionGeometry
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<TemporaryGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<TemporaryGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const TemporaryGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const TemporaryGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<TemporaryGeometry>.
		 */
		typedef boost::intrusive_ptr<TemporaryGeometry> maybe_null_ptr_type;

		/**
		 * Create a TemporaryGeometry instance.
		 */
		static
		const non_null_ptr_type
		create(
				geometry_ptr_type geometry_ptr)
		{
			non_null_ptr_type ptr(
					new TemporaryGeometry(geometry_ptr),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		~TemporaryGeometry()
		{  }

		/**
		 * Get a non-null pointer to a const TemporaryGeometry which points to this
		 * instance.
		 *
		 * Since the TemporaryGeometry constructors are private, it should never
		 * be the case that a TemporaryGeometry instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const;

		/**
		 * Get a non-null pointer to a TemporaryGeometry which points to this
		 * instance.
		 *
		 * Since the TemporaryGeometry constructors are private, it should never
		 * be the case that a TemporaryGeometry instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer();

		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

	private:

		/**
		 * Instantiate a temporary geometry.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		explicit
		TemporaryGeometry(
				geometry_ptr_type geometry_ptr):
			ReconstructionGeometry(geometry_ptr)
		{  }

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		TemporaryGeometry(
				const TemporaryGeometry &other);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive-pointer to another.
		TemporaryGeometry &
		operator=(
				const TemporaryGeometry &);
	};
}

#endif  // GPLATES_MODEL_TEMPORARYGEOMETRY_H
