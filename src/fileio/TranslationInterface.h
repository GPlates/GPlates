/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004 The GPlates Consortium
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
 */

#ifndef _GPLATES_FILEIO_TRANSLATIONINTERFACE_H_
#define _GPLATES_FILEIO_TRANSLATIONINTERFACE_H_

#include <map>
#include <list>
#include <string>

#include "global/types.h"

namespace GPlatesGeo
{
	class DataGroup;
}


namespace GPlatesFileIO
{
	/**
	 * Interface through which data in a file is converted into
	 * the GPlates internal representation.  TranslationInterface
	 * serves as an abstraction of the internal data representation
	 * so that the internal representation may vary without effecting
	 * the various data file handling classes.  It has deliberately
	 * been defined to be as restrictive as possible, for the purpose
	 * of ease of maintainence of backward compatibility.
	 */
	class TranslationInterface
	{
		public:
			/**
			 * @name Interface Types
			 * The following types should be used when communicating
			 * with the file translation interface.
			 */
			/// @{

			/**
			 * Use fpdata_t for floating-point data.
			 */
			typedef GPlatesGlobal::fpdata_t  fpdata_t;

			/**
			 * Use integer_t for integer data.
			 */
			typedef GPlatesGlobal::integer_t integer_t;

			/**
			 * Use rid_t to specify a rotation group identifier.
			 */
			typedef GPlatesGlobal::rid_t     rid_t;

			/**
			 * Use string_t to specify a character string.
			 */
			typedef std::string              string_t;

			/// @}

		
			/**
			 * A location on the globe specified by latitude and longitude.
			 * This is the basic geometric element, the other geometric
			 * elements are built out of LatLonPoints.
			 */
			class LatLonPoint
			{
				public:
					LatLonPoint(
					 const fpdata_t& latitude,
					 const fpdata_t& longitude)
					: _latitude(latitude), _longitude(longitude)
					{
						;
					}

					const fpdata_t&
					GetLatitude() const
					{
						return _latitude;
					}

					const fpdata_t&
					GetLongitude() const
					{
						return _longitude;
					}

				private:
					fpdata_t _latitude, _longitude;
			};

			
			/**
			 * A sequence of LatLonPoints.
			 * Each successive LatLonPoint is (conceptually) connected via a
			 * great circle arc to the next LatLonPoint.  The final 
			 * LatLonPoint is @b NOT connected to the first LatLonPoint.
			 */
			class LatLonLine
			{
				public:
					/**
					 * To ensure that the LatLonLine is always in a 
					 * valid state, the constructor takes two arguments
					 * specifying the start and end points of the initial
					 * line segment respectively.  The line grows in 
					 * length as points are added with AppendLatLonPoint.
					 *
					 * @param p1 Start point of initial line segment.
					 * @param p2 End point of initial line segment.
					 */
					LatLonLine(
					 const LatLonPoint& p1,
					 const LatLonPoint& p2)
					{
						AppendLatLonPoint(p1);
						AppendLatLonPoint(p2);
					}

					/**
					 * Add a point to the sequence of points that defines
					 * this line.
					 *
					 * @param point The point to be appended.
					 */
					void
					AppendLatLonPoint(const LatLonPoint& point)
					{
						_line.push_back(point);
					}
					
					/**
					 * @name Element Access
					 * STL-style iterators to obtain access to the points
					 * that make up this line.
					 *
					 * @warning As yet, the const_iterator type will at most
					 *   be guarenteed to conform to the interface for the STL
					 *   ForwardIterator type.
					 */
					/// @{

					typedef std::list< LatLonPoint >::const_iterator const_iterator;

					const_iterator
					Begin() const
					{
						return _line.begin();
					}

					const_iterator
					End() const
					{
						return _line.end();
					}
					
					/// @}
	
				private:
					std::list< LatLonPoint >  _line;
			};


