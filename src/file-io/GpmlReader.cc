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

#include "GpmlReader.h"

#include <fstream>
#include <sstream>
#include <string>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QtXml/QXmlStreamReader>


#include "ErrorOpeningFileForReadingException.h"
#include "ErrorOpeningPipeFromGzipException.h"
#include "ExternalProgram.h"
#include "FileInfo.h"
#include "GpmlFeatureReaderFactory.h"
#include "GpmlReaderUtils.h"
#include "ReadErrors.h"
#include "ReadErrorOccurrence.h"
#include "GpmlPropertyStructuralTypeReader.h"

#include "global/GPlatesAssert.h"

#include "model/ChangesetHandle.h"
#include "model/Gpgim.h"
#include "model/GpgimVersion.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/XmlElementName.h"
#include "model/XmlNode.h"

#include "property-values/GmlFile.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlScalarField3DFile.h"

#include "utils/Profile.h"
#include "utils/StringUtils.h"
#include "utils/UnicodeStringUtils.h"

const GPlatesFileIO::ExternalProgram &
GPlatesFileIO::GpmlReader::gunzip_program()
{
	if (s_gunzip_program == NULL) {
		s_gunzip_program = new ExternalProgram("gzip -d", "gzip --version");
	}
	return *s_gunzip_program;
}


const GPlatesFileIO::ExternalProgram *GPlatesFileIO::GpmlReader::s_gunzip_program = NULL;


