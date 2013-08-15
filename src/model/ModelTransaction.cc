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

#include <boost/foreach.hpp>

#include "ModelTransaction.h"


void
GPlatesModel::ModelTransaction::commit()
{
	//
	// Switch the property value, top level property, feature, feature collection and feature store
	// objects to reference their new (bubbled up) revisions.

	BOOST_FOREACH(
			PropertyValueTransaction &property_value_transaction,
			d_property_value_transactions)
	{
		// Essentially amounts to a no-throw swap of the current revision for the new revision.
		PropertyValueTransaction current_revisioned_reference(
				property_value_transaction.d_property_value,
				property_value_transaction.d_property_value->d_current_revision);

		property_value_transaction.d_property_value->d_current_revision =
				property_value_transaction.d_revision;

		property_value_transaction = current_revisioned_reference;
	}

#if 0
	if (d_top_level_property_transactions)
	{
		// Essentially amounts to a no-throw swap of the current revision for the new revision.
		TopLevelProperty::RevisionedReference current_revisioned_reference =
				d_top_level_property_transactions->get_top_level_property()->get_current_revisioned_reference();
		d_top_level_property_transactions->set_current_revision();
		d_top_level_property_transactions = current_revisioned_reference;
	}
#endif

	// The destructor of ModelTransaction will release the old revisions which are now stored
	// in our revision references (we swapped the new for the old).
	// So if we hold the last reference then they will be destroyed and the destruction process
	// could potentially throw an exception - if that happens then at least it won't happen
	// during the commit of the new revision (which has already happened above) and leave us with
	// a partially committed model.
}
