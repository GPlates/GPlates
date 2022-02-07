/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLDATABLOCKCOORDINATELIST_H
#define GPLATES_PROPERTYVALUES_GMLDATABLOCKCOORDINATELIST_H

#include <map>
#include <vector>

#include "ValueObjectType.h"

#include "model/ModelTransaction.h"
#include "model/Revisionable.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"

#include "utils/QtStreamable.h"


namespace GPlatesPropertyValues
{
	/**
	 * This associates a ValueObjectType instance with a sequence of "i-th" coordinates from a
	 * @c <gml:tupleList> property in a "gml:DataBlock".
	 *
	 * For info about "gml:DataBlock", see p.251-2 of Lake et al (2004).
	 *
	 * To understand what this class contains and how it fits into GmlDataBlock, consider that
	 * the @a <gml:tupleList> property stores a sequence of coordinate tuples: x1,y1 x2,y2
	 * x3,y3 x4,y4 ... (ie, a "record interleaved" encoding).
	 *
	 * Each coordinate xn in the tuple xn,yn is described by a ValueObject X in a
	 * @a <gml:valueComponent> property in a @a <gml:CompositeValue> element.
	 *
	 * Class GmlDataBlockCoordinateList effectively "de-interleaves" the coordinate tuples,
	 * storing the ValueObject X along with the coordinates it describes x1 x2 x3 x4 ...; a
	 * GmlDataBlock instance contains a sequence of GmlDataBlockCoordinateList instances (one
	 * instance for each coordinate in the coordinate tuple).
	 *
	 * When the GmlDataBlock is output in GPML, it will be necessary to "re-interleave" the
	 * coordinate tuples.
	 */
	class GmlDataBlockCoordinateList :
			public GPlatesModel::Revisionable,
			public GPlatesModel::RevisionContext,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<GmlDataBlockCoordinateList>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlDataBlockCoordinateList>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlDataBlockCoordinateList> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlockCoordinateList>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlockCoordinateList> non_null_ptr_to_const_type;


		/**
		 * The type which contains XML attribute names and values.
		 */
		typedef std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes_type;

		/**
		 * The type containing the coordinates.
		 */
		typedef std::vector<double> coordinates_type;


		~GmlDataBlockCoordinateList()
		{  }


		static
		const non_null_ptr_type
		create(
				const ValueObjectType &value_object_type_,
				const xml_attributes_type &value_object_xml_attributes_,
				const coordinates_type &coordinates_)
		{
			return create(value_object_type_, value_object_xml_attributes_, coordinates_.begin(), coordinates_.end());
		}

		template <typename CoordinatesIter>
		static
		const non_null_ptr_type
		create(
				const ValueObjectType &value_object_type_,
				const xml_attributes_type &value_object_xml_attributes_,
				CoordinatesIter coordinates_begin,
				CoordinatesIter coordinates_end)
		{
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(
					new GmlDataBlockCoordinateList(
							transaction,
							value_object_type_,
							value_object_xml_attributes_,
							coordinates_begin,
							coordinates_end));
			transaction.commit();
			return ptr;
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlDataBlockCoordinateList>(clone_impl());
		}

		// Note that no "setter" is provided:  The value object type is not changeable.
		const ValueObjectType &
		get_value_object_type() const
		{
			return d_value_object_type;
		}

		/**
		 * Return the map of XML attributes contained by this instance.
		 */
		const xml_attributes_type &
		get_value_object_xml_attributes() const
		{
			return get_current_revision<Revision>().value_object_xml_attributes;
		}

		/**
		 * Set the map of XML attributes contained by this instance.
		 */
		void
		set_value_object_xml_attributes(
				const xml_attributes_type &value_object_xml_attributes);

		/**
		 * Return the coordinates contained by this instance.
		 */
		const coordinates_type &
		get_coordinates() const
		{
			return get_current_revision<Revision>().coordinates;
		}

		/**
		 * Set the coordinates contained by this instance.
		 */
		void
		set_coordinates(
				const coordinates_type &coordinates);

	protected:


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template <typename CoordinatesIter>
		GmlDataBlockCoordinateList(
				GPlatesModel::ModelTransaction &transaction_,
				const ValueObjectType &value_object_type_,
				const xml_attributes_type &value_object_xml_attributes_,
				CoordinatesIter coordinates_begin,
				CoordinatesIter coordinates_end) :
			Revisionable(
					Revision::non_null_ptr_type(
							new Revision(
									transaction_,
									*this,
									value_object_xml_attributes_,
									coordinates_begin,
									coordinates_end))),
			d_value_object_type(value_object_type_)
		{  }

		//! Constructor used when cloning.
		GmlDataBlockCoordinateList(
				const GmlDataBlockCoordinateList &other_,
				boost::optional<RevisionContext &> context_) :
			Revisionable(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this))),
			d_value_object_type(other_.d_value_object_type)
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GmlDataBlockCoordinateList(*this, context));
		}

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const GmlDataBlockCoordinateList &other_pv = dynamic_cast<const GmlDataBlockCoordinateList &>(other);

			return d_value_object_type == other_pv.d_value_object_type &&
					// The revisioned data comparisons are handled here...
					Revisionable::equality(other);
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a RevisionContext.
		 */
		virtual
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const GPlatesModel::Revisionable::non_null_ptr_to_const_type &child_revisionable);


		/**
		 * Inherited from @a RevisionContext.
		 */
		virtual
		boost::optional<GPlatesModel::Model &>
		get_model()
		{
			return GPlatesModel::Revisionable::get_model();
		}


		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::Revision
		{
			template <typename CoordinatesIter>
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const xml_attributes_type &value_object_xml_attributes_,
					CoordinatesIter coordinates_begin,
					CoordinatesIter coordinates_end) :
				value_object_xml_attributes(value_object_xml_attributes_),
				coordinates(coordinates_begin, coordinates_end)
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				GPlatesModel::Revision(context_),
				value_object_xml_attributes(other_.value_object_xml_attributes),
				coordinates(other_.coordinates)
			{
				// Clone data members that were not deep copied.
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				GPlatesModel::Revision(context_),
				value_object_xml_attributes(other_.value_object_xml_attributes),
				coordinates(other_.coordinates)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<RevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const;


			xml_attributes_type value_object_xml_attributes;
			coordinates_type coordinates;
		};


		ValueObjectType d_value_object_type;

	};


	std::ostream &
	operator<<(
			std::ostream &os,
			const GmlDataBlockCoordinateList &gml_data_block_coordinate_list);

}

#endif  // GPLATES_PROPERTYVALUES_GMLDATABLOCKCOORDINATELIST_H
