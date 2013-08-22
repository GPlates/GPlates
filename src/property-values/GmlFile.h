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

#ifndef GPLATES_PROPERTYVALUES_GMLFILE_H
#define GPLATES_PROPERTYVALUES_GMLFILE_H

#include <map>
#include <vector>
#include <boost/optional.hpp>

#include "ProxiedRasterCache.h"
#include "RasterType.h"
#include "ValueObjectType.h"
#include "XsString.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/ReadErrorAccumulation.h"

#include "global/unicode.h"

#include "model/PropertyValue.h"
#include "model/PropertyValueRevisionContext.h"
#include "model/PropertyValueRevisionedReference.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlFile, visit_gml_file)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:File".
	 *
	 * If the file is a raster file, GmlFile instances hold a proxied RawRaster
	 * instance for each band in that raster file.
	 */
	class GmlFile:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::PropertyValueRevisionContext
	{
	private:

		/**
		 * This version of a pair class is here because if we use std::pair,
		 * some compilers are complaining that T or U doesn't have a default
		 * constructor even though we don't ever call the default constructor
		 * for std::pair. This version of pair gets around this problem by not
		 * having a default constructor.
		 */
		template<typename T, typename U>
		struct pair
		{
			pair(
					const T &t,
					const U &u) :
				first(t),
				second(u)
			{  }

			bool
			operator==(
					const pair &other) const
			{
				return first == other.first &&
					second == other.second;
			}

			T first;
			U second;
		};

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlFile>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlFile> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlFile>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlFile> non_null_ptr_to_const_type;


		typedef std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes_type;
		typedef pair<ValueObjectType, xml_attributes_type> value_component_type;
		typedef std::vector<value_component_type> composite_value_type;


		virtual
		~GmlFile()
		{  }

		/**
		 * Create a GmlFile instance.
		 */
		static
		const non_null_ptr_type
		create(
				const composite_value_type &range_parameters_,
				const XsString::non_null_ptr_type &file_name_,
				const XsString::non_null_ptr_type &file_structure_,
				const boost::optional<XsString::non_null_ptr_type> &mime_type_ = boost::none,
				const boost::optional<XsString::non_null_ptr_type> &compression_ = boost::none,
				GPlatesFileIO::ReadErrorAccumulation *read_errors_ = NULL);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlFile>(clone_impl());
		}


		const composite_value_type &
		get_range_parameters() const
		{
			return get_current_revision<Revision>().range_parameters;
		}

		void
		set_range_parameters(
				const composite_value_type &range_parameters_);


		const XsString::non_null_ptr_to_const_type
		get_file_name() const
		{
			return get_current_revision<Revision>().file_name.get_property_value();
		}

		const XsString::non_null_ptr_type
		get_file_name()
		{
			return get_current_revision<Revision>().file_name.get_property_value();
		}

		void
		set_file_name(
				XsString::non_null_ptr_type file_name_,
				GPlatesFileIO::ReadErrorAccumulation *read_errors = NULL);


		const XsString::non_null_ptr_to_const_type
		get_file_structure() const
		{
			return get_current_revision<Revision>().file_structure.get_property_value();
		}

		const XsString::non_null_ptr_type
		get_file_structure()
		{
			return get_current_revision<Revision>().file_structure.get_property_value();
		}

		void
		set_file_structure(
				XsString::non_null_ptr_type file_structure_);


		const boost::optional<XsString::non_null_ptr_to_const_type>
		get_mime_type() const;

		const boost::optional<XsString::non_null_ptr_type>
		get_mime_type();

		void
		set_mime_type(
				boost::optional<XsString::non_null_ptr_type> mime_type_);


		const boost::optional<XsString::non_null_ptr_to_const_type>
		get_compression() const;

		const boost::optional<XsString::non_null_ptr_type>
		get_compression();

		void
		set_compression(
				boost::optional<XsString::non_null_ptr_type> compression_);

		/**
		 * If the file is a raster file, and the bands could be read, returns one
		 * proxied RawRaster for each band in that raster file.
		 *
		 * In the exceptional case where the number of bands could be read but a
		 * particular band could not be read, an UninitialisedRawRaster takes the
		 * place of the proxied RawRaster in the vector.
		 */
		std::vector<RawRaster::non_null_ptr_type>
		get_proxied_raw_rasters() const;

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("File");
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
			visitor.visit_gml_file(*this);
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
			visitor.visit_gml_file(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlFile(
				GPlatesModel::ModelTransaction &transaction_,
				const composite_value_type &range_parameters_,
				const XsString::non_null_ptr_type &file_name_,
				const XsString::non_null_ptr_type &file_structure_,
				const boost::optional<XsString::non_null_ptr_type> &mime_type_,
				const boost::optional<XsString::non_null_ptr_type> &compression_,
				GPlatesFileIO::ReadErrorAccumulation *read_errors_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(
									transaction_, *this,
									range_parameters_, file_name_, file_structure_, mime_type_,
									compression_, read_errors_)))
		{  }

		//! Constructor used when cloning.
		GmlFile(
				const GmlFile &other_,
				boost::optional<PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GmlFile(*this, context));
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a PropertyValueRevisionContext.
		 */
		virtual
		GPlatesModel::PropertyValueRevision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const PropertyValue::non_null_ptr_to_const_type &child_property_value);

		/**
		 * Inherited from @a PropertyValueRevisionContext.
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
				public GPlatesModel::PropertyValueRevision
		{
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					PropertyValueRevisionContext &child_context_,
					const composite_value_type &range_parameters_,
					const XsString::non_null_ptr_type &file_name_,
					const XsString::non_null_ptr_type &file_structure_,
					const boost::optional<XsString::non_null_ptr_type> &mime_type_,
					const boost::optional<XsString::non_null_ptr_type> &compression_,
					GPlatesFileIO::ReadErrorAccumulation *read_errors_);

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_,
					PropertyValueRevisionContext &child_context_);

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_);

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone_revision(
					boost::optional<PropertyValueRevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const PropertyValueRevision &other) const;

			void
			update_proxied_raster_cache(
					GPlatesFileIO::ReadErrorAccumulation *read_errors) const;


			composite_value_type range_parameters;
			GPlatesModel::PropertyValueRevisionedReference<XsString> file_name;
			GPlatesModel::PropertyValueRevisionedReference<XsString> file_structure;
			boost::optional<GPlatesModel::PropertyValueRevisionedReference<XsString> > mime_type;
			boost::optional<GPlatesModel::PropertyValueRevisionedReference<XsString> > compression;

			// TODO: Remove caching and updating when filename changes and when image on disk is modified.
			// The image (eg, JPEG) should be converted/updated to GPlates format during import only.
			// And when the filename changes this should be handled by client code by listening
			// to model events that indicate the raster feature (that this property belongs) has been
			// modified in which case it can reference a different GPlates format raster image file.
			mutable boost::optional<ProxiedRasterCache::non_null_ptr_type> proxied_raster_cache;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLFILE_H
