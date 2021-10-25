/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODREGISTRY_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODREGISTRY_H

#include <map>
#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ReconstructMethodInterface.h"
#include "ReconstructMethodType.h"

#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * Registry for information required to find and create @a ReconstructMethodInterface objects.
	 */
	class ReconstructMethodRegistry :
			private boost::noncopyable
	{
	public:
		/**
		 * Convenience typedef for a function that determines if a reconstruct method can
		 * reconstruct a feature.
		 *
		 * The function takes the following arguments:
		 * - A weak reference to a feature.
		 *
		 * Returns true if the reconstruct method can reconstruct the specified feature.
		 */
		typedef boost::function<bool (const GPlatesModel::FeatureHandle::const_weak_ref &)>
				can_reconstruct_feature_function_type;

		/**
		 * Convenience typedef for a function that creates a @a ReconstructMethodInterface.
		 *
		 * The function takes the following arguments:
		 * - A weak reference to a feature associated with the reconstruct method.
		 * - Data to initialise the reconstruction method with.
		 *
		 * Returns the created @a ReconstructMethodInterface.
		 */
		typedef boost::function<
				ReconstructMethodInterface::non_null_ptr_type (
						const GPlatesModel::FeatureHandle::weak_ref &,
						const ReconstructMethodInterface::Context &)>
								create_reconstruct_method_function_type;


		/**
		 * Constructor.
		 *
		 * If @a register_default_reconstruct_method_types_ is true then the default reconstruct
		 * method types are registered.
		 */
		explicit
		ReconstructMethodRegistry(
				bool register_default_reconstruct_method_types_ = true);


		/**
		 * Registers information about the default reconstruct method types.
		 *
		 * Note that this was called by the constructor if it's @a register_default_reconstruct_method_types_ was true.
		 */
		void
		register_default_reconstruct_method_types();

		/**
		 * Registers information about the given @a reconstruct_method_type.
		 */
		void
		register_reconstruct_method(
				ReconstructMethod::Type reconstruct_method_type,
				const can_reconstruct_feature_function_type &can_reconstruct_feature_function_,
				const create_reconstruct_method_function_type &create_reconstruct_method_function_);

		/**
		 * Unregisters the specified reconstruct method.
		 */
		void
		unregister_reconstruct_method(
				ReconstructMethod::Type reconstruct_method_type);


		/**
		 * Returns a list of reconstruct method types of all registered reconstruct methods.
		 */
		std::vector<ReconstructMethod::Type>
		get_registered_reconstruct_methods() const;


		/**
		 * Returns true if the specified feature can be reconstructed by *any* registered reconstruct methods.
		 */
		bool
		can_reconstruct_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const;


		/**
		 * Returns true if the specified feature can be reconstructed by the specified reconstruct method.
		 *
		 * The reconstruct method type must have been already registered.
		 *
		 * @throws PreconditionViolationError if @a reconstruct_method_type has not been registered.
		 */
		bool
		can_reconstruct_feature(
				ReconstructMethod::Type reconstruct_method_type,
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const;


		/**
		 * Returns the first reconstruct method type that can reconstruct the specified feature.
		 *
		 * NOTE: If reconstruct method 'BY_PLATE_ID' *and* another reconstruct method can both
		 * reconstruct the specified feature then preference is given to the other reconstruct method.
		 * This is because 'BY_PLATE_ID' is a bit of a catch-all so preference is given to more
		 * specialised reconstruct methods where available.
		 *
		 * Returns boost::none if no matching reconstruct method types could be found.
		 */
		boost::optional<ReconstructMethod::Type>
		get_reconstruct_method_type(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref) const;


		/**
		 * Creates a reconstruct method of the first type that can reconstruct the specified feature.
		 *
		 * NOTE: If reconstruct method 'BY_PLATE_ID' *and* another reconstruct method can both
		 * reconstruct the specified feature then preference is given to the other reconstruct method.
		 * This is because 'BY_PLATE_ID' is a bit of a catch-all so preference is given to more
		 * specialised reconstruct methods where available.
		 *
		 * @a reconstruct_method_context is the context in which the reconstruct method performs reconstructions.
		 *
		 * Returns boost::none if no matching reconstruct method types could be found.
		 */
		boost::optional<ReconstructMethodInterface::non_null_ptr_type>
		create_reconstruct_method(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const ReconstructMethodInterface::Context &reconstruct_method_context) const;


		/**
		 * Same as @a get_reconstruct_method_type but returns a 'BY_PLATE_ID' reconstruct method type
		 * if no reconstruct method types could be found.
		 */
		ReconstructMethod::Type
		get_reconstruct_method_or_default_type(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref) const;


		/**
		 * Same as @a create_reconstruct_method but creates a 'BY_PLATE_ID' reconstruct method
		 * if no reconstruct method types could be found.
		 *
		 * @a reconstruct_method_context is the context in which the reconstruct method performs reconstructions.
		 *
		 * @throws PreconditionViolationError if BY_PLATE_ID reconstruction method type has not been registered.
		 */
		ReconstructMethodInterface::non_null_ptr_type
		create_reconstruct_method_or_default(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const ReconstructMethodInterface::Context &reconstruct_method_context) const;


		/**
		 * Creates a new reconstruct method of the same type, and associated with the same feature,
		 * as the specified reconstruct method but with the specified context data.
		 */
		ReconstructMethodInterface::non_null_ptr_type
		create_reconstruct_method(
				const ReconstructMethodInterface &reconstruct_method,
				const ReconstructMethodInterface::Context &reconstruct_method_context) const;


		/**
		 * Creates a new reconstruct method of the specified type, and associated with the
		 * specified feature, but with the context data specified.
		 *
		 * NOTE: The reconstruct method type must be the type associated with the specified feature.
		 * For example, calling @a get_reconstruct_method_type on the specified feature should
		 * return the specified reconstruction method type (although this is not checked internally).
		 *
		 * @throws PreconditionViolationError if @a reconstruct_method_type has not been registered.
		 */
		ReconstructMethodInterface::non_null_ptr_type
		create_reconstruct_method(
				ReconstructMethod::Type reconstruct_method_type,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const ReconstructMethodInterface::Context &reconstruct_method_context) const;


	private:
		struct ReconstructMethodInfo
		{
			ReconstructMethodInfo(
					const can_reconstruct_feature_function_type &can_reconstruct_feature_function_,
					const create_reconstruct_method_function_type &create_reconstruct_method_function_) :
				can_reconstruct_feature_function(can_reconstruct_feature_function_),
				create_reconstruct_method_function(create_reconstruct_method_function_)
			{  }

			can_reconstruct_feature_function_type can_reconstruct_feature_function;
			create_reconstruct_method_function_type create_reconstruct_method_function;
		};

		typedef std::map<ReconstructMethod::Type, ReconstructMethodInfo> reconstruct_method_info_map_type;


		/**
		 * Stores a struct of information for each reconstruct method type.
		 */
		reconstruct_method_info_map_type d_reconstruct_method_info_map;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODREGISTRY_H
