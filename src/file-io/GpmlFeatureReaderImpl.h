/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_GPMLFEATUREREADERIMPL_H
#define GPLATES_FILE_IO_GPMLFEATUREREADERIMPL_H

#include <list>

#include "GpmlPropertyReader.h"

#include "GpmlPropertyStructuralTypeReader.h"

#include "model/FeatureHandle.h"
#include "model/GpgimFeatureClass.h"
#include "model/GpgimVersion.h"
#include "model/XmlNode.h"

#include "utils/ReferenceCount.h"


namespace GPlatesFileIO
{
	namespace GpmlReaderUtils
	{
		struct ReaderParams;
	}

	/**
	 * Abstract base class for an implementation that reads a feature from a GPML file.
	 *
	 * Typically different implementations handles different aspects of feature reading and
	 * they will be chained together and used as the implementation for @a GpmlFeatureReaderInterface.
	 */
	class GpmlFeatureReaderImpl :
			public GPlatesUtils::ReferenceCount<GpmlFeatureReaderImpl>
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlFeatureReaderImpl.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlFeatureReaderImpl> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlFeatureReaderImpl.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlFeatureReaderImpl> non_null_ptr_to_const_type;

		//! Typedef for a sequence of @a XmlNode objects.
		typedef std::list<GPlatesModel::XmlNode::non_null_ptr_type> xml_node_seq_type;


		virtual
		~GpmlFeatureReaderImpl()
		{  }


