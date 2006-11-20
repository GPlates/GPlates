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

#ifndef _GPLATES_GEO_GEOLOGICALDATA_H_
#define _GPLATES_GEO_GEOLOGICALDATA_H_

#include <map>
#include <string>
#include "Visitor.h"
#include "TimeWindow.h"
#include "StringValue.h"
#include "global/UnsupportedFunctionException.h"
#include "global/types.h"  /* integer_t, rid_t */

namespace GPlatesGeo
{
	using namespace GPlatesGlobal;

	/**
	 * An abstraction for displayable data.
	 * Each piece of GeologicalData has an associated Attributes_t 
	 * collection, of which one attribute may be displayed at the data's 
	 * location.
	 * @invariant There should be some restriction on the values that
	 *   _rotation_group_id can take.
	 * @todo Make the class invariant a bit more precise.
	 * @role Component in the Composite design pattern (p163). 
	 */
	class GeologicalData
	{
		public:
			/** 
			 * Convenience typedef for the manipulation of the data.
			 */
			typedef std::map<std::string, StringValue*> Attributes_t;

			/**
			 * @todo Form a rationale for these choices.  For example, why
			 *   shouldn't DataType_t be an int?
			 */
			typedef std::string DataType_t;
			typedef rid_t RotationGroupId_t;

			/**
			 * If the data has no associated type, then it's DataType
			 * should be NO_DATATYPE.
			 */
			static const DataType_t
			NO_DATATYPE;

			/**
			 * If the data has no associated rotation group, then it's
			 * RotationGroupId should be NO_ROTATIONGROUP.
			 */
			static const RotationGroupId_t
			NO_ROTATIONGROUP;
			
			/**
			 * If the data has no associated age of appearance/disappearance,
			 * then TimeWindow should be NO_TIMEWINDOW.  This should be 
			 * equivalent to the data being visible forever and ever.
			 */
			static const TimeWindow
			NO_TIMEWINDOW;

			/**
			 * If the data has no associated attributes, then it's
			 * Attributes should be NO_ATTRIBUTES.
			 */
			static const Attributes_t
			NO_ATTRIBUTES;
			
			GeologicalData(const DataType_t&, 
						   const RotationGroupId_t&,
						   const TimeWindow&,
						   const Attributes_t& = NO_ATTRIBUTES);

			virtual ~GeologicalData();
			
			DataType_t
			GetDataType() const { return _data_type; }

			RotationGroupId_t
			GetRotationGroupId() const { return _rotation_group_id; }

			TimeWindow
			GetTimeWindow() const { return _time_window; }


			bool
			ExistsAtTime(const fpdata_t &t) const {

				return (_time_window.ContainsTime(t));
			}


			/**
			 * Allow a Visitor to visit this data.
			 * @role Element::Accept(Visitor) in the Visitor pattern (p331).
			 */
			virtual void
			Accept(Visitor&) const = 0;
			
			/** 
			 * A child management method, for use with DataGroup.
			 * @throws UnsupportedFunctionException is thrown if the object 
			 *   is not of type DataGroup.
			 * @warning The message given to UnsupportedFunctionException
			 *   when it is thrown will be the same when it is thrown 
			 *   from subclasses - this will be inaccurate in some cases.
			 * @role Component::Add(Component) in the Composite design pattern 
			 *   (p163).
			 * @remark We are going for transparency (as opposed to 
			 *   'safety') by declaring it in the parent class.  Some safety 
			 *   is retained by having it raise an exception when called on an 
			 *   object which is not playing the Composite role.
			 * @see DataGroup.
			 */
			virtual void
			Add(GeologicalData*);

			/** 
			 * A child management method, for use with DataGroup.
			 * @throws UnsupportedFunction is thrown if the object is not of
			 *   type DataGroup.
			 * @warning See warning for Add().
			 * @role Component::Remove(Component) in the Composite design 
			 *   pattern (p163).
			 * @remark See remark under Add() for notes on this declaration.
			 * @see Add(), DataGroup.
			 */
			virtual void
			Remove(GeologicalData*);

			/**
			 * Return the value associated with the given @a key,
			 * or NULL if @a key was not found.
			 */
			StringValue*
			GetAttributeValue(const std::string& key) const;

			/**
			 * Adds the given @a key / @a value pair to the map of
			 * attributes.
			 *
			 * If the given key already exists, its associated value is 
			 * overwritten.
			 */
			void
			SetAttributeValue(const std::string& key,
							  StringValue* value);
			
			/** 
			 * Restricted enumerative access to the data's Attributes_t.
			 */
			virtual Attributes_t::const_iterator
			AttributesBegin() const { return _attributes.begin(); }
		
			virtual Attributes_t::const_iterator
			AttributesEnd() const { return _attributes.end(); }

		private:
			/**
			 * A code identifying the type of data being represented.
			 * Examples of these codes can be found in the Plates 4.0
			 * Database Manual.
			 * @todo Find the Plates 4.0 Database Manual and provide some
			 *   concrete examples.
			 */
			DataType_t _data_type;

			/**
			 * An identifier used when calculating rotations.
			 * For example: 801 AUS refers to the Australian Craton (p33,
			 * Gahagan).
			 */
			RotationGroupId_t _rotation_group_id;

			/**
			 * The window of time that the data is visible.
			 * Taken from the ageof{appearance,disappearance} elements.
			 */
			TimeWindow _time_window;

			/** 
			 * The information associated with this piece of data.
			 */
			Attributes_t _attributes;
	};
}

#endif  // _GPLATES_GEO_GEOLOGICALDATA_H_
