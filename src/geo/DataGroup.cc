/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include <algorithm>  // find()
#include "DataGroup.h"

using namespace GPlatesGeo;

DataGroup::DataGroup(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw, const Attributes_t& attrs)
	: GeologicalData(dt, id, tw, attrs)
{  }

DataGroup::DataGroup(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw,  const Attributes_t& attrs, 
	const Children_t& children)
	: GeologicalData(dt, id, tw, attrs), _children(children)
{ }

DataGroup::~DataGroup()
{
	Children_t::iterator iter = _children.begin();
	for ( ; iter != _children.end(); ++iter)
		delete *iter;
}

void
DataGroup::Add(GeologicalData* child)
{
	// XXX: This throws std::bad_alloc when the memory is exhausted.
	_children.push_back(child);
}  

void
DataGroup::Remove(GeologicalData* child)
{
	if (child == NULL)
		return;
	Children_t::iterator pos;
	pos = find(_children.begin(), _children.end(), child);
	if (pos != _children.end())
		_children.erase(pos);
}

void
DataGroup::Accept(Visitor& visitor)
{
	Children_t::iterator iter = _children.begin();
	for ( ; iter != _children.end(); ++iter)
		(*iter)->Accept(visitor);
}

void
DataGroup::Accept(Visitor& visitor) const
{
	Children_t::const_iterator iter = _children.begin();
	for ( ; iter != _children.end(); ++iter)
		(*iter)->Accept(visitor);
}
