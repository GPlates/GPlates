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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYFINDER_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYFINDER_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructHandle.h"
#include "ReconstructionGeometry.h"

#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "model/WeakObserverVisitor.h"


namespace GPlatesAppLogic
{
	/**
	 * This weak observer visitor finds all the reconstruction geometries (RGs) which
	 * are observing a given feature (eg, ReconstructedFeatureGeometry and ResolvedTopologicalGeometry).
	 *
	 * Optionally, it can limit its results to those RG instances which are contained within a
	 * particular reconstruction group, which were reconstructed from geometries with a particular
	 * property name, or both.
	 */
	class ReconstructionGeometryFinder :
			public GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>
	{
	public:
		typedef std::vector<ReconstructionGeometry::non_null_ptr_type> rg_container_type;

		typedef rg_container_type::const_iterator const_iterator;


		/**
		 * Constructor.
		 *
		 * If @a reconstruct_handles_to_match is specified then only matching RGs will be returned.
		 */
		explicit
		ReconstructionGeometryFinder(
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles_to_match = boost::none) :
			d_reconstruct_handles_to_match(reconstruct_handles_to_match)
		{  }

		/**
		 * Constructor.
		 *
		 * Limit the results to those RGs reconstructed from a geometry with the property
		 * name @a property_name_to_match.
		 *
		 * If @a reconstruct_handles_to_match is specified then only matching RGs will be returned.
		 */
		explicit
		ReconstructionGeometryFinder(
				const GPlatesModel::PropertyName &property_name_to_match,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles_to_match = boost::none) :
			d_property_name_to_match(property_name_to_match),
			d_reconstruct_handles_to_match(reconstruct_handles_to_match)
		{  }

		/**
		 * Constructor.
		 *
		 * Limit the result to that RG reconstructed from a geometry with the feature
		 * properties iterator @a properties_iterator_to_match.
		 *
		 * NOTE: Since @a properties_iterator_to_match can only reference a single property in
		 * a single feature, we can find at most one matching RG (so @a num_rgs_found should
		 * only return zero or one).
		 *
		 * If @a reconstruct_handles_to_match is specified then only matching RGs will be returned.
		 */
		explicit
		ReconstructionGeometryFinder(
				const GPlatesModel::FeatureHandle::iterator &properties_iterator_to_match,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles_to_match = boost::none):
			d_properties_iterator_to_match(properties_iterator_to_match),
			d_reconstruct_handles_to_match(reconstruct_handles_to_match)
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

		const_iterator
		found_rgs_begin() const
		{
			return d_found_rgs.begin();
		}

		const_iterator
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
				GPlatesModel::FeatureHandle::weak_ref ref)
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
				GPlatesModel::FeatureHandle *ptr)
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

		// Handles @a ReconstructedFeatureGeometry and its derived classes.
		virtual
		void
		visit_reconstructed_feature_geometry(
				ReconstructedFeatureGeometry &rfg);

		virtual
		void
		visit_resolved_topological_geometry(
				ResolvedTopologicalGeometry &rtg);

		virtual
		void
		visit_resolved_topological_network(
				ResolvedTopologicalNetwork &rtn);

	private:
		boost::optional<GPlatesModel::PropertyName> d_property_name_to_match;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_properties_iterator_to_match;
		boost::optional<std::vector<ReconstructHandle::type> > d_reconstruct_handles_to_match;

		rg_container_type d_found_rgs;


		template <class ReconstructionGeometryDerivedType>
		void
		visit_reconstruction_geometry_derived_type(
				ReconstructionGeometryDerivedType &recon_geom_derived_obj);
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYFINDER_H
