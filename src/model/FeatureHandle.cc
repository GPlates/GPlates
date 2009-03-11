/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "FeatureHandle.h"
#include "WeakObserverVisitor.h"


GPlatesModel::FeatureHandle::~FeatureHandle()
{
	weak_observer_unsubscribe_forward(d_first_const_weak_observer);
	weak_observer_unsubscribe_forward(d_first_weak_observer);
}


void
GPlatesModel::FeatureHandle::apply_weak_observer_visitor(
		WeakObserverVisitor<FeatureHandle> &visitor)
{
	weak_observer_type *curr = first_weak_observer();
	while (curr != NULL) {
		curr->accept_weak_observer_visitor(visitor);
		curr = curr->next_link_ptr();
	}
}
