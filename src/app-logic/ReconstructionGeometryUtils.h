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

#include <iterator>
#include <vector>
#include <boost/optional.hpp>
#include <boost/type_traits/remove_pointer.hpp>
	
#include "model/ReconstructionGeometry.h"
#include "model/ConstReconstructionGeometryVisitor.h"
#include "model/ReconstructionGeometryVisitor.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/ResolvedTopologicalBoundary.h"
#include "model/ResolvedTopologicalNetwork.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesModel
{
	class Reconstruction;
}

namespace GPlatesAppLogic
{
	namespace ReconstructionGeometryUtils
	{
		///////////////
		// Interface //
		///////////////


		/**
		 * Determines if the @a ReconstructionGeometry object pointed to by
		 * ReconstructionGeometryPointer is of type ReconstructionGeometryDerivedType.
		 *
		 * If type matches then returns true and stores pointer to derived type
		 * in @a reconstruction_geom_derived_type_ptr.
		 */
		template <typename ReconstructionGeometryPointer,
				class ReconstructionGeometryDerivedType>
		bool
		get_reconstruction_geometry_derived_type(
				ReconstructionGeometryPointer reconstruction_geom_ptr,
				ReconstructionGeometryDerivedType *&reconstruction_geom_derived_type_ptr);


		/**
		 * Searches a sequence of @a ReconstructionGeometry objects for
		 * a certain type derived from @a ReconstructionGeometry and returns any found.
		 *
		 * Template parameter 'ReconstructionGeometryForwardIter' contains pointers to
		 * @a ReconstructionGeometry objects (or anything that behaves like a pointer).
		 *
		 * NOTE: Template parameter 'ContainerOfReconstructionGeometryDerivedType' *must*
		 * be a standard container supporting the 'push_back()' method and must contain
		 * pointers to a const or non-const type derived from @a ReconstructionGeometry.
		 * It is this exact type that the caller is effectively searching for.
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
		bool
		get_feature_ref(
				ReconstructionGeometryPointer reconstruction_geom_ptr,
				GPlatesModel::FeatureHandle::weak_ref &feature_ref);


		/**
		 * Visits a @a ReconstructionGeometry to get its geometry feature-handle property iterator.
		 * Returns false if derived type of reconstruction geometry has no property iterator or
		 * if property iterator is now invalid.
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 */
		template <typename ReconstructionGeometryPointer>
		bool
		get_geometry_property_iterator(
				ReconstructionGeometryPointer reconstruction_geom_ptr,
				GPlatesModel::FeatureHandle::children_iterator &properties_iterator);


		/**
		 * Visits a @a ReconstructionGeometry to get a plate id (the plate id could be
		 * a reconstruction plate id in @a ReconstructedFeatureGeometry or a plate id
		 * to assign to other features in @a ResolvedTopologicalBoundary).
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
		 * Returns false if derived type of reconstruction geometry has time of formation.
		 * NOTE: @a reconstruction_geom_ptr can be anything that acts like a const or
		 * non-const pointer to a @a ReconstructionGeometry.
		 */
		template <typename ReconstructionGeometryPointer>
		boost::optional<GPlatesPropertyValues::GeoTimeInstant>
		get_time_of_formation(
				ReconstructionGeometryPointer reconstruction_geom_ptr);


		/**
		 * Simply adds @a recon_geom to the list of geometries in @a reconstruction and
		 * sets the @a Reconstruction pointer in @a recon_geom to @a reconstruction.
		 */
		void
		add_reconstruction_geometry_to_reconstruction(
				GPlatesModel::ReconstructionGeometry::non_null_ptr_type recon_geom,
				GPlatesModel::Reconstruction *reconstruction);


		////////////////////
		// Implementation //
		////////////////////


		// Declare template class ReconstructionGeometryDerivedTypeFinder but never define it.
		// We will rely on specialisations of this class for derived type of ReconstructionGeometry.
		template <class ReconstructionGeometryDerivedType>
		class ReconstructionGeometryDerivedTypeFinder;

		// This class and its specialisation for 'const' derived types is used
		// to determine which non-null intrusive pointer to use.
		template <class ReconstructionGeometryDerivedType>
		struct ReconstructionGeometryDerivedTypeTraits
		{
			typedef typename ReconstructionGeometryDerivedType::non_null_ptr_type non_null_ptr_type;
		};


