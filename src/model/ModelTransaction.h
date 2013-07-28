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

#ifndef GPLATES_MODEL_MODELTRANSACTION_H
#define GPLATES_MODEL_MODELTRANSACTION_H

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * Base model transaction class to be inherited for different aspects of the model data.
	 *
	 * A model transaction takes care of committing or rolling back a revision to the model data.
	 *
	 * Each area of the model data hierarchy has a different derived transaction type.
	 * The transaction/revision areas are feature collections, features, properties and property values.
	 */
	class ModelTransaction :
			public GPlatesUtils::ReferenceCount<ModelTransaction>
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a ModelTransaction.
		typedef GPlatesUtils::non_null_intrusive_ptr<ModelTransaction> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a ModelTransaction.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ModelTransaction> non_null_ptr_to_const_type;


		virtual
		~ModelTransaction()
		{  }


		/**
		 * Commit the revision stored in the transaction to the model data.
		 */
		virtual
		void
		commit() = 0;


		/**
		 * Roll back the revision stored in the transaction to the model data.
		 *
		 * Provided commits and roll backs have followed the correct undo/redo order then
		 * the revision stored in this transaction before calling @a rollback should be the
		 * previous revision and after calling @a rollback should be the current revision (undone).
		 */
		virtual
		void
		rollback() = 0;
	};
}

#endif // GPLATES_MODEL_MODELTRANSACTION_H