namespace
{
	namespace IO = GPlatesFileIO;
	namespace Utils = IO::GpmlReaderUtils;
	namespace XmlUtils = GPlatesUtils::XmlNamespaces;
	namespace Model = GPlatesModel;


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
			const GPlatesUtils::UnicodeString &filename = gml_file.get_file_name()->get_value().get();
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
			const GPlatesUtils::UnicodeString &filename = gpml_scalar_field_3d_file.get_file_name()->get_value().get();
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
			BOOST_FOREACH(
					GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type time_window,
					gpml_piecewise_aggregation.time_windows())
			{
				time_window->time_dependent_value()->accept_visitor(*this);
			}
		}

	private:

		QString d_absolute_path;
		GPlatesFileIO::ReadErrorAccumulation &d_read_errors;
	};


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


	void
	read_feature(
			const Model::XmlElementNode::non_null_ptr_type &feature_xml_element,
			const IO::GpmlFeatureReaderFactory &feature_reader_factory,
			const Model::FeatureCollectionHandle::weak_ref &feature_collection,
			Utils::ReaderParams &params)
	{
		// XXX: It's probable that we may wish to in some way preserve any 
		// attributes a feature has, even though we won't use them.
		append_warning_if( ! feature_xml_element->attributes_empty(),
				feature_xml_element,
				params,
				IO::ReadErrors::UnexpectedNonEmptyAttributeList,
				IO::ReadErrors::AttributesIgnored);
	
		const Model::FeatureType feature_type(feature_xml_element->get_name());

		// Get the feature reader associated with the feature type.
		IO::GpmlFeatureReaderInterface feature_reader =
				feature_reader_factory.get_feature_reader(feature_type);

		// Create and read a new feature from the GPML file (from the already-read-in XML feature node).
		GPlatesModel::FeatureHandle::non_null_ptr_type feature =
				feature_reader.read_feature(feature_xml_element, params);

		// Add the new feature to the feature collection.
		feature_collection->add(feature);
	}


	void
	read_feature_member(
			Utils::ReaderParams &params,
			const IO::GpmlFeatureReaderFactory &feature_reader_factory,
			const Model::FeatureCollectionHandle::weak_ref &feature_collection,
			const boost::shared_ptr<Model::XmlElementNode::AliasToNamespaceMap> &alias_map)
	{
		QXmlStreamReader &reader = params.reader;
		// .atEnd() can not be relied upon when reading a QProcess,
		// so we must make sure we block for a moment to make sure
		// the process is ready to feed us data.
		reader.device()->waitForReadyRead(1000);
		while ( ! reader.atEnd())
		{
			reader.readNext();
			if (reader.isEndElement())
			{
				break;
			}
			if (reader.isStartElement())
			{
				Model::XmlElementNode::non_null_ptr_type feature_xml_element = 
						Model::XmlElementNode::create(reader, alias_map);
				read_feature(feature_xml_element, feature_reader_factory, feature_collection, params);
			}
			reader.device()->waitForReadyRead(1000);
		}
	}


	boost::optional<Model::GpgimVersion>
	read_root_element(
			Utils::ReaderParams &params,
			boost::shared_ptr<Model::XmlElementNode::AliasToNamespaceMap> alias_map)
	{
		QXmlStreamReader &reader = params.reader;

		// .atEnd() can not be relied upon when reading a QProcess,
		// so we must make sure we block for a moment to make sure
		// the process is ready to feed us data.
		reader.device()->waitForReadyRead(1000);
		if (Utils::append_failure_to_begin_if(
			reader.atEnd(), params, IO::ReadErrors::FileIsEmpty, IO::ReadErrors::FileNotLoaded))
		{
			return boost::none;
		}

		// Skip over the <?xml ... ?> stuff.
		// .atEnd() can not be relied upon when reading a QProcess,
		// so we must make sure we block for a moment to make sure
		// the process is ready to feed us data.
		reader.device()->waitForReadyRead(1000);
		while ( ! reader.atEnd())
		{
			reader.readNext();
			if (reader.isStartElement())
			{
				break;
			}
			reader.device()->waitForReadyRead(1000);
		}

		if (Utils::append_failure_to_begin_if(
			reader.atEnd(), params, IO::ReadErrors::FileIsEmpty, IO::ReadErrors::FileNotLoaded))
		{
			return boost::none;
		}

		static const Model::XmlElementName FEATURE_COLLECTION = 
				Model::XmlElementName::create_gpml("FeatureCollection");
		Model::XmlElementName current_element(
				reader.namespaceUri().toString(), 
				reader.name().toString());

		QXmlStreamNamespaceDeclarations ns_decls = reader.namespaceDeclarations();
		QXmlStreamNamespaceDeclarations::iterator
			iter = ns_decls.begin(),
			end = ns_decls.end();
		for ( ; iter != end; ++ iter)
		{
			alias_map->insert(
					std::make_pair(
						iter->prefix().toString(),
						iter->namespaceUri().toString()));
		}
		
		append_warning_if(current_element != FEATURE_COLLECTION,
				params,
				IO::ReadErrors::IncorrectRootElementName,
				IO::ReadErrors::ElementNameChanged);

		//
		// Determine the GPGIM version that was used to write the GPML file.
		//
		boost::optional<Model::GpgimVersion> gpml_version;

		const QStringRef file_version_string = reader.attributes().value(
				XmlUtils::get_gpml_namespace_qstring(), "version");
		if (file_version_string == "")
		{
			append_warning(params,
					IO::ReadErrors::MissingVersionAttribute,
					IO::ReadErrors::AssumingCurrentVersion);
		}
		else // Determine the GPGIM version...
		{
			gpml_version = Model::GpgimVersion::create(file_version_string.toString());

			if (!gpml_version)
			{
				// Could not parse the version string.
				append_warning(params,
						IO::ReadErrors::MalformedVersionAttribute,
						IO::ReadErrors::AssumingCurrentVersion);
			}
			else
			{
				// Append warning if the GPML file was created by a more recent version of GPlates.
				append_warning_if(gpml_version.get() > GPlatesModel::Gpgim::instance().get_version(),
						params,
						IO::ReadErrors::PartiallySupportedVersionAttribute,
						IO::ReadErrors::AssumingCurrentVersion);
			}
		}

		// Default to version the base 1.6 version if the GPGIM version could not be obtained.
		if (!gpml_version)
		{
			gpml_version = Model::GpgimVersion(1, 6, Model::GpgimVersion::DEFAULT_ONE_POINT_SIX_REVISION);
		}

		return gpml_version;
	}
}