		// Macro to declare a template specialisation of class ReconstructionGeometryDerivedTypeFinder.
		// NOTE: We wouldn't need to do this if all "visit" methods were called 'visit' instead
		// of 'visit_reconstructed_feature_geometry' for example - in which case a simple template
		// class would suffice.
		// However having "visit" methods named as they are probably help readability and avoids needing
		// 'using ConstReconstructionGeometryVisitor::visit;" declarations in derived visitor classes.
		//
#define DECLARE_RECONSTRUCTION_GEOMETRY_DERIVED_TYPE_FINDER_CLASS( \
		reconstruction_geometry_derived_type, \
		reconstruction_geometry_visitor_type, \
		visit_reconstruction_geometry_derived_type_method \
) \
		template <> \
		class ReconstructionGeometryDerivedTypeFinder<reconstruction_geometry_derived_type> : \
				public reconstruction_geometry_visitor_type \
		{ \
		public: \
			typedef ReconstructionGeometryDerivedTypeTraits<reconstruction_geometry_derived_type> \
					::non_null_ptr_type non_null_intrusive_ptr_type; \
		\
			typedef std::vector<reconstruction_geometry_derived_type *> container_type; \
		\
			const container_type & \
			get_geometry_type_sequence() const \
			{ \
				return d_found_geometries; \
			} \
		\
			virtual \
			void \
			visit_reconstruction_geometry_derived_type_method( \
					non_null_intrusive_ptr_type geometry) \
			{ \
				d_found_geometries.push_back(geometry.get()); \
			} \
		\
		private: \
			container_type d_found_geometries; \
		};


		// First parameter should be the namespace qualified class derived from
		// GPlatesModel::ReconstructionGeometry.
		// Second parameter should be the name of the visitor method that visits the
		// derived class.
		// For example:
		//     DECLARE_RECONSTRUCTION_GEOMETRY_DERIVED_TYPE_FINDER(GPlatesModel::ReconstructedFeatureGeometry,
		//        visit_reconstructed_feature_geometry)
		//
#define DECLARE_RECONSTRUCTION_GEOMETRY_DERIVED_TYPE_FINDER( \
				reconstruction_geometry_derived_type, \
				visit_reconstruction_geometry_derived_type_method \
		) \
		\
		/* Helps determine which non-null intrusive pointer to use based on 'const' */ \
		template <> \
		struct ReconstructionGeometryDerivedTypeTraits<const reconstruction_geometry_derived_type> \
		{ \
			typedef reconstruction_geometry_derived_type::non_null_ptr_to_const_type non_null_ptr_type; \
		}; \
		\
		/* const and non-const ReconstructionGeometry's for const derived types */ \
		DECLARE_RECONSTRUCTION_GEOMETRY_DERIVED_TYPE_FINDER_CLASS( \
				const reconstruction_geometry_derived_type, \
				GPlatesModel::ConstReconstructionGeometryVisitor, \
				visit_reconstruction_geometry_derived_type_method) \
		/* non-const ReconstructionGeometry's for non-const derived types */ \
		DECLARE_RECONSTRUCTION_GEOMETRY_DERIVED_TYPE_FINDER_CLASS( \
				reconstruction_geometry_derived_type, \
				GPlatesModel::ReconstructionGeometryVisitor, \
				visit_reconstruction_geometry_derived_type_method)

		//
		// NOTE: declared here instead of top of respective files because we need to include
		// the respective header files because of the 'non_null_ptr_type' typedefs.
		//

DECLARE_RECONSTRUCTION_GEOMETRY_DERIVED_TYPE_FINDER(GPlatesModel::ReconstructedFeatureGeometry, \
		visit_reconstructed_feature_geometry)

DECLARE_RECONSTRUCTION_GEOMETRY_DERIVED_TYPE_FINDER(GPlatesModel::ResolvedTopologicalBoundary, \
		visit_resolved_topological_boundary)