			/**
			 * A sequence of LatLonPoints whose start and end are connected.
			 * Each successive LatLonPoint is (conceptually) connected via a
			 * great circle arc to the next LatLonPoint.  The final 
			 * LatLonPoint @b IS connected to the first LatLonPoint to
			 * form a loop.  The first and last point need not be coincident
			 * for the loop to be valid.
			 */
			class LatLonLoop
			{
				public:
					/**
					 * To ensure that the LatLonLoop is always in a 
					 * valid state, the constructor takes three arguments
					 * specifying the three corners of a "triangle" on the
					 * globe.  The closed line will take form as additional
					 * points are added using AppendLatLonPoint.  These
					 * points will be the forth, fifth, etc. corners of
					 * the "polygon".
					 *
					 * @param p1 "First" corner of the "triangle".
					 * @param p2 "Second" corner of the "triangle".
					 * @param p3 "Third" corner of the "triangle".
					 */
					LatLonLoop(
					 const LatLonPoint& p1,
					 const LatLonPoint& p2,
					 const LatLonPoint& p3)
					{
						AppendLatLonPoint(p1);
						AppendLatLonPoint(p2);
						AppendLatLonPoint(p3);
					}

					/**
					 * Add a point to the sequence of points that defines
					 * this closed line.
					 *
					 * @param point The point to be appended.
					 */
					void
					AppendLatLonPoint(const LatLonPoint& point)
					{
						_loop.push_back(point);
					}

					/**
					 * @name Element Access
					 * STL-style iterators to obtain access to the points
					 * that make up this closed line.
					 *
					 * @warning As yet, the const_iterator type will at most
					 *   be guarenteed to conform to the interface for the STL
					 *   ForwardIterator type.
					 */
					/// @{

					typedef std::list< LatLonPoint >::const_iterator const_iterator;

					const_iterator
					Begin() const
					{
						return _loop.begin();
					}

					const_iterator
					End() const
					{
						return _loop.end();
					}
					
					/// @}
					
				private:
					/**
					 * Storage space for the points that make up this
					 * closed line.
					 */
					std::list< LatLonPoint >  _loop;
			};


			/**
			 * @name Data Attributes
			 * The following class is used to represent that attributes that
			 * may be attached to a geometry.
			 */
			/// @{
			
			/**
			 * Represents a set of name/value pairs where the possible
			 * values for name are predefined.
			 */
			class Attributes
			{
				public:
					void
					SetDataType(const string_t& type)
					{
						InsertAttribute(string_t("dataType"), type);
					}

					void
					SetSubDataType(const string_t& subtype)
					{
						InsertAttribute(string_t("dataSubType"), subtype);
					}
					
					void
					SetRotationGroup(const string_t& rid)
					{
						InsertAttribute(string_t("rotationGroup"), rid);
					}

					void
					SetRegion(const string_t& region)
					{
						InsertAttribute(string_t("region"), region);
					}

					void
					SetAgeOfAppearance(const string_t& appearance)
					{
						InsertAttribute(string_t("ageOfAppearance"), appearance);
					}

					void
					SetAgeOfDisappearance(const string_t& disappearance)
					{
						InsertAttribute(string_t("ageOfDisappearance"), disappearance);
					}

					void
					SetResponsibleParty(const string_t& responsibleParty)
					{
						InsertAttribute(string_t("responsibleParty"), responsibleParty);
					}

					void
					SetColour(const string_t& colour)
					{
						InsertAttribute(string_t("colour"), colour);
					}

					void
					SetArbitraryAttribute(
					 const string_t& name,
					 const string_t& value)
					{
						// FIXME: Should not insert this value if such an
						// element already exists as part of the definition
						// of GPML.
						InsertAttribute(name, value);
					}

				private:
					typedef std::map< string_t, string_t >  StringMap_t;

					StringMap_t _attributes;

					void
					InsertAttribute(
					 const string_t& name,
					 const string_t& value)
					{
						_attributes.insert(std::make_pair(name, value));
					}
			};
			
			/// @}
			

			/**
			 * @name Data Registration Functions
			 * The following functions are used to create the internal
			 * representation of the data contained in a file.  Each
			 * function takes a geometry element and the attributes
			 * associated with that geometry.
			 */
			/// @{

			void
			RegisterLatLonPointData(
			 const LatLonPoint& point,
			 const Attributes&  attributes);

			
			void
			RegisterLatLonLineData(
			 const LatLonLine& line,
			 const Attributes& attributes);

			
			void
			RegisterLatLonLoopData(
			 const LatLonLoop& loop,
			 const Attributes& attributes);

			/// @}
			
			/**
			 * Temporary: will have "virtual" constructor
			 * eventually.
			 */
			TranslationInterface() { }

		private:
			GPlatesGeo::DataGroup* _data;

			/**
			 * Copy constructor deliberately not implemented.
			 */
			TranslationInterface(const TranslationInterface&);
	};
}

#endif  /* _GPLATES_FILEIO_TRANSLATIONINTERFACE_H_ */
