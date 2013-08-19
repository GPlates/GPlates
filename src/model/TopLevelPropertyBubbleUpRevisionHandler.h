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

#ifndef GPLATES_MODEL_TOPLEVELPROPERTYBUBBLEUPREVISIONHANDLER_H
#define GPLATES_MODEL_TOPLEVELPROPERTYBUBBLEUPREVISIONHANDLER_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ModelTransaction.h"
#include "TopLevelProperty.h"
#include "TopLevelPropertyRevision.h"


namespace GPlatesModel
{
	// Forward declarations.
	class Model;

	/**
	 * A convenience RAII helper class used by derived top-level property classes in their methods
	 * that modify top-level property state.
	 */
	class TopLevelPropertyBubbleUpRevisionHandler :
			private boost::noncopyable
	{
	public:

		/**
		 * Constructor creates the bubble-up revisions from the specified top-level property
		 * up to the model feature store (if connected all the way up).
		 */
		explicit
		TopLevelPropertyBubbleUpRevisionHandler(
				const TopLevelProperty::non_null_ptr_type &top_level_property);

		/**
		 * Calls @a commit if it hasn't already been called.
		 *
		 * Exposing @a commit as a method enables caller to avoid the double-exception-terminates problem.
		 * In other words destructor must block/lose exception but @a commit does not.
		 */
		~TopLevelPropertyBubbleUpRevisionHandler();


		/**
		 * Returns the model transaction used to commit the revision change.
		 */
		ModelTransaction &
		get_model_transaction()
		{
			return d_transaction;
		}

		/**
		 * Returns the new mutable (base class) revision.
		 */
		TopLevelPropertyRevision::non_null_ptr_type
		get_revision()
		{
			return d_revision;
		}

		/**
		 * Returns the new mutable revision (cast to specified derived revision type).
		 * Derived top-level property classes modify the data in the returned derived revision class.
		 */
		template <class RevisionType>
		RevisionType &
		get_revision()
		{
			return dynamic_cast<RevisionType &>(*d_revision);
		}

		/**
		 * Commits the model transaction (of the bubbled-up revisions), and signaling of model
		 * events (unless connected to model and a model notification guard is currently active).
		 *
		 * NOTE: If this is not called then it will be called by the destructor.
		 */
		void
		commit();

	private:

		boost::optional<Model &> d_model;

		//! The model transaction will switch the current revision to the new one.
		ModelTransaction d_transaction;

		TopLevelProperty::non_null_ptr_type d_top_level_property;

		TopLevelPropertyRevision::non_null_ptr_type d_revision;

		bool d_committed;

	};
}

#endif // GPLATES_MODEL_TOPLEVELPROPERTYBUBBLEUPREVISIONHANDLER_H
