/* $Id$ */

/**
 * @file 
 * Contains the definition of class MultiPointVectorField.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_MULTIPOINTVECTORFIELD_H
#define GPLATES_APP_LOGIC_MULTIPOINTVECTORFIELD_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"
#include "model/FeatureHandle.h"
#include "model/WeakObserver.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/Vector3D.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	/**
	 * This class represents a 3-D vector field over a multi-point domain.
	 *
	 * A single instance of this 3-D vector field may span multiple independently-moving plates,
	 * so each 3-D vector has an optional associated plate ID.
	 *
	 * Because a 3-D vector field can be considered a mapping of a set of points to a set of
	 * 3-D vectors, the structure of this class is described using the terminology of mappings:
	 * domain, codomain and range.
	 *
	 * - The @em domain is the multi-point over which the 3-D vector field is sampled.
	 * - The @em codomain is the information which can be associated with points in the domain: a 3-D vector and an optional plate ID.
	 * - The @em range is the set of codomain elements associated with the points in the domain.
	 *
	 * Each element in the domain has a corresponding element in the range.  Thus, there are as
	 * many elements in the range as there are points in the multi-point domain.
	 *
	 * Just as it's possible to iterate through the points in a multi-point as a sequence, so
	 * is it similarly possible to iterate through the elements in the range as a sequence (of
	 * codomain elements).  The i-th element in the range will correspond to the i-th point in
	 * the multi-point domain.  Thus, geographically-speaking, the i-th element in the range is
	 * located at the position of the i-th point in the multi-point.
	 *
	 * Since the i-th element in the range must correspond to the i-th element in the domain,
	 * there cannot be gaps in the range.  However, it is possible to have "null" elements in
	 * the range, represented by boost::none.
	 */
	class MultiPointVectorField:
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		/**
		 * A convenience typedef for a non-null shared pointer to a non-const @a MultiPointVectorField.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MultiPointVectorField>
				non_null_ptr_type;

		/**
		 * A convenience typedef for a non-null shared pointer to a const @a MultiPointVectorField.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const MultiPointVectorField>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<MultiPointVectorField>.
		 */
		typedef boost::intrusive_ptr<MultiPointVectorField> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const MultiPointVectorField>.
		 */
		typedef boost::intrusive_ptr<const MultiPointVectorField> maybe_null_ptr_to_const_type;

		/**
		 * A convenience typedef for the WeakObserver base class of this class.
		 */
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;

		/**
		 * A convenience typedef for a non-null shared pointer to a non-const @a MultiPointOnSphere.
		 */
		typedef GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_ptr_type;

		/**
		 * This class represents an element of the codomain -- primarily a 3-D vector, plus
		 * some other information.
		 *
		 * An instance of this class is used to represent an element in the range, which is
		 * associated with a single element in the domain.
		 */
		struct CodomainElement
		{
			/**
			 * The set of reasons for the value of the 3-D vector.
			 *
			 * These might influence the colouring or rendering of the vector.
			 */
			enum Reason
			{
				NotInAnyBoundaryOrNetwork,
				InPlateBoundary,
				InDeformationNetwork
			};

			/**
			 * The 3-D vector.
			 */
			GPlatesMaths::Vector3D d_vector;

			/**
			 * The reason for the value of the 3-D vector.
			 *
			 * This might influence the colouring or rendering of the vector.
			 */
			Reason d_reason;

			/**
			 * An optional plate ID.
			 *
			 * The plate ID is optional in case the point does not lie inside a plate
			 * boundary, but the client code still wishes to assign a 3-D vector.
			 */
			boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;

			/**
			 * A "maybe-null" reconstruction geometry for the plate boundary that
			 * encloses the point.
			 *
			 * The reconstruction geometry is optional in case the point does not lie
			 * inside a plate boundary, but the client code still wishes to assign a
			 * 3-D vector.
			 */
			ReconstructionGeometry::maybe_null_ptr_to_const_type d_enclosing_boundary;

			/**
			 * Construct a codomain element from a 3-D vector @a v and a reason @a r.
			 *
			 * Optionally, a boost::optional plate ID @opt_p and an enclosing boundary
			 * @a opt_eb may be specified.
			 */
			CodomainElement(
					const GPlatesMaths::Vector3D &v,
					Reason r,
					const boost::optional<GPlatesModel::integer_plate_id_type> &opt_p = boost::none,
					const ReconstructionGeometry::maybe_null_ptr_to_const_type &opt_eb = NULL):
				d_vector(v),
				d_reason(r),
				d_plate_id(opt_p),
				d_enclosing_boundary(opt_eb)
			{  }

			/**
			 * Construct a codomain element from a 3-D vector @a v, a reason @a r, and
			 * a plate ID @p.
			 *
			 * Optionally, an enclosing boundary @a eb may be specified.
			 */
			CodomainElement(
					const GPlatesMaths::Vector3D &v,
					Reason r,
					const GPlatesModel::integer_plate_id_type &p,
					const ReconstructionGeometry::maybe_null_ptr_to_const_type &opt_eb = NULL):
				d_vector(v),
				d_reason(r),
				d_plate_id(p),
				d_enclosing_boundary(opt_eb)
			{  }
		};

		/**
		 * A convenience typedef for the codomain type.
		 */
		typedef std::vector< boost::optional<CodomainElement> > codomain_type;

		/**
		 * Create a MultiPointVectorField instance which is sampled over the supplied
		 * multi-point domain.
		 *
		 * The vector field will be pre-sized to the correct size, but will be empty
		 * (full of "null" elements, represented by boost::none).
		 */
		static
		const non_null_ptr_type
		create_empty(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const multi_point_ptr_type &multi_point_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_)
		{
			return non_null_ptr_type(
					new MultiPointVectorField(
							reconstruction_tree,
							multi_point_ptr,
							feature_handle,
							property_iterator_),
					GPlatesUtils::NullIntrusivePointerHandler());
		}


		virtual
		~MultiPointVectorField()
		{  }

		/**
		 * Get a non-null pointer to a const MultiPointVectorField which points to this
		 * instance.
		 *
		 * Since the MultiPointVectorField constructors are private, it should never
		 * be the case that a MultiPointVectorField instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Get a non-null pointer to a MultiPointVectorField which points to this
		 * instance.
		 *
		 * Since the MultiPointVectorField constructors are private, it should never
		 * be the case that a MultiPointVectorField instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Return whether this RFG references @a that_feature_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const GPlatesModel::FeatureHandle &that_feature_handle) const
		{
			return (feature_handle_ptr() == &that_feature_handle);
		}

		/**
		 * Return the pointer to the FeatureHandle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a
		 * FeatureHandle; non-NULL otherwise.
		 *
		 * This function will not throw.
		 */
		GPlatesModel::FeatureHandle *
		feature_handle_ptr() const
		{
			return WeakObserverType::publisher_ptr();
		}

		/**
		 * Return whether this pointer is valid to be dereferenced (to obtain a
		 * FeatureHandle).
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return (feature_handle_ptr() != NULL);
		}

		/**
		 * Return a weak-ref to the feature whose reconstructed geometry this RFG contains,
		 * or an invalid weak-ref, if this pointer is not valid to be dereferenced.
		 */
		const GPlatesModel::FeatureHandle::weak_ref
		get_feature_ref() const;

		/**
		 * Access the feature property which contained the reconstructed geometry.
		 */
		const GPlatesModel::FeatureHandle::iterator
		property() const
		{
			return d_property_iterator;
		}

		/**
		 * Access the @a MultiPointOnSphere which is the domain of the 3-D vector field.
		 */
		multi_point_ptr_type
		multi_point() const
		{
			return d_multi_point_ptr;
		}

		/**
		 * Return the number of points in the domain.
		 *
		 * Each element in the domain has a corresponding element in the range.  Thus,
		 * there are as many elements in the range as there are points in the domain.
		 */
		codomain_type::size_type
		domain_size() const
		{
			return d_range.size();
		}

		/**
		 * Return a "begin" const-iterator for the elements in the range.
		 *
		 * Just as it's possible to iterate through the points in a multi-point as a
		 * sequence, so is it similarly possible to iterate through the elements in the
		 * range as a sequence (of codomain elements).  The i-th element in the range will
		 * correspond to the i-th point in the multi-point domain.
		 *
		 * The sequence of range elements may be accessed in a bidirectional-iteration or
		 * random-access manner.  Uninitialised elements will have a value of boost::none.
		 */
		codomain_type::const_iterator
		begin() const
		{
			return d_range.begin();
		}

		/**
		 * Return an "end" const-iterator for the elements in the range.
		 */
		codomain_type::const_iterator
		end() const
		{
			return d_range.end();
		}

		/**
		 * Return a "begin" iterator for the elements in the range.
		 *
		 * Just as it's possible to iterate through the points in a multi-point as a
		 * sequence, so is it similarly possible to iterate through the elements in the
		 * range as a sequence (of codomain elements).  The i-th element in the range will
		 * correspond to the i-th point in the multi-point domain.
		 *
		 * The sequence of range elements may be accessed in a bidirectional-iteration or
		 * random-access manner.  Uninitialised elements will have a value of boost::none.
		 */
		codomain_type::iterator
		begin()
		{
			return d_range.begin();
		}

		/**
		 * Return an "end" iterator for the elements in the range.
		 */
		codomain_type::iterator
		end()
		{
			return d_range.end();
		}

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

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);

	protected:
		/**
		 * Instantiate a MultiPointVectorField which is sampled over the supplied
		 * multi-point domain.
		 *
		 * The vector field will be pre-sized to the correct size, but will be empty
		 * (full of "null" elements, represented by boost::none).
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		MultiPointVectorField(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const multi_point_ptr_type &multi_point_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_):
			ReconstructionGeometry(reconstruction_tree_),
			WeakObserverType(feature_handle),
			d_multi_point_ptr(multi_point_ptr),
			d_property_iterator(property_iterator_),
			d_range(multi_point_ptr->number_of_points())
		{  }

	private:
		/**
		 * The multi-point domain over which the 3-D vector field is sampled.
		 */
		multi_point_ptr_type d_multi_point_ptr;

		/**
		 * This is an iterator to the (multi-point-valued) property from which this MPVF
		 * was derived.
		 */
		GPlatesModel::FeatureHandle::iterator d_property_iterator;

		/**
		 * The range (a set of codomain elements) of the multi-point domain.
		 *
		 * This contains the 3-D vectors that are sampled over the multi-point domain, plus
		 * additional per-vector information such as an optional plate ID.
		 *
		 * It will be assumed that there are as many elements in the range as there are
		 * points in the multi-point domain.  It will even be assumed that the i-th element
		 * in the range is located at the position of the i-th point in the multi-point.
		 */
		codomain_type d_range;
	};
}

#endif  // GPLATES_APP_LOGIC_MULTIPOINTVECTORFIELD_H
