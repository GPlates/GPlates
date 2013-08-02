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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PropertyValue.h"
#include "TopLevelProperty.h"


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
		 * Sets the property value revisioned reference for this transaction.
		 */
		void
		set_property_value_revision(
				const PropertyValue::RevisionedReference &revision)
		{
			d_property_value_revision = revision;
		}

		/**
		 * Sets the top level property revisioned reference for this transaction.
		 */
		void
		set_top_level_property_revision(
				const TopLevelProperty::RevisionedReference &revision)
		{
			d_top_level_property_revision = revision;
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

		boost::optional<PropertyValue::RevisionedReference> d_property_value_revision;
		boost::optional<TopLevelProperty::RevisionedReference> d_top_level_property_revision;
	};
}

#endif // GPLATES_MODEL_MODELTRANSACTION_H
