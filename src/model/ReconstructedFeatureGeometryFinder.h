/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H
#define GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "WeakObserverVisitor.h"
#include "global/PointerTraits.h"

namespace GPlatesModel
{
	class ReconstructedFeatureGeometry;

	/**
	 * This weak observer visitor finds all the reconstructed feature geometries (RFGs) which
	 * are observing a given feature.
	 *
	 * Optionally, it can limit its results to those RFG instances which are contained within a
	 * particular Reconstruction, which were reconstructed from geometries with a particular
	 * property name, or both.
	 */
	class ReconstructedFeatureGeometryFinder:
			public WeakObserverVisitor<FeatureHandle>
	{
	public:
		typedef std::vector<GPlatesGlobal::PointerTraits<ReconstructedFeatureGeometry>::non_null_ptr_type> rfg_container_type;

		/**
		 * Constructor.
		 *
		 * If a non-NULL Reconstruction pointer is supplied to the optional parameter
		 * @a reconstruction_to_match, the results will be limited to those RFGs contained
		 * in that Reconstruction instance.
		 */
		explicit
		ReconstructedFeatureGeometryFinder(
				const Reconstruction *reconstruction_to_match = NULL):
			d_reconstruction_to_match(reconstruction_to_match)
		{  }

		/**
		 * Constructor.
		 *
		 * Limit the results to those RFGs reconstructed from a geometry with the property
		 * name @a property_name_to_match.
		 *
		 * If a non-NULL Reconstruction pointer is supplied to the optional parameter
		 * @a reconstruction_to_match, the results will be limited to those RFGs contained
		 * in that Reconstruction instance.
		 */
		explicit
		ReconstructedFeatureGeometryFinder(
				const GPlatesModel::PropertyName &property_name_to_match,
				const Reconstruction *reconstruction_to_match = NULL):
			d_property_name_to_match(property_name_to_match),
			d_reconstruction_to_match(reconstruction_to_match)
		{  }

		/**
		 * Constructor.
		 *
		 * Limit the result to that RFG reconstructed from a geometry with the feature
		 * properties iterator @a properties_iterator_to_match.
		 *
		 * NOTE: Since @a properties_iterator_to_match can only reference a single property in
		 * a single feature, we can find at most one matching RFG (so @a num_rfgs_found should
		 * only return zero or one).
		 *
		 * If a non-NULL Reconstruction pointer is supplied to the optional parameter
		 * @a reconstruction_to_match, the results will be limited to those RFGs contained
		 * in that Reconstruction instance.
		 */
		explicit
		ReconstructedFeatureGeometryFinder(
				const GPlatesModel::FeatureHandle::iterator &properties_iterator_to_match,
				const Reconstruction *reconstruction_to_match = NULL):
			d_properties_iterator_to_match(properties_iterator_to_match),
			d_reconstruction_to_match(reconstruction_to_match)
		{  }

		/**
		 * Destructor.
		 */
		virtual
		~ReconstructedFeatureGeometryFinder()
		{  }

		rfg_container_type::size_type
		num_rfgs_found() const
		{
			return d_found_rfgs.size();
		}

		rfg_container_type::const_iterator
		found_rfgs_begin() const
		{
			return d_found_rfgs.begin();
		}

		rfg_container_type::const_iterator
		found_rfgs_end() const
		{
			return d_found_rfgs.end();
		}

		/**
		 * Find the RFGs of the feature referenced by @a ref.
		 *
		 * If @a ref is not valid to be dereferenced, do nothing.
		 */
		void
		find_rfgs_of_feature(
				FeatureHandle::weak_ref ref)
		{
			if (ref.is_valid()) {
				ref->apply_weak_observer_visitor(*this);
			}
		}

		/**
		 * Find the RFGs of the feature pointed-to by @a ptr.
		 *
		 * If @a ptr is NULL, do nothing.
		 */
		void
		find_rfgs_of_feature(
				FeatureHandle *ptr)
		{
			if (ptr) {
				ptr->apply_weak_observer_visitor(*this);
			}
		}

		void
		clear_found_rfgs()
		{
			d_found_rfgs.clear();
		}

	protected:
		virtual
		void
		visit_reconstructed_feature_geometry(
				ReconstructedFeatureGeometry &rfg);

	private:
		boost::optional<GPlatesModel::PropertyName> d_property_name_to_match;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_properties_iterator_to_match;
		const Reconstruction *d_reconstruction_to_match;
		rfg_container_type d_found_rfgs;
	};
}

#endif  // GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H
