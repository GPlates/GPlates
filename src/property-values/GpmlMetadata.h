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

		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlMetadata> non_null_ptr_type;

		explicit
		GpmlMetadata(
				const GPlatesModel::FeatureCollectionMetadata& metadata):
			d_metadata(metadata)
		{ }

		static
		const non_null_ptr_type
		create(
				const GPlatesModel::FeatureCollectionMetadata& metadata)
		{

			return non_null_ptr_type(new GpmlMetadata(metadata));
		}

		const GpmlMetadata::non_null_ptr_type
		deep_clone() const
		{
			GpmlMetadata* p = new GpmlMetadata(d_metadata);
			return non_null_ptr_type(p);
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) 
		{
			visitor.visit_gpml_metadata(*this);
			return;
		}

		std::ostream &
		print_to(
				std::ostream &os) const
		{
			qWarning() << "TODO: implement this function.";
			os << "TODO: implement this function.";
			return  os;
		}

		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gpml_metadata(*this);
		}

		GPlatesPropertyValues::StructuralType
		get_structural_type() const 
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("GpmlMetadata");
			return STRUCTURAL_TYPE;
		}

		std::multimap<QString, QString>
		get_feature_collection_metadata_as_map() const
		{
			return d_metadata.get_metadata_as_map();
		}

		QString
		get_feature_collection_metadata_as_xml() const
		{
			return d_metadata.to_xml();
		}

		void
		serialize(
				GPlatesFileIO::XmlWriter& writer) const
		{
			d_metadata.serialize(writer);
		}

		GPlatesModel::FeatureCollectionMetadata
		get_data() const
		{
			return d_metadata;
		}

		GPlatesModel::FeatureCollectionMetadata&
		get_data() 
		{
			return d_metadata;
		}

	protected:
		GPlatesModel::FeatureCollectionMetadata d_metadata;
	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLMETADATA_H
