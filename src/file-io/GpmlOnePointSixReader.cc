/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include "GpmlOnePointSixReader.h"

#include <fstream>
#include <sstream>
#include <string>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QtXml/QXmlStreamReader>


#include "FileInfo.h"
#include "ReadErrors.h"
#include "ReadErrorOccurrence.h"
#include "ExternalProgram.h"
#include "PropertyCreationUtils.h"
#include "FeaturePropertiesMap.h"
#include "ErrorOpeningPipeFromGzipException.h"
#include "ErrorOpeningFileForReadingException.h"

#include "global/GPlatesAssert.h"

#include "maths/LatLonPoint.h"

#include "model/ChangesetHandle.h"
#include "model/Model.h"
#include "model/FeatureRevision.h"
#include "model/TopLevelPropertyInline.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/XmlNode.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/XsBoolean.h"

#include "utils/Profile.h"
#include "utils/StringUtils.h"
#include "utils/UnicodeStringUtils.h"


const GPlatesFileIO::ExternalProgram &
GPlatesFileIO::GpmlOnePointSixReader::gunzip_program()
{
	if (s_gunzip_program == NULL) {
		s_gunzip_program = new ExternalProgram("gzip -d", "gzip --version");
	}
	return *s_gunzip_program;
}


const GPlatesFileIO::ExternalProgram *GPlatesFileIO::GpmlOnePointSixReader::s_gunzip_program = NULL;


namespace
{
	namespace IO = GPlatesFileIO;
	namespace Utils = IO::GpmlReaderUtils;
	namespace XmlUtils = GPlatesUtils::XmlNamespaces;
	namespace Model = GPlatesModel;


	/**
	 * Returns true if the given @a namespaceUri and @a name match @a reader.namespaceUri()
	 * and @a reader.name(), false otherwise.
	 */
	bool
	qualified_names_are_equal(
			const QXmlStreamReader &reader,
			const QString &namespaceUri,
			const QString &name)
	{
		return (reader.namespaceUri() == namespaceUri) && (reader.name() == name);
	}

	typedef std::pair< 
		Model::XmlElementNode::non_null_ptr_type, 
		Model::PropertyValue::non_null_ptr_type >
			Property;
	typedef std::list< Property > PropertyList;
	typedef PropertyList::iterator property_iterator;


	void
	read_property(
			const Model::XmlElementNode::non_null_ptr_type &node,
			PropertyList &properties,
			const IO::PropertyCreationUtils::PropertyCreatorMap &prop_map,
			Utils::ReaderParams &params)
	{
		const Model::PropertyName &prop_name = node->get_name();
		
		IO::PropertyCreationUtils::PropertyCreatorMap::const_iterator pc = 
			prop_map.find(prop_name);

		if (pc != prop_map.end()) {
			try {
				properties.push_back(std::make_pair(node, (pc->second)(node, params.errors)));
				return;
			} catch (const IO::PropertyCreationUtils::GpmlReaderException &ex) {
				qWarning()
					<< "GpmlOnePointSixReader.cc:read_property: caught exception originating at "
					<< ex.source_location();
				Utils::append_warning(
						ex.location(), params, ex.description(),
						IO::ReadErrors::FeatureNotInterpreted);
			} catch (std::exception &exc) {
				qWarning() << "GpmlOnePointSixReader.cc:read_property: " << exc.what();
				Utils::append_warning(
						node, params, 
						IO::ReadErrors::ParseError,
						IO::ReadErrors::FeatureNotInterpreted);
			} catch(...) {
				qWarning() << "GpmlOnePointSixReader.cc:read_property: Unknown error";
				Utils::append_warning(
						node, params, 
						IO::ReadErrors::ParseError,
						IO::ReadErrors::FeatureNotInterpreted);
			}
		} 
		// Read the property value uninterpreted.
		Model::PropertyValue::non_null_ptr_type prop_val =
			GPlatesPropertyValues::UninterpretedPropertyValue::create(node);
		properties.push_back(std::make_pair(node, prop_val));
	}


