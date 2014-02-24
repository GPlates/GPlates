/* $Id$ */

/**
 * \file Convenience functions for @a ReconstructionGeometry.
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

#ifndef GPLATES_APPLOGIC_RECONSTRUCTIONGEOMETRYUTILS_H
#define GPLATES_APPLOGIC_RECONSTRUCTIONGEOMETRYUTILS_H

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/optional.hpp>
#include <boost/type_traits/remove_pointer.hpp>

#include "GeometryUtils.h"
#include "MultiPointVectorField.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedFlowline.h"
#include "ReconstructedMotionPath.h"
#include "ReconstructedSmallCircle.h"
#include "ReconstructedVirtualGeomagneticPole.h"
#include "ReconstructHandle.h"
#include "ReconstructionGeometry.h"
#include "ReconstructionGeometryVisitor.h"
#include "ReconstructionTree.h"
#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalGeometrySubSegment.h"
#include "ResolvedTopologicalNetwork.h"

#include "maths/PolygonOnSphere.h"
	
#include "model/FeatureVisitor.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	namespace ReconstructionGeometryUtils
	{
		///////////////
		// Interface //
		///////////////


		/**
		 * Determines if the @a ReconstructionGeometry object pointed to by
		 * @a reconstruction_geom_ptr is of type ReconstructionGeometryDerivedType or
		 * a type derived from it.
		 *
		 * Example usage:
		 *   const ReconstructionGeometry *reconstruction_geometry_ptr = ...;
		 *   boost::optional<const ReconstructedFeatureGeometry *> rfg =
		 *        get_reconstruction_geometry_derived_type<const ReconstructedFeatureGeometry>(
		 *              reconstruction_geometry_ptr);
		 *
		 * If type matches then returns pointer to derived type otherwise returns false.
		 */
		template <class ReconstructionGeometryDerivedType, typename ReconstructionGeometryPointer>
		boost::optional<ReconstructionGeometryDerivedType *>
		get_reconstruction_geometry_derived_type(
				ReconstructionGeometryPointer reconstruction_geom_ptr);


		/**
		 * Searches a sequence of @a ReconstructionGeometry objects for
		 * a certain type derived from @a ReconstructionGeometry and appends any found
		 * to @a reconstruction_geom_derived_type_seq.
		 *
		 * Template parameter 'ReconstructionGeometryForwardIter' contains pointers to
		 * @a ReconstructionGeometry objects (or anything that behaves like a pointer).
		 *
		 * NOTE: Template parameter 'ContainerOfReconstructionGeometryDerivedType' *must*
		 * be a standard container supporting the 'push_back()' method and must contain
		 * pointers to a const or non-const type derived from @a ReconstructionGeometry.
		 * It is this exact type that the caller is effectively searching for -
		 * the objects found will be of this type or a type derived from it.
		 *
		 * Returns true if any found from input sequence.
		 */
		template <typename ReconstructionGeometryForwardIter,
				class ContainerOfReconstructionGeometryDerivedType>
		bool
		get_reconstruction_geometry_derived_type_sequence(
				ReconstructionGeometryForwardIter reconstruction_geoms_begin,
				ReconstructionGeometryForwardIter reconstruction_geoms_end,
				ContainerOfReconstructionGeometryDerivedType &reconstruction_geom_derived_type_seq);


		/**
		 * Visits a @a ReconstructionGeometry to get its feature-handle reference.
		 * Returns false if derived type of reconstruction geometry has an invalid
		 * feature handle reference.
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesModel::FeatureHandle::weak_ref>
		get_feature_ref(
				ReconstructionGeometryPointer reconstruction_geom_ptr);


		/**
		 * Visits a @a ReconstructionGeometry to get a pointer to its feature handle.
		 * Returns false if derived type of reconstruction geometry has an invalid
		 * feature handle reference.
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesModel::FeatureHandle *>
		get_feature_handle_ptr(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
					get_feature_ref(reconstruction_geom_ptr);
			if (!feature_ref)
			{
				return boost::none;
			}

			return feature_ref->handle_ptr();
		}


		/**
		 * Visits a @a ReconstructionGeometry to get its geometry feature-handle property iterator.
		 * Returns false if derived type of reconstruction geometry has no property iterator or
		 * if property iterator is now invalid.
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesModel::FeatureHandle::iterator>
		get_geometry_property_iterator(
				ReconstructionGeometryPointer reconstruction_geom_ptr);


		/**
		 * Visits a @a ReconstructionGeometry to get a plate id (the plate id could be
		 * a reconstruction plate id in @a ReconstructedFeatureGeometry or a plate id
		 * of a resolved topology).
		 * Returns boost::none if derived type of reconstruction geometry has no plate id.
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 */
		template <typename ReconstructionGeometryPointer>
		const boost::optional<GPlatesModel::integer_plate_id_type>
		get_plate_id(
				ReconstructionGeometryPointer reconstruction_geom_ptr);


		/**
		 * Visits a @a ReconstructionGeometry to get the time of formation.
		 * This function is unlike the above in that it returns result in a boost::optional.
		 * Returns false if derived type of reconstruction geometry has no time of formation.
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesPropertyValues::GeoTimeInstant>
		get_time_of_formation(
				ReconstructionGeometryPointer reconstruction_geom_ptr);


		/**
		 * Visits a @a ReconstructionGeometry to get the reconstruction tree for the specified time.
		 *
		 * If @a reconstruction_time is boost::none then the reconstruction tree at the time of
		 * reconstruction of @a reconstruction_geom_ptr is returned.
		 *
		 * Returns false if derived type of reconstruction geometry has no reconstruction tree
		 * (because not all derived types use a reconstruction tree).
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 *
		 * Note that not all ReconstructionGeoemtry derived types are supported.
		 * For example, @a MultiPointVectorField do not provide a reconstruction tree.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<ReconstructionTree::non_null_ptr_to_const_type>
		get_reconstruction_tree(
				ReconstructionGeometryPointer reconstruction_geom_ptr,
				boost::optional<double> reconstruction_time = boost::none);


		/**
		 * Visits a @a ReconstructionGeometry to get the reconstruction tree creator.
		 *
		 * Returns false if derived type of reconstruction geometry has no reconstruction tree creator
		 * (because not all derived types use a reconstruction tree creator).
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 *
		 * Note that not all ReconstructionGeoemtry derived types are supported.
		 * For example, @a MultiPointVectorField do not provide a reconstruction tree creator.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<ReconstructionTreeCreator>
		get_reconstruction_tree_creator(
				ReconstructionGeometryPointer reconstruction_geom_ptr);


		/**
		 * Returns the *boundary* subsegment sequence for the specified resolved topology.
		 *
		 * @a reconstruction_geom_ptr should be either a @a ResolvedTopologicalGeometry (with a
		 * *polygon* geometry - not a polyline) or a @a ResolvedTopologicalNetwork (the network boundary).
		 * Resolved topological lines are excluded as they do not form a closed boundary.
		 *
		 * Returns boost::none if the specified reconstruction geometry is not a resolved topology.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<const sub_segment_seq_type &>
		get_resolved_topological_boundary_sub_segment_sequence(
				ReconstructionGeometryPointer reconstruction_geom_ptr);

		/**
		 * Returns the boundary polygon of the specified resolved topological geometry.
		 *
		 * @a reconstruction_geom_ptr can be either a @a ResolvedTopologicalGeometry or @a ResolvedTopologicalNetwork.
		 * However only @a ResolvedTopologicalGeometry objects containing *polylines* are ignored.
		 *
		 * Returns boost::none if the specified reconstruction geometry is not a resolved topological geometry.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
		get_resolved_topological_boundary_polygon(
				ReconstructionGeometryPointer reconstruction_geom_ptr);

		/**
		 * Returns the boundary polygon of the specified reconstruction geometry.
		 *
		 * @a reconstruction_geom_ptr can be a @a ReconstructedFeatureGeometry (or derived from it),
		 * a @a ResolvedTopologicalGeometry or a @a ResolvedTopologicalNetwork.
		 *
		 * Returns boost::none if the specified reconstruction geometry does not contain a *polygon* geometry.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
		get_boundary_polygon(
				ReconstructionGeometryPointer reconstruction_geom_ptr);


		//! Typedef for sequence of @a ReconstructionGeometry objects.
		typedef std::vector<ReconstructionGeometry::non_null_ptr_to_const_type> reconstruction_geom_seq_type;

		/**
		 * Finds the @a ReconstructionGeometry objects that were generated from the same geometry property
		 * as @a reconstruction_geometry and that were optionally reconstructed using @a reconstruction_tree
		 * and that are from the subset of reconstruction geometries in @a reconstruction_geometries_subset.
		 *
		 * Returns true if any were found.
		 *
		 * This is useful for tracking reconstruction geometries as the reconstruction time,
		 * and hence reconstruction tree, changes.
		 */
		bool
		find_reconstruction_geometries_observing_feature(
				reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
				const reconstruction_geom_seq_type &reconstruction_geometries_subset,
				const ReconstructionGeometry &reconstruction_geometry,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles = boost::none);


		/**
		 * Finds the @a ReconstructionGeometry objects from feature @a feature_ref and that were
		 * optionally reconstructed using @a reconstruction_tree and that are from the subset of
		 * reconstruction geometries in @a reconstruction_geometries_subset.
		 *
		 * Returns true if any were found.
		 *
		 * This is useful for tracking reconstruction geometries as the reconstruction time,
		 * and hence reconstruction tree, changes.
		 *
		 * This is useful when the old @a ReconstructionGeometry does not exist, for example,
		 * when the reconstruction time changes to a time that is outside the valid time range
		 * of the feature. Later the reconstruction time might change to a time that is inside
		 * a feature's valid time range and we'd like to find the @a ReconstructionGeometry
		 * but don't have the old @a ReconstructionGeometry any more - in this case we can
		 * keep track of the feature and the geometry property and supply a new reconstruction tree.
		 */
		bool
		find_reconstruction_geometries_observing_feature(
				reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
				const reconstruction_geom_seq_type &reconstruction_geometries_subset,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles = boost::none);


		/**
		 * Finds the @a ReconstructionGeometry objects that were optionally generated from the
		 * geometry property @a geometry_property_iterator in feature @a feature_ref and that were
		 * optionally reconstructed using @a reconstruction_tree and that are from the subset of
		 * reconstruction geometries in @a reconstruction_geometries_subset.
		 *
		 * Returns true if any were found.
		 */
		bool
		find_reconstruction_geometries_observing_feature(
				reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
				const reconstruction_geom_seq_type &reconstruction_geometries_subset,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator,
				boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles = boost::none);


		////////////////////
		// Implementation //
		////////////////////


		/**
		 * Template visitor class to find instances of a class derived from @a ReconstructionGeometry.
		 */
		template <class ReconstructionGeometryDerivedType>
		class ReconstructionGeometryDerivedTypeFinder :
				public ReconstructionGeometryVisitorBase<
						typename GPlatesUtils::CopyConst<
								ReconstructionGeometryDerivedType, ReconstructionGeometry>::type >
		{
		public:
			//! Typedef for base class type.
			typedef ReconstructionGeometryVisitorBase<
					typename GPlatesUtils::CopyConst<
							ReconstructionGeometryDerivedType, ReconstructionGeometry>::type > base_class_type;

			/**
			 * Convenience typedef for the template parameter which is a type derived
			 * from @a ReconstructionGeometry.
			 */
			typedef ReconstructionGeometryDerivedType reconstruction_geometry_derived_type;

			//! Convenience typedef for sequence of pointers to reconstruction geometry derived type.
			typedef std::vector<reconstruction_geometry_derived_type *> container_type;

			// Bring base class visit methods into scope of current class.
			using base_class_type::visit;

			//! Returns a sequence of reconstruction geometries of type ReconstructionGeometryDerivedType
			const container_type &
			get_geometry_type_sequence() const
			{
				return d_found_geometries;
			}

			/**
			 * Visit method for the derived @a ReconstructionGeometry type.
			 *
			 * NOTE: If 'reconstruction_geometry_derived_type' is @a ReconstructedFeatureGeometry
			 * then this will also capture types derived from @a ReconstructedFeatureGeometry due to
			 * the default implementation for these derived types in the base visitor class (the
			 * default implementation delegates to the @a ReconstructedFeatureGeometry visit method).
			 */
			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstruction_geometry_derived_type> &rg)
			{
				d_found_geometries.push_back(rg.get());
			}

		private:
			container_type d_found_geometries;
		};


		template <class ReconstructionGeometryDerivedType, typename ReconstructionGeometryPointer>
		boost::optional<ReconstructionGeometryDerivedType *>
		get_reconstruction_geometry_derived_type(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			// Type of finder class.
			typedef ReconstructionGeometryDerivedTypeFinder<ReconstructionGeometryDerivedType>
					reconstruction_geometry_derived_type_finder_type;

			reconstruction_geometry_derived_type_finder_type recon_geom_derived_type_finder;

			// Visit ReconstructionGeometry.
			reconstruction_geom_ptr->accept_visitor(recon_geom_derived_type_finder);

			// Get the sequence of any found ReconstructionGeometry derived types.
			// Can only be one as most though.
			const typename reconstruction_geometry_derived_type_finder_type::container_type &
					derived_type_seq = recon_geom_derived_type_finder.get_geometry_type_sequence();

			if (derived_type_seq.empty())
			{
				// Return false since found no derived type.
				return boost::none;
			}

			return derived_type_seq.front();
		}


		template <typename ReconstructionGeometryForwardIter,
				class ContainerOfReconstructionGeometryDerivedType>
		bool
		get_reconstruction_geometry_derived_type_sequence(
				ReconstructionGeometryForwardIter reconstruction_geoms_begin,
				ReconstructionGeometryForwardIter reconstruction_geoms_end,
				ContainerOfReconstructionGeometryDerivedType &reconstruction_geom_derived_type_seq)
		{
			// We're expecting a container of pointers so get the type being pointed at.
			typedef typename boost::remove_pointer<
					typename ContainerOfReconstructionGeometryDerivedType::value_type>::type
							recon_geom_derived_type;

			// Type of finder class.
			typedef ReconstructionGeometryDerivedTypeFinder<recon_geom_derived_type>
					reconstruction_geometry_derived_type_finder_type;

			reconstruction_geometry_derived_type_finder_type recon_geom_derived_type_finder;

			// Visit each ReconstructionGeometry in the input sequence.
			for (ReconstructionGeometryForwardIter recon_geom_iter = reconstruction_geoms_begin;
				recon_geom_iter != reconstruction_geoms_end;
				++recon_geom_iter)
			{
				(*recon_geom_iter)->accept_visitor(recon_geom_derived_type_finder);
			}

			// Get the sequence of any found ReconstructionGeometry derived types.
			const typename reconstruction_geometry_derived_type_finder_type::container_type &
					derived_type_seq = recon_geom_derived_type_finder.get_geometry_type_sequence();

			// Append to the end of the output sequence of derived types.
			std::copy(
					derived_type_seq.begin(),
					derived_type_seq.end(),
					std::back_inserter(reconstruction_geom_derived_type_seq));

			// Return true if found any derived types.
			return !derived_type_seq.empty();
		}


		class GetFeatureRef :
				public ConstReconstructionGeometryVisitor
		{
		public:
			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;

			const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &
			get_feature_ref() const
			{
				return d_feature_ref;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
			{
				// A MultiPointVectorField references both a velocity point location and
				// a plate polygon of some sort.
				// Here we just return whichever feature reference is stored in the
				// MultiPointVectorField object itself - currently this is velocity point location.
				d_feature_ref = mpvf->get_feature_ref();
			}

			// Derivations of ReconstructedFeatureGeometry default to its implementation...

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
			{
				d_feature_ref = rfg->get_feature_ref();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				d_feature_ref = rtg->get_feature_ref();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				d_feature_ref = rtn->get_feature_ref();
			}

		private:
			boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_feature_ref;
		};


		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesModel::FeatureHandle::weak_ref>
		get_feature_ref(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			GetFeatureRef get_feature_ref_visitor;
			reconstruction_geom_ptr->accept_visitor(get_feature_ref_visitor);

			const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &feature_ref_opt =
					get_feature_ref_visitor.get_feature_ref();
			if (!feature_ref_opt || !feature_ref_opt->is_valid())
			{
				return boost::none;
			}

			return feature_ref_opt;
		}


		class GetGeometryProperty :
				public ConstReconstructionGeometryVisitor
		{
		public:
			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;

			const boost::optional<GPlatesModel::FeatureHandle::iterator> &
			get_property() const
			{
				return d_property;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
			{
				// A MultiPointVectorField references both a velocity point location and
				// a plate polygon of some sort.
				// Here we just return whichever geometry property is stored in the
				// MultiPointVectorField object itself - currently this is velocity point location.
				d_property = mpvf->property();
			}

			// Derivations of ReconstructedFeatureGeometry default to its implementation...

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
			{
				d_property = rfg->property();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				d_property = rtg->property();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				d_property = rtn->property();
			}

		private:
			boost::optional<GPlatesModel::FeatureHandle::iterator> d_property;
		};


		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesModel::FeatureHandle::iterator>
		get_geometry_property_iterator(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			GetGeometryProperty get_geometry_property_visitor;
			reconstruction_geom_ptr->accept_visitor(get_geometry_property_visitor);

			const boost::optional<GPlatesModel::FeatureHandle::iterator> &property_opt =
					get_geometry_property_visitor.get_property();
			if (!property_opt || !property_opt->is_still_valid())
			{
				return boost::none;
			}

			return property_opt;
		}


		class GetPlateId :
				public ConstReconstructionGeometryVisitor
		{
		public:
			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;

			const boost::optional<GPlatesModel::integer_plate_id_type> &
			get_plate_id() const
			{
				return d_plate_id;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
			{
				// A MultiPointVectorField instance does not correspond to any
				// single plate, and hence does not contain a plate ID, so nothing
				// to do here.
			}

			// Derivations of ReconstructedFeatureGeometry default to its implementation...

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
			{
				d_plate_id = rfg->reconstruction_plate_id();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				d_plate_id = rtg->plate_id();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				d_plate_id = rtn->plate_id();
			}

		private:
			boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;
		};


		template <typename ReconstructionGeometryPointer>
		const boost::optional<GPlatesModel::integer_plate_id_type>
		get_plate_id(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			GetPlateId get_plate_id_visitor;
			reconstruction_geom_ptr->accept_visitor(get_plate_id_visitor);
			return get_plate_id_visitor.get_plate_id();
		}


		class GetTimeOfFormation :
				public ConstReconstructionGeometryVisitor
		{
		public:
			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;

			const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
			get_time_of_formation() const
			{
				return d_time_of_formation;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
			{
				// A MultiPointVectorField instance does not reference a feature,
				// and hence there is no time of formation, so nothing to do here.
			}

			// Derivations of ReconstructedFeatureGeometry default to its implementation...

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
			{
				d_time_of_formation = rfg->time_of_formation();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				d_time_of_formation = rtg->time_of_formation();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				d_time_of_formation = rtn->time_of_formation();
			}

		private:
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_formation;
		};


		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesPropertyValues::GeoTimeInstant>
		get_time_of_formation(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			GetTimeOfFormation get_time_of_formation_visitor;
			reconstruction_geom_ptr->accept_visitor(get_time_of_formation_visitor);

			return get_time_of_formation_visitor.get_time_of_formation();
		}


		class GetReconstructionTree :
				public ConstReconstructionGeometryVisitor
		{
		public:

			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;

			explicit
			GetReconstructionTree(
					boost::optional<double> reconstruction_time) :
				d_reconstruction_time(reconstruction_time)
			{ }

			const boost::optional<ReconstructionTree::non_null_ptr_to_const_type> &
			get_reconstruction_tree() const
			{
				return d_reconstruction_tree;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
			{
				// multi_point_vector_field_type does not need/support reconstruction trees.
			}

			// Derivations of ReconstructedFeatureGeometry default to its implementation...

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
			{
				d_reconstruction_tree = d_reconstruction_time
						? rfg->get_reconstruction_tree_creator().get_reconstruction_tree(d_reconstruction_time.get())
						: rfg->get_reconstruction_tree();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				d_reconstruction_tree = d_reconstruction_time
						? rtg->get_reconstruction_tree_creator().get_reconstruction_tree(d_reconstruction_time.get())
						: rtg->get_reconstruction_tree();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				// resolved_topological_network_type does not need/support reconstruction trees.
			}

		private:
			boost::optional<double> d_reconstruction_time;
			boost::optional<ReconstructionTree::non_null_ptr_to_const_type> d_reconstruction_tree;
		};


		class GetReconstructionTreeCreator :
				public ConstReconstructionGeometryVisitor
		{
		public:

			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;


			const boost::optional<ReconstructionTreeCreator> &
			get_reconstruction_tree_creator() const
			{
				return d_reconstruction_tree_creator;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
			{
				// multi_point_vector_field_type does not need/support reconstruction trees.
			}

			// Derivations of ReconstructedFeatureGeometry default to its implementation...

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
			{
				d_reconstruction_tree_creator = rfg->get_reconstruction_tree_creator();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				d_reconstruction_tree_creator = rtg->get_reconstruction_tree_creator();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				// resolved_topological_network_type does not need/support reconstruction trees.
			}

		private:
			boost::optional<ReconstructionTreeCreator> d_reconstruction_tree_creator;
		};


		template <typename ReconstructionGeometryPointer>
		boost::optional<ReconstructionTree::non_null_ptr_to_const_type>
		get_reconstruction_tree(
				ReconstructionGeometryPointer reconstruction_geom_ptr,
				boost::optional<double> reconstruction_time)
		{
			GetReconstructionTree get_reconstruction_tree_visitor(reconstruction_time);
			reconstruction_geom_ptr->accept_visitor(get_reconstruction_tree_visitor);

			return get_reconstruction_tree_visitor.get_reconstruction_tree();
		}


		template <typename ReconstructionGeometryPointer>
		boost::optional<ReconstructionTreeCreator>
		get_reconstruction_tree_creator(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			GetReconstructionTreeCreator get_reconstruction_tree_creator_visitor;
			reconstruction_geom_ptr->accept_visitor(get_reconstruction_tree_creator_visitor);

			return get_reconstruction_tree_creator_visitor.get_reconstruction_tree_creator();
		}


		class GetResolvedTopologicalBoundarySubSegmentSequence :
				public ConstReconstructionGeometryVisitor
		{
		public:
			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;

			boost::optional<const sub_segment_seq_type &>
			get_sub_segment_sequence() const
			{
				return d_sub_segment_sequence;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				// Only a resolved topological geometry with a *polygon* is a resolved topological *boundary*.
				if (rtg->resolved_topology_boundary())
				{
					d_sub_segment_sequence = rtg->get_sub_segment_sequence();
				}
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				d_sub_segment_sequence = rtn->get_boundary_sub_segment_sequence();
			}

		private:
			boost::optional<const sub_segment_seq_type &> d_sub_segment_sequence;
		};


		template <typename ReconstructionGeometryPointer>
		boost::optional<const sub_segment_seq_type &>
		get_resolved_topological_boundary_sub_segment_sequence(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			GetResolvedTopologicalBoundarySubSegmentSequence visitor;
			reconstruction_geom_ptr->accept_visitor(visitor);

			return visitor.get_sub_segment_sequence();
		}


		class GetResolvedTopologicalBoundaryPolygon :
				public ConstReconstructionGeometryVisitor
		{
		public:
			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;

			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
			get_boundary_polygon() const
			{
				return d_boundary_polygon;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				// See if the resolved topology geometry is a polygon.
				// It might be a polyline in which case boost::none is returned.
				d_boundary_polygon = rtg->resolved_topology_boundary();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				d_boundary_polygon = rtn->boundary_polygon();
			}

		private:
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_boundary_polygon;
		};


		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
		get_resolved_topological_boundary_polygon(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			GetResolvedTopologicalBoundaryPolygon visitor;
			reconstruction_geom_ptr->accept_visitor(visitor);

			return visitor.get_boundary_polygon();
		}


		class GetBoundaryPolygon :
				public ConstReconstructionGeometryVisitor
		{
		public:
			// Bring base class visit methods into scope of current class.
			using ConstReconstructionGeometryVisitor::visit;

			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
			get_boundary_polygon() const
			{
				return d_boundary_polygon;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
			{
				// See if the reconstructed feature geometry is a polygon.
				// It might be a polyline in which case boost::none is returned.
				d_boundary_polygon = GeometryUtils::get_polygon_on_sphere(*rfg->reconstructed_geometry());
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
			{
				// See if the resolved topology geometry is a polygon.
				// It might be a polyline in which case boost::none is returned.
				d_boundary_polygon = rtg->resolved_topology_boundary();
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
			{
				d_boundary_polygon = rtn->boundary_polygon();
			}

		private:
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_boundary_polygon;
		};


		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
		get_boundary_polygon(
				ReconstructionGeometryPointer reconstruction_geom_ptr)
		{
			GetBoundaryPolygon visitor;
			reconstruction_geom_ptr->accept_visitor(visitor);

			return visitor.get_boundary_polygon();
		}
	}
}

#endif // GPLATES_APPLOGIC_RECONSTRUCTIONGEOMETRYUTILS_H