		/**
		 * Creates and reads a feature from the specified sequence of XML nodes representing
		 * properties of the feature that have not yet been processed (by other feature reader impls).
		 *
		 * XML property nodes that are processed by a feature readers should be removed from
		 * the list so that other (chained) feature readers do not attempt to process them.
		 * This should happen even if a feature reader create an 'UninterpretedPropertyValue' from
		 * from an XML node.
		 */
		virtual
		GPlatesModel::FeatureHandle::non_null_ptr_type
		read_feature(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
				xml_node_seq_type &unprocessed_feature_property_xml_nodes,
				GpmlReaderUtils::ReaderParams &reader_params) const = 0;

	};


	/**
	 * The feature reader at the end of the chain of parent readers.
	 *
	 * When the root of the feature class inheritance tree is reached then this feature reader is
	 * used as the final delegated-to reader.
	 *
	 * This class creates a feature and reads the feature id and identity.
	 */
	class GpmlFeatureCreator :
			public GpmlFeatureReaderImpl
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlFeatureCreator.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlFeatureCreator> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlFeatureCreator.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlFeatureCreator> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GpmlFeatureCreator.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesModel::GpgimVersion &gpml_version)
		{
			return non_null_ptr_type(new GpmlFeatureCreator(gpml_version));
		}


		/**
		 * Creates a feature and reads the feature-id and revision-id from the specified property nodes.
		 */
		virtual
		GPlatesModel::FeatureHandle::non_null_ptr_type
		read_feature(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
				xml_node_seq_type &unprocessed_feature_property_xml_nodes,
				GpmlReaderUtils::ReaderParams &reader_params) const;

	private:

		/**
		 * The version of the GPGIM used to create the GPML file being read.
		 */
		GPlatesModel::GpgimVersion d_gpml_version;


		GpmlFeatureCreator(
				const GPlatesModel::GpgimVersion &gpml_version) :
			d_gpml_version(gpml_version)
		{  }

	};


	/**
	 * Default concrete class for reading a feature from a GPML file.
	 *
	 * This class defers completely to its associated GPGIM feature class in the @a Gpgim and then
	 * delegates reading of the remaining properties of the feature to its parent GPGIM feature reader
	 * (the reader associated with the parent feature class).
	 */
	class GpmlFeatureReader :
			public GpmlFeatureReaderImpl
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlFeatureReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlFeatureReader> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlFeatureReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlFeatureReader> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GpmlFeatureReader.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &gpgim_feature_class,
				const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &parent_feature_reader,
				const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version)
		{
			return non_null_ptr_type(
					new GpmlFeatureReader(
							gpgim_feature_class, parent_feature_reader, structural_type_reader, gpml_version));
		}


		/**
		 * Reads properties associated with our feature class and delegates remaining properties
		 * to the reader associated with the parent feature class.
		 *
		 * NOTE: Only property names allowed by the GPGIM (for the feature class) will
		 * generate property values and add them to the feature.
		 * These can be 'UninterpretedPropertyValue' property values if the property structural type
		 * is *not* allowed (when the property name *is* allowed).
		 *
		 * NOTE: Property names that are not allowed by the GPGIM (for the feature class)
		 * will not have 'UninterpretedPropertyValue' property values created for them and hence
		 * those nodes will not be removed from @a unprocessed_feature_property_xml_nodes.
		 */
		virtual
		GPlatesModel::FeatureHandle::non_null_ptr_type
		read_feature(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
				xml_node_seq_type &unprocessed_feature_property_xml_nodes,
				GpmlReaderUtils::ReaderParams &reader_params) const;

	private:

		//! Typedef for a sequence of property readers.
		typedef std::vector<GpmlPropertyReader::non_null_ptr_to_const_type> property_reader_seq_type;


		/**
		 * Property readers for only the properties in the GPGIM feature class associated with this reader.
		 *
		 * Ancestor properties are taken care of by the feature reader associated with out parent feature class.
		 */
		property_reader_seq_type d_property_readers;

		/**
		 * The feature reader associated with the parent GPGIM feature class.
		 *
		 * If there's no parent GPGIM feature class then the parent feature reader will be a
		 * @a GpmlFeatureCreator that creates the feature and reads its feature identity and revision.
		 */
		GpmlFeatureReaderImpl::non_null_ptr_to_const_type d_parent_feature_reader;


		GpmlFeatureReader(
				const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &gpgim_feature_class,
				const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &parent_feature_reader,
				const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version);

	};


	/**
	 * A feature reader that reads all unprocessed properties as 'UninterpretedPropertyValue' property values.
	 *
	 * This is used for feature types that are not recognised by the GPGIM.
	 */
	class GpmlUninterpretedFeatureReader :
			public GpmlFeatureReaderImpl
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlUninterpretedFeatureReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlUninterpretedFeatureReader> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlUninterpretedFeatureReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlUninterpretedFeatureReader> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GpmlUninterpretedFeatureReader that handles any feature properties not
		 * processed by @a feature_reader.
		 */
		static
		non_null_ptr_type
		create(
				const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader)
		{
			return non_null_ptr_type(new GpmlUninterpretedFeatureReader(feature_reader));
		}


		/**
		 * Reads all unprocessed properties as 'UninterpretedPropertyValue's.
		 */
		virtual
		GPlatesModel::FeatureHandle::non_null_ptr_type
		read_feature(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
				xml_node_seq_type &unprocessed_feature_property_xml_nodes,
				GpmlReaderUtils::ReaderParams &reader_params) const;

	private:

		//! Wraps each visited property in an 'UninterpretedPropertyValue' property value and adds to feature.
		struct UninterpretedPropertyValueCreator :
				public GPlatesModel::XmlNodeVisitor
		{
			UninterpretedPropertyValueCreator(
					const GPlatesModel::FeatureHandle::non_null_ptr_type &feature,
					GpmlReaderUtils::ReaderParams &reader_params);

			virtual
			void
			visit_text_node(
					const GPlatesModel::XmlTextNode::non_null_ptr_type &xml_text_node);

			virtual
			void
			visit_element_node(
					const GPlatesModel::XmlElementNode::non_null_ptr_type &xml_element_node);

			GPlatesModel::FeatureHandle::non_null_ptr_type d_feature;
			GpmlReaderUtils::ReaderParams &d_reader_params;
		};


		//! Delegate reading of feature properties to this feature reader.
		const GpmlFeatureReaderImpl::non_null_ptr_to_const_type d_feature_reader;


		GpmlUninterpretedFeatureReader(
				const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader);

	};
}

#endif // GPLATES_FILE_IO_GPMLFEATUREREADERIMPL_H
