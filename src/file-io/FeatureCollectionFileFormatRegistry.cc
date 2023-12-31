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

#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QByteArray>
#include <QDebug>
#include <QFile>

#include "FeatureCollectionFileFormatRegistry.h"

#include "ArbitraryXmlReader.h"
#include "ErrorOpeningFileForReadingException.h"
#include "ErrorOpeningPipeFromGzipException.h"
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
#include "PlatesRotationFileProxy.h"
#include "PlatesRotationFormatReader.h"
#include "PlatesRotationFormatWriter.h"
#include "OgrReader.h"
#include "OgrFeatureCollectionWriter.h"

#include "app-logic/AppLogicUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"
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
			const QString FILE_FORMAT_EXT_GPLATES_ROTATION = "grot";
			const QString FILE_FORMAT_EXT_SHAPEFILE = "shp";
			const QString FILE_FORMAT_EXT_OGRGMT = "gmt";
			const QString FILE_FORMAT_EXT_GEOJSON = "geojson";
			const QString FILE_FORMAT_EXT_GEOJSON_ALTERNATIVE = "json";
			const QString FILE_FORMAT_EXT_GEOPACKAGE = "gpkg";
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

				return true;
			}


			/**
			 * Reads an OGR-supported feature collection.
			 */
			void
			ogr_read_feature_collection(
					File::Reference &file_ref,
					const Registry &file_format_registry,
					Format file_format,
					ReadErrorAccumulation &read_errors,
					bool &contains_unsaved_changes)
			{
				// Get the current default OGR configuration in case file does not have one.
				boost::optional<FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_to_const_type>
						default_ogr_file_configuration =
								FeatureCollectionFileFormat::dynamic_cast_configuration<
										const FeatureCollectionFileFormat::OGRConfiguration>(
												file_format_registry.get_default_configuration(file_format));
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						default_ogr_file_configuration,
						GPLATES_ASSERTION_SOURCE);

				OgrReader::read_file(file_ref, default_ogr_file_configuration.get(), read_errors, contains_unsaved_changes);
			}


			/**
			 * Reads a GPlates rotation (".grot") feature collection.
			 */
			void
			gplates_rotation_read_feature_collection(
					File::Reference &file_ref,
					const Registry &file_format_registry,
					ReadErrorAccumulation &read_errors,
					bool &contains_unsaved_changes)
			{
				// Note that we're not passing in the default configuration because each configuration
				// is specific to a particular rotation file.
				RotationFileReader::read_file(file_ref, read_errors, contains_unsaved_changes);
			}


			/**
			 * Reads a GSML feature collection.
			 */
			void
			gsml_read_feature_collection(
					File::Reference &file_ref,
					ReadErrorAccumulation &read_errors,
					bool &contains_unsaved_changes)
			{
				ArbitraryXmlReader::instance()->read_file(
						file_ref,
						boost::shared_ptr<ArbitraryXmlProfile>(new GeoscimlProfile()),
						read_errors,
						contains_unsaved_changes);
			}


			/**
			 * Creates a GPML feature visitor writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_gpml_feature_collection_writer(
					File::Reference &file_ref)
			{
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new GpmlOutputVisitor(
								file_ref.get_file_info(),
								file_ref.get_feature_collection(),
								false/*use_gzip*/));
			}

			/**
			 * Creates a GPMLZ feature visitor writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_gpmlz_feature_collection_writer(
					File::Reference &file_ref)
			{
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new GpmlOutputVisitor(
								file_ref.get_file_info(),
								file_ref.get_feature_collection(),
								true/*use_gzip*/));
			}

			/**
			 * Creates a PLATES4_LINE feature visitor writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_plates_line_feature_collection_writer(
					File::Reference &file_ref)
			{
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new PlatesLineFormatWriter(file_ref.get_file_info()));
			}

			/**
			 * Creates a PLATES4_ROTATION feature visitor writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_plates_rotation_feature_collection_writer(
					File::Reference &file_ref)
			{
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new PlatesRotationFormatWriter(file_ref.get_file_info(), false/*grot_format*/));
			}


			/**
			 * Creates a GPlates rotation (".grot") file writer.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_gplates_rotation_feature_collection_writer(
					File::Reference &file_ref)
			{
				const boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> cfg = 
					file_ref.get_file_configuration();
				if(cfg)
				{
					boost::shared_ptr<const RotationFileConfiguration> rotation_cfg_const =
						boost::dynamic_pointer_cast<const RotationFileConfiguration>(*cfg);
					boost::shared_ptr<RotationFileConfiguration> rotation_cfg = 
						boost::const_pointer_cast<RotationFileConfiguration>(rotation_cfg_const);
					if(rotation_cfg)
					{
						boost::shared_ptr<GrotWriterWithCfg> writer = 
							rotation_cfg->get_rotation_file_proxy().create_file_writer(file_ref);
						if(writer)
						{
							return boost::dynamic_pointer_cast<GPlatesModel::ConstFeatureVisitor>(writer);
						}
					}
				}
				
				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new GrotWriterWithoutCfg(file_ref));
			}


			/**
			 * Creates a feature visitor writer for OGR-supported file formats.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_ogr_feature_collection_writer(
					File::Reference &file_ref,
					const Registry &file_format_registry,
					Format file_format)
			{
				// Get the current default OGR configuration in case file does not have one.
				boost::optional<FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_to_const_type>
						default_ogr_file_configuration =
								FeatureCollectionFileFormat::dynamic_cast_configuration<
										const FeatureCollectionFileFormat::OGRConfiguration>(
												file_format_registry.get_default_configuration(file_format));
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						default_ogr_file_configuration,
						GPLATES_ASSERTION_SOURCE);

				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new OgrFeatureCollectionWriter(file_ref, default_ogr_file_configuration.get()));
			}

			/**
			 * Creates a feature visitor writer for the old write-only ".xy" GMT format.
			 */
			boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
			create_write_only_xy_gmt_feature_collection_writer(
					File::Reference &file_ref,
					const Registry &file_format_registry)
			{
				// Get the current default GMT configuration in case file does not have one.
				boost::optional<FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type>
						default_gmt_file_configuration =
								FeatureCollectionFileFormat::dynamic_cast_configuration<
										const FeatureCollectionFileFormat::GMTConfiguration>(
												file_format_registry.get_default_configuration(
														FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT));
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						default_gmt_file_configuration,
						GPLATES_ASSERTION_SOURCE);

				return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
						new GMTFormatWriter(file_ref, default_gmt_file_configuration.get()));
			}
		}
	}
}


