/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_MODEL_WEAKOBSERVERVISITOR_H
#define GPLATES_MODEL_WEAKOBSERVERVISITOR_H


namespace GPlatesAppLogic
{
	class MultiPointVectorField;
	class ReconstructedFeatureGeometry;
	class ReconstructedFlowline;
	class ReconstructedMotionPath;
	class ReconstructedSmallCircle;
	class ReconstructedVirtualGeomagneticPole;
	class ResolvedRaster;
	class ResolvedScalarField3D;
	class ResolvedTopologicalGeometry;
	class ResolvedTopologicalNetwork;
}

namespace GPlatesModel
{
	class FeatureHandle;
	template<class H> class WeakReference;

	/**
	 * This class defines an abstract interface for a Visitor to visit weak observers.
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
	template<typename H>
	class WeakObserverVisitor
	{
	public:

		virtual
		~WeakObserverVisitor()
		{
		}

		// Please keep these reconstruction geometry derivations ordered alphabetically.

#if 0
		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_reconstructed_feature_geometry(
				GPlatesAppLogic::ReconstructedFeatureGeometry &rfg)
		{  }
#endif

#if 0
		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_revision_aware_iterator(
				RevisionAwareIterator<H> &rai)
		{  }
#endif

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_weak_reference(
				WeakReference<H> &wr)
		{  }

	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		WeakObserverVisitor &
		operator=(
				const WeakObserverVisitor &);

	};


	/**
	 * This class defines an abstract interface for a Visitor to visit weak observers.
	 *
	 * This is the template specialization for weak observers of FeatureHandle, one of which is
	 * class ReconstructedFeatureGeometry.
	 */
	template<>
	class WeakObserverVisitor<FeatureHandle>
	{
	public:

		virtual
		~WeakObserverVisitor()
		{
		}

		// Please keep these reconstruction geometry derivations ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_multi_point_vector_field(
				GPlatesAppLogic::MultiPointVectorField &mpvf)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_reconstructed_feature_geometry(
				GPlatesAppLogic::ReconstructedFeatureGeometry &rfg)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_reconstructed_flowline(
				GPlatesAppLogic::ReconstructedFlowline &rf)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_reconstructed_motion_path(
				GPlatesAppLogic::ReconstructedMotionPath &rmp)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_reconstructed_small_circle(
				GPlatesAppLogic::ReconstructedSmallCircle &rsc)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_reconstructed_virtual_geomagnetic_pole(
				GPlatesAppLogic::ReconstructedVirtualGeomagneticPole &rvgp)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_resolved_raster(
				GPlatesAppLogic::ResolvedRaster &rr)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_resolved_scalar_field_3d(
				GPlatesAppLogic::ResolvedScalarField3D &rsf)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_resolved_topological_geometry(
				GPlatesAppLogic::ResolvedTopologicalGeometry &rtb)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_resolved_topological_network(
				GPlatesAppLogic::ResolvedTopologicalNetwork &rtn)
		{  }

#if 0
		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_revision_aware_iterator(
				RevisionAwareIterator<H> &rai)
		{  }
#endif

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_weak_reference(
				WeakReference<FeatureHandle> &wr)
		{  }

	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		WeakObserverVisitor &
		operator=(
				const WeakObserverVisitor &);

	};

}

#endif  // GPLATES_MODEL_WEAKOBSERVERVISITOR_H