	class AppendFeaturePropertiesVisitor
		: public Model::XmlNodeVisitor
	{
	public:
		AppendFeaturePropertiesVisitor(
				PropertyList &properties,
				Utils::ReaderParams &params,
				boost::optional<Model::RevisionId> &revision_id,
				boost::optional<Model::FeatureId> &feature_id,
				boost::optional<const IO::PropertyCreationUtils::PropertyCreatorMap &> prop_map
					= boost::optional<const IO::PropertyCreationUtils::PropertyCreatorMap &>()):
			d_properties(properties), d_prop_map(prop_map), d_params(params),
			d_revision_id(revision_id), d_feature_id(feature_id)
		{ }


		virtual
		~AppendFeaturePropertiesVisitor()
		{ }


		virtual
		void
		visit_text_node(
				const Model::XmlTextNode::non_null_ptr_type &text)
		{
			// Read the property value uninterpreted.
			Model::PropertyName prop_name = Model::PropertyName::create_gpml("unnamed-element");
			Model::XmlElementNode::non_null_ptr_type elem =
				Model::XmlElementNode::create(text, prop_name);
			Model::PropertyValue::non_null_ptr_type prop_val =
				GPlatesPropertyValues::UninterpretedPropertyValue::create(elem);
			d_properties.push_back(std::make_pair(elem, prop_val));
		}


		virtual
		void
		visit_element_node(
				const Model::XmlElementNode::non_null_ptr_type &elem)
		{
			const Model::PropertyName 
				REVISION_ID = Model::PropertyName::create_gpml("revision"),
				FEATURE_ID = Model::PropertyName::create_gpml("identity");

			if ((elem->get_name() == REVISION_ID) && (! d_revision_id)) {
				d_revision_id = IO::PropertyCreationUtils::create_revision_id(elem, d_params.errors);
				if (d_revision_id) {
					return;
				}
			}

			if ((elem->get_name() == FEATURE_ID) && (! d_feature_id)) {
				d_feature_id = IO::PropertyCreationUtils::create_feature_id(elem, d_params.errors);
				if (d_feature_id) {
					return;
				}
			}

			if (d_prop_map) {
				read_property(elem, d_properties, *d_prop_map, d_params);
			} 
			else {
				Model::PropertyValue::non_null_ptr_type prop_val =
					GPlatesPropertyValues::UninterpretedPropertyValue::create(elem);
				d_properties.push_back(std::make_pair(elem, prop_val));
			}
		}

	
	private:
		PropertyList &d_properties;
		boost::optional<const IO::PropertyCreationUtils::PropertyCreatorMap &> d_prop_map;
		Utils::ReaderParams &d_params;
		boost::optional<Model::RevisionId> &d_revision_id;
		boost::optional<Model::FeatureId> &d_feature_id;
	};

	
	void
	create_unclassifed_feature(
			const Model::XmlElementNode::non_null_ptr_type &xml_elem,
			Model::ModelInterface &model,
			Model::FeatureCollectionHandle::weak_ref &collection,
			Utils::ReaderParams &params)
	{
		Model::FeatureType feature_type(xml_elem->get_name());

		Model::FeatureHandle::weak_ref feature = 
			Model::FeatureHandle::create(collection, feature_type);

		// Read properties of the feature.
		PropertyList properties;

		boost::optional<Model::RevisionId> dummy_revision_id;
		boost::optional<Model::FeatureId> dummy_feature_id;
		AppendFeaturePropertiesVisitor visitor(properties, params, 
				dummy_revision_id, dummy_feature_id);
		std::for_each(
				xml_elem->children_begin(), xml_elem->children_end(),
				boost::bind(&Model::XmlNode::accept_visitor, _1, visitor));

		// Add properties to feature.
		property_iterator 
			iter = properties.begin(),
			end = properties.end();
		for ( ; iter != end; ++iter) {



// Top-level properties which also contain xml attributes
// may be having their attributes read twice (at both the property
// level, and here). To attempt to get round this, do not read
// xml attributes at the top level.
//
// If this turns out to cause problems with other property types
// we will have to find another solution.
//
// A similar modification has been made in the GpmlOnePointSixOutputVisitor - see
// GpmlOnePointSixOutputVisitor::visit_top_level_property_inline.

#if 0
			feature->add(
				Model::TopLevelPropertyInline::create(
					iter->first->get_name(),
					iter->second,
					iter->first->attributes_begin(),
					iter->first->attributes_end()));
#else
			// Don't write any xml attributes.
			feature->add(
				Model::TopLevelPropertyInline::create(
					iter->first->get_name(),
					iter->second));
#endif
		}
	}


