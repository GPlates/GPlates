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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructHandle.h"

#include "global/PointerTraits.h"

#include "model/WeakObserverVisitor.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
	class ReconstructionTree;

	/**
	 * This weak observer visitor finds all the reconstructed feature geometries (RFGs) which
	 * are observing a given feature.
	 *
	 * Optionally, it can limit its results to those RFG instances which are contained within a
	 * particular Reconstruction, which were reconstructed from geometries with a particular
	 * property name, or both.
	 */
	class ReconstructedFeatureGeometryFinder:
			public GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>
	{
	public:
		typedef std::vector<
				GPlatesGlobal::PointerTraits<ReconstructedFeatureGeometry>::non_null_ptr_type>
						rfg_container_type;

		typedef rfg_container_type::const_iterator const_iterator;


		/**
		 * Constructor.
		 *
		 * If a ReconstructionTree is supplied to the optional parameter @a reconstruction_tree_to_match,
		 * the results will be limited to those RFGs that reference that ReconstructionTree instance.
		 */
		explicit
		ReconstructedFeatureGeometryFinder(
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles_to_match = boost::none,
				boost::optional<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree_to_match = boost::none) :
			d_reconstruct_handles_to_match(reconstruct_handles_to_match),
			d_reconstruction_tree_to_match(reconstruction_tree_to_match)
		{  }

		/**
		 * Constructor.
		 *
		 * Limit the results to those RFGs reconstructed from a geometry with the property
		 * name @a property_name_to_match.
		 *
		 * If a ReconstructionTree is supplied to the optional parameter @a reconstruction_tree_to_match,
		 * the results will be limited to those RFGs that reference that ReconstructionTree instance.
		 */
		explicit
		ReconstructedFeatureGeometryFinder(
				const GPlatesModel::PropertyName &property_name_to_match,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles_to_match = boost::none,
				boost::optional<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree_to_match = boost::none) :
			d_property_name_to_match(property_name_to_match),
			d_reconstruct_handles_to_match(reconstruct_handles_to_match),
			d_reconstruction_tree_to_match(reconstruction_tree_to_match)
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
		 * If a ReconstructionTree is supplied to the optional parameter @a reconstruction_tree_to_match,
		 * the results will be limited to those RFGs that reference that ReconstructionTree instance.
		 */
		explicit
		ReconstructedFeatureGeometryFinder(
				const GPlatesModel::FeatureHandle::iterator &properties_iterator_to_match,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles_to_match = boost::none,
				boost::optional<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree_to_match = boost::none) :
			d_properties_iterator_to_match(properties_iterator_to_match),
			d_reconstruct_handles_to_match(reconstruct_handles_to_match),
			d_reconstruction_tree_to_match(reconstruction_tree_to_match)
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

		const_iterator
		found_rfgs_begin() const
		{
			return d_found_rfgs.begin();
		}

		const_iterator
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
				GPlatesModel::FeatureHandle::weak_ref ref)
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
				GPlatesModel::FeatureHandle *ptr)
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

		virtual
		void
		visit_reconstructed_flowline(
				ReconstructedFlowline &rf);

		virtual
		void
		visit_reconstructed_motion_path(
				ReconstructedMotionPath &rmp);

		virtual
		void
		visit_reconstructed_virtual_geomagnetic_pole(
				ReconstructedVirtualGeomagneticPole &rvgp);

	private:
		boost::optional<GPlatesModel::PropertyName> d_property_name_to_match;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_properties_iterator_to_match;
		boost::optional<std::vector<ReconstructHandle::type> > d_reconstruct_handles_to_match;
		boost::optional<ReconstructionTree::non_null_ptr_to_const_type> d_reconstruction_tree_to_match;

		rfg_container_type d_found_rfgs;
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H
