/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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
#include <cmath>
#include <ostream>
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <gtest/gtest.h>

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "maths/MathsUtils.h"
#include "maths/UnitQuaternion3D.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/PropertyValueFinder.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/StructuralType.h"


// Saving a '.grot' file loaded with its configuration, as GPlates does, through the file-format
// registry. The configuration holds a line-by-line copy of the file, which keeps the file's own
// text (comments, spacing) when saved. But it follows only the edits made through the rotation
// dialogs, so it must be saved only while it holds the same poles as the model; otherwise the model
// is saved. pyGPlates never saves with a configuration, so nothing else tests this path.
namespace
{
	// A pole as (moving plate, fixed plate, time, disabled, rotation angle in degrees).
	typedef boost::tuple<int, int, double, bool, double> pole_type;
}

namespace boost
{
	namespace tuples
	{
		// So that a failure prints the poles (GoogleTest finds this by argument-dependent lookup).
		void
		PrintTo(
				const ::pole_type &pole,
				std::ostream *os)
		{
			*os << "(moving " << pole.get<0>() << ", fixed " << pole.get<1>() << ", " << pole.get<2>()
					<< " Ma, " << (pole.get<3>() ? "disabled" : "enabled") << ", " << pole.get<4>() << " deg)";
		}
	}
}

namespace
{

	const GPlatesModel::PropertyName &
	total_reconstruction_pole_name()
	{
		static const GPlatesModel::PropertyName NAME =
				GPlatesModel::PropertyName::create_gpml("totalReconstructionPole");
		return NAME;
	}