GPlatesFileIO::FeatureCollectionFileFormat::Registry::Registry(
		bool register_default_file_formats_)
{
	if (register_default_file_formats_)
	{
		register_default_file_formats();
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
	return static_cast<bool>(get_file_format_info(file_format).read_feature_collection_function);
}


bool
GPlatesFileIO::FeatureCollectionFileFormat::Registry::does_file_format_support_writing(
		Format file_format) const
{
	return static_cast<bool>(get_file_format_info(file_format).create_feature_collection_writer_function);
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
	return get_all_filename_extensions_for_format(file_format).front();
}


const std::vector<QString> &
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_all_filename_extensions_for_format(
		Format file_format) const
{
	return get_file_format_info(file_format).filename_extensions;
}


void
GPlatesFileIO::FeatureCollectionFileFormat::Registry::get_all_filename_extensions(
		std::vector<QString> &filename_extensions) const
{
	for (int f = 0; f < FeatureCollectionFileFormat::NUM_FORMATS; ++f)
	{
		const FeatureCollectionFileFormat::Format file_format =
				static_cast<FeatureCollectionFileFormat::Format>(f);

		file_format_info_map_type::const_iterator iter = d_file_format_info_map.find(file_format);
		if (iter == d_file_format_info_map.end())
		{
			// For some reason there is a gap in the file format enum values or
			// a value has not been registered.
			continue;
		}

		filename_extensions.insert(
				filename_extensions.end(),
				iter->second.filename_extensions.begin(),
				iter->second.filename_extensions.end());
	}
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
		ReadErrorAccumulation &read_errors,
		boost::optional<bool &> contains_unsaved_changes_opt) const
{
	const boost::optional<Format> file_format = get_file_format(file_ref.get_file_info().get_qfileinfo());
	if (!file_format)
	{
		// Add a read error before throwing an exception.
		read_errors.d_failures_to_begin.push_back(
				make_read_error_occurrence(
					file_ref.get_file_info().get_qfileinfo().filePath(),
					DataFormats::Unspecified,
					0/*line_num*/,
					ReadErrors::FileFormatNotSupported,
					ReadErrors::FileNotLoaded));

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
 		bool contains_unsaved_changes = false;
		read_feature_collection_function(file_ref, read_errors, contains_unsaved_changes);

		// Return unsaved changes state to caller if requested.
		if (contains_unsaved_changes_opt)
		{
			contains_unsaved_changes_opt.get() = contains_unsaved_changes;
		}
	}
	catch (ErrorOpeningFileForReadingException &e)
	{
		// FIXME: File readers should probably add a read error in addition to throwing
		// ErrorOpeningFileForReadingException so that we don't have to do the conversion.
		read_errors.d_failures_to_begin.push_back(
				make_read_error_occurrence(
					e.filename(),
					DataFormats::Unspecified,
					0/*line_num*/,
					ReadErrors::ErrorOpeningFileForReading,
					ReadErrors::FileNotLoaded));

		// Rethrow the exception to let caller know that an error occurred.
		// This is important because the caller is expecting a valid feature collection
		// unless an exception is thrown so if we don't throw one then the caller
		// will try to dereference the feature collection and crash.
		throw;
	}
	catch (ErrorOpeningPipeFromGzipException &e)
	{
		// FIXME: File readers should probably add a read error in addition to throwing
		// ErrorOpeningFileForReadingException so that we don't have to do the conversion.
		read_errors.d_failures_to_begin.push_back(
				make_read_error_occurrence(
					e.filename(),
					DataFormats::Gpml,
					0/*line_num*/,
					ReadErrors::ErrorOpeningFileForReading,
					ReadErrors::FileNotLoaded));

		// Rethrow the exception to let caller know that an error occurred.
		// This is important because the caller is expecting a valid feature collection
		// unless an exception is thrown so if we don't throw one then the caller
		// will try to dereference the feature collection and crash.
		throw;
	}
	catch (GPlatesGlobal::Exception &)
	{
		// We add a generic read error just in case the file reader didn't add a read error
		// when it threw an exception.
		read_errors.d_failures_to_begin.push_back(
				make_read_error_occurrence(
					file_ref.get_file_info().get_qfileinfo().filePath(),
					DataFormats::Unspecified,
					0/*line_num*/,
					ReadErrors::ErrorReadingFile,
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
GPlatesFileIO::FeatureCollectionFileFormat::Registry::register_default_file_formats()
{
	using namespace boost::placeholders;  // For _1, _2, etc

	// Used to read structural types from a GPML file.
	GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type
			gpml_property_structural_type_reader =
					GpmlPropertyStructuralTypeReader::create();

	classifications_type gpml_classification;
	gpml_classification.set(); // Set all flags - GPML can handle everything.
	register_file_format(
			GPML,
			"GPlates Markup Language",
			std::vector<QString>(1, FILE_FORMAT_EXT_GPML),
			gpml_classification,
			&is_gpml_format_file,
			Registry::read_feature_collection_function_type(
					boost::bind(&GpmlReader::read_file,
							_1, gpml_property_structural_type_reader, _2, _3, false)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_gpml_feature_collection_writer, _1)),
			// No configuration options yet for this file format...
			boost::none);

	classifications_type gpmlz_classification;
	gpmlz_classification.set(); // Set all flags - GPMLZ can handle everything.
	std::vector<QString> gpmlz_filename_extensions;
	gpmlz_filename_extensions.push_back(FILE_FORMAT_EXT_GPMLZ);
	gpmlz_filename_extensions.push_back(FILE_FORMAT_EXT_GPMLZ_ALTERNATIVE);
	register_file_format(
			GPMLZ,
			"Compressed GPML",
			gpmlz_filename_extensions,
			gpmlz_classification,
			&is_gpmlz_format_file,
			Registry::read_feature_collection_function_type(
					boost::bind(&GpmlReader::read_file,
							_1, gpml_property_structural_type_reader, _2, _3, true)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_gpmlz_feature_collection_writer, _1)),
			// No configuration options yet for this file format...
			boost::none);

	classifications_type plate4_line_classification;
	plate4_line_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	std::vector<QString> plate4_line_filename_extensions;
	plate4_line_filename_extensions.push_back(FILE_FORMAT_EXT_PLATES4_LINE);
	plate4_line_filename_extensions.push_back(FILE_FORMAT_EXT_PLATES4_LINE_ALTERNATIVE);
	register_file_format(
			PLATES4_LINE,
			"PLATES4 line",
			plate4_line_filename_extensions,
			plate4_line_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&PlatesLineFormatReader::read_file, _1, _2, _3)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_plates_line_feature_collection_writer, _1)),
			// No configuration options yet for this file format...
			boost::none);

	classifications_type gplates_rotation_classification;
	Configuration::shared_ptr_to_const_type grot_default_configuration(new RotationFileConfiguration());
	gplates_rotation_classification.set(RECONSTRUCTION);
	register_file_format(
			GPLATES_ROTATION,
			"GPlates rotation",
			std::vector<QString>(1, FILE_FORMAT_EXT_GPLATES_ROTATION),
			gplates_rotation_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&gplates_rotation_read_feature_collection,
							_1, boost::cref(*this), _2, _3)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_gplates_rotation_feature_collection_writer, _1)),
			grot_default_configuration);

	classifications_type plate4_rotation_classification;
	plate4_rotation_classification.set(RECONSTRUCTION);
	register_file_format(
			PLATES4_ROTATION,
			"PLATES4 rotation",
			std::vector<QString>(1, FILE_FORMAT_EXT_PLATES4_ROTATION),
			plate4_rotation_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&PlatesRotationFormatReader::read_file, _1, _2, _3)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_plates_rotation_feature_collection_writer, _1)),
			// No configuration options yet for this file format...
			boost::none);

	classifications_type shapefile_classification;
	shapefile_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	shapefile_classification.set(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	// FIXME: Should load this up with the standard GPlates model-to-attribute mapping.
	Configuration::shared_ptr_to_const_type shapefile_default_configuration(
			new OGRConfiguration(SHAPEFILE, true/*wrap_to_dateline*/));
	register_file_format(
			SHAPEFILE,
			"ESRI Shapefile",
			std::vector<QString>(1, FILE_FORMAT_EXT_SHAPEFILE),
			shapefile_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&ogr_read_feature_collection,
							_1, boost::cref(*this), SHAPEFILE, _2, _3)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_ogr_feature_collection_writer,
							_1, boost::cref(*this), SHAPEFILE)),
			shapefile_default_configuration);

	classifications_type ogr_gmt_classification;
	ogr_gmt_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	ogr_gmt_classification.set(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	// FIXME: Should load this up with the standard GPlates model-to-attribute mapping.
	Configuration::shared_ptr_to_const_type ogr_gmt_default_configuration(
			new OGRConfiguration(OGRGMT, false/*wrap_to_dateline*/));
	register_file_format(
			OGRGMT,
			"OGR GMT",
			std::vector<QString>(1, FILE_FORMAT_EXT_OGRGMT),
			ogr_gmt_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&ogr_read_feature_collection,
							_1, boost::cref(*this), OGRGMT, _2, _3)),
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_ogr_feature_collection_writer,
							_1, boost::cref(*this), OGRGMT)),
			ogr_gmt_default_configuration);

	classifications_type geojson_classification;
	geojson_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	geojson_classification.set(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	std::vector<QString> geojson_filename_extensions;
	geojson_filename_extensions.push_back(FILE_FORMAT_EXT_GEOJSON);
	geojson_filename_extensions.push_back(FILE_FORMAT_EXT_GEOJSON_ALTERNATIVE);
	// FIXME: Should load this up with the standard GPlates model-to-attribute mapping.
	Configuration::shared_ptr_to_const_type geojson_default_configuration(
			new OGRConfiguration(GEOJSON, false/*wrap_to_dateline*/));
	register_file_format(
				GEOJSON,
				"GeoJSON",
				geojson_filename_extensions,
				geojson_classification,
				&file_name_ends_with,
				Registry::read_feature_collection_function_type(
					boost::bind(&ogr_read_feature_collection,
								_1, boost::cref(*this), GEOJSON, _2, _3)),
				Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_ogr_feature_collection_writer,
								_1, boost::cref(*this), GEOJSON)),
				geojson_default_configuration);

	classifications_type geopackage_classification;
	geopackage_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	geopackage_classification.set(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	std::vector<QString> geopackage_filename_extensions;
	geopackage_filename_extensions.push_back(FILE_FORMAT_EXT_GEOPACKAGE);
	// FIXME: Should load this up with the standard GPlates model-to-attribute mapping.
	Configuration::shared_ptr_to_const_type geopackage_default_configuration(
			new OGRConfiguration(GEOPACKAGE, false/*wrap_to_dateline*/));
	register_file_format(
				GEOPACKAGE,
				"GeoPackage",
				geopackage_filename_extensions,
				geopackage_classification,
				&file_name_ends_with,
				Registry::read_feature_collection_function_type(
					boost::bind(&ogr_read_feature_collection,
								_1, boost::cref(*this), GEOPACKAGE, _2, _3)),
				Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_ogr_feature_collection_writer,
								_1, boost::cref(*this), GEOPACKAGE)),
				geopackage_default_configuration);

	classifications_type write_only_gmt_classification;
	write_only_gmt_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	write_only_gmt_classification.set(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	Configuration::shared_ptr_to_const_type write_only_gmt_default_configuration(new GMTConfiguration());
	register_file_format(
			WRITE_ONLY_XY_GMT,
			"GMT xy",
			std::vector<QString>(1, FILE_FORMAT_EXT_WRITE_ONLY_XY_GMT),
			write_only_gmt_classification,
			&file_name_ends_with,
			// Reading not currently supported...
			boost::none,
			Registry::create_feature_collection_writer_function_type(
					boost::bind(&create_write_only_xy_gmt_feature_collection_writer,
							_1, boost::cref(*this))),
			write_only_gmt_default_configuration);

	classifications_type gmap_classification;
	gmap_classification.set(GPlatesAppLogic::ReconstructMethod::VIRTUAL_GEOMAGNETIC_POLE);
	register_file_format(
			GMAP,
			"GMAP Virtual Geomagnetic Poles",
			std::vector<QString>(1, FILE_FORMAT_EXT_GMAP),
			gmap_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&GmapReader::read_file, _1, _2, _3)),
			// Writing not currently supported...
			boost::none,
			// No configuration options yet for this file format...
			boost::none);

	classifications_type gsml_classification;
	gsml_classification.set(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	register_file_format(
			GSML,
			"GeoSciML",
			std::vector<QString>(1, FILE_FORMAT_EXT_GSML),
			gsml_classification,
			&file_name_ends_with,
			Registry::read_feature_collection_function_type(
					boost::bind(&gsml_read_feature_collection, _1, _2, _3)),
			// Writing not currently supported...
			boost::none,
			// No configuration options yet for this file format...
			boost::none);
}
