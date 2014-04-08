/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney
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

#include <algorithm>
#include <iterator>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QString>
#include <QStringList>
#include <QMessageBox>

#include "ImportRasterDialog.h"

#include "RasterGeoreferencingPage.h"
#include "RasterBandPage.h"
#include "RasterFeatureCollectionPage.h"
#include "TimeDependentRasterPage.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Layer.h"
#include "app-logic/RasterLayerTask.h"
#include "app-logic/ReconstructGraph.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/RasterReader.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "global/PreconditionViolationError.h"

#include "gui/FileIOFeedback.h"
#include "gui/UnsavedChangesTracker.h"

#include "maths/MathsUtils.h"

#include "model/FeatureHandle.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlRectifiedGrid.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/XsString.h"

#include "utils/Parse.h"
#include "utils/UnicodeStringUtils.h"

const QString
GPlatesQtWidgets::ImportRasterDialog::GPML_EXT = ".gpml";


bool
GPlatesQtWidgets::TimeDependentRasterSequence::empty() const
{
	return d_sequence.empty();
}


void
GPlatesQtWidgets::TimeDependentRasterSequence::push_back(
		boost::optional<double> time,
		const QString &absolute_file_path,
		const QString &file_name,
		const std::vector<GPlatesPropertyValues::RasterType::Type> &band_types_,
		unsigned int width,
		unsigned int height)
{
	d_sequence.push_back(
			element_type(
				time,
				absolute_file_path,
				file_name,
				band_types_,
				width,
				height));
}


void
GPlatesQtWidgets::TimeDependentRasterSequence::add_all(
		const TimeDependentRasterSequence &other)
{
	d_sequence.reserve(d_sequence.size() + other.d_sequence.size());
	std::copy(
			other.d_sequence.begin(),
			other.d_sequence.end(),
			std::back_inserter(d_sequence));
}


void
GPlatesQtWidgets::TimeDependentRasterSequence::clear()
{
	d_sequence.clear();
}


void
GPlatesQtWidgets::TimeDependentRasterSequence::erase(
		unsigned int begin_index,
		unsigned int end_index)
{
	d_sequence.erase(
			d_sequence.begin() + begin_index,
			d_sequence.begin() + end_index);
}


void
GPlatesQtWidgets::TimeDependentRasterSequence::set_time(
		unsigned int index,
		const boost::optional<double> &time)
{
	d_sequence[index].time = time;
}


void
GPlatesQtWidgets::TimeDependentRasterSequence::sort_by_time()
{
	std::sort(d_sequence.begin(), d_sequence.end(),
			boost::bind(&element_type::time, _1) <
			boost::bind(&element_type::time, _2));
}


void
GPlatesQtWidgets::TimeDependentRasterSequence::sort_by_file_name()
{
	std::sort(d_sequence.begin(), d_sequence.end(),
			boost::bind(&element_type::file_name, _1) <
			boost::bind(&element_type::file_name, _2));
}