DECLARE_RECONSTRUCTION_GEOMETRY_DERIVED_TYPE_FINDER(GPlatesModel::ResolvedTopologicalNetwork, \
		visit_resolved_topological_network)


		template <typename ReconstructionGeometryPointer,
				class ReconstructionGeometryDerivedType>
		bool
		get_reconstruction_geometry_derived_type(
				ReconstructionGeometryPointer reconstruction_geom_ptr,
				ReconstructionGeometryDerivedType *&reconstruction_geom_derived_type_ptr)
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
				return false;
			}

			reconstruction_geom_derived_type_ptr = derived_type_seq.front();

			return true;
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
			ReconstructionGeometryForwardIter recon_geom_iter;
			for (recon_geom_iter = reconstruction_geoms_begin;
				recon_geom_iter != reconstruction_geoms_end;
				++recon_geom_iter)
			{
				(*recon_geom_iter)->accept_visitor(recon_geom_derived_type_finder);
			}

			// Get the sequence of any found ReconstructionGeometry derived types.
			const typename reconstruction_geometry_derived_type_finder_type::container_type &
					derived_type_seq = recon_geom_derived_type_finder.get_geometry_type_sequence();

			// Going to insert at back of the output sequence of derived types.
			std::back_insert_iterator<ContainerOfReconstructionGeometryDerivedType> back_insert_iter =
					std::back_inserter(reconstruction_geom_derived_type_seq);

			// Insert at the end of the output sequence of derived types.
			typedef typename reconstruction_geometry_derived_type_finder_type::
					container_type::const_iterator derived_type_seq_iterator_type;
			derived_type_seq_iterator_type derived_seq_iter;
			for (derived_seq_iter = derived_type_seq.begin();
				derived_seq_iter != derived_type_seq.end();
				++derived_seq_iter)
			{
				*back_insert_iter++ = *derived_seq_iter;
			}

			// Return true if found any derived types.
			return !derived_type_seq.empty();
		}


		class GetFeatureRef :
				public GPlatesModel::ConstReconstructionGeometryVisitor
		{
		public:
			const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &
			get_feature_ref() const
			{
				return d_feature_ref;
			}

			virtual
			void
			visit_reconstructed_feature_geometry(
					GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_to_const_type rfg)
			{
				d_feature_ref = rfg->get_feature_ref();
			}

			virtual
			void
			visit_resolved_topological_boundary(
					GPlatesModel::ResolvedTopologicalBoundary::non_null_ptr_to_const_type rtb)
			{
				d_feature_ref = rtb->get_feature_ref();
			}

			virtual
			void
			visit_resolved_topological_network(
					GPlatesModel::ResolvedTopologicalNetwork::non_null_ptr_to_const_type rtn)
			{
				d_feature_ref = rtn->get_feature_ref();
			}

		private:
			boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_feature_ref;
		};


		template <typename ReconstructionGeometryPointer>
		bool
		get_feature_ref(
				ReconstructionGeometryPointer reconstruction_geom_ptr,
				GPlatesModel::FeatureHandle::weak_ref &feature_ref)
		{
			GetFeatureRef get_feature_ref_visitor;
			reconstruction_geom_ptr->accept_visitor(get_feature_ref_visitor);

			const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &feature_ref_opt =
					get_feature_ref_visitor.get_feature_ref();
			if (!feature_ref_opt || !feature_ref_opt->is_valid())
			{
				return false;
			}

			feature_ref = *feature_ref_opt;

			return true;
		}


		class GetGeometryProperty :
				public GPlatesModel::ConstReconstructionGeometryVisitor
		{
		public:
			const boost::optional<GPlatesModel::FeatureHandle::children_iterator> &
			get_property() const
			{
				return d_property;
			}

			virtual
			void
			visit_reconstructed_feature_geometry(
					GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_to_const_type rfg)
			{
				d_property = rfg->property();
			}

			virtual
			void
			visit_resolved_topological_boundary(
					GPlatesModel::ResolvedTopologicalBoundary::non_null_ptr_to_const_type rtb)
			{
				d_property = rtb->property();
			}

			virtual
			void
			visit_resolved_topological_network(
					GPlatesModel::ResolvedTopologicalNetwork::non_null_ptr_to_const_type rtn)
			{
				d_property = rtn->property();
			}

		private:
			boost::optional<GPlatesModel::FeatureHandle::children_iterator> d_property;
		};


		template <typename ReconstructionGeometryPointer>
		bool
		get_geometry_property_iterator(
				ReconstructionGeometryPointer reconstruction_geom_ptr,
				GPlatesModel::FeatureHandle::children_iterator &properties_iterator)
		{
			GetGeometryProperty get_geometry_property_visitor;
			reconstruction_geom_ptr->accept_visitor(get_geometry_property_visitor);

			const boost::optional<GPlatesModel::FeatureHandle::children_iterator> &property_opt =
					get_geometry_property_visitor.get_property();
			if (!property_opt || !property_opt->is_valid())
			{
				return false;
			}

			properties_iterator = *property_opt;

			return true;
		}


		class GetPlateId :
				public GPlatesModel::ConstReconstructionGeometryVisitor
		{
		public:
			const boost::optional<GPlatesModel::integer_plate_id_type> &
			get_plate_id() const
			{
				return d_plate_id;
			}

			virtual
			void
			visit_reconstructed_feature_geometry(
					GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_to_const_type rfg)
			{
				d_plate_id = rfg->reconstruction_plate_id();
			}

			virtual
			void
			visit_resolved_topological_boundary(
					GPlatesModel::ResolvedTopologicalBoundary::non_null_ptr_to_const_type rtb)
			{
				d_plate_id = rtb->plate_id();
			}

			virtual
			void
			visit_resolved_topological_network(
					GPlatesModel::ResolvedTopologicalNetwork::non_null_ptr_to_const_type rtn)
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
			return boost::optional<GPlatesModel::integer_plate_id_type>(
					get_plate_id_visitor.get_plate_id());
		}


		class GetTimeOfFormation :
				public GPlatesModel::ConstReconstructionGeometryVisitor
		{
		public:
			const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
			get_time_of_formation() const
			{
				return d_time_of_formation;
			}

			virtual
			void
			visit_reconstructed_feature_geometry(
					GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_to_const_type rfg)
			{
				d_time_of_formation = rfg->time_of_formation();
			}

			virtual
			void
			visit_resolved_topological_boundary(
					GPlatesModel::ResolvedTopologicalBoundary::non_null_ptr_to_const_type rtb)
			{
				d_time_of_formation = rtb->time_of_formation();
			}

			virtual
			void
			visit_resolved_topological_network(
					GPlatesModel::ResolvedTopologicalNetwork::non_null_ptr_to_const_type rtn)
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
	}
}

#endif // GPLATES_APPLOGIC_RECONSTRUCTIONGEOMETRYUTILS_H
