/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_PROPERTYVALUEREVISIONCONTEXT_H
#define GPLATES_MODEL_PROPERTYVALUEREVISIONCONTEXT_H

#include <boost/optional.hpp>

#include "PropertyValue.h"
#include "PropertyValueRevision.h"


namespace GPlatesModel
{
	// Forward declarations.
	class Model;
	class ModelTransaction;


	class PropertyValueRevisionContext
	{
	public:

		virtual
		~PropertyValueRevisionContext()
		{  }


		/**
		 * Bubbles up a modification from a property value (initiated by property value).
		 *
		 * The bubble-up mechanism creates a new revision as it bubbles up the (parent) context chain
		 * towards the top of the model hierarchy (feature store) if connected all the way up.
		 */
		virtual
		PropertyValueRevision::non_null_ptr_type
		bubble_up(
				ModelTransaction &transaction,
				const PropertyValue::non_null_ptr_to_const_type &property_value) = 0;

		
		/**
		 * Returns a (non-const) reference to the Model.
		 *
		 * Returns none if this context is not currently attached to the model - this can happen
		 * if this context has no parent context, etc, all the way up to the model feature store.
		 */
		virtual
		boost::optional<Model &>
		get_model() = 0;

	};
}

#endif // GPLATES_MODEL_PROPERTYVALUEREVISIONCONTEXT_H
