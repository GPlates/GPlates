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

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PropertyValue.h"
#include "PropertyValueRevision.h"
#include "TopLevelProperty.h"
#include "TopLevelPropertyRevision.h"


namespace GPlatesModel
{
	/**
	 * A model transaction takes care of committing a revision to the model data.
	 *
	 * A revision consists of a linear chain of (bubbled-up) revisions that can follow the model data
	 * hierarchy from the property value level up to the feature store level. In some situations
	 * the revision chain does not reach the feature store level (eg, if creating a new feature
	 * collection and populating it before adding it to the feature store). Also in some situations
	 * the revision chain does not start at the property value level (eg, if adding a feature to a
	 * feature collection).
	 */
	class ModelTransaction :
			private boost::noncopyable
	{
	public:

		/**
		 * Transaction to set a top-level property's revision.
		 */
		class TopLevelPropertyTransaction
		{
		public:

			TopLevelPropertyTransaction(
					const TopLevelProperty::non_null_ptr_to_const_type &top_level_property,
					const TopLevelPropertyRevision::non_null_ptr_to_const_type &revision) :
				d_top_level_property(top_level_property),
				d_revision(revision)
			{  }

		private:

			TopLevelProperty::non_null_ptr_to_const_type d_top_level_property;
			TopLevelPropertyRevision::non_null_ptr_to_const_type d_revision;

			friend class ModelTransaction;
		};

		/**
		 * Transaction to set a property value's revision.
		 */
		class PropertyValueTransaction
		{
		public:

			PropertyValueTransaction(
					const PropertyValue::non_null_ptr_to_const_type &property_value,
					const PropertyValueRevision::non_null_ptr_to_const_type &revision) :
				d_property_value(property_value),
				d_revision(revision)
			{  }

		private:

			PropertyValue::non_null_ptr_to_const_type d_property_value;
			PropertyValueRevision::non_null_ptr_to_const_type d_revision;

			friend class ModelTransaction;
		};


		/**
		 * Sets the top level property revision for this transaction.
		 */
		void
		set_top_level_property_transaction(
				const TopLevelPropertyTransaction &revision)
		{
			d_top_level_property_transaction = revision;
		}

		/**
		 * Adds the specified property value revision to this transaction.
		 */
		void
		add_property_value_transaction(
				const PropertyValueTransaction &revision)
		{
			d_property_value_transactions.push_back(revision);
		}


		/**
		 * The final commit (of the revisions added to this transaction) to the model data.
		 *
		 * This points the relevant model data (property value, top level property, feature,
		 * feature collection, feature store) at their new revisions.
		 */
		void
		commit();

	private:

		std::vector<PropertyValueTransaction> d_property_value_transactions;
		boost::optional<TopLevelPropertyTransaction> d_top_level_property_transaction;
	};
}

#endif // GPLATES_MODEL_MODELTRANSACTION_H