GPlatesQtWidgets::ImportRasterDialog::ImportRasterDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		GPlatesGui::UnsavedChangesTracker *unsaved_changes_tracker,
		GPlatesGui::FileIOFeedback *file_io_feedback,
		QWidget *parent_) :
	QWizard(parent_, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_application_state(application_state),
	d_view_state(view_state),
	d_unsaved_changes_tracker(unsaved_changes_tracker),
	d_file_io_feedback(file_io_feedback),
	d_open_file_dialog(
			parentWidget(),
			tr("Import Raster"),
			GPlatesFileIO::RasterReader::get_file_dialog_filters(),
			view_state),
	d_raster_width(0),
	d_raster_height(0),
	d_georeferencing(
			GPlatesPropertyValues::Georeferencing::create()),
	d_save_after_finish(true)
{
	setPage(
			TIME_DEPENDENT_RASTER_PAGE_ID,
			new TimeDependentRasterPage(
					view_state,
					d_raster_width,
					d_raster_height,
					d_raster_sequence,
					boost::bind(
						&ImportRasterDialog::set_number_of_bands,
						boost::ref(*this),
						_1),
					this));
	setPage(
			RASTER_BAND_PAGE_ID,
			new RasterBandPage(
					d_band_names,
					this));
	setPage(
			GEOREFERENCING_PAGE_ID,
			new RasterGeoreferencingPage(
					d_georeferencing,
					d_raster_width,
					d_raster_height,
					this));
	setPage(
			RASTER_FEATURE_COLLECTION_PAGE_ID,
			new RasterFeatureCollectionPage(
					d_save_after_finish,
					this));

	setOptions(options() | QWizard::NoDefaultButton /* by default, the dialog eats Enter keys */);

	// Note: I would've preferred to use resize() instead, but at least on
	// Windows Vista with Qt 4.4, the dialog doesn't respect the call to resize().
	QSize desired_size(724, 600);
	setMinimumSize(desired_size);
}


int
GPlatesQtWidgets::ImportRasterDialog::nextId() const
{
	switch (currentId())
	{
	case TIME_DEPENDENT_RASTER_PAGE_ID:
		return RASTER_BAND_PAGE_ID;

	case RASTER_BAND_PAGE_ID:
		{
			// If the (first) raster has georeferencing then skip the georeferencing page.
			boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type>
					raster_georeferencing = get_raster_georeferencing();
			if (raster_georeferencing)
			{
				d_georeferencing->set_parameters(raster_georeferencing.get()->parameters());

				return RASTER_FEATURE_COLLECTION_PAGE_ID;
			}

			return GEOREFERENCING_PAGE_ID;
		}

	case GEOREFERENCING_PAGE_ID:
		return RASTER_FEATURE_COLLECTION_PAGE_ID;

	case RASTER_FEATURE_COLLECTION_PAGE_ID:
	default:
		return -1;
	}
}


boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type>
GPlatesQtWidgets::ImportRasterDialog::get_raster_georeferencing() const
{
	// We shouldn't have an empty sequence but check in case.
	if (d_raster_sequence.empty())
	{
		return boost::none;
	}

	// Get the first raster in the sequence.
	// If sequence is not time-dependent then there'll only be one entry in the sequence.
	const QString filename = d_raster_sequence.get_sequence().front().absolute_file_path;

	// If the raster contains valid georeferencing then use that.
	GPlatesFileIO::ReadErrorAccumulation read_errors;
	GPlatesFileIO::RasterReader::non_null_ptr_type reader =
			GPlatesFileIO::RasterReader::create(filename, &read_errors);
	if (!reader->can_read())
	{
		return boost::none;
	}

	return reader->get_georeferencing();
}


void
GPlatesQtWidgets::ImportRasterDialog::set_number_of_bands(
		unsigned int number_of_bands)
{
	if (number_of_bands > d_band_names.size())
	{
		// Not enough currently, so we need to generate some more!
		for (unsigned int i = d_band_names.size(); i != number_of_bands; ++i)
		{
			static const QString BAND_NAME_FORMAT = "band_%1";
			d_band_names.push_back(BAND_NAME_FORMAT.arg(i + 1));
		}
	}
	else if (number_of_bands < d_band_names.size())
	{
		// Too many, remove some.
		d_band_names.erase(d_band_names.begin() + number_of_bands, d_band_names.end());
	}
}


void
GPlatesQtWidgets::ImportRasterDialog::display(
		bool time_dependent_raster,
		GPlatesFileIO::ReadErrorAccumulation *read_errors)
{
	if (!time_dependent_raster)
	{
		QString filename = d_open_file_dialog.get_open_file_name();
		if (filename.isEmpty())
		{
			return;
		}

		d_view_state.get_last_open_directory() = QFileInfo(filename).path();

		// Check whether there is a GPML file of the same name in the same directory.
		// If so, ask the user if they actually meant to open that.
		QFileInfo file_info(filename);
		QString base_gpml_filename = file_info.completeBaseName() + GPML_EXT;
		QString dir = file_info.absolutePath();
		if (!dir.endsWith("/"))
		{
			dir.append("/");
		}
		QString gpml_filename = dir + base_gpml_filename;
		if (QFile(gpml_filename).exists())
		{
			static const QString QUESTION =
					"There is a GPML file %1 in the same directory as the raster file that you selected. "
					"Do you wish to open this existing GPML file instead of importing the raster file?";
			QMessageBox::StandardButton answer = QMessageBox::question(
					parentWidget(),
					"Import Raster",
					QUESTION.arg(base_gpml_filename),
					QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
			if (answer == QMessageBox::Yes)
			{
				d_file_io_feedback->open_files(QStringList(gpml_filename));
				return;
			}
			else if (answer == QMessageBox::Cancel)
			{
				return;
			}
			// else fall through.
		}

		// Read the number of bands and their type in the raster file.
		GPlatesFileIO::RasterReader::non_null_ptr_type reader =
			GPlatesFileIO::RasterReader::create(filename, read_errors);
		if (!reader->can_read())
		{
			QMessageBox::critical(parentWidget(), "Import Raster",
				"The raster file that you selected could not be read.");
			return;
		}

		unsigned int number_of_bands = reader->get_number_of_bands(read_errors);
		if (number_of_bands == 0)
		{
			QMessageBox::critical(parentWidget(), "Import Raster",
				"The raster file that you selected contains no bands. Raster files must have at least one band.");
			return;
		}
		std::vector<GPlatesPropertyValues::RasterType::Type> band_types;
		for (unsigned int i = 1; i <= number_of_bands; ++i)
		{
			band_types.push_back(reader->get_type(i));
		}
		set_number_of_bands(number_of_bands);
		
		// Read the size of the raster.
		std::pair<unsigned int, unsigned int> raster_size = reader->get_size(read_errors);
		if (raster_size.first == 0 || raster_size.second == 0)
		{
			QMessageBox::critical(parentWidget(), "Import Raster",
					"The width and height could not be read from the raster file that you selected.");
			return;
		}

		// Save all of this information for later.
		// We pretend that this is a time-dependent raster sequence of length 1,
		// with the time set to boost::none.
		d_raster_sequence.push_back(
				boost::none,
				file_info.absoluteFilePath(),
				file_info.fileName(),
				band_types,
				raster_size.first,
				raster_size.second);

		// Set the raster width and height for the next stage (wizard page) since we're
		// skipping past the time-dependent raster sequence page which normally sets them.
		d_raster_width = raster_size.first;
		d_raster_height = raster_size.second;

		// Jump past the time-dependent raster sequence page.
		setStartId(RASTER_BAND_PAGE_ID);
	}

	if (time_dependent_raster)
	{
		setWindowTitle("Import Time-Dependent Raster");
	}
	else
	{
		setWindowTitle("Import Raster");
	}

	if (exec() == QDialog::Accepted)
	{
		// We want to merge model events across this scope so that only one model event
		// is generated instead of many as we incrementally modify the feature below.
		GPlatesModel::NotificationGuard model_notification_guard(
				d_application_state.get_model_interface().access_model());

		// By the time that we got up to here, we would've collected all the
		// information we need to create the raster feature.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_raster_sequence.empty(), GPLATES_ASSERTION_SOURCE);

		GPlatesModel::PropertyValue::non_null_ptr_type domain_set = create_domain_set();
		GPlatesModel::PropertyValue::non_null_ptr_type range_set = create_range_set(time_dependent_raster);
		GPlatesModel::PropertyValue::non_null_ptr_type band_names = create_band_names();

		static const GPlatesModel::FeatureType RASTER = GPlatesModel::FeatureType::create_gpml("Raster");
		static const GPlatesModel::PropertyName DOMAIN_SET = GPlatesModel::PropertyName::create_gpml("domainSet");
		static const GPlatesModel::PropertyName RANGE_SET = GPlatesModel::PropertyName::create_gpml("rangeSet");
		static const GPlatesModel::PropertyName BAND_NAMES = GPlatesModel::PropertyName::create_gpml("bandNames");

		GPlatesModel::FeatureHandle::non_null_ptr_type feature = GPlatesModel::FeatureHandle::create(RASTER);
		feature->add(GPlatesModel::TopLevelPropertyInline::create(DOMAIN_SET, domain_set));
		feature->add(GPlatesModel::TopLevelPropertyInline::create(RANGE_SET, range_set));
		feature->add(GPlatesModel::TopLevelPropertyInline::create(BAND_NAMES, band_names));

		// Create a new file and add it to file state.
		QString gpml_file_path = create_gpml_file_path(time_dependent_raster);
		GPlatesFileIO::FileInfo file_info(gpml_file_path);
		GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(file_info);
		GPlatesAppLogic::FeatureCollectionFileState::file_reference app_logic_file_ref =
			d_application_state.get_feature_collection_file_state().add_file(file);
		GPlatesFileIO::File::Reference &file_io_file_ref = app_logic_file_ref.get_file();

		// Add feature to feature collection in file.
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				file_io_file_ref.get_feature_collection();
		feature_collection->add(feature);
		
		// Release the model notification guard now that we've finished modifying the feature.
		// Provided there are no nested guards this should notify model observers.
		// We want any observers to see the changes before continuing so that everyone's in sync.
		model_notification_guard.release_guard();

		// Then save the file.
		if (d_save_after_finish)
		{
			try
			{
				d_file_io_feedback->save_file(app_logic_file_ref);
			}
			catch (std::exception &exc)
			{
				QString message = tr("An error occurred while saving the file '%1': '%2' -"
						" Please use the Manage Feature Collections dialog "
						"on the File menu to save the new feature collection manually.")
						.arg(file_info.get_display_name(false/*use_absolute_path_name*/)
						.arg(exc.what()));
				QMessageBox::critical(parentWidget(), "Save Raster", message);
			}
			catch (...)
			{
				QMessageBox::critical(parentWidget(), "Save Raster",
						"The GPML file could not be saved. Please use the Manage Feature Collections dialog "
						"on the File menu to save the new feature collection manually.");
			}
		}
	}
}


namespace
{
	using namespace GPlatesPropertyValues;

	const GmlFile::value_component_type &
	create_gml_file_templated_value_object(
			RasterType::Type band_type)
	{
		if (band_type == RasterType::INT8)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_xsi("byte"),
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}
		else if (band_type == RasterType::UINT8)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_xsi("unsignedByte"),
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}
		else if (band_type == RasterType::INT16)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_xsi("short"),
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}
		else if (band_type == RasterType::UINT16)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_xsi("unsignedShort"),
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}
		else if (band_type == RasterType::INT32)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_xsi("int"),
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}
		else if (band_type == RasterType::UINT32)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_xsi("unsignedInt"),
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}
		else if (band_type == RasterType::FLOAT)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_xsi("float"),
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}
		else if (band_type == RasterType::DOUBLE)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_xsi("double"),
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}
		else if (band_type == RasterType::RGBA8)
		{
			static const GmlFile::value_component_type VALUE_OBJECT(
					ValueObjectType::create_gpml("Rgba"), // I made this up.
					GmlFile::xml_attributes_type());
			return VALUE_OBJECT;
		}

		throw GPlatesGlobal::PreconditionViolationError(GPLATES_EXCEPTION_SOURCE);
	}

	std::map<QString, XsString::non_null_ptr_to_const_type>
	build_mime_type_map()
	{
		std::map<QString, XsString::non_null_ptr_to_const_type> mime_types;
		typedef std::map<QString, GPlatesFileIO::RasterReader::FormatInfo> formats_map_type;
		const formats_map_type &formats = GPlatesFileIO::RasterReader::get_supported_formats();

		BOOST_FOREACH(const formats_map_type::value_type &format, formats)
		{
			mime_types.insert(std::make_pair(
					format.first, 
					XsString::create(
						GPlatesUtils::make_icu_string_from_qstring(
							format.second.mime_type))));
		}

		return mime_types;
	}

	boost::optional<XsString::non_null_ptr_to_const_type>
	get_mime_type(
			const QString &file_name)
	{
		typedef std::map<QString, XsString::non_null_ptr_to_const_type> mime_types_map_type;
		static const mime_types_map_type MIME_TYPES = build_mime_type_map();

		QString suffix = QFileInfo(file_name).suffix().toLower();
		mime_types_map_type::const_iterator iter = MIME_TYPES.find(suffix);

		if (iter == MIME_TYPES.end())
		{
			return boost::none;
		}
		else
		{
			return iter->second;
		}
	}

	GmlFile::non_null_ptr_type
	create_gml_file(
			const GPlatesQtWidgets::TimeDependentRasterSequence::FileInfo &file_info)
	{
		// Create the GmlFile's rangeParameters using the band types.
		GmlFile::composite_value_type range_parameters;
		BOOST_FOREACH(RasterType::Type band_type, file_info.band_types)
		{
			range_parameters.push_back(create_gml_file_templated_value_object(band_type));
		}

		static const XsString::non_null_ptr_to_const_type EMPTY_FILE_STRUCTURE =
			XsString::create(GPlatesUtils::UnicodeString());
		return GmlFile::create(
				range_parameters,
				XsString::create(GPlatesUtils::make_icu_string_from_qstring(file_info.absolute_file_path)),
				EMPTY_FILE_STRUCTURE,
				get_mime_type(file_info.file_name),
				boost::none /* compression */);
	}
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::ImportRasterDialog::create_range_set(
		bool time_dependent_raster)
{
	using namespace GPlatesPropertyValues;

	// Items common to both branches.
	const TimeDependentRasterSequence::sequence_type &sequence = d_raster_sequence.get_sequence();
	static const StructuralType GML_FILE_VALUE_TYPE =
		StructuralType::create_gml("File");

	if (time_dependent_raster)
	{
		// We need to build a series of GpmlTimeWindows to create a GpmlPiecewiseAggregation.
		// FIXME: The following code simply inserts fenceposts in between the user's
		// chosen times, without regard to how far away those fenceposts are from the
		// chosen times. We should add an option later to allow the user to restrict
		// the maximum width of a time window.
		d_raster_sequence.sort_by_time();

		// By the time we got to here, there should be at least one element in
		// the sequence, and all times should not be boost::none.
		// We build the sequence from the present day, going back in time.
		GeoTimeInstant prev_fence_post = GeoTimeInstant::create_distant_future();
		std::vector<GpmlTimeWindow> time_windows;
		for (TimeDependentRasterSequence::sequence_type::const_iterator iter = sequence.begin();
			iter != sequence.end(); ++iter)
		{
			// For each iteration of the for loop, we create the time window that
			// covers the current file in the sequence.
			// If there are n files, there are n time windows.
			GeoTimeInstant curr_fence_post = (iter == sequence.end() - 1) ?
				GeoTimeInstant::create_distant_past() :
				GeoTimeInstant(
						(*iter->time + *(iter + 1)->time) / 2); // take the average of this time and the next time.

			// Note that because we are going back in time, curr_fence_post is older
			// prev_fence_post.
			GmlTimePeriod::non_null_ptr_type time_period =
				GPlatesModel::ModelUtils::create_gml_time_period(
						curr_fence_post, prev_fence_post);

			// Create the GmlFile and then wrap it up inside a GpmlConstantValue
			// (because the children of GpmlTimeWindow have to be a time dependent property).
			GmlFile::non_null_ptr_type gml_file = create_gml_file(*iter);
			GpmlConstantValue::non_null_ptr_type gml_file_as_constant_value =
				GpmlConstantValue::create(gml_file, GML_FILE_VALUE_TYPE);

			GpmlTimeWindow time_window(gml_file_as_constant_value, time_period, GML_FILE_VALUE_TYPE);
			time_windows.push_back(time_window);

			prev_fence_post = curr_fence_post;
		}

		return GpmlPiecewiseAggregation::create(time_windows, GML_FILE_VALUE_TYPE);
	}
	else
	{
		// There should be just the one element in the sequence for a constant value.
		GmlFile::non_null_ptr_type gml_file = create_gml_file(sequence[0]);
		return GpmlConstantValue::create(gml_file, GML_FILE_VALUE_TYPE);
	}
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::ImportRasterDialog::create_band_names() const
{
	using namespace GPlatesPropertyValues;

	std::vector<XsString::non_null_ptr_to_const_type> xs_strings;
	BOOST_FOREACH(const QString &band_name, d_band_names)
	{
		XsString::non_null_ptr_to_const_type band_name_as_xs_string =
			XsString::create(GPlatesUtils::make_icu_string_from_qstring(band_name));
		xs_strings.push_back(band_name_as_xs_string);
	}

	return GpmlRasterBandNames::create(xs_strings);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::ImportRasterDialog::create_domain_set() const
{
	using namespace GPlatesPropertyValues;

	const TimeDependentRasterSequence::sequence_type &sequence = d_raster_sequence.get_sequence();
	
	// By the time we got to here, there should be at least one element in the sequence.
	unsigned int raster_width = sequence[0].width;
	unsigned int raster_height = sequence[0].height;

	// Create the GmlRectifiedGrid.
	GmlRectifiedGrid::xml_attributes_type xml_attributes;
	static const GPlatesModel::XmlAttributeName DIMENSION =
		GPlatesModel::XmlAttributeName::create_gml("dimension");
	static const GPlatesModel::XmlAttributeValue TWO("2");
	xml_attributes.insert(std::make_pair(DIMENSION, TWO));

	GmlRectifiedGrid::non_null_ptr_type rectified_grid = GmlRectifiedGrid::create(
			d_georeferencing,
			raster_width,
			raster_height,
			xml_attributes);

	// TODO: Remove this once we reference a ".gpr" GPlates raster data file instead of the actual
	// imported raster image file (and associated ".cache" files). When we do this we will be breaking
	// compatibility with older versions of GPlates (ie, older versions will not be able to load ".gpr" files).
	// At that point in time we will also stop doing the hack below.
	// The hack below helps ensure older versions of GPlates can load rasters created by this version.
	// It does this by ensuring the georeferencing origin has a lat/lon in the valid lat/lon range
	// in order to avoid an exception when an older version of GPlates loads a GPML raster file
	// generated by this version (resulting in the raster not displaying).This version of GPlates does
	// not have this problem because it does not expect the origin to be within any particular range.
	// Note that we don't apply the hack when the raster has a *projected* coordinate system because
	// then we cannot assume anything about the range of georeferenced coordinates - and, in any case,
	// older versions of GPlates can't handle projected coordinate systems so they will have problems
	// regardless.
#if 1
	bool is_raster_spatial_reference_system_projected = false;

	// Find out if raster's coordinate system is projected.
	if (!d_raster_sequence.empty()) // We shouldn't have an empty sequence but check in case.
	{
		// Get the first raster in the sequence.
		// If sequence is not time-dependent then there'll only be one entry in the sequence.
		const QString filename = d_raster_sequence.get_sequence().front().absolute_file_path;

		// If the raster contains a valid coordinate transformation then query that.
		GPlatesFileIO::ReadErrorAccumulation read_errors;
		GPlatesFileIO::RasterReader::non_null_ptr_type reader =
				GPlatesFileIO::RasterReader::create(filename, &read_errors);
		if (reader->can_read())
		{
			boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type>
					raster_srs = reader->get_spatial_reference_system();
			if (raster_srs)
			{
				is_raster_spatial_reference_system_projected = raster_srs.get()->is_projected();
			}
		}
	}

	// If the coordinate system is projected then we cannot assume anything about the range
	// of georeferenced coordinates, otherwise we can clamp to a valid lat/lon range to avoid
	// generating an exception when older versions of GPlates load a raster generated by this version.
	if (!is_raster_spatial_reference_system_projected)
	{
		GPlatesPropertyValues::Georeferencing::parameters_type hacked_geo_parameters =
				d_georeferencing->parameters();

		// Clamp to valid latitude range to avoid problems with older versions of GPlates.
		if (hacked_geo_parameters.top_left_y_coordinate < -90)
		{
			hacked_geo_parameters.top_left_y_coordinate = -90;
		}
		else if (hacked_geo_parameters.top_left_y_coordinate > 90)
		{
			hacked_geo_parameters.top_left_y_coordinate = 90;
		}

		// Wrap longitude into the range [-360,360] to avoid problems with older versions of GPlates.
		if (hacked_geo_parameters.top_left_x_coordinate < -360)
		{
			do 
			{
				hacked_geo_parameters.top_left_x_coordinate += 360;
			}
			while (hacked_geo_parameters.top_left_x_coordinate < -360);
		}
		else if (hacked_geo_parameters.top_left_x_coordinate > 360)
		{
			do 
			{
				hacked_geo_parameters.top_left_x_coordinate -= 360;
			}
			while (hacked_geo_parameters.top_left_x_coordinate > 360);
		}

		GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type hacked_georeferencing =
				GPlatesPropertyValues::Georeferencing::create(hacked_geo_parameters);

		rectified_grid = GmlRectifiedGrid::create(
				hacked_georeferencing,
				raster_width,
				raster_height,
				xml_attributes);
	}
#endif

	// Then wrap it up in a constant value.
	// FIXME: We need to allow the user to create time-dependent georeferencing.
	static const StructuralType VALUE_TYPE =
		StructuralType::create_gml("RectifiedGrid");
	return GpmlConstantValue::create(rectified_grid, VALUE_TYPE);
}


QString
GPlatesQtWidgets::ImportRasterDialog::create_gpml_file_path(
		bool time_dependent_raster) const
{
	const TimeDependentRasterSequence::element_type &first_file =
		d_raster_sequence.get_sequence()[0];
	QString base_name = QFileInfo(first_file.file_name).completeBaseName();

	QString fixed_file_name;

	if (time_dependent_raster)
	{
		// Strip off the time from the file name if it is there.
		QStringList tokens = base_name.split("-");

		if (tokens.count() >= 2)
		{
			try
			{
				GPlatesUtils::Parse<double> parse;
				if (GPlatesMaths::are_almost_exactly_equal(
							parse(tokens.last()), *first_file.time))
				{
					tokens.removeLast();
				}
				fixed_file_name = tokens.join("-") + GPML_EXT;
			}
			catch (const GPlatesUtils::ParseError &)
			{
				fixed_file_name = base_name + GPML_EXT;
			}
		}
		else
		{
			fixed_file_name = base_name + GPML_EXT;
		}
	}
	else
	{
		fixed_file_name = base_name + GPML_EXT;
	}

	QString dir = QFileInfo(first_file.absolute_file_path).absolutePath();
	if (!dir.endsWith("/"))
	{
		dir += "/";
	}

	return dir + fixed_file_name;
}

