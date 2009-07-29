/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_CONSTRECONSTRUCTIONGEOMETRYVISITOR_H
#define GPLATES_MODEL_CONSTRECONSTRUCTIONGEOMETRYVISITOR_H

#include "ReconstructedFeatureGeometry.h"
#include "ResolvedTopologicalGeometry.h"
#include "TemporaryGeometry.h"


namespace GPlatesModel
{
	/**
	 * This class defines an abstract interface for a Visitor to visit reconstruction
	 * geometries.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 *  - The "visit" functions explicitly include the name of the target type in the function
	 *    name, to avoid the problem of name hiding in derived classes.  (If you declare *any*
	 *    functions named 'f' in a derived class, the declaration will hide *all* functions
	 *    named 'f' in the base class; if all the "visit" functions were simply called 'visit'
	 *    and you wanted to override *any* of them in a derived class, you would have to
	 *    override them all.)
	 */
	class ConstReconstructionGeometryVisitor
	{
	public:

		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~ConstReconstructionGeometryVisitor() = 0;

		// Please keep these reconstruction geometry derivations ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_reconstructed_feature_geometry(
				ReconstructedFeatureGeometry::non_null_ptr_to_const_type rfg)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_resolved_topological_geometry(
				ResolvedTopologicalGeometry::non_null_ptr_to_const_type rtg)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_temporary_geometry(
				TemporaryGeometry::non_null_ptr_to_const_type tg)
		{  }

	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		ConstReconstructionGeometryVisitor &
		operator=(
				const ConstReconstructionGeometryVisitor &);

	};


	inline
	ConstReconstructionGeometryVisitor::~ConstReconstructionGeometryVisitor()
	{  }

}

#endif // GPLATES_MODEL_CONSTRECONSTRUCTIONGEOMETRYVISITOR_H