	void
	create_feature(
			const Model::XmlElementNode::non_null_ptr_type &xml_elem,
			Model::ModelInterface &model,
			Model::FeatureCollectionHandle::weak_ref &collection,
			const IO::PropertyCreationUtils::PropertyCreatorMap &prop_map,
			Utils::ReaderParams &params)
	{
		// Save the feature type name.
		Model::FeatureType feature_type(xml_elem->get_name());
	
		// Read properties of the feature.
		PropertyList properties;

		boost::optional<Model::FeatureId> feature_id;
		boost::optional<Model::RevisionId> revision_id;

		AppendFeaturePropertiesVisitor visitor(properties, params, 
				revision_id, feature_id, prop_map);
		std::for_each(
				xml_elem->children_begin(), xml_elem->children_end(),
				boost::bind(&Model::XmlNode::accept_visitor, _1, visitor));

		Model::FeatureHandle::weak_ref feature;

		if (feature_id && revision_id) {
			feature = Model::FeatureHandle::create(
					collection,
					feature_type,
					*feature_id,
					*revision_id);
		} else if (feature_id) {
			feature = Model::FeatureHandle::create(
					collection,
					feature_type,
					*feature_id);
		} else {
			// Without a feature ID, a revision ID is meaningless.  So, even if we have
			// a revision ID, if we don't have a feature ID, regenerate both.
			feature = Model::FeatureHandle::create(
					collection,
					feature_type);
		}

		// Add properties to feature.
		property_iterator
			iter = properties.begin(),
			end = properties.end();
		for ( ; iter != end; ++iter) {
			// Top-level properties which also contain xml attributes
			// may be having their attributes read twice (at both the property
			// level, and here). To attempt to get round this, do not read
			// xml attributes at the top level.
			//
			// If this turns out to cause problems with other property types
			// we will have to find another solution.
			//
			// A similar modification has been made in the GpmlOnePointSixOutputVisitor - see
			// GpmlOnePointSixOutputVisitor::visit_top_level_property_inline.

#if 0
			feature->add(
				Model::TopLevelPropertyInline::create(
				iter->first->get_name(),
				iter->second,
				iter->first->attributes_begin(),
				iter->first->attributes_end()));
#else
			// Don't write any xml attributes.
			feature->add(
				Model::TopLevelPropertyInline::create(
				iter->first->get_name(),
				iter->second));
#endif
		}
	}


	void
	read_feature(
			const Model::XmlElementNode::non_null_ptr_type &xml_elem,
			Model::ModelInterface &model,
			Model::FeatureCollectionHandle::weak_ref &collection,
			Utils::ReaderParams &params)
	{
		// XXX: It's probable that we may wish to in some way preserve any 
		// attributes a feature has, even though we won't use them.
		append_warning_if( ! xml_elem->attributes_empty(), xml_elem, params,
				IO::ReadErrors::UnexpectedNonEmptyAttributeList,
				IO::ReadErrors::AttributesIgnored);
	
		IO::FeaturePropertiesMap &feature_properties_map = IO::FeaturePropertiesMap::instance();
		IO::FeaturePropertiesMap::const_iterator property_creator_map =
		   	feature_properties_map.find(
					Model::FeatureType(xml_elem->get_name()));

		if (property_creator_map != feature_properties_map.end()) {
			create_feature(xml_elem, model, collection, property_creator_map->second, params);
		} else {
			append_recoverable_error_if(true, xml_elem, params,
					IO::ReadErrors::UnrecognisedFeatureType,
					IO::ReadErrors::FeatureNotInterpreted);
			create_unclassifed_feature(xml_elem, model, collection, params);
		}
	}


	void
	read_feature_member(
			Utils::ReaderParams &params,
			Model::ModelInterface &model,
			const boost::shared_ptr<Model::XmlElementNode::AliasToNamespaceMap> &alias_map,
			Model::FeatureCollectionHandle::weak_ref &collection)
	{
		QXmlStreamReader &reader = params.reader;
		// .atEnd() can not be relied upon when reading a QProcess,
		// so we must make sure we block for a moment to make sure
		// the process is ready to feed us data.
		reader.device()->waitForReadyRead(1000);
		while ( ! reader.atEnd()) {
			reader.readNext();
			if (reader.isEndElement()) {
				break;
			}
			if (reader.isStartElement()) {
				Model::XmlElementNode::non_null_ptr_type elem = 
					Model::XmlElementNode::create(reader, alias_map);
				read_feature(elem, model, collection, params);
			}
			reader.device()->waitForReadyRead(1000);
		}
	}


