/* $Id:  $ */
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-04-12 18:48:29 +1000 (Mon, 12 Apr 2010) $
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLMETADATA_H
#define GPLATES_PROPERTYVALUES_GPMLMETADATA_H

#include <QDebug>

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/XmlWriter.h"

#include "model/Metadata.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlMetadata, visit_gpml_metadata)

namespace GPlatesPropertyValues 
{
	class GpmlMetadata:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlMetadata>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlMetadata> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlMetadata>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlMetadata> non_null_ptr_to_const_type;


		static
		const non_null_ptr_type
		create(
				const GPlatesModel::FeatureCollectionMetadata &metadata)
		{
			return non_null_ptr_type(new GpmlMetadata(metadata));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlMetadata>(clone_impl());
		}


		const GPlatesModel::FeatureCollectionMetadata &
		get_data() const
		{
			return get_current_revision<Revision>().metadata;
		}

		void
		set_data(
				const GPlatesModel::FeatureCollectionMetadata &metadata)
		{
			MutableRevisionHandler revision_handler(this);
			revision_handler.get_mutable_revision<Revision>().metadata = metadata;
			revision_handler.handle_revision_modification();
		}

		std::multimap<QString, QString>
		get_feature_collection_metadata_as_map() const
		{
			return get_data().get_metadata_as_map();
		}

		QString
		get_feature_collection_metadata_as_xml() const
		{
			return get_data().to_xml();
		}

		void
		serialize(
				GPlatesFileIO::XmlWriter& writer) const
		{
			get_data().serialize(writer);
		}

		GPlatesPropertyValues::StructuralType
		get_structural_type() const 
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("GpmlMetadata");
			return STRUCTURAL_TYPE;
		}

		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) 
		{
			visitor.visit_gpml_metadata(*this);
			return;
		}

		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gpml_metadata(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const
		{
			qWarning() << "TODO: implement this function.";
			os << "TODO: implement this function.";
			return  os;
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlMetadata(
				const GPlatesModel::FeatureCollectionMetadata &metadata) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(metadata)))
		{ }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlMetadata(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValueRevision
		{
			explicit
			Revision(
					const GPlatesModel::FeatureCollectionMetadata &metadata_) :
				metadata(metadata_)
			{  }

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const PropertyValueRevision &other) const
			{
				// Compare the feature collectin metadata.
				// TODO: Implement.
				qWarning() << "GpmlMetadata::Revision::equality not implemented";
				return false;
			}

			GPlatesModel::FeatureCollectionMetadata metadata;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLMETADATA_H
