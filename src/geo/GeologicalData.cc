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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <map>
#include <string>
#include "GeologicalData.h"
#include "StringValue.h"

using namespace GPlatesGeo;

const GeologicalData::DataType_t
GeologicalData::NO_DATATYPE;

const GeologicalData::RotationGroupId_t
// AAAAAAAAAAAAAAAAARRRRRRRRRRRRRRRRRGGGGGGGGGGGGGGGGGGGHHHHHHHHHHH!!!!!!!!!!!!
// XXX XXX XXX XXX XXX XXX XXX
GeologicalData::NO_ROTATIONGROUP = static_cast<GPlatesGlobal::rid_t>(1000000);

const TimeWindow
GeologicalData::NO_TIMEWINDOW;

const GeologicalData::Attributes_t
GeologicalData::NO_ATTRIBUTES;

GeologicalData::GeologicalData(const DataType_t& dt, 
	const RotationGroupId_t& id, const TimeWindow& tw, 
	const Attributes_t& attrs)
	: _data_type(dt), _rotation_group_id(id), _time_window(tw),
	  _attributes(attrs)
{ }

void
GeologicalData::Add(GeologicalData*)
{
	throw UnsupportedFunctionException("GeologicalData::Add");
}

void
GeologicalData::Remove(GeologicalData*)
{
	throw UnsupportedFunctionException("GeologicalData::Remove");
}

StringValue *GeologicalData::GetAttributeValue (const std::string &key) const
{
	Attributes_t::const_iterator it;

	it = _attributes.find (key);
	if (it == _attributes.end ())
		return 0;
	return it->second;
}

void GeologicalData::SetAttributeValue (const std::string &key,
					StringValue *value)
{
	Attributes_t::iterator it;

	// Remove existing attribute
	it = _attributes.find (key);
	if (it != _attributes.end ()) {
		delete it->second;
		_attributes.erase (it);
	}

	_attributes.insert (
		std::make_pair<std::string, StringValue *> (key, value)
		);
}
