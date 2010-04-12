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

#ifndef GPLATES_MODEL_RECONSTRUCTIONGEOMETRYFINDER_H
#define GPLATES_MODEL_RECONSTRUCTIONGEOMETRYFINDER_H

#include <vector>
#include <boost/optional.hpp>

#include "FeatureHandle.h"
#include "PropertyName.h"
#include "WeakObserverVisitor.h"
#include "ReconstructionGeometry.h"


namespace GPlatesModel
{
	class Reconstruction;
	
	/**
	 * This weak observer visitor finds all the reconstruction geometries (RGs) which
	 * are observing a given feature (eg, ReconstructedFeatureGeometry
	 * and ResolvedTopologicalBoundary).
	 *
	 * Optionally, it can limit its results to those RG instances which are contained within a
	 * particular Reconstruction, which were reconstructed from geometries with a particular
	 * property name, or both.
	 */
	class ReconstructionGeometryFinder :
			public WeakObserverVisitor<FeatureHandle>
	{
	public:
		typedef std::vector<ReconstructionGeometry::non_null_ptr_type> rg_container_type;

		/**
		 * Constructor.
		 *
		 * If a non-NULL Reconstruction pointer is supplied to the optional parameter
		 * @a reconstruction_to_match, the results will be limited to those RGs contained
		 * in that Reconstruction instance.
		 */
		explicit
		ReconstructionGeometryFinder(
				const Reconstruction *reconstruction_to_match = NULL):
			d_reconstruction_to_match(reconstruction_to_match)
		{  }

		/**
		 * Constructor.
		 *
		 * Limit the results to those RGs reconstructed from a geometry with the property
		 * name @a property_name_to_match.
		 *
		 * If a non-NULL Reconstruction pointer is supplied to the optional parameter
		 * @a reconstruction_to_match, the results will be limited to those RGs contained
		 * in that Reconstruction instance.
		 */
		explicit
		ReconstructionGeometryFinder(
				const GPlatesModel::PropertyName &property_name_to_match,
				const Reconstruction *reconstruction_to_match = NULL):
			d_property_name_to_match(property_name_to_match),
			d_reconstruction_to_match(reconstruction_to_match)
		{  }

		/**
		 * Destructor.
		 */
		virtual
		~ReconstructionGeometryFinder()
		{  }

		rg_container_type::size_type
		num_rgs_found() const
		{
			return d_found_rgs.size();
		}

		rg_container_type::const_iterator
		found_rgs_begin() const
		{
			return d_found_rgs.begin();
		}

		rg_container_type::const_iterator
		found_rgs_end() const
		{
			return d_found_rgs.end();
		}

		/**
		 * Find the RGs of the feature referenced by @a ref.
		 *
		 * If @a ref is not valid to be dereferenced, do nothing.
		 */
		void
		find_rgs_of_feature(
				FeatureHandle::weak_ref ref)
		{
			if (ref.is_valid()) {
				ref->apply_weak_observer_visitor(*this);
			}
		}

		/**
		 * Find the RGs of the feature pointed-to by @a ptr.
		 *
		 * If @a ptr is NULL, do nothing.
		 */
		void
		find_rgs_of_feature(
				FeatureHandle *ptr)
		{
			if (ptr) {
				ptr->apply_weak_observer_visitor(*this);
			}
		}

		void
		clear_found_rgs()
		{
			d_found_rgs.clear();
		}

	protected:
		virtual
		void
		visit_reconstructed_feature_geometry(
				ReconstructedFeatureGeometry &rfg);

		virtual
		void
		visit_resolved_topological_boundary(
				ResolvedTopologicalBoundary &rtb);

		virtual
		void
		visit_resolved_topological_network(
				ResolvedTopologicalNetwork &rtn);

	private:
		boost::optional<GPlatesModel::PropertyName> d_property_name_to_match;
		const Reconstruction *d_reconstruction_to_match;
		rg_container_type d_found_rgs;


		template <class ReconstructionGeometryDerivedType>
		void
		visit_reconstruction_geometry_derived_type(
				ReconstructionGeometryDerivedType &recon_geom_derived_obj);
	};
}

#endif // GPLATES_MODEL_RECONSTRUCTIONGEOMETRYFINDER_H