	bool
	read_root_element(
		Utils::ReaderParams &params,
		boost::shared_ptr<Model::XmlElementNode::AliasToNamespaceMap> alias_map)
	{
		QXmlStreamReader &reader = params.reader;

		// .atEnd() can not be relied upon when reading a QProcess,
		// so we must make sure we block for a moment to make sure
		// the process is ready to feed us data.
		reader.device()->waitForReadyRead(1000);
		if (Utils::append_failure_to_begin_if(reader.atEnd(), params, 
				IO::ReadErrors::FileIsEmpty, IO::ReadErrors::FileNotLoaded)) {
			return false;
		}

		// Skip over the <?xml ... ?> stuff.
		// .atEnd() can not be relied upon when reading a QProcess,
		// so we must make sure we block for a moment to make sure
		// the process is ready to feed us data.
		reader.device()->waitForReadyRead(1000);
		while ( ! reader.atEnd()) {
			reader.readNext();
			if (reader.isStartElement()) {
				break;
			}
			reader.device()->waitForReadyRead(1000);
		}

		if (Utils::append_failure_to_begin_if(reader.atEnd(), params, 
				IO::ReadErrors::FileIsEmpty, IO::ReadErrors::FileNotLoaded)) {
			return false;
		}

		static const Model::PropertyName FEATURE_COLLECTION = 
			Model::PropertyName::create_gpml("FeatureCollection");
		Model::PropertyName current_element(
				reader.namespaceUri().toString(), 
				reader.name().toString());

		QXmlStreamNamespaceDeclarations ns_decls = reader.namespaceDeclarations();
		QXmlStreamNamespaceDeclarations::iterator
			iter = ns_decls.begin(),
			end = ns_decls.end();
		for ( ; iter != end; ++ iter) {
			alias_map->insert(
					std::make_pair(
						iter->prefix().toString(),
						iter->namespaceUri().toString()));
		}
		
		append_warning_if(current_element != FEATURE_COLLECTION, params,
				IO::ReadErrors::IncorrectRootElementName,
				IO::ReadErrors::ElementNameChanged);

		QStringRef file_version = reader.attributes().value(
				XmlUtils::GPML_NAMESPACE_QSTRING, "version");
		append_warning_if(file_version == "", params,
				IO::ReadErrors::MissingVersionAttribute,
				IO::ReadErrors::AssumingCorrectVersion);
		append_warning_if(file_version != "1.6", params,
				IO::ReadErrors::IncorrectVersionAttribute,
				IO::ReadErrors::AssumingCorrectVersion);

		return true;
	}
}


namespace
{
	/**
	 * Turns the relative file paths in the GPML into absolute file paths in the model.
	 */
	class MakeFilePathsAbsoluteVisitor :
			public GPlatesModel::FeatureVisitor
	{
	public:

		MakeFilePathsAbsoluteVisitor(
				const QString &absolute_path,
				GPlatesFileIO::ReadErrorAccumulation &read_errors) :
			d_absolute_path(absolute_path),
			d_read_errors(read_errors)
		{
			if (!d_absolute_path.endsWith("/"))
			{
				d_absolute_path.append("/");
			}
		}

		virtual
		void
		visit_gml_file(
				GPlatesPropertyValues::GmlFile &gml_file)
		{
			const GPlatesUtils::UnicodeString &filename = gml_file.file_name()->value().get();
			QString filename_qstring = GPlatesUtils::make_qstring_from_icu_string(filename);
			
			// Only fix if the filename in the GPML is relative.
			// Even if GPlates only ever writes relative filenames, there's
			// nothing to stop an absolute filename appearing
			if (QFileInfo(filename_qstring).isRelative())
			{
				QString result_qstring = QDir::cleanPath(d_absolute_path + filename_qstring);

				// qDebug() << result_qstring;

				GPlatesUtils::UnicodeString result = GPlatesUtils::make_icu_string_from_qstring(result_qstring);
				gml_file.set_file_name(GPlatesPropertyValues::XsString::create(result), &d_read_errors);
			}
		}

		virtual
		void
		visit_gpml_scalar_field_3d_file(
				GPlatesPropertyValues::GpmlScalarField3DFile &gpml_scalar_field_3d_file)
		{
			const GPlatesUtils::UnicodeString &filename = gpml_scalar_field_3d_file.file_name()->value().get();
			QString filename_qstring = GPlatesUtils::make_qstring_from_icu_string(filename);
			
			// Only fix if the filename in the GPML is relative.
			// Even if GPlates only ever writes relative filenames, there's
			// nothing to stop an absolute filename appearing
			if (QFileInfo(filename_qstring).isRelative())
			{
				QString result_qstring = QDir::cleanPath(d_absolute_path + filename_qstring);

				// qDebug() << result_qstring;

				GPlatesUtils::UnicodeString result = GPlatesUtils::make_icu_string_from_qstring(result_qstring);
				gpml_scalar_field_3d_file.set_file_name(GPlatesPropertyValues::XsString::create(result));
			}
		}

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			std::vector<GPlatesPropertyValues::GpmlTimeWindow> &time_windows =
				gpml_piecewise_aggregation.time_windows();
			BOOST_FOREACH(GPlatesPropertyValues::GpmlTimeWindow &time_window, time_windows)
			{
				time_window.time_dependent_value()->accept_visitor(*this);
			}
		}

