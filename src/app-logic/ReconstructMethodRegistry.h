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
		 * The function takes no arguments.
		 *
		 * Returns the created @a ReconstructMethodInterface.
		 */
		typedef boost::function<
				ReconstructMethodInterface::non_null_ptr_type ()>
								create_reconstruct_method_function_type;


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
		 * Returns boost::none if no reconstruct method types could be found.
		 */
		boost::optional<ReconstructMethod::Type>
		get_reconstruct_method_type(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const;


		/**
		 * Same as @a get_reconstruct_method_type but returns 'BY_PLATE_ID' if no reconstruct
		 * method types could be found.
		 */
		ReconstructMethod::Type
		get_reconstruct_method_type_or_default(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const;


		/**
		 * Returns an internally stored reconstruct method associated with the specified
		 * reconstruct method type.
		 *
		 * Since reconstruct methods don't store any state they can be shared by many different clients.
		 *
		 * The reconstruct method type must have been already registered.
		 *
		 * @throws PreconditionViolationError if @a reconstruct_method_type has not been registered.
		 */
		ReconstructMethodInterface::non_null_ptr_type
		get_reconstruct_method(
				ReconstructMethod::Type reconstruct_method_type) const;


		/**
		 * Causes a new reconstruct method of the given type.
		 *
		 * This is only useful if you want to create and own your own reconstruct method,
		 * otherwise using the internal reconstruct method returned by @a get_reconstruct_method
		 * is easier.
		 *
		 * The reconstruct method type must have been already registered.
		 *
		 * @throws PreconditionViolationError if @a reconstruct_method_type has not been registered.
		 */
		ReconstructMethodInterface::non_null_ptr_type
		create_reconstruct_method(
				ReconstructMethod::Type reconstruct_method_type) const;


	private:
		struct ReconstructMethodInfo
		{
			ReconstructMethodInfo(
					const can_reconstruct_feature_function_type &can_reconstruct_feature_function_,
					const create_reconstruct_method_function_type &create_reconstruct_method_function_) :
				can_reconstruct_feature_function(can_reconstruct_feature_function_),
				create_reconstruct_method_function(create_reconstruct_method_function_),
				// Create a reconstruct method that can be shared by all clients.
				shared_reconstruct_method(create_reconstruct_method_function_())
			{  }

			can_reconstruct_feature_function_type can_reconstruct_feature_function;
			create_reconstruct_method_function_type create_reconstruct_method_function;
			ReconstructMethodInterface::non_null_ptr_type shared_reconstruct_method;
		};

		typedef std::map<ReconstructMethod::Type, ReconstructMethodInfo> reconstruct_method_info_map_type;


		/**
		 * Stores a struct of information for each reconstruct method type.
		 */
		reconstruct_method_info_map_type d_reconstruct_method_info_map;
	};

	/**
	 * Registers information about the default reconstruct method types with the given @a registry.
	 */
	void
	register_default_reconstruct_method_types(
			ReconstructMethodRegistry &registry);
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODREGISTRY_H
