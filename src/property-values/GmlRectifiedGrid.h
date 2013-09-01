/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLRECTIFIEDGRID_H
#define GPLATES_PROPERTYVALUES_GMLRECTIFIEDGRID_H

#include <vector>
#include <map>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "Georeferencing.h"
#include "GmlGridEnvelope.h"
#include "GmlPoint.h"
#include "XsString.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


// Enable GPlatesFeatureVisitors::get_revisionable() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlRectifiedGrid, visit_gml_rectified_grid)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:RectifiedGrid".
	 */
	class GmlRectifiedGrid:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlRectifiedGrid>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlRectifiedGrid> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlRectifiedGrid>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlRectifiedGrid> non_null_ptr_to_const_type;


		/**
		 * An axis.
		 */
		class Axis :
				public boost::equality_comparable<Axis>
		{
		public:

			/**
			 * Axis has value semantics where each @a Axis instance has its own state.
			 * So if you create a copy and modify the copy's state then it will not modify the state
			 * of the original object.
			 *
			 * The constructor first clones the property value and then copy-on-write is used to allow
			 * multiple @a Axis objects to share the same state (until the state is modified).
			 */
			Axis(
					XsString::non_null_ptr_type name) :
				d_name(name)
			{  }

			/**
			 * Returns the 'const' band name.
			 */
			const XsString::non_null_ptr_to_const_type
			get_name() const
			{
				return d_name;
			}

			/**
			 * Returns the 'non-const' band name.
			 */
			const XsString::non_null_ptr_type
			get_name()
			{
				return d_name;
			}

			void
			set_name(
					XsString::non_null_ptr_type name)
			{
				d_name = name;
			}

			/**
			 * Value equality comparison operator.
			 *
			 * Inequality provided by boost equality_comparable.
			 */
			bool
			operator==(
					const Axis &other) const
			{
				return *d_name == *other.d_name;
			}

		private:
			XsString::non_null_ptr_type d_name;
		};

		//! Typedef for a sequence of axes.
		typedef std::vector<Axis> axes_list_type;

		typedef std::vector<double> offset_vector_type;
		typedef std::vector<offset_vector_type> offset_vector_list_type;
		typedef std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes_type;


		virtual
		~GmlRectifiedGrid()
		{  }

		/**
		 * Create a GmlRectifiedGrid instance.
		 *
		 * The @a xml_attributes_ could contain srsName, the identifier of a
		 * coordinate reference system, and dimension, which this class assumes to be
		 * 2, but is ignored if present.
		 *
		 * We don't check if the number of dimensions in the axes list or in the
		 * offset vectors list or in the origin specification match up with each other
		 * or with the dimension XML attribute.
		 */
		static
		const non_null_ptr_type
		create(
				const GmlGridEnvelope::non_null_ptr_type &limits_,
				const axes_list_type &axes_,
				const GmlPoint::non_null_ptr_type &origin_,
				const offset_vector_list_type &offset_vectors_,
				const xml_attributes_type &xml_attributes_);

		/**
		 * Convenience function for creating a GmlRectifiedGrid from georeferencing
		 * information, and raster width and height.
		 */
		static
		const non_null_ptr_type
		create(
				const Georeferencing::non_null_ptr_to_const_type &georeferencing,
				unsigned int raster_width,
				unsigned int raster_height,
				const xml_attributes_type &xml_attributes_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlRectifiedGrid>(clone_impl());
		}

		/**
		 * Returns the 'const' limits.
		 */
		const GmlGridEnvelope::non_null_ptr_to_const_type
		limits() const
		{
			return get_current_revision<Revision>().limits.get_revisionable();
		}

		/**
		 * Returns the 'non-const' limits.
		 */
		const GmlGridEnvelope::non_null_ptr_type
		limits()
		{
			return get_current_revision<Revision>().limits.get_revisionable();
		}

		/**
		 * Sets the internal limits.
		 */
		void
		set_limits(
				const GmlGridEnvelope::non_null_ptr_type &limits_);

		/**
		 * Returns the axes.
		 *
		 * To modify any members:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_axes to set them.
		 *
		 * The returned axes implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const axes_list_type &
		get_axes() const
		{
			return get_current_revision<Revision>().axes;
		}

		/**
		 * Sets the internal axes.
		 */
		void
		set_axes(
				const axes_list_type &axes_);

		/**
		 * Returns the 'const' origin.
		 */
		const GmlPoint::non_null_ptr_to_const_type
		origin() const
		{
			return get_current_revision<Revision>().origin.get_revisionable();
		}

		/**
		 * Returns the 'non-const' origin.
		 */
		const GmlPoint::non_null_ptr_type
		origin()
		{
			return get_current_revision<Revision>().origin.get_revisionable();
		}

		/**
		 * Sets the internal origin.
		 */
		void
		set_origin(
				GmlPoint::non_null_ptr_type origin_);

		/**
		 * Returns the offset vectors.
		 *
		 * To modify any offset vectors:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_offset_vectors to set them.
		 *
		 * The returned offset vectors implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const offset_vector_list_type &
		get_offset_vectors() const
		{
			return get_current_revision<Revision>().offset_vectors;
		}

		void
		set_offset_vectors(
				const offset_vector_list_type &offset_vectors_);

		const xml_attributes_type &
		get_xml_attributes() const
		{
			return get_current_revision<Revision>().xml_attributes;
		}

		void
		set_xml_attributes(
				const xml_attributes_type &xml_attributes_);

		const boost::optional<Georeferencing::non_null_ptr_to_const_type>
		convert_to_georeferencing() const;

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("RectifiedGrid");
			return STRUCTURAL_TYPE;
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
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gml_rectified_grid(*this);
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
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gml_rectified_grid(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlRectifiedGrid(
				GPlatesModel::ModelTransaction &transaction_,
				const GmlGridEnvelope::non_null_ptr_type &limits_,
				const axes_list_type &axes_,
				const GmlPoint::non_null_ptr_type &origin_,
				const offset_vector_list_type &offset_vectors_,
				const xml_attributes_type xml_attributes_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(
									transaction_, *this,
									limits_, axes_, origin_, offset_vectors_, xml_attributes_)))
		{  }

		//! Constructor used when cloning.
		GmlRectifiedGrid(
				const GmlRectifiedGrid &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GmlRectifiedGrid(*this, context));
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
				const Revisionable::non_null_ptr_to_const_type &child_revisionable);

		/**
		 * Inherited from @a RevisionContext.
		 */
		virtual
		boost::optional<GPlatesModel::Model &>
		get_model()
		{
			return PropertyValue::get_model();
		}

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const GmlGridEnvelope::non_null_ptr_type &limits_,
					const axes_list_type &axes_,
					const GmlPoint::non_null_ptr_type &origin_,
					const offset_vector_list_type &offset_vectors_,
					const xml_attributes_type xml_attributes_) :
				limits(
						GPlatesModel::RevisionedReference<GmlGridEnvelope>::attach(
								transaction_, child_context_, limits_)),
				axes(axes_),
				origin(
						GPlatesModel::RevisionedReference<GmlPoint>::attach(
								transaction_, child_context_, origin_)),
				offset_vectors(offset_vectors_),
				xml_attributes(xml_attributes_)
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				limits(other_.limits),
				axes(other_.axes),
				origin(other_.origin),
				offset_vectors(other_.offset_vectors),
				xml_attributes(other_.xml_attributes)
			{
				// Clone data members that were not deep copied.
				limits.clone(child_context_);
				origin.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				limits(other_.limits),
				axes(other_.axes),
				origin(other_.origin),
				offset_vectors(other_.offset_vectors),
				xml_attributes(other_.xml_attributes)
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

			GPlatesModel::RevisionedReference<GmlGridEnvelope> limits;
			axes_list_type axes;
			GPlatesModel::RevisionedReference<GmlPoint> origin;
			offset_vector_list_type offset_vectors;

			xml_attributes_type xml_attributes;

			mutable boost::optional<Georeferencing::non_null_ptr_to_const_type> cached_georeferencing;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLRECTIFIEDGRID_H
