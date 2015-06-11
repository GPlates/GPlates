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
#include <boost/optional.hpp>

#include "app-logic/ReconstructionGeometry.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
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
				typename ReconstructionGeometryType::non_null_ptr_to_const_type reconstruction_geometry_type);

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

	private:

		static
		boost::any
		create_reconstruction_geometry_type_wrapper(
				const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry);


		//! The wrapped reconstruction geometry itself.
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type d_reconstruction_geometry;

		//! The derived reconstruction geometry wrapper (keeps feature/property alive).
		boost::any d_reconstruction_geometry_type_wrapper;

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
}

//
// Implementation.
//

namespace GPlatesApi
{
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	reconstruction_geometry_get_feature(
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry);

	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
	reconstruction_geometry_get_property(
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry);


	template <class ReconstructionGeometryType>
	ReconstructionGeometryTypeWrapper<ReconstructionGeometryType>::ReconstructionGeometryTypeWrapper(
			typename ReconstructionGeometryType::non_null_ptr_to_const_type reconstruction_geometry_type) :
		d_reconstruction_geometry_type(
				// Boost-python currently does not compile when wrapping *const* objects
				// (eg, 'ReconstructionGeometry::non_null_ptr_to_const_type') - see:
				//   https://svn.boost.org/trac/boost/ticket/857
				//   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
				//
				// ...so the current solution is to wrap *non-const* objects (to keep boost-python happy)
				// and cast away const (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<ReconstructionGeometryType>(reconstruction_geometry_type)),
		d_feature(reconstruction_geometry_get_feature(*reconstruction_geometry_type)),
		d_property(reconstruction_geometry_get_property(*reconstruction_geometry_type))
	{  }


	ReconstructionGeometryTypeWrapper<GPlatesAppLogic::ReconstructionGeometry>::ReconstructionGeometryTypeWrapper(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry) :
		d_reconstruction_geometry(
				// Boost-python currently does not compile when wrapping *const* objects
				// (eg, 'ReconstructionGeometry::non_null_ptr_to_const_type') - see:
				//   https://svn.boost.org/trac/boost/ticket/857
				//   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
				//
				// ...so the current solution is to wrap *non-const* objects (to keep boost-python happy)
				// and cast away const (which is dangerous since Python user could modify)...
				GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ReconstructionGeometry>(reconstruction_geometry)),
		d_reconstruction_geometry_type_wrapper(
				create_reconstruction_geometry_type_wrapper(reconstruction_geometry))
	{  }
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYRECONSTRUCTIONGEOMETRIES_H
