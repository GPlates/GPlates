/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */

#include <algorithm>  // find()
#include "DataGroup.h"

using namespace GPlatesGeo;

DataGroup::DataGroup(const DataType_t& dt, const RotationGroupId_t& id,
	const Attributes_t& attrs)
	: GeologicalData(dt, id, attrs)
{ }

DataGroup::DataGroup(const DataType_t& dt, const RotationGroupId_t& id,
	const Attributes_t& attrs, const Children_t& children)
	: GeologicalData(dt, id, attrs), _children(children)
{ }

inline void
DataGroup::Add(GeologicalData* child) throw()
{
	// XXX: This throws std::bad_alloc when the memory is exhausted.
	_children.push_back(child);
}  

void
DataGroup::Remove(GeologicalData* child) throw()
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
	for ( ; iter != _children.end(); iter++)
		(*iter)->Accept(visitor);
}

void
DataGroup::Accept(Visitor& visitor) const
{
	Children_t::const_iterator iter = _children.begin();
	for ( ; iter != _children.end(); iter++)
		(*iter)->Accept(visitor);
}
