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

#include "BubbleUpRevisionHandler.h"

#include "Model.h"


GPlatesModel::BubbleUpRevisionHandler::BubbleUpRevisionHandler(
		const Revisionable::non_null_ptr_type &revisionable) :
	d_model(revisionable->get_model()),
	d_revisionable(revisionable),
	d_revision(revisionable->create_bubble_up_revision(d_transaction)),
	d_committed(false)
{
}


GPlatesModel::BubbleUpRevisionHandler::~BubbleUpRevisionHandler()
{
	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		commit();
	}
	catch (...)
	{
	}
}


void
GPlatesModel::BubbleUpRevisionHandler::commit()
{
	if (d_committed)
	{
		return;
	}

	d_committed = true;

	// Committing the transaction switches over to the new revision.
	d_transaction.commit();

	// Emit the model events if either there's no model (ie, not attached to model), or
	// we are attached to the model but the model notification guard is not currently active
	// (if it's active then the events will be re-determined and emitted when the notification
	// guard is released).
	if (!d_model ||
		!d_model->has_notification_guard())
	{
		// TODO: Emit model events.
		//
		// Not yet implementable: the bubble-up chain currently ends at the top-level property
		// (FeatureHandle/BasicHandle are not RevisionContexts), so 'd_model' is always none for
		// a property value inside a feature and there is no feature to notify. Consequently an
		// in-place property-value edit (eg, GpmlPlateId::set_value()) does not notify the owning
		// feature's listeners - no reconstruction, no unsaved-changes flag - whereas
		// FeatureHandle::add()/set()/remove() do. The desktop app's non-const FeatureVisitor path
		// bridges this gap in the interim (see FeatureVisitorBase<FeatureHandle>::visit_feature_property()
		// in "FeatureVisitor.h"); Python-side edits of a model-attached property value do not.
		// This becomes implementable once FeatureHandle is a RevisionContext (see the
		// 'feature/pygplates-model-revisions' branch).
	}
}
