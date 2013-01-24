/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QByteArray>
#include <QDebug>
#include <QFile>

#include "FeatureCollectionFileFormatRegistry.h"

#include "ArbitraryXmlReader.h"
#include "ErrorOpeningFileForReadingException.h"
#include "ExternalProgram.h"
#include "FeatureCollectionFileFormatConfigurations.h"
#include "FileFormatNotSupportedException.h"
#include "FileInfo.h"
#include "GeoscimlProfile.h"
#include "GmapReader.h"
#include "GMTFormatWriter.h"
#include "GpmlOutputVisitor.h"
#include "GpmlReader.h"
#include "GpmlPropertyStructuralTypeReader.h"
#include "PlatesLineFormatReader.h"
#include "PlatesLineFormatWriter.h"
#include "PlatesRotationFormatReader.h"
#include "PlatesRotationFormatWriter.h"
#include "ShapefileReader.h"
#include "OgrFeatureCollectionWriter.h"

#include "app-logic/AppLogicUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		namespace
		{
			enum FileMagic
			{
				FILE_MAGIC_UNKNOWN, FILE_MAGIC_XML, FILE_MAGIC_GZIP
			};

			/**
			 * Returns file type (or @a FILE_MAGIC_UNKNOWN if not recognised), or boost::none
			 * if unable to open file for reading.
			 */
			boost::optional<FileMagic>
			identify_gpml_or_gpmlz_by_magic_number(
					const QFileInfo& file_info)
			{
				static const QByteArray MAGIC_UTF8 = QByteArray::fromHex("efbbbf");
				static const QByteArray MAGIC_UTF16_BIG_ENDIAN = QByteArray::fromHex("feff");
				static const QByteArray MAGIC_UTF16_LITTLE_ENDIAN = QByteArray::fromHex("fffe");
				static const QByteArray MAGIC_GZIP = QByteArray::fromHex("1f8b");
				static const QByteArray MAGIC_XML = QByteArray("<?xml");

				QString filename(file_info.filePath());
				QFile f(filename);

				// If we can't open the file then maybe it doesn't exist yet so we
				// report UNKNOWN format instead of throwing an exception.
				if ( ! f.open(QIODevice::ReadOnly)) 
				{
					return boost::none;
				}

				FileMagic magic = FILE_MAGIC_UNKNOWN;

				// Skip over any Unicode byte-order-marks.
				if (f.peek(MAGIC_UTF8.size()) == MAGIC_UTF8) {
					f.read(MAGIC_UTF8.size());
				} else if (f.peek(MAGIC_UTF16_BIG_ENDIAN.size()) == MAGIC_UTF16_BIG_ENDIAN) {
					f.read(MAGIC_UTF16_BIG_ENDIAN.size());
				} else if (f.peek(MAGIC_UTF16_LITTLE_ENDIAN.size()) == MAGIC_UTF16_LITTLE_ENDIAN) {
					f.read(MAGIC_UTF16_LITTLE_ENDIAN.size());
				}

				// Look for magic.
				if (f.peek(MAGIC_GZIP.size()) == MAGIC_GZIP) {
					magic = FILE_MAGIC_GZIP;
				} else if (f.peek(MAGIC_XML.size()) == MAGIC_XML) {
					magic = FILE_MAGIC_XML;
				}

				f.close();
				return magic;
			}


			//
			// Filename extensions for the various file formats.
			//

			const QString FILE_FORMAT_EXT_GPML = "gpml";
			const QString FILE_FORMAT_EXT_GPMLZ = "gpmlz";
			const QString FILE_FORMAT_EXT_GPMLZ_ALTERNATIVE = "gpml.gz";
			const QString FILE_FORMAT_EXT_PLATES4_LINE = "dat";
			const QString FILE_FORMAT_EXT_PLATES4_LINE_ALTERNATIVE = "pla";
			const QString FILE_FORMAT_EXT_PLATES4_ROTATION = "rot";
			const QString FILE_FORMAT_EXT_SHAPEFILE = "shp";
			const QString FILE_FORMAT_EXT_OGRGMT = "gmt";
			const QString FILE_FORMAT_EXT_WRITE_ONLY_XY_GMT = "xy";
			const QString FILE_FORMAT_EXT_GMAP = "vgp";
			const QString FILE_FORMAT_EXT_GSML = "gsml";


			/**
			 * Returns true if the filename of @a file_info ends with @a suffix.
			 */
			bool
			file_name_ends_with(
					const QFileInfo &file_info,
					const QString &suffix)
			{
				return file_info.completeSuffix().endsWith(QString(suffix), Qt::CaseInsensitive);
			}


			bool
			is_gpml_format_file(
					const QFileInfo &file_info,
					const QString &filename_extension)
			{
				if (!file_name_ends_with(file_info, filename_extension))
				{
					return false;
				}

				boost::optional<FileMagic> file_magic = identify_gpml_or_gpmlz_by_magic_number(file_info);
				if (file_magic &&
					(file_magic.get() == FILE_MAGIC_GZIP))
				{
					// We're ok with the file type being unknown.
					return false;
				}

				// NOTE: This includes the case where boost::none was returned meaning
				// that the file could not be read in which case we're fine with just the
				// filename extension matching.
				return true;
			}

			bool
			is_gpmlz_format_file(
					const QFileInfo &file_info,
					const QString &filename_extension)
			{
				if (!file_name_ends_with(file_info, filename_extension))
				{
					return false;
				}

				boost::optional<FileMagic> file_magic = identify_gpml_or_gpmlz_by_magic_number(file_info);
				if (file_magic &&
					(file_magic.get() != FILE_MAGIC_GZIP))
				{
					// The file type, if determined, must match gzip.
					return false;
				}

				// NOTE: This includes the case where boost::none was returned meaning
				// that the file could not be read in which case we're fine with just the
				// filename extension matching.

				// Test if we can offer on-the-fly gzip decompression/compression.
				// FIXME: Ideally we should let the user know WHY we're concealing this option.
				// The user will still be able to type in a .gpml.gz file name and activate the
				// gzipped saving code, however this will produce an exception which pops up
				// a suitable message box (See ViewportWindow.cc).
				//
				// There's also 'gunzip_program' but testing either determines if both are available.
				return GpmlOutputVisitor::gzip_program().test();
			}


			/**
			 * Reads a SHAPEFILE feature collection.
			 */
			void
			shapefile_read_feature_collection(
					File::Reference &file_ref,
					const Registry &file_format_registry,
					GPlatesModel::ModelInterface &model,
					const GPlatesModel::Gpgim &gpgim,
					ReadErrorAccumulation &read_errors)
			{
				// Get the current default OGR configuration in case file does not have one.
				boost::optional<FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_to_const_type>
						ogr_file_configuration =
								FeatureCollectionFileFormat::dynamic_cast_configuration<
										const FeatureCollectionFileFormat::OGRConfiguration>(
												file_format_registry.get_default_configuration(SHAPEFILE));
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						ogr_file_configuration,
						GPLATES_ASSERTION_SOURCE);

				ShapefileReader::read_file(file_ref, ogr_file_configuration.get(), model, gpgim, read_errors);
			}


			/**
			 * Reads a GSML feature collection.
			 */
			void
			gsml_read_feature_collection(
					File::Reference &file_ref,
					GPlatesModel::ModelInterface &model,
					const GPlatesModel::Gpgim &gpgim,
					ReadErrorAccumulation &read_errors)
			{
				ArbitraryXmlReader::instance()->read_file(
						file_ref,
						boost::shared_ptr<ArbitraryXmlProfile>(new GeoscimlProfile()),
						model,
						gpgim,
						read_errors);
			}


			/**
			 * Creates a GPML feature visitor writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_gpml_feature_collection_writer(
					File::Reference &file_ref,
					const GPlatesModel::Gpgim &gpgim)
			{
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new GpmlOutputVisitor(file_ref.get_file_info(), gpgim, false/*use_gzip*/));
			}

			/**
			 * Creates a GPMLZ feature visitor writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_gpmlz_feature_collection_writer(
					File::Reference &file_ref,
					const GPlatesModel::Gpgim &gpgim)
			{
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new GpmlOutputVisitor(file_ref.get_file_info(), gpgim, true/*use_gzip*/));
			}
			/**
			 * Creates a PLATES4_LINE feature visitor writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_plates_line_feature_collection_writer(
					File::Reference &file_ref,
					const GPlatesModel::Gpgim &gpgim)
			{
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new PlatesLineFormatWriter(file_ref.get_file_info(), gpgim));
			}

			/**
			 * Creates a PLATES4_ROTATION feature visitor writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_plates_rotation_feature_collection_writer(
					File::Reference &file_ref,
					const GPlatesModel::Gpgim &gpgim)
			{
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new PlatesRotationFormatWriter(file_ref.get_file_info(), gpgim));
			}

			/**
			 * Creates a feature visitor writer for the SHAPEFILE and OGRGMT file formats.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_ogr_feature_collection_writer(
					File::Reference &file_ref,
					const Registry &file_format_registry,
					Format file_format,
					const GPlatesModel::Gpgim &gpgim)
			{
				// Get the current default OGR configuration in case file does not have one.
				boost::optional<FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_to_const_type>
						ogr_file_configuration =
								FeatureCollectionFileFormat::dynamic_cast_configuration<
										const FeatureCollectionFileFormat::OGRConfiguration>(
												file_format_registry.get_default_configuration(file_format));
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						ogr_file_configuration,
						GPLATES_ASSERTION_SOURCE);

				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new OgrFeatureCollectionWriter(file_ref, ogr_file_configuration.get(), gpgim));
			}

			/**
			 * Creates a feature visitor writer for the old write-only ".xy" GMT format.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_write_only_xy_gmt_feature_collection_writer(
					File::Reference &file_ref,
					const Registry &file_format_registry,
					const GPlatesModel::Gpgim &gpgim)
			{
				// Get the current default GMT configuration in case file does not have one.
				boost::optional<FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type>
						gmt_file_configuration =
								FeatureCollectionFileFormat::dynamic_cast_configuration<
										const FeatureCollectionFileFormat::GMTConfiguration>(
												file_format_registry.get_default_configuration(
														FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT));
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						gmt_file_configuration,
						GPLATES_ASSERTION_SOURCE);

				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new GMTFormatWriter(file_ref, gmt_file_configuration.get(), gpgim));
			}
		}
	}
}


void
GPlatesFileIO::FeatureCollectionFileFormat::Registry::register_file_format(
		Format file_format,
		const QString &short_description,
		const std::vector<QString> &filename_extensions,
		const classifications_type &feature_classification,
		const is_file_format_function_type &is_file_format_function,
		const boost::optional<read_feature_collection_function_type> &read_feature_collection_function,
		const boost::optional<create_feature_collection_writer_function_type> &create_feature_collection_writer_function,
		const boost::optional<Configuration::shared_ptr_to_const_type> &default_configuration)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!filename_extensions.empty(),
			GPLATES_ASSERTION_SOURCE);

	d_file_format_info_map.insert(
			std::make_pair(
				file_format,
				FileFormatInfo(
					short_description,
					filename_extensions,
					feature_classification,
					is_file_format_function,
					read_feature_collection_function,
					create_feature_collection_writer_function,
					default_configuration)));
}


void
GPlatesFileIO::FeatureCollectionFileFormat::Registry::unregister_file_format(
		Format file_format)
{
	d_file_format_info_map.erase(file_format);
}


std::vector<GPlatesFileIO::FeatureCollectionFileFormat::Format>
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_registered_file_formats() const
{
	std::vector<Format> file_formats;

	BOOST_FOREACH(
			const file_format_info_map_type::value_type &file_format_entry,
			d_file_format_info_map)
	{
		file_formats.push_back(file_format_entry.first);
	}

	return file_formats;
}


boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Format>
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_file_format(
		const QFileInfo& file_info) const
{
	BOOST_FOREACH(
			const file_format_info_map_type::value_type &file_format_entry,
			d_file_format_info_map)
	{
		const FileFormatInfo &file_format_info = file_format_entry.second;

		//
		// See if the file is recognised by the current file format.
		//

		// Iterate over the filename extensions supported by the current file format.
		std::vector<QString>::const_iterator ext_iter = file_format_info.filename_extensions.begin();
		std::vector<QString>::const_iterator ext_end = file_format_info.filename_extensions.end();
		for ( ; ext_iter != ext_end; ++ext_iter)
		{
			const QString &filename_extension = *ext_iter;

			if (file_format_info.is_file_format_function(file_info, filename_extension))
			{
				const Format file_format = file_format_entry.first;
				return file_format;
			}
		}
	}

	return boost::none;
}


bool
GPlatesFileIO::FeatureCollectionFileFormat::Registry::does_file_format_support_reading(
		Format file_format) const
{
	return get_file_format_info(file_format).read_feature_collection_function;
}


bool
GPlatesFileIO::FeatureCollectionFileFormat::Registry::does_file_format_support_writing(
		Format file_format) const
{
	return get_file_format_info(file_format).create_feature_collection_writer_function;
}


const QString &
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_short_description(
		Format file_format) const
{
	return get_file_format_info(file_format).short_description;
}


QString
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_primary_filename_extension(
		Format file_format) const
{
	// The first listed filename extension is the primary extension.
	return get_all_filename_extensions(file_format).front();
}


const std::vector<QString> &
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_all_filename_extensions(
		Format file_format) const
{
	return get_file_format_info(file_format).filename_extensions;
}


GPlatesFileIO::FeatureCollectionFileFormat::classifications_type
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_feature_classification(
		Format file_format) const
{
	return get_file_format_info(file_format).feature_classification;
}


void
GPlatesFileIO::FeatureCollectionFileFormat::Registry::read_feature_collection(
		File::Reference &file_ref,
		ReadErrorAccumulation &read_errors) const
{
	const boost::optional<Format> file_format = get_file_format(file_ref.get_file_info().get_qfileinfo());
	if (!file_format)
	{

		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE,
				("No registered file formats for this file: " +
				file_ref.get_file_info().get_display_name(true)).toStdString().c_str());
	}

	const FileFormatInfo &file_format_info = get_file_format_info(file_format.get());

	// If there's no reader then don't read anything.
	if (!file_format_info.read_feature_collection_function)
	{
		// We shouldn't really get here since the caller should have checked 'does_file_format_support_reading()'.
		qWarning() << "Reading feature collections from file's with extension '."
			<< file_format_info.filename_extensions.front() << "' is not currently supported.";
		return;
	}
	const read_feature_collection_function_type &read_feature_collection_function =
			file_format_info.read_feature_collection_function.get();

	try
	{
		read_feature_collection_function(file_ref, read_errors);
	}
	catch (ErrorOpeningFileForReadingException &e)
	{
		// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
		// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
		// QFileDialog support.
		boost::shared_ptr<DataSource> e_source(
				new LocalFileDataSource(
						e.filename(), DataFormats::Unspecified));
		boost::shared_ptr<LocationInDataSource> e_location(
				new LineNumber(0));
		read_errors.d_failures_to_begin.push_back(
				ReadErrorOccurrence(
						e_source,
						e_location,
						ReadErrors::ErrorOpeningFileForReading,
						ReadErrors::FileNotLoaded));

		// Rethrow the exception to let caller know that an error occurred.
		// This is important because the caller is expecting a valid feature collection
		// unless an exception is thrown so if we don't throw one then the caller
		// will try to dereference the feature collection and crash.
		throw;
	}
}


void
GPlatesFileIO::FeatureCollectionFileFormat::Registry::write_feature_collection(
		File::Reference &file_ref) const
{
	const boost::optional<Format> file_format = get_file_format(file_ref.get_file_info().get_qfileinfo());
	if (!file_format)
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE,
				"No registered file formats for this file.");
	}

	const FileFormatInfo &file_format_info = get_file_format_info(file_format.get());

	// If there's no writer then don't write anything.
	if (!file_format_info.create_feature_collection_writer_function)
	{
		// We shouldn't really get here since the caller should have checked 'does_file_format_support_writing()'.
		qWarning() << "Writing feature collections to file's with extension '."
			<< file_format_info.filename_extensions.front() << "' is not currently supported.";
		return;
	}
	const create_feature_collection_writer_function_type &create_feature_collection_writer_function =
			file_format_info.create_feature_collection_writer_function.get();

	// Create the feature collection writer.
	const boost::shared_ptr<GPlatesModel::ConstFeatureVisitor> feature_collection_writer =
			create_feature_collection_writer_function(file_ref);

	// Write the feature collection.
	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			file_ref.get_feature_collection(),
			*feature_collection_writer);
}


const boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> &
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_default_configuration(
		Format file_format) const
{
	return get_file_format_info(file_format).default_configuration;
}


void
GPlatesFileIO::FeatureCollectionFileFormat::Registry::set_default_configuration(
		Format file_format,
		const Configuration::shared_ptr_to_const_type &default_read_write_options)
{
	get_file_format_info(file_format).default_configuration = default_read_write_options;
}


const GPlatesFileIO::FeatureCollectionFileFormat::Registry::FileFormatInfo &
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_file_format_info(
		Format file_format) const
{
	file_format_info_map_type::const_iterator iter = d_file_format_info_map.find(file_format);
	if (iter == d_file_format_info_map.end())
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE,
				"Chosen feature collection file format has not been registered.");
	}

	return iter->second;
}


GPlatesFileIO::FeatureCollectionFileFormat::Registry::FileFormatInfo &
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_file_format_info(
		Format file_format)
{
	file_format_info_map_type::iterator iter = d_file_format_info_map.find(file_format);
	if (iter == d_file_format_info_map.end())
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE,
				"Chosen feature collection file format has not been registered.");
	}

	return iter->second;
}


void
GPlatesFileIO::FeatureCollectionFileFormat::register_default_file_formats(
		Registry &registry,
		GPlatesModel::ModelInterface model,
		const GPlatesModel::Gpgim &gpgim)
{
	// Used to read structural types from a GPML file.
	GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type
			gpml_property_structural_type_reader =
					GpmlPropertyStructuralTypeReader::create(gpgim);

	classifications_type gpml_classification;
	gpml_classification.set(); // Set all flags - GPML can handle everything.
	registry.register_file_format(
			GPML,
			"GPlates Markup Language",
			std::vector<QString>(1, FILE_FORMAT_EXT_GPML),
			gpml_classification,
			&is_gpml_format_file,
			Registry::read_feature_collection_function_type(
					boost::bind(&GpmlReader::read_file,
							_1, model, boost::cref(gpgim), gpml_property_structural_type_reader, _2, false)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_gpml_feature_collection_writer, _1, boost::cref(gpgim))),
			// No configuration options yet for this file format...
			boost::none);

	classifications_type gpmlz_classification;
	gpmlz_classification.set(); // Set all flags - GPMLZ can handle everything.
	std::vector<QString> gpmlz_filename_extensions;
	gpmlz_filename_extensions.push_back(FILE_FORMAT_EXT_GPMLZ);
	gpmlz_filename_extensions.push_back(FILE_FORMAT_EXT_GPMLZ_ALTERNATIVE);
	registry.register_file_format(
			GPMLZ,
			"Compressed GPML",
			gpmlz_filename_extensions,
			gpmlz_classification,
			&is_gpmlz_format_file,
			Registry::read_feature_collection_function_type(
					boost::bind(&GpmlReader::read_file,
							_1, model, boost::cref(gpgim), gpml_property_structural_type_reader, _2, true)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_gpmlz_feature_collection_writer, _1, boost::cref(gpgim))),
			// No configuration options yet for this file format...
			boost::none);

	classifications_type plate4_line_classification;
	plate4_line_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	std::vector<QString> plate4_line_filename_extensions;
	plate4_line_filename_extensions.push_back(FILE_FORMAT_EXT_PLATES4_LINE);
	plate4_line_filename_extensions.push_back(FILE_FORMAT_EXT_PLATES4_LINE_ALTERNATIVE);
	registry.register_file_format(
			PLATES4_LINE,
			"PLATES4 line",
			plate4_line_filename_extensions,
			plate4_line_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&PlatesLineFormatReader::read_file, _1, model, boost::cref(gpgim), _2)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_plates_line_feature_collection_writer, _1, boost::cref(gpgim))),
			// No configuration options yet for this file format...
			boost::none);

	classifications_type plate4_rotation_classification;
	plate4_rotation_classification.set(RECONSTRUCTION);
	registry.register_file_format(
			PLATES4_ROTATION,
			"PLATES4 rotation",
			std::vector<QString>(1, FILE_FORMAT_EXT_PLATES4_ROTATION),
			plate4_rotation_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&PlatesRotationFormatReader::read_file, _1, model, boost::cref(gpgim), _2)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_plates_rotation_feature_collection_writer, _1, boost::cref(gpgim))),
			// No configuration options yet for this file format...
			boost::none);

	classifications_type shapefile_classification;
	shapefile_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	shapefile_classification.set(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	// FIXME: Should load this up with the standard GPlates model-to-attribute mapping.
	Configuration::shared_ptr_to_const_type shapefile_default_configuration(new OGRConfiguration(SHAPEFILE));
	registry.register_file_format(
			SHAPEFILE,
			"ESRI shapefile",
			std::vector<QString>(1, FILE_FORMAT_EXT_SHAPEFILE),
			shapefile_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&shapefile_read_feature_collection,
							_1, boost::cref(registry), model, boost::cref(gpgim), _2)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_ogr_feature_collection_writer,
							_1, boost::cref(registry), SHAPEFILE, boost::cref(gpgim))),
			shapefile_default_configuration);

	classifications_type ogr_gmt_classification;
	ogr_gmt_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	ogr_gmt_classification.set(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	// FIXME: Should load this up with the standard GPlates model-to-attribute mapping.
	Configuration::shared_ptr_to_const_type ogr_gmt_default_configuration(new OGRConfiguration(OGRGMT));
	registry.register_file_format(
			OGRGMT,
			"OGR GMT",
			std::vector<QString>(1, FILE_FORMAT_EXT_OGRGMT),
			ogr_gmt_classification,
			&file_name_ends_with,
			// Reading not currently supported...
			boost::none,
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_ogr_feature_collection_writer,
							_1, boost::cref(registry), OGRGMT, boost::cref(gpgim))),
			ogr_gmt_default_configuration);

	classifications_type write_only_gmt_classification;
	write_only_gmt_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	write_only_gmt_classification.set(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	Configuration::shared_ptr_to_const_type write_only_gmt_default_configuration(new GMTConfiguration());
	registry.register_file_format(
			WRITE_ONLY_XY_GMT,
			"GMT xy",
			std::vector<QString>(1, FILE_FORMAT_EXT_WRITE_ONLY_XY_GMT),
			write_only_gmt_classification,
			&file_name_ends_with,
			// Reading not currently supported...
			boost::none,
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_write_only_xy_gmt_feature_collection_writer,
							_1, boost::cref(registry), boost::cref(gpgim))),
			write_only_gmt_default_configuration);

	classifications_type gmap_classification;
	gmap_classification.set(GPlatesAppLogic::ReconstructMethod::VIRTUAL_GEOMAGNETIC_POLE);
	registry.register_file_format(
			GMAP,
			"GMAP Virtual Geomagnetic Poles",
			std::vector<QString>(1, FILE_FORMAT_EXT_GMAP),
			gmap_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&GmapReader::read_file, _1, model, boost::cref(gpgim), _2)),
			// Writing not currently supported...
			boost::none,
			// No configuration options yet for this file format...
			boost::none);

	classifications_type gsml_classification;
	gsml_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	registry.register_file_format(
			GSML,
			"GeoSciML",
			std::vector<QString>(1, FILE_FORMAT_EXT_GSML),
			gsml_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&gsml_read_feature_collection, _1, model, boost::cref(gpgim), _2)),
			// Writing not currently supported...
			boost::none,
			// No configuration options yet for this file format...
			boost::none);
}
