/* $Id$ */

/**
 * @file
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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_GENERALISEDDATA_H_
#define _GPLATES_GEO_GENERALISEDDATA_H_

#include <iostream>

namespace GPlatesGeo
{
	/** 
	 * A measurement or piece of data.
	 * Represents a piece of ancilliary data that may accompany a
	 * piece of GeologicalData, such as a place-name or measurement 
	 * of some variable.  It's primary purpose is to allow various
	 * pieces of GeneralisedData (subclasses such as Scalar, etc.) to
	 * be contained in a single collection.
	 * @see GeologicalData::Attributes_t.
	 */
	class GeneralisedData
	{
		public:
			virtual
			~GeneralisedData() { }
			
			/**
			 * Set the value from an istream.
			 * @param is The istream to read from.
			 * @pre @a is is in a legal state.
			 * @todo Decide whether this method should return something
			 *   to indicate a parse error.
			 * @role AbstractClass::PrimitiveOperation1() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			ReadIn(std::istream& is) = 0;

			/**
			 * Print data to an ostream.
			 * @param os The stream to print to.
			 * @pre @a os is in a legal state.
			 * @role AbstractClass::PrimitiveOperation2() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			PrintOut(std::ostream& os) const = 0;
	};

	/**
	 * Synonym for GeneralisedData::PrintOut().
	 * @role AbstractClass::TemplateMethod() in the Template Method design
	 *   pattern (p325).
	 */
	std::ostream&
	operator<<(std::ostream& os, const GeneralisedData& data);

	/**
	 * Synonym for GeneralisedData::ReadIn().
	 * @role AbstractClass::TemplateMethod() in the Template Method design
	 *   pattern (p325).
	 */
	std::istream&
	operator>>(std::istream& is, GeneralisedData& data);
}

#endif  // _GPLATES_GEO_GENERALISEDDATA_H_