void
GPlatesFileIO::GpmlReader::read_file(
		File::Reference &file,
		const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
		ReadErrorAccumulation &read_errors,
		bool &contains_unsaved_changes,
		bool use_gzip)
{
	PROFILE_FUNC();

	contains_unsaved_changes = false;

	const FileInfo &fileinfo = file.get_file_info();

	QString filename(fileinfo.get_qfileinfo().filePath());
	QXmlStreamReader reader;

	QProcess input_process;
	QFile input_file(filename);
	if (use_gzip)
	{
		// gzipped, assuming gzipped GPML.
		// Set up the gzip process.
		input_process.setStandardInputFile(filename);

		// FIXME: Assuming gzip is in a standard place on the path. Not true on MS/Win32. Not true at all.
		// In fact, it may need to be a user preference.
		input_process.start(gunzip_program().command(), QIODevice::ReadWrite | QIODevice::Unbuffered);

		// Checking the error code is a workaround for a Qt bug that causes a crash on Mac/Unix
		// when the input file does not exist - see https://bugreports.qt.io/browse/QTBUG-33021.
		// Without the bug only QProcess::waitForStarted() needs to be called.
		if (input_process.error() != QProcess::UnknownError ||
			!input_process.waitForStarted())
		{
			throw ErrorOpeningPipeFromGzipException(GPLATES_EXCEPTION_SOURCE,
					gunzip_program().command(), filename);
		}
		input_process.waitForReadyRead(20000);
		reader.setDevice(&input_process);

	}
	else
	{
		if ( ! input_file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
		}
		reader.setDevice(&input_file);
	}
	

	boost::shared_ptr<DataSource> source( 
			new LocalFileDataSource(filename, DataFormats::Gpml));

	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = file.get_feature_collection();

	GpmlReaderUtils::ReaderParams params(reader, source, read_errors, contains_unsaved_changes);
	boost::shared_ptr<GPlatesModel::XmlElementNode::AliasToNamespaceMap> alias_map(
			new GPlatesModel::XmlElementNode::AliasToNamespaceMap);

	// Read the root element and get the GPGIM version that was used to write the GPML file.
	const boost::optional<GPlatesModel::GpgimVersion> gpml_version =
			read_root_element(params, alias_map);
	if (gpml_version)
	{
		// Store the GPGIM version in the feature collection as a tag.
		//
		// This is so other areas of the code can query the version.
		// If a feature collection does not contain this tag (eg, some other area of GPlates creates
		// a feature collection) then it should be assumed to be current GPGIM version since new
		// (empty) feature collections created by this instance of GPlates will have features added
		// according to the GPGIM version built into this instance of GPlates.
		feature_collection->tags()[GPlatesModel::GpgimVersion::FEATURE_COLLECTION_TAG] = gpml_version.get();

		// Create a GPML feature reader factory that matches the GPGIM version in the GPML file.
		const GpmlFeatureReaderFactory feature_reader_factory(
				property_structural_type_reader, gpml_version.get());

		// .atEnd() can not be relied upon when reading a QProcess,
		// so we must make sure we block for a moment to make sure
		// the process is ready to feed us data.
		reader.device()->waitForReadyRead(1000);
		while ( ! reader.atEnd())
		{
			reader.readNext();
			if (reader.isEndElement())
			{
				break;
			}
			if (reader.isStartElement())
			{
				append_warning_if( 
					! qualified_names_are_equal(
							reader, XmlUtils::get_gml_namespace_qstring(), "featureMember"), 
					// FIXME: What do I use for the XmlNode here?  Maybe append the error
					// manually instead?
					params, 
					ReadErrors::UnrecognisedFeatureCollectionElement,
					ReadErrors::ElementNameChanged);
				read_feature_member(params, feature_reader_factory, feature_collection, alias_map);
			}
			reader.device()->waitForReadyRead(1000);
		}
	}

	if (reader.error())
	{
		// The XML was malformed somewhere along the line.
		boost::shared_ptr<LocationInDataSource> loc(
				new LineNumber(reader.lineNumber()));
		read_errors.d_terminating_errors.push_back(
				ReadErrorOccurrence(source, loc,
					ReadErrors::ParseError, 
					ReadErrors::ParsingStoppedPrematurely));
	}

	// Turns relative paths into absolute paths in all GmlFile instances.
	MakeFilePathsAbsoluteVisitor visitor(fileinfo.get_qfileinfo().absolutePath(), read_errors);
	for (GPlatesModel::FeatureCollectionHandle::iterator iter = feature_collection->begin();
			iter != feature_collection->end(); ++iter)
	{
		visitor.visit_feature(iter);
	}
}

