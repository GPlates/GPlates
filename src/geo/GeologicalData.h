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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_GEOLOGICALDATA_H_
#define _GPLATES_GEO_GEOLOGICALDATA_H_

#include <vector>
#include <string>
#include "Visitor.h"
#include "GeneralisedData.h"
#include "global/UnsupportedFunctionException.h"
#include "global/types.h"  /* integer_t */

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
			typedef std::vector<GeneralisedData*> Attributes_t;

			/**
			 * @todo Form a rationale for these choices.  For example, why
			 *   shouldn't DataType_t be an int?
			 */
			typedef std::string DataType_t;
			typedef integer_t RotationGroupId_t;

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
			 * If the data has no associated attributes, then it's
			 * Attributes should be NO_ATTRIBUTES.
			 */
			static const Attributes_t
			NO_ATTRIBUTES;
			
			GeologicalData(const DataType_t&, const RotationGroupId_t&, 
				const Attributes_t&);

			virtual
			~GeologicalData() { }
			
			DataType_t
			GetDataType() const { return _data_type; }

			RotationGroupId_t
			GetRotationGroupId() const { return _rotation_group_id; }

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
			 * Enumerative access to the data's Attributes_t.
			 */
			virtual std::back_insert_iterator<Attributes_t>
			AttributesInserter() { return std::back_inserter(_attributes); }

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
			 * The information associated with this piece of data.
			 */
			Attributes_t _attributes;
	};
}

#endif  // _GPLATES_GEO_GEOLOGICALDATA_H_