	boost::optional<int>
	plate_id(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			const char *property_name)
	{
		const boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> plate_id =
				GPlatesModel::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
						feature, GPlatesModel::PropertyName::create_gpml(property_name));
		if (!plate_id)
		{
			return boost::none;
		}
		return static_cast<int>(plate_id.get()->get_value());
	}


	GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type
	create_time_sample(
			const double &time,
			const double &latitude,
			const double &longitude,
			const double &angle,
			bool disabled = false)
	{
		// As the '.grot' reader makes them.
		return GPlatesPropertyValues::GpmlTimeSample::create(
				GPlatesPropertyValues::GpmlFiniteRotation::create(std::make_pair(longitude, latitude), angle),
				GPlatesModel::ModelUtils::create_gml_time_instant(GPlatesPropertyValues::GeoTimeInstant(time)),
				boost::none,
				GPlatesPropertyValues::StructuralType::create_gpml("FiniteRotation"),
				disabled);
	}


	class GrotFileTest :
			public ::testing::Test
	{
	protected:

		void
		SetUp() override
		{
			ASSERT_TRUE(d_tmp_dir.isValid());
			d_filename = d_tmp_dir.filePath("rotations.grot");
			// Tests save over the file, so never in the source tree.
			ASSERT_TRUE(QFile::copy(GPLATES_UNIT_TEST_DATA_DIR "/grot/rotations.grot", d_filename));
			// A copied read-only file stays read-only.
			QFile(d_filename).setPermissions(QFile::ReadOwner | QFile::WriteOwner);
		}

		GPlatesFileIO::File::non_null_ptr_type
		read()
		{
			GPlatesFileIO::File::non_null_ptr_type file =
					GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(d_filename));
			GPlatesFileIO::ReadErrorAccumulation read_errors;
			d_registry.read_feature_collection(file->get_reference(), read_errors);
			return file;
		}

		void
		save(
				GPlatesFileIO::File::non_null_ptr_type file)
		{
			d_registry.write_feature_collection(file->get_reference());
		}

		QByteArray
		contents() const
		{
			QFile file(d_filename);
			EXPECT_TRUE(file.open(QIODevice::ReadOnly));
			return file.readAll();
		}

		/**
		 * The poles of every rotation feature, sorted.
		 */
		static
		std::vector<pole_type>
		poles(
				GPlatesFileIO::File::non_null_ptr_type file)
		{
			std::vector<pole_type> result;
			GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
					file->get_reference().get_feature_collection();
			for (GPlatesModel::FeatureCollectionHandle::iterator iter = feature_collection->begin();
				iter != feature_collection->end();
				++iter)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature = (*iter)->reference();
				const boost::optional<int> moving_plate_id = plate_id(feature, "movingReferenceFrame");
				const boost::optional<int> fixed_plate_id = plate_id(feature, "fixedReferenceFrame");
				const boost::optional<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_to_const_type> sampling =
						GPlatesModel::get_property_value<GPlatesPropertyValues::GpmlIrregularSampling>(
								feature, total_reconstruction_pole_name());
				if (!moving_plate_id || !fixed_plate_id || !sampling)
				{
					continue;  // eg, the header's metadata feature
				}

				for (const GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_to_const_type &time_sample :
						sampling.get()->time_samples())
				{
					const GPlatesPropertyValues::GpmlFiniteRotation *finite_rotation =
							dynamic_cast<const GPlatesPropertyValues::GpmlFiniteRotation *>(time_sample->value().get());
					if (!finite_rotation)
					{
						continue;
					}
					const double w = finite_rotation->get_finite_rotation().unit_quat().w().dval();
					const double angle = GPlatesMaths::convert_rad_to_deg(2 * std::acos(std::min(1.0, std::fabs(w))));
					result.push_back(
							pole_type(
									moving_plate_id.get(),
									fixed_plate_id.get(),
									time_sample->valid_time()->get_time_position().value(),
									time_sample->is_disabled(),
									// To the four decimal places a pole is written with.
									std::round(angle * 1e4) / 1e4));
				}
			}
			std::sort(result.begin(), result.end());
			return result;
		}

		/**
		 * The rotation feature for @a moving_plate_id.
		 */
		static
		GPlatesModel::FeatureCollectionHandle::iterator
		find_rotation_feature(
				GPlatesFileIO::File::non_null_ptr_type file,
				int moving_plate_id)
		{
			GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
					file->get_reference().get_feature_collection();
			for (GPlatesModel::FeatureCollectionHandle::iterator iter = feature_collection->begin();
				iter != feature_collection->end();
				++iter)
			{
				if (plate_id((*iter)->reference(), "movingReferenceFrame") == moving_plate_id)
				{
					return iter;
				}
			}
			ADD_FAILURE() << "no rotation feature for moving plate " << moving_plate_id;
			return feature_collection->end();
		}

		static
		std::vector<pole_type>
		fixture_poles()
		{
			std::vector<pole_type> result;
			result.push_back(pole_type(701, 0, 0.0, false, 0.0));
			result.push_back(pole_type(701, 0, 40.0, false, 20.0));
			result.push_back(pole_type(801, 0, 0.0, false, 0.0));
			result.push_back(pole_type(801, 0, 10.0, false, 5.0));
			result.push_back(pole_type(801, 0, 20.0, true, 9.0));
			result.push_back(pole_type(801, 0, 30.0, false, 13.0));
			return result;
		}

		static const char *const FREE_TEXT_COMMENT;

		QTemporaryDir d_tmp_dir;
		QString d_filename;
		GPlatesFileIO::FeatureCollectionFileFormat::Registry d_registry;
	};

	const char *const GrotFileTest::FREE_TEXT_COMMENT = "# A free-text comment.";


	TEST_F(GrotFileTest, unchanged_save_keeps_the_files_own_text)
	{
		GPlatesFileIO::File::non_null_ptr_type file = read();
		ASSERT_EQ(poles(file), fixture_poles());

		save(file);
		const QByteArray saved = contents();
		EXPECT_TRUE(saved.contains(FREE_TEXT_COMMENT)) << saved.constData();
		EXPECT_EQ(poles(read()), fixture_poles());

		// And saving again writes the same file.
		save(file);
		EXPECT_EQ(contents(), saved);
	}


	TEST_F(GrotFileTest, a_pole_changed_only_in_the_model_is_saved)
	{
		GPlatesFileIO::File::non_null_ptr_type file = read();

		// Replace plate 801's poles in the model only, as Edit Feature Properties or the Python
		// console can: its 10 Ma angle becomes 7 degrees. The file's line-by-line copy doesn't
		// know, so saving that copy would lose the edit.
		GPlatesModel::FeatureHandle::weak_ref feature = (*find_rotation_feature(file, 801))->reference();
		std::vector<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type> time_samples;
		time_samples.push_back(create_time_sample(0.0, 90.0, 0.0, 0.0));
		time_samples.push_back(create_time_sample(10.0, 10.0, 20.0, 7.0));
		time_samples.push_back(create_time_sample(20.0, 11.0, 21.0, 9.0, true/*disabled*/));
		time_samples.push_back(create_time_sample(30.0, 12.0, 22.0, 13.0));
		const GPlatesPropertyValues::StructuralType value_type = time_samples.front()->get_value_type();
		const GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type interpolation =
				GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(value_type);
		GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type sampling =
				GPlatesPropertyValues::GpmlIrregularSampling::create(time_samples, interpolation, value_type);
		for (GPlatesModel::FeatureHandle::iterator iter = feature->begin(); iter != feature->end(); ++iter)
		{
			if ((*iter)->get_property_name() == total_reconstruction_pole_name())
			{
				feature->remove(iter);
				break;
			}
		}
		feature->add(GPlatesModel::TopLevelPropertyInline::create(total_reconstruction_pole_name(), sampling));

		save(file);

		std::vector<pole_type> expected = fixture_poles();
		expected[3] = pole_type(801, 0, 10.0, false, 7.0);
		EXPECT_EQ(poles(read()), expected) << contents().constData();
	}


	TEST_F(GrotFileTest, a_rotation_feature_deleted_only_in_the_model_is_saved)
	{
		GPlatesFileIO::File::non_null_ptr_type file = read();

		file->get_reference().get_feature_collection()->remove(find_rotation_feature(file, 701));
		save(file);

		std::vector<pole_type> expected = fixture_poles();
		expected.erase(expected.begin(), expected.begin() + 2);  // the 701 poles
		EXPECT_EQ(poles(read()), expected) << contents().constData();
	}


	TEST_F(GrotFileTest, a_non_rotation_feature_does_not_empty_the_file)
	{
		GPlatesFileIO::File::non_null_ptr_type file = read();

		// The configured writer once wrote only after counting as many rotation features as the
		// collection holds, so any other feature left the file empty (it was truncated first).
		GPlatesModel::FeatureHandle::create(
				file->get_reference().get_feature_collection(),
				GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"));
		save(file);

		const QByteArray saved = contents();
		EXPECT_TRUE(saved.contains(FREE_TEXT_COMMENT)) << saved.constData();
		EXPECT_EQ(poles(read()), fixture_poles());
	}
}
