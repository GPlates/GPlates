/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYRECONSTRUCTIONGEOMETRIES_H
#define GPLATES_API_PYRECONSTRUCTIONGEOMETRIES_H

#include <boost/any.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalGeometrySubSegment.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTopologicalSection.h"
#include "app-logic/ResolvedTopologicalSharedSubSegment.h"

#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Utility class used by the reconstruction geometry wrappers to keep their referenced feature/property alive.
	 */
	class KeepReconstructionGeometryFeatureAndPropertyAlive
	{
	public:
		explicit
		KeepReconstructionGeometryFeatureAndPropertyAlive(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry);

	private:
		/**
		 * Keep the feature alive (by using intrusive pointer instead of weak ref)
		 * since derived reconstruction geometry types usually only store a weak reference.
		 */
		boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> d_feature;

		/**
		 * Keep the geometry feature property alive (by using intrusive pointer instead of weak ref)
		 * since derived reconstruction geometry types usually only store an iterator and
		 * someone could remove the feature property in the meantime.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> d_property;
	};


	/**
	 * A Python wrapper around a derived @a ReconstructionGeometry that keeps the referenced feature
	 * (and referenced property) alive.
	 *
	 * Keeping the referenced feature alive (and the referenced property alive in case subsequently removed
	 * from feature) is important because most unwrapped 'ReconstructionGeometryType' types
	 * store only weak references which will be invalid if the referenced features are no longer
	 * used (kept alive) in the user's python code.
	 *
	 * This is the wrapper type that gets stored in the python object.
	 *
	 * This is exposed in a header file because other Python-wrapped objects might store a
	 * reconstruction geometry internally and hence wrapped versions of those objects will need
	 * to store a 'ReconstructionGeometryTypeWrapper<>' to ensure the internal reconstruction
	 * geometry keeps references to its feature/property alive.
	 *
	 * NOTE: If this wrapper does not suit all derived reconstruction geometry types then it can be
	 * specialised for those types that need it.
	 */
	template <class ReconstructionGeometryType>
	class ReconstructionGeometryTypeWrapper
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
	    typedef ReconstructionGeometryType element_type;

		explicit
		ReconstructionGeometryTypeWrapper(
				typename ReconstructionGeometryType::non_null_ptr_to_const_type reconstruction_geometry_type) :
			d_reconstruction_geometry_type(
					// We wrap *non-const* reconstruction geometries and hence cast away const
					// (which is dangerous since Python user could modify)...
					GPlatesUtils::const_pointer_cast<ReconstructionGeometryType>(reconstruction_geometry_type)),
			d_keep_feature_property_alive(*reconstruction_geometry_type)
		{  }

		/**
		 * Get the wrapped reconstruction geometry type.
		 */
		typename ReconstructionGeometryType::non_null_ptr_type
		get_reconstruction_geometry_type() const
		{
			return d_reconstruction_geometry_type;
		}

	private:

		//! The wrapped reconstruction geometry type itself.
		typename ReconstructionGeometryType::non_null_ptr_type d_reconstruction_geometry_type;

		//! Keep the feature/property alive.
		KeepReconstructionGeometryFeatureAndPropertyAlive d_keep_feature_property_alive;
	};


	/**
	 * Boost-python requires 'get_pointer(HeldType)' for wrapped types ('HeldType') that
	 * are not already smart pointers.
	 */
	template <class ReconstructionGeometryType>
	ReconstructionGeometryType *
	get_pointer(
			const ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> &wrapper)
	{
		return wrapper.get_reconstruction_geometry_type().get();
	}


	/**
	 * Specialise class 'ReconstructionGeometryTypeWrapper' for the base ReconstructionGeometry class.
	 *
	 * This is for those cases where a 'ReconstructionGeometry::non_null_ptr_type' is wrapped into
	 * a Python object (rather than the derived class 'non_null_ptr_type').
	 *
	 * In this case we have no feature (or property) to keep alive (like in the derived
	 * ReconstructionGeometry classes) but we still need to ensure that it is wrapped as if was the
	 * actual derived ReconstructionGeometry object. This is necessary because these derived objects
	 * need to keep their referenced feature (and property) alive.
	 */
	template <>
	class ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
	    typedef GPlatesAppLogic::ReconstructionGeometry element_type;

		explicit
		ReconstructionGeometryTypeWrapper(
				GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry);

		/**
		 * Get the wrapped reconstruction geometry type.
		 */
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type
		get_reconstruction_geometry_type() const
		{
			return d_reconstruction_geometry;
		}

		/**
		 * Get the wrapped reconstruction geometry type.
		 *
		 * Returns none if the wrapper reconstruction geometry type is not 'ReconstructionGeometryType'.
		 */
		template <class ReconstructionGeometryType>
		boost::optional<const ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> &>
		get_reconstruction_geometry_type_wrapper() const
		{
			// See if requested wrapper type.
			const ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> *wrapper =
					boost::any_cast< ReconstructionGeometryTypeWrapper<ReconstructionGeometryType> >(
							&d_reconstruction_geometry_type_wrapper);
			if (!wrapper)
			{
				return boost::none;
			}

			return *wrapper;
		}

	private:

		static
		boost::any
		create_reconstruction_geometry_type_wrapper(
				const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry);


		//! The wrapped reconstruction geometry itself.
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type d_reconstruction_geometry;

		//! The derived reconstruction geometry wrapper (keeps feature/property alive).
		boost::any d_reconstruction_geometry_type_wrapper;

	};


	/**
	 * A Python wrapper around a derived @a ResolvedTopologicalGeometrySubSegment that contains
	 * a reconstruction geometry (which must be wrapped in order to keep its feature/property alive).
	 */
	class ResolvedTopologicalGeometrySubSegmentWrapper
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
		typedef GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment element_type;

		explicit
		ResolvedTopologicalGeometrySubSegmentWrapper(
				const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &resolved_topological_geometry_sub_segment) :
			d_resolved_topological_geometry_sub_segment(resolved_topological_geometry_sub_segment),
			d_reconstruction_geometry(resolved_topological_geometry_sub_segment.get_reconstruction_geometry())
		{  }

		/**
		 * Get the sub-segment.
		 */
		const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &
		get_resolved_topological_geometry_sub_segment() const
		{
			return d_resolved_topological_geometry_sub_segment;
		}

		/**
		 * Get the reconstruction geometry of the sub-segment (for passing to Python).
		 */
		const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &
		get_reconstruction_geometry() const
		{
			return d_reconstruction_geometry;
		}

		/**
		 * Clones the feature returned by 'get_topological_section_feature()' and sets the geometry to
		 * the resolved sub-segment.
		 *
		 * The feature reference could be invalid, in which case we cannot clone it.
		 * It should normally be valid though so we don't document that Py_None could be returned to the caller.
		 */
		boost::python::object
		get_resolved_feature() const;

	private:

		//! The wrapped sub-segment itself.
		GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment d_resolved_topological_geometry_sub_segment;

		/**
		 * The reconstruction geometry that the sub-segment was obtained from.
		 *
		 * We need to store a Python-wrapped version of it to keep its feature/property alive.
		 */
		ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> d_reconstruction_geometry;

		/**
		 * A feature containing the resolved (not present day) sub-segment geometry.
		 *
		 * NOTE: It is only created when first requested by @a get_resolved_feature.
		 */
		mutable boost::python::object d_resolved_feature_object;
	};


	/**
	 * Boost-python requires 'get_pointer(HeldType)' for wrapped types ('HeldType') that
	 * are not already smart pointers.
	 */
	GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment *
	get_pointer(
			const ResolvedTopologicalGeometrySubSegmentWrapper &wrapper)
	{
		// Boost-python wants a non-const return pointer (but wants a const wrapper).
		return const_cast<GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment *>(
				&wrapper.get_resolved_topological_geometry_sub_segment());
	}


	/**
	 * Specialise class 'ReconstructionGeometryTypeWrapper' for the ResolvedTopologicalLine.
	 *
	 * We need to also keep the features/properties referenced by its sub-segments alive.
	 */
	template <>
	class ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalLine>
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
	    typedef GPlatesAppLogic::ResolvedTopologicalLine element_type;

		explicit
		ReconstructionGeometryTypeWrapper(
				GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_to_const_type resolved_topological_line);

		/**
		 * Get the wrapped reconstruction geometry type.
		 */
		GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type
		get_reconstruction_geometry_type() const
		{
			return d_resolved_topological_line;
		}

		/**
		 * Clones the feature returned by 'get_feature()' and sets the geometry to the resolved line geometry.
		 *
		 * The feature reference could be invalid, in which case we cannot clone it.
		 * It should normally be valid though so we don't document that Py_None could be returned to the caller.
		 */
		boost::python::object
		get_resolved_feature() const;

	private:

		//! The wrapped reconstruction geometry type itself.
		GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type d_resolved_topological_line;

		//! Keep the feature/property alive.
		KeepReconstructionGeometryFeatureAndPropertyAlive d_keep_feature_property_alive;

		/**
		 * The sub-segments.
		 *
		 * We need to store Python-wrapped versions to keep their feature/property alive.
		 */
		std::vector<ResolvedTopologicalGeometrySubSegmentWrapper> d_sub_segments;

		/**
		 * A feature containing the resolved (not present day) line geometry.
		 *
		 * NOTE: It is only created when first requested by @a get_resolved_feature.
		 */
		mutable boost::python::object d_resolved_feature_object;
	};


	/**
	 * Specialise class 'ReconstructionGeometryTypeWrapper' for the ResolvedTopologicalBoundary.
	 *
	 * We need to also keep the features/properties referenced by its sub-segments alive.
	 */
	template <>
	class ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalBoundary>
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
	    typedef GPlatesAppLogic::ResolvedTopologicalBoundary element_type;

		explicit
		ReconstructionGeometryTypeWrapper(
				GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type resolved_topological_boundary);

		/**
		 * Get the wrapped reconstruction geometry type.
		 */
		GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type
		get_reconstruction_geometry_type() const
		{
			return d_resolved_topological_boundary;
		}

		/**
		 * Clones the feature returned by 'get_feature()' and sets the geometry to the resolved boundary geometry.
		 *
		 * The feature reference could be invalid, in which case we cannot clone it.
		 * It should normally be valid though so we don't document that Py_None could be returned to the caller.
		 */
		boost::python::object
		get_resolved_feature() const;

	private:

		//! The wrapped reconstruction geometry type itself.
		GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type d_resolved_topological_boundary;

		//! Keep the feature/property alive.
		KeepReconstructionGeometryFeatureAndPropertyAlive d_keep_feature_property_alive;

		/**
		 * The sub-segments.
		 *
		 * We need to store Python-wrapped versions to keep their feature/property alive.
		 */
		std::vector<ResolvedTopologicalGeometrySubSegmentWrapper> d_sub_segments;

		/**
		 * A feature containing the resolved (not present day) line geometry.
		 *
		 * NOTE: It is only created when first requested by @a get_resolved_feature.
		 */
		mutable boost::python::object d_resolved_feature_object;
	};


	/**
	 * Specialise class 'ReconstructionGeometryTypeWrapper' for the ResolvedTopologicalNetwork.
	 *
	 * We need to also keep the features/properties referenced by its sub-segments alive.
	 */
	template <>
	class ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ResolvedTopologicalNetwork>
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
	    typedef GPlatesAppLogic::ResolvedTopologicalNetwork element_type;

		explicit
		ReconstructionGeometryTypeWrapper(
				GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_topological_network);

		/**
		 * Get the wrapped reconstruction geometry type.
		 */
		GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type
		get_reconstruction_geometry_type() const
		{
			return d_resolved_topological_network;
		}

		/**
		 * Clones the feature returned by 'get_feature()' and sets the geometry to the resolved boundary geometry.
		 *
		 * The feature reference could be invalid, in which case we cannot clone it.
		 * It should normally be valid though so we don't document that Py_None could be returned to the caller.
		 */
		boost::python::object
		get_resolved_feature() const;

	private:

		//! The wrapped reconstruction geometry type itself.
		GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type d_resolved_topological_network;

		//! Keep the feature/property alive.
		KeepReconstructionGeometryFeatureAndPropertyAlive d_keep_feature_property_alive;

		/**
		 * The boundary sub-segments.
		 *
		 * We need to store Python-wrapped versions to keep their feature/property alive.
		 */
		std::vector<ResolvedTopologicalGeometrySubSegmentWrapper> d_boundary_sub_segments;

		/**
		 * A feature containing the resolved (not present day) line geometry.
		 *
		 * NOTE: It is only created when first requested by @a get_resolved_feature.
		 */
		mutable boost::python::object d_resolved_feature_object;
	};


	/**
	 * A Python wrapper around a derived @a ResolvedTopologicalSharedSubSegment that contains
	 * a section reconstruction geometry and a sequence of resolved topologies (all which must be
	 * wrapped in order to keep their feature/property alive).
	 */
	class ResolvedTopologicalSharedSubSegmentWrapper
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
		typedef GPlatesAppLogic::ResolvedTopologicalSharedSubSegment element_type;

		explicit
		ResolvedTopologicalSharedSubSegmentWrapper(
				const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &resolved_topological_shared_sub_segment);

		/**
		 * Get the shared sub-segment.
		 */
		const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment &
		get_resolved_topological_shared_sub_segment() const
		{
			return d_resolved_topological_shared_sub_segment;
		}

		/**
		 * Get the reconstruction geometry of the sub-segment (for passing to Python).
		 */
		const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &
		get_reconstruction_geometry() const
		{
			return d_reconstruction_geometry;
		}

		/**
		 * Clones the feature returned by 'get_topological_section_feature()' and sets the geometry to
		 * the resolved sub-segment.
		 *
		 * The feature reference could be invalid, in which case we cannot clone it.
		 * It should normally be valid though so we don't document that Py_None could be returned to the caller.
		 */
		boost::python::object
		get_resolved_feature() const;

		/**
		 * Get the resolved topologies sharing the sub-segment (for passing to Python).
		 */
		boost::python::list
		get_sharing_resolved_topologies() const;

	private:

		//! The wrapped sub-segment itself.
		GPlatesAppLogic::ResolvedTopologicalSharedSubSegment d_resolved_topological_shared_sub_segment;

		/**
		 * The reconstruction geometry that the sub-segment was obtained from.
		 *
		 * We need to store a Python-wrapped version of it to keep its feature/property alive.
		 */
		ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> d_reconstruction_geometry;

		/**
		 * The resolved topologies (reconstruction geometries) that share the sub-segment.
		 *
		 * We need to store Python-wrapped versions to keep their feature/property alive.
		 */
		std::vector< ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> >
				d_sharing_resolved_topologies;

		/**
		 * A feature containing the resolved (not present day) sub-segment geometry.
		 *
		 * NOTE: It is only created when first requested by @a get_resolved_feature.
		 */
		mutable boost::python::object d_resolved_feature_object;
	};


	/**
	 * Boost-python requires 'get_pointer(HeldType)' for wrapped types ('HeldType') that
	 * are not already smart pointers.
	 */
	GPlatesAppLogic::ResolvedTopologicalSharedSubSegment *
	get_pointer(
			const ResolvedTopologicalSharedSubSegmentWrapper &wrapper)
	{
		// Boost-python wants a non-const return pointer (but wants a const wrapper).
		return const_cast<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment *>(
				&wrapper.get_resolved_topological_shared_sub_segment());
	}


	/**
	 * A Python wrapper around a derived @a ResolvedTopologicalSection that contains a sequence of
	 * sharing resolved topologies (all which must be wrapped in order to keep their feature/property alive).
	 */
	class ResolvedTopologicalSectionWrapper
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
		typedef GPlatesAppLogic::ResolvedTopologicalSection element_type;

		explicit
		ResolvedTopologicalSectionWrapper(
				const GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_to_const_type &resolved_topological_section);

		/**
		 * Get the resolved topological section.
		 */
		GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type
		get_resolved_topological_section() const
		{
			return d_resolved_topological_section;
		}

		/**
		 * Get the reconstruction geometry of the sub-segment (for passing to Python).
		 */
		const ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> &
		get_reconstruction_geometry() const
		{
			return d_reconstruction_geometry;
		}

		/**
		 * Get the shared sub-segments (for passing to Python).
		 */
		boost::python::list
		get_shared_sub_segments() const;

	private:

		//! The wrapped sub-segment itself.
		GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type d_resolved_topological_section;

		/**
		 * The reconstruction geometry that the sub-segment was obtained from.
		 *
		 * We need to store a Python-wrapped version of it to keep its feature/property alive.
		 */
		ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry> d_reconstruction_geometry;

		/**
		 * The shared sub-segments.
		 *
		 * We need to store Python-wrapped versions to keep their feature/property alive.
		 */
		std::vector<ResolvedTopologicalSharedSubSegmentWrapper> d_shared_sub_segments;
	};


	/**
	 * Boost-python requires 'get_pointer(HeldType)' for wrapped types ('HeldType') that
	 * are not already smart pointers.
	 */
	GPlatesAppLogic::ResolvedTopologicalSection *
	get_pointer(
			const ResolvedTopologicalSectionWrapper &wrapper)
	{
		return wrapper.get_resolved_topological_section().get();
	}
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYRECONSTRUCTIONGEOMETRIES_H