	private:

		QString d_absolute_path;
		GPlatesFileIO::ReadErrorAccumulation &d_read_errors;
	};
}


void
GPlatesFileIO::GpmlOnePointSixReader::read_file(
		File::Reference &file,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors,
		bool use_gzip)
{
	PROFILE_FUNC();

	const FileInfo &fileinfo = file.get_file_info();

	// By placing all changes to the model under the one changeset, we ensure that
	// feature revision ids don't get changed from what was loaded from file no
	// matter what we do to the features.
	GPlatesModel::ChangesetHandle changeset(
			model.access_model(),
			"open " + fileinfo.get_qfileinfo().fileName().toStdString());

	QString filename(fileinfo.get_qfileinfo().filePath());
	QXmlStreamReader reader;

	QProcess input_process;
	QFile input_file(filename);
	if (use_gzip) {
		// gzipped, assuming gzipped GPML.
		// Set up the gzip process.
		input_process.setStandardInputFile(filename);
		// FIXME: Assuming gzip is in a standard place on the path. Not true on MS/Win32. Not true at all.
		// In fact, it may need to be a user preference.
		input_process.start(gunzip_program().command(), QIODevice::ReadWrite | QIODevice::Unbuffered);
		if ( ! input_process.waitForStarted()) {
			throw ErrorOpeningPipeFromGzipException(GPLATES_EXCEPTION_SOURCE,
					gunzip_program().command(), filename);
		}
		input_process.waitForReadyRead(20000);
		reader.setDevice(&input_process);

	} else {
	
		if ( ! input_file.open(QIODevice::ReadOnly | QIODevice::Text)) 
		{
			throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
		}
		reader.setDevice(&input_file);
	}
	

	boost::shared_ptr<DataSource> source( 
			new LocalFileDataSource(filename, DataFormats::GpmlOnePointSix));

	GPlatesModel::FeatureCollectionHandle::weak_ref collection = file.get_feature_collection();

	GpmlReaderUtils::ReaderParams params(reader, source, read_errors);
	boost::shared_ptr<Model::XmlElementNode::AliasToNamespaceMap> alias_map(
			new Model::XmlElementNode::AliasToNamespaceMap);

	if (read_root_element(params, alias_map)) {
		// FIXME: This try block is temporary; any errors encountered in read_root_element() should
		// be appended to read_errors.
		try {
			// .atEnd() can not be relied upon when reading a QProcess,
			// so we must make sure we block for a moment to make sure
			// the process is ready to feed us data.
			reader.device()->waitForReadyRead(1000);
			while ( ! reader.atEnd()) {
				reader.readNext();
				if (reader.isEndElement()) {
					break;
				}
				if (reader.isStartElement()) {
					append_warning_if( 
						! qualified_names_are_equal(
								reader, XmlUtils::GML_NAMESPACE_QSTRING, "featureMember"), 
						// FIXME: What do I use for the XmlNode here?  Maybe append the error
						// manually instead?
						params, 
						ReadErrors::UnrecognisedFeatureCollectionElement,
						ReadErrors::ElementNameChanged);
					read_feature_member(params, model, alias_map, collection);
				}
				reader.device()->waitForReadyRead(1000);
			}
		} catch (std::exception &exc) {
			qWarning() << "GpmlOnePointSixReader.cc:read_root_element: " << exc.what();
		} catch(...) {
			qWarning() << "GpmlOnePointSixReader.cc:read_root_element: Unknown error";
		}

		if (reader.error()) {
			// The XML was malformed somewhere along the line.
			boost::shared_ptr<LocationInDataSource> loc(
					new LineNumber(reader.lineNumber()));
			read_errors.d_terminating_errors.push_back(
					ReadErrorOccurrence(source, loc,
						ReadErrors::ParseError, 
						ReadErrors::ParsingStoppedPrematurely));
		}
	}

	// Turns relative paths into absolute paths in all GmlFile instances.
	MakeFilePathsAbsoluteVisitor visitor(fileinfo.get_qfileinfo().absolutePath(), read_errors);
	for (GPlatesModel::FeatureCollectionHandle::iterator iter = collection->begin();
			iter != collection->end(); ++iter)
	{
		visitor.visit_feature(iter);
	}
}

