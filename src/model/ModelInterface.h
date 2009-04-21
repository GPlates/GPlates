/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_MODELINTERFACE_H
#define GPLATES_MODEL_MODELINTERFACE_H

#include <boost/shared_ptr.hpp>
#include "FeatureIdRegistry.h"


namespace GPlatesModel
{
	class Model;

	/**
	 * This class serves as a very simple "p-impl" interface to class Model.
	 *
	 * This will create a "compiler firewall" (in order to speed up build times) as well as an
	 * architectural separation between the Model tier (embodied by the Model class) and the
	 * other GPlates tiers which use the Model.
	 *
	 * Classes outside the Model tier should pass around ModelInterface instances, never Model
	 * instances.  A ModelInterface instance can be copy-constructed cheaply (since the
	 * copy-construction involves only the copy of a pointer member and the increment of a
	 * ref-count integer) and copy-assigned cheaply.
	 *
	 * Header files outside of the Model tier should only #include "model/ModelInterface.h",
	 * never "model/Model.h".  "model/Model.h" should only be #included in ".cc" files when
	 * necessary (ie, whenever you want to access the members of the Model instance through the
	 * ModelInterface, and hence need the class definition of Model).
	 */
	class ModelInterface
	{
	public:
		/**
		 * Construct a new ModelInterface instance.
		 *
		 * This will also create a new Model instance, which will be managed by the
		 * ModelInterface instance.
		 */
		ModelInterface();

		/**
		 * Copy-construct a new ModelInterface instance.
		 *
		 * The new ModelInterface instance will share the Model instance which is managed
		 * by @other.
		 */
		ModelInterface(
				const ModelInterface &other):
			d_model_ptr(other.d_model_ptr)
		{  }

		/**
		 * Destroy this ModelInterface instance.
		 *
		 * This will also deallocate the Model instance which is managed by this
		 * ModelInterface instance.
		 */
		~ModelInterface()
		{  }

		/**
		 * Access the members of the Model instance.
		 *
		 * This is syntactic sugar for the equivalent function @a access_model.
		 */
		Model *
		operator->()
		{
			return access_model();
		}

		/**
		 * Access the members of the Model instance.
		 */
		Model *
		access_model()
		{
			return d_model_ptr.get();
		}

	private:
		boost::shared_ptr<Model> d_model_ptr;
	};
}

#endif  // GPLATES_MODEL_MODELINTERFACE_H
