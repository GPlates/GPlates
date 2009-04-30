
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
#define GPLATES_PROPERTYVALUES_GMLDATABLOCK_H

#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "model/PropertyValue.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"

#include "ValueObjectType.h"


namespace GPlatesPropertyValues {

	//
	// This class is used by GmlDataBlock below
	//
	class DataBlockCoordinateList 
	{

	public:

		static
		const boost::shared_ptr< DataBlockCoordinateList >
		create( 
			const ValueObjectType &value_object_type,
			const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
					value_object_xml_attributes, 
			std::vector<double>::size_type list_len )
		{
			boost::shared_ptr< DataBlockCoordinateList > prt = 
				new DataBlockCoordinateList( 
						value_object_type, 
						value_object_xml_attributes,
						list_len );

			return ptr;
		}

		//
		// access the Value Object
		//
		// FIXME ???

		//
		// access the xml attibutes
		//
 
		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map?  (For consistency with the non-const
		// overload...)
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		value_object_xml_attributes() const {
			return d_value_object_xml_attributes;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map, as well as per-index assignment (setter) and
		// removal operations?  This would ensure that revisioning is correctly handled...
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		value_object_xml_attributes() {
			return d_value_object_xml_attributes;
		}

		//
		// access the tuple list
		//
		// FIXME ???

		virtual
		~DataBlockCoordinateList() {  }

	protected:

		// constructor 
		DataBlockCoordinateList(
				const ValueObjectType &value_object_type,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &value_object_xml_attributes,
				std::vector<double>::size_type list_len ) :
			d_value_object_type(value_object_type);
			d_value_object_xml_attributes( value_object_xml_attributes )
		{
			// reserve the list
			d_tuple_list_values.reserve( list_len );
		}

		DataBlockCoordinateList(
				const DataBlockCoordinateList &other) :
			d_value_object_type( other.d_value_object_type ),
			d_value_object_xml_attributes( other.d_value_object_xml_attributes )
		{
			d_tuple_list_values.reserve( other.d_tuple_list_values.size() );
		}
		
	private:

		// value object type
		ValueObjectType d_value_object_type;

		// value object xml attibutes
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> 
			d_value_object_xml_attributes;
		
		// the tuple list data : <gml:tupleList>1,10 2,20 3,30 ... 9,90</gml:tupleList>
		std::vector<double> d_tuple_list_values;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		DataBlockCoordinateList &
		operator=(const DataBlockCoordinateList &);

	};

	class GmlDataBlock :
			public GPlatesModel::PropertyValue {

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlDataBlock,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlDataBlock,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		virtual
		~GmlDataBlock() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				std::list< boost::shared_ptr< DataBlockCoordinateList > > &data_block_list) 
		{
			non_null_ptr_type ptr(
					new GmlDataBlock( data_blokc_list ),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(new GmlDataBlock(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}


		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const {
			visitor.visit_gml_data_block(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) {
			visitor.visit_gml_data_block(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlDataBlock(
				std::list< boost::shared_ptr< DataBlockCoordinateList > > data_block_list):
			d_data_block_list( data_block_list )
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlDataBlock(
				const GmlDataBlock &other) :
			d_data_block_list(other.d_data_block_list),
		{  }

	private:

		// FIXME: what is this ?
		std::list< boost::shared_ptr< DataBlockCoordinateList > > d_data_block_list;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlDataBlock &
		operator=(const GmlDataBlock &);

	};


}

#endif  // GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
