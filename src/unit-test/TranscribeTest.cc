/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#include <cmath>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <boost/optional.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <gtest/gtest.h>
#include <QtGlobal>
#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

#include "TranscribeTest.h"

#include "global/GPlatesAssert.h"

#include "maths/Real.h"

#include "scribe/Scribe.h"
#include "scribe/ScribeExceptions.h"
#include "scribe/ScribeBinaryArchiveReader.h"
#include "scribe/ScribeBinaryArchiveWriter.h"
#include "scribe/ScribeTextArchiveReader.h"
#include "scribe/ScribeTextArchiveWriter.h"
#include "scribe/ScribeXmlArchiveReader.h"
#include "scribe/ScribeXmlArchiveWriter.h"
#include "scribe/TranscribeEnumProtocol.h"
#include "scribe/TranscribeDelegateProtocol.h"
#include "scribe/TranscribeUtils.h"

#include "utils/non_null_intrusive_ptr.h"


Q_DECLARE_METATYPE(GPlatesUnitTest::TranscribePrimitivesTest::Data::StringWithEmbeddedZeros);


// Equivalent of BOOST_CHECK_CLOSE: tolerance is a *percentage* of the expected value.
#define GPLATES_EXPECT_CLOSE_PERCENT(actual, expected, percent) \
		EXPECT_NEAR(actual, expected, std::fabs(expected) * (percent) / 100.0)


namespace
{
	/**
	 * Returns a path (inside a process-lifetime temporary directory) for a named scratch file
	 * used by the transcription tests, so test archive files are not written into the source tree.
	 */
	QString
	test_scratch_file_path(
			const QString &file_name)
	{
		static QTemporaryDir scratch_dir;
		return scratch_dir.path() + '/' + file_name;
	}
}


void
GPlatesUnitTest::TranscribePrimitivesTest::test_case_primitives_1()
{
	boost::scoped_ptr<Data> before_data_scoped_ptr(new Data(10));
	before_data_scoped_ptr->initialise();
	Data before_data(20);
	before_data.initialise();
	// Test an array of 'const' objects.
	// We're doing this here instead of inside class Data because
	// C++ does not support initialising const array non-static member data.
	const std::string before_string_array[2] = { std::string("test1"), std::string("test2") };
	const char before_char_array[1][2][6] = { { "test1", "test2" } };
	// Test an array of non-default constructable objects.
	// We're doing this here instead of inside class Data because C++ does not support
	// initialising non-default constructable array non-static member data.
	const Data::NonDefaultConstructable before_non_default_constructable_array[1][2] = { { 100, 102 } };
	const Data::NonDefaultConstructable (*before_non_default_constructable_array_ptr)[1][2] =
			&before_non_default_constructable_array;
	const Data::NonDefaultConstructable (*const before_non_default_constructable_sub_array_ptr)[2] =
			before_non_default_constructable_array;
	const Data::NonDefaultConstructable (*const *const before_non_default_constructable_sub_array_ptr_ptr)[2] =
			&before_non_default_constructable_sub_array_ptr;
	const Data::NonDefaultConstructable *before_non_default_constructable_array_element_ptr =
			&before_non_default_constructable_array[0][1];

	try
	{
		//
		// Text archive
		//
		{
			SCOPED_TRACE("text archive");

			std::stringstream text_archive;

			test_case_1_write(
					GPlatesScribe::TextArchiveWriter::create(text_archive),
					before_data_scoped_ptr,
					before_data,
					before_string_array,
					before_char_array,
					before_non_default_constructable_array,
					before_non_default_constructable_array_ptr,
					before_non_default_constructable_sub_array_ptr,
					before_non_default_constructable_sub_array_ptr_ptr,
					before_non_default_constructable_array_element_ptr);

			text_archive.seekp(0);

			test_case_1_read(
					GPlatesScribe::TextArchiveReader::create(text_archive),
					before_data_scoped_ptr,
					before_data,
					before_string_array,
					before_char_array,
					before_non_default_constructable_array,
					before_non_default_constructable_array_ptr,
					before_non_default_constructable_sub_array_ptr,
					before_non_default_constructable_sub_array_ptr_ptr,
					before_non_default_constructable_array_element_ptr);
		}

		//
		// Binary archive
		//
		{
			SCOPED_TRACE("binary archive");

			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			test_case_1_write(
					GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer),
					before_data_scoped_ptr,
					before_data,
					before_string_array,
					before_char_array,
					before_non_default_constructable_array,
					before_non_default_constructable_array_ptr,
					before_non_default_constructable_sub_array_ptr,
					before_non_default_constructable_sub_array_ptr_ptr,
					before_non_default_constructable_array_element_ptr);

			binary_archive.close();

#if 0
			QFile archive_binary_file(test_scratch_file_path("archive_primitives_1.bin"));
			bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
			EXPECT_TRUE(archive_binary_open_for_writing);
			if (archive_binary_open_for_writing)
			{
				archive_binary_file.write(binary_archive.data());
			}
			archive_binary_file.close();
#endif

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			test_case_1_read(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader),
					before_data_scoped_ptr,
					before_data,
					before_string_array,
					before_char_array,
					before_non_default_constructable_array,
					before_non_default_constructable_array_ptr,
					before_non_default_constructable_sub_array_ptr,
					before_non_default_constructable_sub_array_ptr_ptr,
					before_non_default_constructable_array_element_ptr);
		}

		//
		// XML archive
		//
		{
			SCOPED_TRACE("XML archive");

			QBuffer xml_archive;
			xml_archive.open(QBuffer::WriteOnly);

			QXmlStreamWriter xml_stream_writer(&xml_archive);
			xml_stream_writer.writeStartDocument();

			test_case_1_write(
					GPlatesScribe::XmlArchiveWriter::create(xml_stream_writer),
					before_data_scoped_ptr,
					before_data,
					before_string_array,
					before_char_array,
					before_non_default_constructable_array,
					before_non_default_constructable_array_ptr,
					before_non_default_constructable_sub_array_ptr,
					before_non_default_constructable_sub_array_ptr_ptr,
					before_non_default_constructable_array_element_ptr);

			xml_stream_writer.writeEndDocument();

			xml_archive.close();

#if 0
			QFile archive_xml_file(test_scratch_file_path("archive_primitives_1.xml"));
			bool archive_xml_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
			EXPECT_TRUE(archive_xml_open_for_writing);
			if (archive_xml_open_for_writing)
			{
				archive_xml_file.write(xml_archive.data());
			}
			archive_xml_file.close();
#endif

			xml_archive.open(QBuffer::ReadOnly);
			xml_archive.seek(0);

			QXmlStreamReader xml_stream_reader(&xml_archive);
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isStartDocument());

			GPlatesScribe::XmlArchiveReader::non_null_ptr_type xml_archive_reader =
					GPlatesScribe::XmlArchiveReader::create(xml_stream_reader);

			test_case_1_read(
					xml_archive_reader,
					before_data_scoped_ptr,
					before_data,
					before_string_array,
					before_char_array,
					before_non_default_constructable_array,
					before_non_default_constructable_array_ptr,
					before_non_default_constructable_sub_array_ptr,
					before_non_default_constructable_sub_array_ptr_ptr,
					before_non_default_constructable_array_element_ptr);

			xml_archive_reader->close();
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isEndDocument());
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}

void
GPlatesUnitTest::TranscribePrimitivesTest::test_case_1_write(
		const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
		boost::scoped_ptr<Data> &before_data_scoped_ptr,
		Data &before_data,
		const std::string (&before_string_array)[2],
		const char (&before_char_array)[1][2][6],
		const Data::NonDefaultConstructable (&before_non_default_constructable_array)[1][2],
		const Data::NonDefaultConstructable (*&before_non_default_constructable_array_ptr)[1][2],
		const Data::NonDefaultConstructable (*const &before_non_default_constructable_sub_array_ptr)[2],
		const Data::NonDefaultConstructable (*const *const before_non_default_constructable_sub_array_ptr_ptr)[2],
		const Data::NonDefaultConstructable *&before_non_default_constructable_array_element_ptr)
{
	SCOPED_TRACE("test_case_1_write");

	GPlatesScribe::Scribe scribe;

	scribe.transcribe(TRANSCRIBE_SOURCE, before_data_scoped_ptr, "data_scoped_ptr", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_string_array, "string_array", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_char_array, "char_array", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_non_default_constructable_array, "2d", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_non_default_constructable_array_ptr, "p2d", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_non_default_constructable_sub_array_ptr, "ps2d", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_non_default_constructable_sub_array_ptr_ptr, "pps2d", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_non_default_constructable_array_element_ptr, "pe2d", GPlatesScribe::TRACK);

	EXPECT_TRUE(scribe.is_transcription_complete());

	archive_writer->write_transcription(*scribe.get_transcription());
}

void
GPlatesUnitTest::TranscribePrimitivesTest::test_case_1_read(
		const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
		boost::scoped_ptr<Data> &before_data_scoped_ptr,
		Data &before_data,
		const std::string (&before_string_array)[2],
		const char (&before_char_array)[1][2][6],
		const Data::NonDefaultConstructable (&before_non_default_constructable_array)[1][2],
		const Data::NonDefaultConstructable (*&before_non_default_constructable_array_ptr)[1][2],
		const Data::NonDefaultConstructable (*const &before_non_default_constructable_sub_array_ptr)[2],
		const Data::NonDefaultConstructable (*const *const before_non_default_constructable_sub_array_ptr_ptr)[2],
		const Data::NonDefaultConstructable *&before_non_default_constructable_array_element_ptr)
{
	SCOPED_TRACE("test_case_1_read");

	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	boost::scoped_ptr<Data> after_data_scoped_ptr;
	Data after_data(0);
	const std::string after_string_array[2] = { std::string(""), std::string("") };
	// NOTE: We remove the top-level 'const' because otherwise these variables are sometimes moved to read-only memory
	// and when the scribe writes to them then it crashes.
	// In any case string literals, for example, wouldn't normally get transcribed (becase they're literal
	// and don't change) - we're only doing it here to test the scribe system.
	char after_char_array[1][2][6];
	Data::NonDefaultConstructable after_non_default_constructable_array[1][2] = { { -1, -1 } };
	const Data::NonDefaultConstructable (*after_non_default_constructable_array_ptr)[1][2];
	const Data::NonDefaultConstructable (*after_non_default_constructable_sub_array_ptr)[2] = NULL;
	const Data::NonDefaultConstructable (*const *after_non_default_constructable_sub_array_ptr_ptr)[2] = NULL;
	const Data::NonDefaultConstructable *after_non_default_constructable_array_element_ptr;

	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_scoped_ptr, "data_scoped_ptr", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_string_array, "string_array", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_char_array, "char_array", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_array, "2d", GPlatesScribe::TRACK));
	Data::NonDefaultConstructable relocated_after_non_default_constructable_array[1][2] =
	{
		{ after_non_default_constructable_array[0][0], after_non_default_constructable_array[0][1] }
	};
	scribe.relocated(
			TRANSCRIBE_SOURCE,
			relocated_after_non_default_constructable_array,
			after_non_default_constructable_array);
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_array_ptr, "p2d", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_sub_array_ptr, "ps2d", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_sub_array_ptr_ptr, "pps2d", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_array_element_ptr, "pe2d", GPlatesScribe::TRACK));

	EXPECT_TRUE(scribe.is_transcription_complete());

	EXPECT_TRUE(after_data_scoped_ptr);
	if (after_data_scoped_ptr)
	{
		before_data_scoped_ptr->check_equality(*after_data_scoped_ptr);
	}
	after_data.check_equality(before_data);

	for (unsigned int n = 0; n < 2; ++n)
	{
		EXPECT_TRUE(after_string_array[n] == before_string_array[n]);
		for (unsigned int c = 0; c < 6; ++c)
		{
			EXPECT_TRUE(after_char_array[0][n][c] == before_char_array[0][n][c]);
		}
	}

	EXPECT_TRUE(relocated_after_non_default_constructable_array[0][0] == before_non_default_constructable_array[0][0]);
	EXPECT_TRUE(relocated_after_non_default_constructable_array[0][1] == before_non_default_constructable_array[0][1]);
	EXPECT_TRUE(after_non_default_constructable_array_element_ptr == &relocated_after_non_default_constructable_array[0][1]);
	EXPECT_TRUE(after_non_default_constructable_array_ptr == &relocated_after_non_default_constructable_array);
	EXPECT_TRUE(after_non_default_constructable_sub_array_ptr == relocated_after_non_default_constructable_array);
	EXPECT_TRUE((*after_non_default_constructable_sub_array_ptr)[1] == (*before_non_default_constructable_sub_array_ptr)[1]);
	EXPECT_TRUE((*after_non_default_constructable_sub_array_ptr)[1] == before_non_default_constructable_array[0][1]);
	EXPECT_TRUE(*after_non_default_constructable_sub_array_ptr_ptr == after_non_default_constructable_sub_array_ptr);
	EXPECT_TRUE(&(*after_non_default_constructable_sub_array_ptr)[1] == after_non_default_constructable_array_element_ptr);
}

GPlatesUnitTest::TranscribePrimitivesTest::Data::Data(
		int bv2_) :
	d(0), // dummy value
	geo_real_time(0), // dummy value
	geo_distant_past(0), // dummy value
	geo_distant_future(0), // dummy value
	pk(NULL),
	bv2(NonDefaultConstructable(bv2_))
{
}

GPlatesUnitTest::TranscribePrimitivesTest::Data::Data(
		const boost::variant<NonDefaultConstructable, char, QString, double> &bv2_) :
	d(0), // dummy value
	geo_real_time(0), // dummy value
	geo_distant_past(0), // dummy value
	geo_distant_future(0), // dummy value
	pk(NULL),
	bv2(bv2_)
{
}

GPlatesUnitTest::TranscribePrimitivesTest::Data::~Data()
{
	delete pk;
}

void
GPlatesUnitTest::TranscribePrimitivesTest::Data::initialise()
{
	ia[0][0] = 0;
	ia[0][1] = -2147483647; // most-negative 32-bit signed number plus one
	ia[1][0] = 2000000;
	ia[1][1] = -3000000;
	e = ENUM_VALUE_2;
	e2 = ENUM2_VALUE_3;
	b = true;
	f = 1432.812938f;
	const_cast<double &>(d) = 1.873822137385623e6;
	f_pos_inf = GPlatesMaths::positive_infinity<float>();
	f_neg_inf = GPlatesMaths::negative_infinity<float>();
	f_nan = GPlatesMaths::quiet_nan<float>();
	d_pos_inf = GPlatesMaths::positive_infinity<double>();
	d_neg_inf = GPlatesMaths::negative_infinity<double>();
	d_nan = GPlatesMaths::quiet_nan<double>();
	real = GPlatesMaths::Real(0.234991232);
	const_cast<GPlatesPropertyValues::GeoTimeInstant &>(geo_real_time) = GPlatesPropertyValues::GeoTimeInstant(100);
	const_cast<GPlatesPropertyValues::GeoTimeInstant &>(geo_distant_past) = GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
	const_cast<GPlatesPropertyValues::GeoTimeInstant &>(geo_distant_future) = GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	c = 'a';
	s = 0x7fff;
	l = 0x7fffffff;
	i = 10;
	j = static_cast<int>(0x80000000); // most-negative 32-bit signed number
	// Test a range of negative and positive signed integers (to test varint encoding in binary archive).
	for (int count = -40; count <= 40; ++count)
	{
		const int abs_count = std::abs(count);
		if (abs_count < 10)
		{
			signed_ints.push_back(count * 25);
		}
		else if (abs_count < 20)
		{
			signed_ints.push_back(count * 327);
		}
		else if (abs_count < 30)
		{
			signed_ints.push_back(count * 54623);
		}
		else
		{
			signed_ints.push_back(count * 7000123);
		}
	}
	u = QString("Test String");
	uw.str = QString("Test String Wrapper");
	pi = &i;
	pj = &j;
	pk = NULL;
	pl = NULL;
	ps = NULL;
	ppi = &pi;
	pr = std::make_pair(10, "10");
	str_deq.push_front("front");
	str_deq.push_back("back");
	double_stack.push(20.4);
	double_stack.push(-190.6);
	double_stack.push(3.234e100);
	string_stack_queue.push(std::stack<std::string>());
	string_stack_queue.push(std::stack<std::string>());
	string_stack_queue.front().push("str1");
	string_stack_queue.front().push("str2");
	string_stack_queue.back().push("str3");
	string_stack_queue.back().push("str4");
	int_priority_queue.push(-100);
	int_priority_queue.push(-27341232);
	int_priority_queue.push(472623682);
	v.push_back(13);
	v.push_back(-14);
	vv.push_back(v);
	pk = new int(-12000);
	pl = &vv[0][1];
	ilist.push_back(4);
	ilist.push_back(-50);
	str_set.insert("dog");
	str_set.insert("cat");
	int_str_map_vec.resize(1);
	int_str_map_vec[0].insert(std::make_pair(3, "3"));
	int_str_map_vec[0].insert(std::make_pair(4, "4"));
	ps = &int_str_map_vec[0][4];
	int_str_map_vec[0].insert(std::make_pair(5, "5"));
	int_qstr_qmap_qvec.resize(2);
	int_qstr_qmap_qvec[0].insert(3, "3");
	int_qstr_qmap_qvec[0].insert(4, "4");
	int_qstr_qmap_qvec[1].insert(5, "5");
	pqs = &int_qstr_qmap_qvec[0][4];
	qstr_set.insert("one"); qstr_set.insert("two"); qstr_set.insert("three");
	qstr_list.push_back("one"); qstr_list.push_back("two"); qstr_list.push_back("three");
	pqs2 = &const_cast<QString &>(*qstr_list.begin());

	// Test a string with an embedded zero in it.
	const std::string test_std_string("Test S\0tring", 12);
	const QString test_q_string = QString::fromLatin1(test_std_string.data(), test_std_string.length());
	const StringWithEmbeddedZeros string_with_zeros = { test_q_string };
	vu.push_back(string_with_zeros);

	// Leave 'bin' and 'brin' as boost::none.

	bi.reset(new boost::optional<int>(213));
	bri = bi->get(); // boost::optional<const int &> with reference to the integer inside 'bi'.

	bv = "variant string";
	pbv = boost::get<QString>(&bv);

	// Leave 'bv2' to what it was initialised via 'Data' constructor.
	pbv2 = boost::get<NonDefaultConstructable>(&bv2);

	qv.setValue(QString("qvar_string_value"));

	// Test wrapping a user-defined type into QVariant - requires registration with Qt.
	qRegisterMetaType<StringWithEmbeddedZeros>(
			"GPlatesUnitTest::TranscribePrimitivesTest::Data::StringWithEmbeddedZeros");
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
	// No longer required in Qt6 (it can determine it directly from the type).
	qRegisterMetaTypeStreamOperators<StringWithEmbeddedZeros>(
			"GPlatesUnitTest::TranscribePrimitivesTest::Data::StringWithEmbeddedZeros");
#endif
	const StringWithEmbeddedZeros qvar_string_with_zeros = { test_q_string };
	qv_reg.setValue(qvar_string_with_zeros);

	lqv.append(QVariant(20.5));
	lqv.append(QVariant("test_lqv_string"));
	lqv.append(QVariant::fromValue(qvar_string_with_zeros));

	qv_list.setValue(lqv);
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribePrimitivesTest::Data::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, e, "e", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, e2, "e2", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, c, "c", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, s, "s", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, l, "l", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, ppi, "ppi", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, pi, "pi", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, pj, "pj", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, pk, "pk", GPlatesScribe::EXCLUSIVE_OWNER | GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, ps, "ps", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, pqs, "pqs", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, pqs2, "pqs2", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, j, "j", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, i, "i", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, pr, "pr", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, str_deq, "str_deq", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, double_stack, "double_stack", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, string_stack_queue, "string_stack_queue", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, int_priority_queue, "int_priority_queue", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, v, "v", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, vu, "vu", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, ilist, "ilist", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, str_set, "str_set", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, int_str_map_vec, "int_str_map_vec", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, int_qstr_qmap_qvec, "int_qstr_qmap_qvec", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, qstr_set, "qstr_set", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, qstr_list, "qstr_list", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, bin, "bin", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, brin, "brin", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, bi, "bi", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, bri, "bri", GPlatesScribe::TRACK) || // Must be transcribed after 'bi' since references it.
		!scribe.transcribe(TRANSCRIBE_SOURCE, pbv, "pbv", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, pbv2, "pbv2", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, bv, "bv", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, qv, "qv", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, qv_reg, "qv_reg", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, lqv, "lqv", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, qv_list, "qv_list", GPlatesScribe::TRACK))
	{
		return scribe.get_transcribe_result();
	}

	// Should be able to save to double and load as float (and vice versa) provided the double value
	// is within the range of a 'float'.
	// They should also be transcription compatible with Real, and GeoTimeInstant (except for NaN).
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, f, "f", GPlatesScribe::TRACK);
		scribe.save(TRANSCRIBE_SOURCE, d, "d", GPlatesScribe::TRACK);
		scribe.save(TRANSCRIBE_SOURCE, real, "real", GPlatesScribe::TRACK);
		scribe.save(TRANSCRIBE_SOURCE, geo_real_time, "geo_real_time", GPlatesScribe::TRACK);
		scribe.save(TRANSCRIBE_SOURCE, geo_distant_past, "geo_distant_past", GPlatesScribe::TRACK);
		scribe.save(TRANSCRIBE_SOURCE, geo_distant_future, "geo_distant_future", GPlatesScribe::TRACK);
	}
	else
	{
		// Get the actual saved values to start with.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, f, "f") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d, "d") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, real, "real") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, geo_real_time, "geo_real_time") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, geo_distant_past, "geo_distant_past") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, geo_distant_future, "geo_distant_future"))
		{
			return scribe.get_transcribe_result();
		}

		float f_from_d;
		float f_from_geo_distant_future;
		double d_from_f;
		double d_from_real;
		double d_from_f_pos_inf;
		GPlatesMaths::Real real_from_f;
		GPlatesMaths::Real real_from_f_nan;
		GPlatesMaths::Real real_from_geo_distant_past;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, f_from_d, "d") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, f_from_geo_distant_future, "geo_distant_future") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_from_f, "f") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_from_real, "real") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_from_f_pos_inf, "f_pos_inf") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, real_from_f, "f") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, real_from_f_nan, "f_nan") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, real_from_geo_distant_past, "geo_distant_past"))
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GPlatesPropertyValues::GeoTimeInstant> geo_from_f =
				scribe.load<GPlatesPropertyValues::GeoTimeInstant>(TRANSCRIBE_SOURCE, "f");
		if (!geo_from_f.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GPlatesPropertyValues::GeoTimeInstant> geo_from_f_pos_inf =
				scribe.load<GPlatesPropertyValues::GeoTimeInstant>(TRANSCRIBE_SOURCE, "f_pos_inf");
		if (!geo_from_f_pos_inf.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GPlatesPropertyValues::GeoTimeInstant> geo_from_real =
				scribe.load<GPlatesPropertyValues::GeoTimeInstant>(TRANSCRIBE_SOURCE, "real");
		if (!geo_from_real.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPLATES_EXPECT_CLOSE_PERCENT(f_from_d, d, 0.0001);
		EXPECT_TRUE(GPlatesMaths::is_negative_infinity(f_from_geo_distant_future));
		GPLATES_EXPECT_CLOSE_PERCENT(d_from_f, f, 0.0001);
		GPLATES_EXPECT_CLOSE_PERCENT(d_from_real, real.dval(), 0.000000001);
		EXPECT_TRUE(GPlatesMaths::is_positive_infinity(d_from_f_pos_inf));
		GPLATES_EXPECT_CLOSE_PERCENT(real_from_f.dval(), f, 0.0001);
		EXPECT_TRUE(real_from_f_nan.is_nan());
		EXPECT_TRUE(real_from_geo_distant_past.is_positive_infinity());
		GPLATES_EXPECT_CLOSE_PERCENT(real_from_f.dval(), f, 0.0001);
		GPLATES_EXPECT_CLOSE_PERCENT(geo_from_f->value(), f, 0.0001);
		EXPECT_TRUE(geo_from_f_pos_inf->is_distant_past());
		GPLATES_EXPECT_CLOSE_PERCENT(geo_from_real->value(), real.dval(), 0.000000001);

		// Read them in again but with tracking enabled.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, f, "f", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d, "d", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, real, "real", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, geo_real_time, "geo_real_time", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, geo_distant_past, "geo_distant_past", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, geo_distant_future, "geo_distant_future", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Do after above code since it loaded untracked objects from tags "f_pos_inf" and "f_nan".
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, f_pos_inf, "f_pos_inf", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, f_neg_inf, "f_neg_inf", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, f_nan, "f_nan", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_pos_inf, "d_pos_inf", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_neg_inf, "d_neg_inf", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_nan, "d_nan", GPlatesScribe::TRACK))
	{
		return scribe.get_transcribe_result();
	}

	// Test transcription compatibility of a native array and a sequence container (std::vector).
	// Should be able to save one type and load the other type (and vice versa).
	if (scribe.is_saving())
	{
		scribe.transcribe(TRANSCRIBE_SOURCE, ia, "ia", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, vv, "vv", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, pl, "pl", GPlatesScribe::TRACK);
	}
	else
	{
		// Get the actual saved values to start with.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, ia, "ia") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, vv, "vv"))
		{
			return scribe.get_transcribe_result();
		}

		// Load 'ia' and 'vv' using each others tags.
		int ia_from_vv[1][2];
		std::vector< std::vector<int> > vv_from_ia;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, ia_from_vv, "vv") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, vv_from_ia, "ia"))
		{
			return scribe.get_transcribe_result();
		}

		EXPECT_TRUE(vv.size() == 1 &&
				vv[0].size() == 2 &&
				ia_from_vv[0][0] == vv[0][0] &&
				ia_from_vv[0][1] == vv[0][1]);
		EXPECT_TRUE(vv_from_ia.size() == 2 &&
				vv_from_ia[0].size() == 2 &&
				vv_from_ia[1].size() == 2 &&
				vv_from_ia[0][0] == ia[0][0] &&
				vv_from_ia[0][1] == ia[0][1] &&
				vv_from_ia[1][0] == ia[1][0] &&
				vv_from_ia[1][1] == ia[1][1]);

		// Read them in again but with tracking enabled.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, ia, "ia", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, vv, "vv", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}

		// Transcribe this after 'vv' since it has a pointer into 'vv' and we transcribe 'vv'
		// with tracking disabled above (which generates an error if it already has a transcribed
		// pointer referencing it).
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, pl, "pl", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Test transcription compatibility of QString and QStringWrapper (later uses 'transcribe_delegate_protocol()').
	// Should be able to save one type and load the other type (and vice versa).
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, u, "u", GPlatesScribe::TRACK);
		scribe.save(TRANSCRIBE_SOURCE, uw, "uw", GPlatesScribe::TRACK);
	}
	else
	{
		// Get the actual saved values to start with.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, u, "u") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, uw, "uw"))
		{
			return scribe.get_transcribe_result();
		}

		// Load 'u' and 'uw' using each others tags.
		const QString u_from_uw;
		const QStringWrapper uw_from_u;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, u_from_uw, "uw") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, uw_from_u, "u"))
		{
			return scribe.get_transcribe_result();
		}

		EXPECT_TRUE(u_from_uw == uw.str);
		EXPECT_TRUE(uw_from_u.str == u);

		// Read them in again but with tracking enabled.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, u, "u", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, uw, "uw", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Test 'signed_ints' a little differently to ensure that a std::vector's elements also get
	// untracked when the std::vector itself is untracked. The load will succeed if we transcribe it
	// twice and it doesn't complain that elements are being transcribed twice. We can't transcribe
	// the save twice though since we can't overwrite an entry in the scribed transcription.
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, signed_ints, "signed_ints", GPlatesScribe::TRACK);
	}
	else
	{
		// Should be able to load untracked any number of times.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, signed_ints, "signed_ints") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, signed_ints, "signed_ints"))
		{
			return scribe.get_transcribe_result();
		}

		// Can only load tracked once though.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, signed_ints, "signed_ints", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	// If already transcribed using (non-default) constructor then nothing left to do.
	if (!scribe.has_been_transcribed(bv2))
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, bv2, "bv2", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


// There's two ways to construct class Data (one using 'int' constructor and one using 'variant' constructor).
#define SAVE_LOAD_CLASS_DATA_USING_VARIANT

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribePrimitivesTest::Data::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<Data> &data)
{
	if (scribe.is_saving())
	{
#ifdef SAVE_LOAD_CLASS_DATA_USING_VARIANT
		// Mirror load path.
		scribe.save(TRANSCRIBE_SOURCE, data->bv2, "bv2", GPlatesScribe::TRACK);
#else
		// Nothing to transcribe - it happens when 'bv2' is transcribed in 'transcribe()'.
#endif
	}
	else // loading...
	{
#ifdef SAVE_LOAD_CLASS_DATA_USING_VARIANT
		GPlatesScribe::LoadRef<
				boost::variant<
						GPlatesUnitTest::TranscribePrimitivesTest::Data::NonDefaultConstructable,
						char,
						QString,
						double> > bv2 = scribe.load<
								boost::variant<
										GPlatesUnitTest::TranscribePrimitivesTest::Data::NonDefaultConstructable,
										char,
										QString,
										double> >(TRANSCRIBE_SOURCE, "bv2", GPlatesScribe::TRACK);
		if (!bv2.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		data.construct_object(bv2);

		scribe.relocated(TRANSCRIBE_SOURCE, data->bv2, bv2);
#else
		data.construct_object(10/*dummy integer value*/);
#endif
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
GPlatesUnitTest::TranscribePrimitivesTest::Data::check_equality(
		const Data &other)
{
	SCOPED_TRACE("check_equality");

	EXPECT_TRUE(ia[0][0] == other.ia[0][0]);
	EXPECT_TRUE(ia[0][1] == other.ia[0][1]);
	EXPECT_TRUE(ia[1][0] == other.ia[1][0]);
	EXPECT_TRUE(ia[1][1] == other.ia[1][1]);
	EXPECT_TRUE(e == other.e);
	EXPECT_TRUE(e2 == other.e2);
	EXPECT_TRUE(b == other.b);
	// Cast to 'double' to avoid compiler warning in boost unit test code...
	GPLATES_EXPECT_CLOSE_PERCENT(double(f), double(other.f), 0.001);
	GPLATES_EXPECT_CLOSE_PERCENT(d, other.d, 0.000000001);
	EXPECT_TRUE(GPlatesMaths::is_positive_infinity(f_pos_inf) && GPlatesMaths::is_positive_infinity(other.f_pos_inf));
	EXPECT_TRUE(GPlatesMaths::is_negative_infinity(f_neg_inf) && GPlatesMaths::is_negative_infinity(other.f_neg_inf));
	EXPECT_TRUE(GPlatesMaths::is_nan(f_nan) && GPlatesMaths::is_nan(other.f_nan));
	EXPECT_TRUE(GPlatesMaths::is_positive_infinity(d_pos_inf) && GPlatesMaths::is_positive_infinity(other.d_pos_inf));
	EXPECT_TRUE(GPlatesMaths::is_negative_infinity(d_neg_inf) && GPlatesMaths::is_negative_infinity(other.d_neg_inf));
	EXPECT_TRUE(GPlatesMaths::is_nan(d_nan) && GPlatesMaths::is_nan(other.d_nan));
	EXPECT_TRUE(real == other.real);
	EXPECT_TRUE(geo_real_time == other.geo_real_time);
	EXPECT_TRUE(geo_distant_past == other.geo_distant_past);
	EXPECT_TRUE(geo_distant_future == other.geo_distant_future);
	EXPECT_TRUE(c == other.c);
	EXPECT_TRUE(s == other.s);
	EXPECT_TRUE(l == other.l);
	EXPECT_TRUE(pi && other.pi && (*pi == *other.pi));
	EXPECT_TRUE(pi == &i);
	EXPECT_TRUE(pj && other.pj && (*pj == *other.pj));
	EXPECT_TRUE(pj == &j);
	EXPECT_TRUE(pk && other.pk && (*pk == *other.pk));
	EXPECT_TRUE(pl && other.pl && (*pl == *other.pl));
	EXPECT_TRUE(pl == &vv[0][1]);
	EXPECT_TRUE(ps && other.ps && (*ps == *other.ps));
	EXPECT_TRUE(ps == &int_str_map_vec[0][4]);
	EXPECT_TRUE(pqs && other.pqs && (*pqs == *other.pqs));
	EXPECT_TRUE(pqs == &int_qstr_qmap_qvec[0][4]);
	EXPECT_TRUE(pqs2 && other.pqs2 && (*pqs2 == *other.pqs2));
	EXPECT_TRUE(pqs2 == &const_cast<QString &>(*qstr_list.begin()));
	EXPECT_TRUE(ppi && other.ppi && *pi && *other.pi && (**ppi == **other.ppi));
	EXPECT_TRUE(ppi == &pi && *ppi == &i);
	EXPECT_TRUE(signed_ints == other.signed_ints);
	EXPECT_TRUE(j == other.j);
	EXPECT_TRUE(i == other.i);
	EXPECT_TRUE(u == other.u);
	EXPECT_TRUE(uw == other.uw);
	EXPECT_TRUE(pr == other.pr);
	EXPECT_TRUE(str_deq == other.str_deq);
	EXPECT_TRUE(double_stack == other.double_stack);
	EXPECT_TRUE(string_stack_queue == other.string_stack_queue);

	// Compare std::priority_queue<int>, but there's no equality operator...
	std::priority_queue<int> int_priority_queue_copy = int_priority_queue;
	std::priority_queue<int> other_int_priority_queue_copy = other.int_priority_queue;
	EXPECT_TRUE(int_priority_queue_copy.size() == other_int_priority_queue_copy.size());
	if (int_priority_queue_copy.size() == other_int_priority_queue_copy.size())
	{
		while (!int_priority_queue_copy.empty())
		{
			EXPECT_TRUE(int_priority_queue_copy.top() == other_int_priority_queue_copy.top());
			int_priority_queue_copy.pop();
			other_int_priority_queue_copy.pop();
		}
	}

	EXPECT_TRUE(v == other.v);
	EXPECT_TRUE(vv == other.vv);
	EXPECT_TRUE(vu == other.vu);
	EXPECT_TRUE(vu.size() == 1 && vu[0].str.length() == 12); // Ensure string wasn't clipped at first embedded zero.
	EXPECT_TRUE(ilist == other.ilist);
	EXPECT_TRUE(str_set == other.str_set);
	EXPECT_TRUE(int_str_map_vec == other.int_str_map_vec);
	EXPECT_TRUE(int_qstr_qmap_qvec == other.int_qstr_qmap_qvec);
	EXPECT_TRUE(qstr_set == other.qstr_set);
	EXPECT_TRUE(qstr_list == other.qstr_list);
	EXPECT_TRUE(bin == other.bin && bin == boost::none);
	EXPECT_TRUE(brin == other.brin && brin == boost::none);
	EXPECT_TRUE(bi && other.bi && (*bi == *other.bi));
	EXPECT_TRUE(bri == other.bri);
	// boost::optional<const int &> with reference to the integer inside 'bi'...
	EXPECT_TRUE(bri && bi && *bi && (&bri.get() == &bi->get()));
	EXPECT_TRUE(pbv && other.pbv && (*pbv == *other.pbv));
	EXPECT_TRUE(pbv == boost::get<QString>(&bv));
	EXPECT_TRUE(pbv2 && other.pbv2 && (*pbv2 == *other.pbv2));
	EXPECT_TRUE(pbv2 == boost::get<NonDefaultConstructable>(&bv2));
	EXPECT_TRUE(bv == other.bv);
	EXPECT_TRUE(bv2 == other.bv2);
	EXPECT_TRUE(qv == other.qv);
	// Qt6: custom type ids start at QMetaType::User (Qt5 always returned User).
	EXPECT_TRUE(qv_reg.userType() >= QMetaType::User && other.qv_reg.userType() >= QMetaType::User &&
			qv_reg.userType() == other.qv_reg.userType() &&
			qv_reg.canConvert<StringWithEmbeddedZeros>() && other.qv_reg.canConvert<StringWithEmbeddedZeros>() &&
			qv_reg.value<StringWithEmbeddedZeros>() == other.qv_reg.value<StringWithEmbeddedZeros>());
	EXPECT_TRUE(lqv.size() == other.lqv.size());
	// 'qv_list' is just a QVariant wrapped around 'lqv'.
	EXPECT_TRUE(qv_list.userType() == QMetaType::QVariantList && other.qv_list.userType() == QMetaType::QVariantList &&
			qv_list.canConvert< QList<QVariant> >() && other.qv_list.canConvert< QList<QVariant> >() &&
			qv_list.value< QList<QVariant> >().size() == lqv.size() &&
			other.qv_list.value< QList<QVariant> >().size() == other.lqv.size());
	for (int n = 0; n < lqv.size(); ++n)
	{
		EXPECT_TRUE(lqv[n].userType() == other.lqv[n].userType());
		// 'qv_list' is just a QVariant wrapped around 'lqv'.
		EXPECT_TRUE(qv_list.value< QList<QVariant> >()[n].userType() == other.qv_list.value< QList<QVariant> >()[n].userType());

		// Qt6: custom type ids start at QMetaType::User (Qt5 always returned User).
		if (lqv[n].userType() >= QMetaType::User)
		{
			EXPECT_TRUE(
					lqv[n].canConvert<StringWithEmbeddedZeros>() && other.lqv[n].canConvert<StringWithEmbeddedZeros>() &&
					lqv[n].value<StringWithEmbeddedZeros>() == other.lqv[n].value<StringWithEmbeddedZeros>());
			// 'qv_list' is just a QVariant wrapped around 'lqv'.
			EXPECT_TRUE(
					qv_list.value< QList<QVariant> >()[n].canConvert<StringWithEmbeddedZeros>() &&
						other.qv_list.value< QList<QVariant> >()[n].canConvert<StringWithEmbeddedZeros>() &&
					qv_list.value< QList<QVariant> >()[n].value<StringWithEmbeddedZeros>() ==
						other.qv_list.value< QList<QVariant> >()[n].value<StringWithEmbeddedZeros>());
		}
		else
		{
			EXPECT_TRUE(lqv[n] == other.lqv[n]);
			// 'qv_list' is just a QVariant wrapped around 'lqv'.
			EXPECT_TRUE(qv_list.value< QList<QVariant> >()[n] == other.qv_list.value< QList<QVariant> >()[n]);
		}
	}
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribePrimitivesTest::Data::StringWithEmbeddedZeros::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	QByteArray byte_array;

	if (scribe.is_saving())
	{
		byte_array = str.toUtf8();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, byte_array, "byte_array", GPlatesScribe::TRACK))
	{
		return scribe.get_transcribe_result();
	}

	if (scribe.is_loading())
	{
		str = QString::fromUtf8(byte_array.data(), byte_array.size());
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribePrimitivesTest::Data::QStringWrapper::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
#if 1 // test using transcribe delegate protocol...

	if (!transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, str))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;

#else // test using save/load delegate protocol...

	if (scribe.is_saving())
	{
		save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, str);
	}
	else
	{
		GPlatesScribe::LoadRef<QString> str_ref =
				GPlatesScribe::load_delegate_protocol<QString>(TRANSCRIBE_SOURCE, scribe);
		if (!str_ref.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		str = str_ref;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;

#endif
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::transcribe(
		GPlatesScribe::Scribe &scribe,
		TranscribePrimitivesTest::Data::Enum &e,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("ENUM_VALUE_1", TranscribePrimitivesTest::Data::ENUM_VALUE_1),
		GPlatesScribe::EnumValue("ENUM_VALUE_2", TranscribePrimitivesTest::Data::ENUM_VALUE_2),
		GPlatesScribe::EnumValue("ENUM_VALUE_3", TranscribePrimitivesTest::Data::ENUM_VALUE_3)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			e,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::transcribe(
		GPlatesScribe::Scribe &scribe,
		TranscribePrimitivesTest::Data::NonDefaultConstructable &ndc,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, ndc.i, "i", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<TranscribePrimitivesTest::Data::NonDefaultConstructable> &ndc)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, ndc->i, "i", GPlatesScribe::TRACK);
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<int> i = scribe.load<int>(TRANSCRIBE_SOURCE, "i", GPlatesScribe::TRACK);
		if (!i.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		ndc.construct_object(i);
		scribe.relocated(TRANSCRIBE_SOURCE, ndc->i, i);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

void
GPlatesUnitTest::TranscribeUntrackedTest::test_case_untracked_exception()
{
	variant_type var(11);
	variant_type *var_ptr = &var;
	const variant_type *const *const var_ptr_ptr = &var_ptr;

	//
	// Transcribing an *untracked* pointer before transcribing object should throw an exception.
	//

	// Skip this test in debug build because GPlatesGlobal::Assert() aborts instead of
	// throwing an exception and this test checks for exceptions...
#ifndef GPLATES_DEBUG
	{
		GPlatesScribe::Scribe scribe;

		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr, "var_ptr"),
				GPlatesScribe::Exceptions::TranscribedUntrackedPointerBeforeReferencedObject);
	}
#endif
	// Skip this test in debug build because GPlatesGlobal::Assert() aborts instead of
	// throwing an exception and this test checks for exceptions...
#ifndef GPLATES_DEBUG
	{
		GPlatesScribe::Scribe scribe;

		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr_ptr, "var_ptr_ptr"),
				GPlatesScribe::Exceptions::TranscribedUntrackedPointerBeforeReferencedObject);
	}
#endif

	//
	// Transcribing an *untracked* object that has pointers referencing it should throw an exception.
	//

	// Skip this test in debug build because GPlatesGlobal::Assert() aborts instead of
	// throwing an exception and this test checks for exceptions...
#ifndef GPLATES_DEBUG
	{
		GPlatesScribe::Scribe scribe;

		scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr, "var_ptr", GPlatesScribe::TRACK);

		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, var, "var"),
				GPlatesScribe::Exceptions::UntrackingObjectWithReferences);
	}
#endif
	{
		GPlatesScribe::Scribe scribe;

		scribe.transcribe(TRANSCRIBE_SOURCE, var, "var");

		// This won't find 'var'.
		scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr, "var_ptr", GPlatesScribe::TRACK);

		EXPECT_TRUE(
				!scribe.is_transcription_complete(false/*emit_warnings*/));
	}
	// Skip this test in debug build because GPlatesGlobal::Assert() aborts instead of
	// throwing an exception and this test checks for exceptions...
#ifndef GPLATES_DEBUG
	{
		GPlatesScribe::Scribe scribe;

		scribe.transcribe(TRANSCRIBE_SOURCE, var, "var", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr_ptr, "var_ptr_ptr", GPlatesScribe::TRACK);

		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr, "var_ptr"),
				GPlatesScribe::Exceptions::UntrackingObjectWithReferences);
	}
#endif
	{
		GPlatesScribe::Scribe scribe;

		scribe.transcribe(TRANSCRIBE_SOURCE, var, "var", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr, "var_ptr");

		// This won't find 'var_ptr'.
		scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr_ptr, "var_ptr_ptr", GPlatesScribe::TRACK);

		EXPECT_TRUE(
				!scribe.is_transcription_complete(false/*emit_warnings*/));
	}
}

void 
GPlatesUnitTest::TranscribeUntrackedTest::test_case_untracked_1()
{
	variant_type before_var(10);

	try
	{
		//
		// Text archive
		//
		{
			SCOPED_TRACE("text archive");

			std::stringstream text_archive;

			test_case_untracked_1_write(
					GPlatesScribe::TextArchiveWriter::create(text_archive),
					before_var);

			text_archive.seekp(0);

			test_case_untracked_1_read(
					GPlatesScribe::TextArchiveReader::create(text_archive),
					before_var);
		}

		//
		// Binary archive
		//
		{
			SCOPED_TRACE("binary archive");

			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			test_case_untracked_1_write(
					GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer),
					before_var);

			binary_archive.close();

#if 0
			QFile archive_binary_file(test_scratch_file_path("archive_untracked_1.bin"));
			bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
			EXPECT_TRUE(archive_binary_open_for_writing);
			if (archive_binary_open_for_writing)
			{
				archive_binary_file.write(binary_archive.data());
			}
			archive_binary_file.close();
#endif

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			test_case_untracked_1_read(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader),
					before_var);
		}

		//
		// XML archive
		//
		{
			SCOPED_TRACE("XML archive");

			QBuffer xml_archive;
			xml_archive.open(QBuffer::WriteOnly);

			QXmlStreamWriter xml_stream_writer(&xml_archive);
			xml_stream_writer.writeStartDocument();

			test_case_untracked_1_write(
					GPlatesScribe::XmlArchiveWriter::create(xml_stream_writer),
					before_var);

			xml_stream_writer.writeEndDocument();

			xml_archive.close();

#if 0
			QFile archive_xml_file(test_scratch_file_path("archive_untracked_1.xml"));
			bool archive_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
			EXPECT_TRUE(archive_open_for_writing);
			if (archive_open_for_writing)
			{
				archive_xml_file.write(xml_archive.data());
			}
			archive_xml_file.close();
#endif

			xml_archive.open(QBuffer::ReadOnly);
			xml_archive.seek(0);

			QXmlStreamReader xml_stream_reader(&xml_archive);
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isStartDocument());

			GPlatesScribe::XmlArchiveReader::non_null_ptr_type xml_archive_reader =
					GPlatesScribe::XmlArchiveReader::create(xml_stream_reader);

			test_case_untracked_1_read(
					xml_archive_reader,
					before_var);

			xml_archive_reader->close();
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isEndDocument());
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}

void
GPlatesUnitTest::TranscribeUntrackedTest::test_case_untracked_1_write(
		const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
		variant_type &before_variant)
{
	SCOPED_TRACE("test_case_untracked_1_write");

	GPlatesScribe::Scribe scribe;

	scribe.transcribe(TRANSCRIBE_SOURCE, before_variant, "variant", GPlatesScribe::TRACK);

	EXPECT_TRUE(scribe.is_transcription_complete());

	archive_writer->write_transcription(*scribe.get_transcription());
}

void
GPlatesUnitTest::TranscribeUntrackedTest::test_case_untracked_1_read(
		const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
		variant_type &before_variant)
{
	SCOPED_TRACE("test_case_untracked_1_read");

	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	variant_type after_variant;

	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_variant, "variant", GPlatesScribe::TRACK));

	EXPECT_TRUE(scribe.is_transcription_complete());

	EXPECT_TRUE(after_variant == before_variant);
}

void 
GPlatesUnitTest::TranscribeInheritanceTest::test_case_inheritance_1()
{
	UntranscribedClass untranscribed_object;
	boost::optional<int> before_d = 300;
	D before_data(before_d.get(), 300, int_pair_type(11, 22), int_pair_type(111, 122), untranscribed_object);
	before_data.initialise(100, 200);
	B *before_data_ptr = &before_data;
	int *before_x_ptr = before_data.x.get();
	// Reference internal sub-object 'a' of another D object.
	D before_data2(before_data.a, 900, int_pair_type(711, 722), int_pair_type(811, 822), untranscribed_object);
	before_data2.initialise(700, 800);
	E before_e(before_data);

	try
	{
		//
		// Text archive
		//
		{
			SCOPED_TRACE("text archive");

			std::stringstream text_archive;

			test_case_inheritance_1_write(
					GPlatesScribe::TextArchiveWriter::create(text_archive),
					untranscribed_object,
					before_d,
					before_data,
					before_data_ptr,
					before_x_ptr,
					before_data2,
					before_e);

			text_archive.seekp(0);

			test_case_inheritance_1_read(
					GPlatesScribe::TextArchiveReader::create(text_archive),
					untranscribed_object,
					before_d,
					before_data,
					before_data_ptr,
					before_x_ptr,
					before_data2,
					before_e);
		}

		//
		// Binary archive
		//
		{
			SCOPED_TRACE("binary archive");

			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			test_case_inheritance_1_write(
					GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer),
					untranscribed_object,
					before_d,
					before_data,
					before_data_ptr,
					before_x_ptr,
					before_data2,
					before_e);

			binary_archive.close();

#if 0
			QFile archive_binary_file(test_scratch_file_path("archive_inheritance_1.bin"));
			bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
			EXPECT_TRUE(archive_binary_open_for_writing);
			if (archive_binary_open_for_writing)
			{
				archive_binary_file.write(binary_archive.data());
			}
			archive_binary_file.close();
#endif

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			test_case_inheritance_1_read(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader),
					untranscribed_object,
					before_d,
					before_data,
					before_data_ptr,
					before_x_ptr,
					before_data2,
					before_e);
		}

		//
		// XML archive
		//
		{
			SCOPED_TRACE("XML archive");

			QBuffer xml_archive;
			xml_archive.open(QBuffer::WriteOnly);

			QXmlStreamWriter xml_stream_writer(&xml_archive);
			xml_stream_writer.writeStartDocument();

			test_case_inheritance_1_write(
					GPlatesScribe::XmlArchiveWriter::create(xml_stream_writer),
					untranscribed_object,
					before_d,
					before_data,
					before_data_ptr,
					before_x_ptr,
					before_data2,
					before_e);

			xml_stream_writer.writeEndDocument();

			xml_archive.close();

#if 0
			QFile archive_xml_file(test_scratch_file_path("archive_inheritance_1.xml"));
			bool archive_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
			EXPECT_TRUE(archive_open_for_writing);
			if (archive_open_for_writing)
			{
				archive_xml_file.write(xml_archive.data());
			}
			archive_xml_file.close();
#endif

			xml_archive.open(QBuffer::ReadOnly);
			xml_archive.seek(0);

			QXmlStreamReader xml_stream_reader(&xml_archive);
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isStartDocument());

			GPlatesScribe::XmlArchiveReader::non_null_ptr_type xml_archive_reader =
					GPlatesScribe::XmlArchiveReader::create(xml_stream_reader);

			test_case_inheritance_1_read(
					xml_archive_reader,
					untranscribed_object,
					before_d,
					before_data,
					before_data_ptr,
					before_x_ptr,
					before_data2,
					before_e);

			xml_archive_reader->close();
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isEndDocument());
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}

void
GPlatesUnitTest::TranscribeInheritanceTest::test_case_inheritance_1_write(
		const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
		UntranscribedClass &untranscribed_object,
		boost::optional<int> &before_d,
		D &before_data,
		B *&before_data_ptr,
		int *&before_x_ptr,
		D &before_data2,
		E &before_e)
{
	SCOPED_TRACE("test_case_inheritance_1_write");

	GPlatesScribe::Scribe scribe;

	GPlatesScribe::TranscribeContext<A> transcribe_context_a(untranscribed_object);
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<A> transcribe_context_guard_a(
			scribe,
			transcribe_context_a);

	scribe.transcribe(TRANSCRIBE_SOURCE, before_x_ptr, "x", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_d, "d", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_data_ptr, "data_ptr", GPlatesScribe::TRACK);
	scribe.save(TRANSCRIBE_SOURCE, before_data2, "data2", GPlatesScribe::TRACK);
	scribe.save(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::TRACK);
	scribe.save(TRANSCRIBE_SOURCE, before_e, "data_e", GPlatesScribe::TRACK);

	EXPECT_TRUE(scribe.is_transcription_complete());

	archive_writer->write_transcription(*scribe.get_transcription());
}

void
GPlatesUnitTest::TranscribeInheritanceTest::test_case_inheritance_1_read(
		const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
		UntranscribedClass &untranscribed_object,
		boost::optional<int> &before_d,
		D &before_data,
		B *&before_data_ptr,
		int *&before_x_ptr,
		D &before_data2,
		E &before_e)
{
	SCOPED_TRACE("test_case_inheritance_1_read");

	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	GPlatesScribe::TranscribeContext<A> transcribe_context_a(untranscribed_object);
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<A> transcribe_context_guard_a(
			scribe,
			transcribe_context_a);

	boost::optional<int> after_d;
	B *after_data_ptr;
	int *after_x_ptr;

	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_x_ptr, "x", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_d, "d", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_ptr, "data_ptr", GPlatesScribe::TRACK));

	// 'after_data2' has reference to internal 'a' object of 'after_data' and
	// 'after_data' gets relocated below so transcribe 'after_data2' first so we can check
	// that its pointer reference points to the relocated 'after_data'.
	GPlatesScribe::LoadRef<D> after_data2_ref = scribe.load<D>(TRANSCRIBE_SOURCE, "data2", GPlatesScribe::TRACK);
	EXPECT_TRUE(after_data2_ref.is_valid());
	D after_data2 = after_data2_ref;
	scribe.relocated(TRANSCRIBE_SOURCE, after_data2, after_data2_ref);

	// Test object relocation where object ('D') has a non-empty abstract base class ('A').
	GPlatesScribe::LoadRef<D> after_data = scribe.load<D>(TRANSCRIBE_SOURCE, "data", GPlatesScribe::TRACK);
	EXPECT_TRUE(after_data.is_valid());
	D relocated_after_data(after_data);
	scribe.relocated(TRANSCRIBE_SOURCE, relocated_after_data, after_data);

	GPlatesScribe::LoadRef<E> after_e_ref = scribe.load<E>(TRANSCRIBE_SOURCE, "data_e", GPlatesScribe::TRACK);
	EXPECT_TRUE(after_e_ref.is_valid());
	E after_e = after_e_ref;
	scribe.relocated(TRANSCRIBE_SOURCE, after_e, after_e_ref);

	EXPECT_TRUE(scribe.is_transcription_complete());

	EXPECT_TRUE(after_x_ptr && (*after_x_ptr == *before_x_ptr));
	// Check relocation of 'D' and hence its explicit relocation handler properly relocates
	// its pointed-to 'x' integer which should update 'after_x_ptr'.
	EXPECT_TRUE(after_x_ptr && (after_x_ptr == relocated_after_data.x.get()));

	EXPECT_TRUE(after_d == before_d);
	// Make sure points to relocated object (not original object).
	EXPECT_TRUE(after_data_ptr && (after_data_ptr == static_cast<B *>(&relocated_after_data)));
	relocated_after_data.check_equality(before_data);
	after_data2.check_equality(before_data2);
	// Make sure points to relocated object (not original object).
	EXPECT_TRUE(after_data2.d == &relocated_after_data.a);
	if (after_data_ptr)
	{
		static_cast<D &>(*after_data_ptr).check_equality(before_data);
	}
	// Make sure points to untranscribed object.
	EXPECT_TRUE(&relocated_after_data.untranscribed_object == &untranscribed_object);
	EXPECT_TRUE(&after_data2.untranscribed_object == &untranscribed_object);
	// Make sure points to relocated object (not original object).
	EXPECT_TRUE(&after_e.b == static_cast<B *>(&relocated_after_data));
	after_e.check_equality(before_e);
}

void 
GPlatesUnitTest::TranscribeInheritanceTest::test_case_inheritance_2()
{
	UntranscribedClass untranscribed_object;
	boost::scoped_ptr<int> before_d( new int(300));
	boost::shared_ptr<B> before_data_ptr(
			new D(*before_d, 300, int_pair_type(11, 22), int_pair_type(111, 122), untranscribed_object));
	boost::weak_ptr<B> before_data_weak_ptr = before_data_ptr;
	boost::shared_ptr<D> before_data_ptr2 = boost::static_pointer_cast<D>(before_data_ptr);
	// Initialise the full 'D' object.
	static_cast<D &>(*before_data_ptr).initialise(100, 200, before_data_ptr2);
	GPlatesUtils::non_null_intrusive_ptr<E> before_intrusive_ptr(new E(static_cast<D &>(*before_data_ptr)));

	try
	{
		//
		// Text archive
		//
		{
			SCOPED_TRACE("text archive");

			std::stringstream text_archive;

			test_case_inheritance_2_write(
					GPlatesScribe::TextArchiveWriter::create(text_archive),
					untranscribed_object,
					before_d,
					before_data_ptr,
					before_data_weak_ptr,
					before_data_ptr2,
					before_intrusive_ptr);

			text_archive.seekp(0);

			test_case_inheritance_2_read(
					GPlatesScribe::TextArchiveReader::create(text_archive),
					untranscribed_object,
					before_d,
					before_data_ptr,
					before_data_weak_ptr,
					before_data_ptr2,
					before_intrusive_ptr);
		}

		//
		// Binary archive
		//
		{
			SCOPED_TRACE("binary archive");

			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			test_case_inheritance_2_write(
					GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer),
					untranscribed_object,
					before_d,
					before_data_ptr,
					before_data_weak_ptr,
					before_data_ptr2,
					before_intrusive_ptr);

			binary_archive.close();

#if 0
			QFile archive_binary_file(test_scratch_file_path("archive_inheritance_2.bin"));
			bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
			EXPECT_TRUE(archive_binary_open_for_writing);
			if (archive_binary_open_for_writing)
			{
				archive_binary_file.write(binary_archive.data());
			}
			archive_binary_file.close();
#endif

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			test_case_inheritance_2_read(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader),
					untranscribed_object,
					before_d,
					before_data_ptr,
					before_data_weak_ptr,
					before_data_ptr2,
					before_intrusive_ptr);
		}

		//
		// XML archive
		//
		{
			SCOPED_TRACE("XML archive");

			QBuffer xml_archive;
			xml_archive.open(QBuffer::WriteOnly);

			QXmlStreamWriter xml_stream_writer(&xml_archive);
			xml_stream_writer.writeStartDocument();

			test_case_inheritance_2_write(
					GPlatesScribe::XmlArchiveWriter::create(xml_stream_writer),
					untranscribed_object,
					before_d,
					before_data_ptr,
					before_data_weak_ptr,
					before_data_ptr2,
					before_intrusive_ptr);

			xml_stream_writer.writeEndDocument();

			xml_archive.close();

#if 0
			QFile archive_xml_file(test_scratch_file_path("archive_inheritance_2.xml"));
			bool archive_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
			EXPECT_TRUE(archive_open_for_writing);
			if (archive_open_for_writing)
			{
				archive_xml_file.write(xml_archive.data());
			}
			archive_xml_file.close();
#endif

			xml_archive.open(QBuffer::ReadOnly);
			xml_archive.seek(0);

			QXmlStreamReader xml_stream_reader(&xml_archive);
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isStartDocument());

			GPlatesScribe::XmlArchiveReader::non_null_ptr_type xml_archive_reader =
					GPlatesScribe::XmlArchiveReader::create(xml_stream_reader);

			test_case_inheritance_2_read(
					xml_archive_reader,
					untranscribed_object,
					before_d,
					before_data_ptr,
					before_data_weak_ptr,
					before_data_ptr2,
					before_intrusive_ptr);

			xml_archive_reader->close();
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isEndDocument());
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}

void
GPlatesUnitTest::TranscribeInheritanceTest::test_case_inheritance_2_write(
		const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
		UntranscribedClass &untranscribed_object,
		boost::scoped_ptr<int> &before_d,
		boost::shared_ptr<B> &before_data_ptr,
		boost::weak_ptr<B> &before_data_weak_ptr,
		boost::shared_ptr<D> &before_data_ptr2,
		GPlatesUtils::non_null_intrusive_ptr<E> &before_intrusive_ptr)
{
	SCOPED_TRACE("test_case_inheritance_2_write");

	GPlatesScribe::Scribe scribe;

	GPlatesScribe::TranscribeContext<A> transcribe_context_a(untranscribed_object);
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<A> transcribe_context_guard_a(
			scribe,
			transcribe_context_a);

	scribe.transcribe(TRANSCRIBE_SOURCE, before_d, "d", GPlatesScribe::TRACK);
	// Transcribe through base class pointer.
	scribe.transcribe(TRANSCRIBE_SOURCE, before_data_weak_ptr, "data_weak_ptr", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_data_ptr, "data_ptr", GPlatesScribe::TRACK);
	scribe.transcribe(TRANSCRIBE_SOURCE, before_data_ptr2, "data_ptr2", GPlatesScribe::TRACK);
	scribe.save(TRANSCRIBE_SOURCE, before_intrusive_ptr, "data_intrusive_ptr", GPlatesScribe::TRACK);

	EXPECT_TRUE(scribe.is_transcription_complete());

	archive_writer->write_transcription(*scribe.get_transcription());
}

void
GPlatesUnitTest::TranscribeInheritanceTest::test_case_inheritance_2_read(
		const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
		UntranscribedClass &untranscribed_object,
		boost::scoped_ptr<int> &before_d,
		boost::shared_ptr<B> &before_data_ptr,
		boost::weak_ptr<B> &before_data_weak_ptr,
		boost::shared_ptr<D> &before_data_ptr2,
		GPlatesUtils::non_null_intrusive_ptr<E> &before_intrusive_ptr)
{
	SCOPED_TRACE("test_case_inheritance_2_read");

	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	GPlatesScribe::TranscribeContext<A> transcribe_context_a(untranscribed_object);
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<A> transcribe_context_guard_a(
			scribe,
			transcribe_context_a);

	boost::scoped_ptr<int> after_d;
	boost::shared_ptr<B> after_data_ptr;
	boost::weak_ptr<B> after_data_weak_ptr;
	boost::shared_ptr<D> after_data_ptr2;

	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_d, "d", GPlatesScribe::TRACK));
	// Transcribe through base class pointer.
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_weak_ptr, "data_weak_ptr", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_ptr, "data_ptr", GPlatesScribe::TRACK));
	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_ptr2, "data_ptr2", GPlatesScribe::TRACK));

	GPlatesScribe::LoadRef< GPlatesUtils::non_null_intrusive_ptr<E> > after_intrusive_ptr_ref =
			scribe.load< GPlatesUtils::non_null_intrusive_ptr<E> >(
					TRANSCRIBE_SOURCE, "data_intrusive_ptr", GPlatesScribe::TRACK);
	EXPECT_TRUE(after_intrusive_ptr_ref.is_valid());
	GPlatesUtils::non_null_intrusive_ptr<E> after_intrusive_ptr = after_intrusive_ptr_ref;
	scribe.relocated(TRANSCRIBE_SOURCE, after_intrusive_ptr, after_intrusive_ptr_ref);

	EXPECT_TRUE(scribe.is_transcription_complete());

	EXPECT_TRUE(after_d);
	EXPECT_TRUE(*after_d == *before_d);
	EXPECT_TRUE(after_data_ptr);
	EXPECT_TRUE(!after_data_weak_ptr.expired());
	EXPECT_TRUE(after_data_weak_ptr.lock() == after_data_ptr);
	EXPECT_TRUE(after_data_ptr2);
	// Apply typeid through a raw pointer, not the smart-pointer operator* (a
	// function call), so Clang does not flag -Wpotentially-evaluated-expression;
	// the operand is still evaluated to obtain the dynamic (RTTI) type.
	const B *after_data_ptr_raw = after_data_ptr.get();
	EXPECT_TRUE(after_data_ptr_raw && (typeid(*after_data_ptr_raw) == typeid(D)));
	const D *after_data_ptr2_raw = after_data_ptr2.get();
	EXPECT_TRUE(after_data_ptr2_raw && (typeid(*after_data_ptr2_raw) == typeid(D)));
	if (after_data_ptr)
	{
		static_cast<D &>(*after_data_ptr).check_equality(static_cast<const D &>(*before_data_ptr));
		EXPECT_TRUE(&static_cast<D &>(*after_data_ptr).untranscribed_object == &untranscribed_object);
	}
	if (after_data_ptr2)
	{
		static_cast<D &>(*after_data_ptr2).check_equality(static_cast<const D &>(*before_data_ptr));
		EXPECT_TRUE(&static_cast<D &>(*after_data_ptr2).untranscribed_object == &untranscribed_object);
	}
	EXPECT_TRUE(after_intrusive_ptr);
	if (after_intrusive_ptr)
	{
		after_intrusive_ptr->check_equality(*before_intrusive_ptr);
	}
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeInheritanceTest::A::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Derived class probably transcribed and passed in via our constructor.
	if (!scribe.has_been_transcribed(b_object))
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, b_object, "b_object", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Derived class probably transcribed and passed in via our constructor.
	if (!scribe.has_been_transcribed(a))
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "a", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

void
GPlatesUnitTest::TranscribeInheritanceTest::A::initialise(
		int b_)
{
	b_object.initialise(b_);
}

void
GPlatesUnitTest::TranscribeInheritanceTest::A::check_equality(
		const A &other) const
{
	SCOPED_TRACE("check_equality");

	b_object.check_equality(other.b_object);
	EXPECT_TRUE(a == other.a);
}

void
GPlatesUnitTest::TranscribeInheritanceTest::B::initialise(
		int b_)
{
	b = b_;
}

void
GPlatesUnitTest::TranscribeInheritanceTest::B::check_equality(
		const B &other) const
{
	SCOPED_TRACE("check_equality");

	EXPECT_TRUE(b == other.b);
	EXPECT_TRUE(int_pair == other.int_pair);
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeInheritanceTest::B::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK))
	{
		return scribe.get_transcribe_result();
	}

	// Derived class probably transcribed and passed in via our constructor.
	if (!scribe.has_been_transcribed(int_pair))
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, int_pair, "int_pair", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<TranscribeInheritanceTest::B> &b)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, b->int_pair, "int_pair", GPlatesScribe::TRACK);
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<TranscribeInheritanceTest::int_pair_type> int_pair =
				scribe.load<TranscribeInheritanceTest::int_pair_type>(
						TRANSCRIBE_SOURCE, "int_pair", GPlatesScribe::TRACK);
		if (!int_pair.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		b.construct_object(int_pair);

		scribe.relocated(TRANSCRIBE_SOURCE, b->int_pair, int_pair);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeInheritanceTest::D::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Check to see if our constructor data was transcribed and passed in via our constructor.
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d, "d", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, x, "x", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, y, "y", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, self, "self", GPlatesScribe::TRACK))
	{
		return scribe.get_transcribe_result();
	}

	if (!scribe.transcribe_base<A>(TRANSCRIBE_SOURCE, *this, "A") ||
		!scribe.transcribe_base<B>(TRANSCRIBE_SOURCE, *this, "B"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

void
GPlatesUnitTest::TranscribeInheritanceTest::D::relocated(
		GPlatesScribe::Scribe &scribe,
		const D &relocated_d,
		const D &transcribed_d)
{
	// Let the scribe system know that the object pointed-to by 'x' was essentially copied
	// when D's copy constructor allocated a new integer for 'x' (and copied the integer across).
	if (transcribed_d.x)
	{
		scribe.relocated(TRANSCRIBE_SOURCE, *relocated_d.x, *transcribed_d.x);
	}
}

void
GPlatesUnitTest::TranscribeInheritanceTest::D::check_equality(
		const D &other) const
{
	SCOPED_TRACE("check_equality");

	A::check_equality(other);
	B::check_equality(other);

	EXPECT_TRUE(d && (*d == *other.d));
	EXPECT_TRUE(x && (*x == *other.x));
	EXPECT_TRUE(y == other.y);

	if (self.expired())
	{
		EXPECT_TRUE(other.self.expired());
	}
	else
	{
		EXPECT_TRUE(self.lock() && (self.lock().get() == this));
		EXPECT_TRUE(other.self.lock() && (other.self.lock().get() == &other));
	}
}

void
GPlatesUnitTest::TranscribeInheritanceTest::D::initialise(
		int b_for_a,
		int b_for_b,
		boost::weak_ptr<D> self_)
{
	A::initialise(b_for_a);
	B::initialise(b_for_b);

	*x = 101;
	y = 21;
	self = self_;
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<TranscribeInheritanceTest::D> &d)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, d->d, "d", GPlatesScribe::TRACK);
		scribe.save(TRANSCRIBE_SOURCE, d->a, "a", GPlatesScribe::TRACK);

		scribe.save(TRANSCRIBE_SOURCE, d->A::b_object.int_pair, "a_int_pair", GPlatesScribe::TRACK);
		scribe.save(TRANSCRIBE_SOURCE, d->B::int_pair, "b_int_pair", GPlatesScribe::TRACK);
	}
	else // loading...
	{
		// Get information that is not transcribed into the archive.
		boost::optional<GPlatesScribe::TranscribeContext<TranscribeInheritanceTest::A> &>
				transcribe_context_a = scribe.get_transcribe_context<TranscribeInheritanceTest::A>();
		GPlatesGlobal::Assert<GPlatesScribe::Exceptions::ScribeUserError>(
				transcribe_context_a,
				GPLATES_ASSERTION_SOURCE,
				"No transcribe context available for the object type 'TranscribeInheritanceTest::A'.");

		GPlatesScribe::LoadRef<int *> dp = scribe.load<int *>(TRANSCRIBE_SOURCE, "d", GPlatesScribe::TRACK);
		if (!dp.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<int> a = scribe.load<int>(TRANSCRIBE_SOURCE, "a", GPlatesScribe::TRACK);
		if (!a.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<TranscribeInheritanceTest::int_pair_type> a_int_pair =
				scribe.load<TranscribeInheritanceTest::int_pair_type>(
						TRANSCRIBE_SOURCE, "a_int_pair", GPlatesScribe::TRACK);
		if (!a_int_pair.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<TranscribeInheritanceTest::int_pair_type> b_int_pair =
				scribe.load<TranscribeInheritanceTest::int_pair_type>(
						TRANSCRIBE_SOURCE, "b_int_pair", GPlatesScribe::TRACK);
		if (!b_int_pair.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		d.construct_object(
				boost::ref(*dp.get()), a, a_int_pair, b_int_pair,
				transcribe_context_a->untranscribed_object);

		scribe.relocated(TRANSCRIBE_SOURCE, d->d, dp);
		scribe.relocated(TRANSCRIBE_SOURCE, d->A::a, a);
		scribe.relocated(TRANSCRIBE_SOURCE, d->A::b_object.int_pair, a_int_pair);
		scribe.relocated(TRANSCRIBE_SOURCE, d->B::int_pair, b_int_pair);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

void
GPlatesUnitTest::TranscribeInheritanceTest::E::check_equality(
		const E &other) const
{
	static_cast<const D &>(b).check_equality(static_cast<const D &>(other.b));
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<TranscribeInheritanceTest::E> &e)
{
	if (scribe.is_saving())
	{
		scribe.save_reference(TRANSCRIBE_SOURCE, e->b, "b");
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<TranscribeInheritanceTest::B> b =
				scribe.load_reference<TranscribeInheritanceTest::B>(TRANSCRIBE_SOURCE, "b");
		if (!b.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		e.construct_object(b);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

void 
GPlatesUnitTest::TranscribeCompatibilityTest::test_case_compatibility_1()
{
	SmartPtrData before_smart_ptr_data;
	before_smart_ptr_data.initialise("test_string");

	try
	{
		//
		// Text archive
		//
		{
			SCOPED_TRACE("text archive");

			std::stringstream text_archive;

			test_case_compatibility_1_write(
					GPlatesScribe::TextArchiveWriter::create(text_archive),
					before_smart_ptr_data);

			text_archive.seekp(0);

			test_case_compatibility_1_read(
					GPlatesScribe::TextArchiveReader::create(text_archive),
					before_smart_ptr_data);
		}

		//
		// Binary archive
		//
		{
			SCOPED_TRACE("binary archive");

			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			test_case_compatibility_1_write(
					GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer),
					before_smart_ptr_data);

			binary_archive.close();

#if 0
			QFile archive_binary_file(test_scratch_file_path("archive_compatibility_1.bin"));
			bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
			EXPECT_TRUE(archive_binary_open_for_writing);
			if (archive_binary_open_for_writing)
			{
				archive_binary_file.write(binary_archive.data());
			}
			archive_binary_file.close();
#endif

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			test_case_compatibility_1_read(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader),
					before_smart_ptr_data);
		}

		//
		// XML archive
		//
		{
			SCOPED_TRACE("XML archive");

			QBuffer xml_archive;
			xml_archive.open(QBuffer::WriteOnly);

			QXmlStreamWriter xml_stream_writer(&xml_archive);
			xml_stream_writer.writeStartDocument();

			test_case_compatibility_1_write(
					GPlatesScribe::XmlArchiveWriter::create(xml_stream_writer),
					before_smart_ptr_data);

			xml_stream_writer.writeEndDocument();

			xml_archive.close();

#if 0
			QFile archive_xml_file(test_scratch_file_path("archive_compatibility_1.xml"));
			bool archive_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
			EXPECT_TRUE(archive_open_for_writing);
			if (archive_open_for_writing)
			{
				archive_xml_file.write(xml_archive.data());
			}
			archive_xml_file.close();
#endif

			xml_archive.open(QBuffer::ReadOnly);
			xml_archive.seek(0);

			QXmlStreamReader xml_stream_reader(&xml_archive);
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isStartDocument());

			GPlatesScribe::XmlArchiveReader::non_null_ptr_type xml_archive_reader =
					GPlatesScribe::XmlArchiveReader::create(xml_stream_reader);

			test_case_compatibility_1_read(
					xml_archive_reader,
					before_smart_ptr_data);

			xml_archive_reader->close();
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isEndDocument());
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::test_case_compatibility_1_write(
		const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
		SmartPtrData &before_smart_ptr_data)
{
	SCOPED_TRACE("test_case_compatibility_1_write");

	GPlatesScribe::Scribe scribe;

	scribe.transcribe(TRANSCRIBE_SOURCE, before_smart_ptr_data, "smart_ptr_data", GPlatesScribe::TRACK);

	EXPECT_TRUE(scribe.is_transcription_complete());

	archive_writer->write_transcription(*scribe.get_transcription());
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::test_case_compatibility_1_read(
		const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
		SmartPtrData &before_smart_ptr_data)
{
	SCOPED_TRACE("test_case_compatibility_1_read");

	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	SmartPtrData after_smart_ptr_data;

	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_smart_ptr_data, "smart_ptr_data", GPlatesScribe::TRACK));
	before_smart_ptr_data.check_equality(after_smart_ptr_data);

	EXPECT_TRUE(scribe.is_transcription_complete());
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::Derived::check_equality(
		const Derived &other) const
{
	SCOPED_TRACE("check_equality");

	EXPECT_TRUE(d_value == other.d_value);
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeCompatibilityTest::Derived::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.has_been_transcribed(d_value))
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_value, "d_value", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}
	}

	if (!scribe.transcribe_base<Base, Derived>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeCompatibilityTest::Derived::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<Derived> &derived)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, derived->d_value, "d_value", GPlatesScribe::TRACK);
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<std::string> value =
				scribe.load<std::string>(TRANSCRIBE_SOURCE, "d_value", GPlatesScribe::TRACK);
		if (!value.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		derived.construct_object(value);
		scribe.relocated(TRANSCRIBE_SOURCE, derived->d_value, value);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::SmartPtrData::initialise(
		const std::string &value)
{
	d_scoped_ptr.reset(new Derived(value));
	d_shared_ptr.reset(new Derived(value));
	d_shared_ptr2 = d_shared_ptr;
	d_intrusive_ptr.reset(new Derived(value));
	d_intrusive_ptr2 = d_intrusive_ptr;
	d_unique_ptr.reset(new Derived(value));
	d_non_null_intrusive_ptr = GPlatesUtils::non_null_intrusive_ptr<Base>(new Derived(value));

	d_pre_derived_object1 = Derived(value);
	d_pre_derived_object_ptr2.reset(new Derived(value));
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::SmartPtrData::check_equality(
		const SmartPtrData &other) const
{
	SCOPED_TRACE("check_equality");

	EXPECT_TRUE(d_scoped_ptr && other.d_scoped_ptr);
	if (d_scoped_ptr && other.d_scoped_ptr)
	{
		EXPECT_TRUE(dynamic_cast<Derived *>(d_scoped_ptr.get()) && dynamic_cast<Derived *>(other.d_scoped_ptr.get()));
		dynamic_cast<Derived *>(d_scoped_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_scoped_ptr));
	}

	EXPECT_TRUE(d_shared_ptr && other.d_shared_ptr);
	if (d_shared_ptr && other.d_shared_ptr)
	{
		EXPECT_TRUE(dynamic_cast<Derived *>(d_shared_ptr.get()) && dynamic_cast<Derived *>(other.d_shared_ptr.get()));
		dynamic_cast<Derived *>(d_shared_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_shared_ptr));
	}

	EXPECT_TRUE(d_shared_ptr2 && other.d_shared_ptr2);
	if (d_shared_ptr2 && other.d_shared_ptr2)
	{
		EXPECT_TRUE(dynamic_cast<Derived *>(d_shared_ptr2.get()) && dynamic_cast<Derived *>(other.d_shared_ptr2.get()));
		dynamic_cast<Derived *>(d_shared_ptr2.get())->check_equality(dynamic_cast<Derived &>(*other.d_shared_ptr2));
	}

	EXPECT_TRUE(d_intrusive_ptr && other.d_intrusive_ptr);
	if (d_intrusive_ptr && other.d_intrusive_ptr)
	{
		EXPECT_TRUE(dynamic_cast<Derived *>(d_intrusive_ptr.get()) && dynamic_cast<Derived *>(other.d_intrusive_ptr.get()));
		dynamic_cast<Derived *>(d_intrusive_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_intrusive_ptr));
	}

	EXPECT_TRUE(d_intrusive_ptr2 && other.d_intrusive_ptr2);
	if (d_intrusive_ptr2 && other.d_intrusive_ptr2)
	{
		EXPECT_TRUE(dynamic_cast<Derived *>(d_intrusive_ptr2.get()) && dynamic_cast<Derived *>(other.d_intrusive_ptr2.get()));
		dynamic_cast<Derived *>(d_intrusive_ptr2.get())->check_equality(dynamic_cast<Derived &>(*other.d_intrusive_ptr2));
	}

	EXPECT_TRUE(d_unique_ptr.get() && other.d_unique_ptr.get());
	if (d_unique_ptr.get() && other.d_unique_ptr.get())
	{
		EXPECT_TRUE(dynamic_cast<Derived *>(d_unique_ptr.get()) && dynamic_cast<Derived *>(other.d_unique_ptr.get()));
		dynamic_cast<Derived *>(d_unique_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_unique_ptr));
	}

	EXPECT_TRUE(dynamic_cast<Derived *>(d_non_null_intrusive_ptr.get()) && dynamic_cast<Derived *>(other.d_non_null_intrusive_ptr.get()));
	dynamic_cast<Derived *>(d_non_null_intrusive_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_non_null_intrusive_ptr));

	EXPECT_TRUE(dynamic_cast<Derived *>(other.d_post_derived_object_ptr1.get()));
	d_pre_derived_object1.check_equality(dynamic_cast<Derived &>(*other.d_post_derived_object_ptr1));

	EXPECT_TRUE(dynamic_cast<Derived *>(d_pre_derived_object_ptr2.get()));
	dynamic_cast<Derived &>(*d_pre_derived_object_ptr2).check_equality(other.d_post_derived_object2);
	EXPECT_TRUE(other.d_post_derived_object_ptr2 == &other.d_post_derived_object2);
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeCompatibilityTest::SmartPtrData::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (scribe.is_saving())
	{
		scribe.transcribe(TRANSCRIBE_SOURCE, d_scoped_ptr, "d_scoped_ptr", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr, "d_shared_ptr", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr2, "d_shared_ptr2", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr, "d_intrusive_ptr", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr2, "d_intrusive_ptr2", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, d_unique_ptr, "d_unique_ptr", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, d_non_null_intrusive_ptr, "d_non_null_intrusive_ptr", GPlatesScribe::TRACK);

		scribe.transcribe(TRANSCRIBE_SOURCE, d_pre_derived_object_ptr1, "d_derived_object_ptr1", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, d_pre_derived_object1, "d_derived_object1", GPlatesScribe::TRACK);

		scribe.transcribe(TRANSCRIBE_SOURCE, d_pre_derived_object_ptr2, "d_derived_object_ptr2", GPlatesScribe::TRACK);
	}
	else // loading...
	{
		//
		// We can transcribe the object tags in any order and since all these smart pointers follow
		// the smart pointer transcribe protocol we can mix up the object tags...
		//

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_scoped_ptr, "d_non_null_intrusive_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr, "d_unique_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr2, "d_unique_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr, "d_shared_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr2, "d_shared_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_unique_ptr, "d_intrusive_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_non_null_intrusive_ptr, "d_scoped_ptr"))
		{
			return scribe.get_transcribe_result();
		}

		//
		// Shared pointers won't reference the same object if they are transcribed without tracking.
		//

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr, "d_shared_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr2, "d_shared_ptr2") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr, "d_intrusive_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr2, "d_intrusive_ptr2"))
		{
			return scribe.get_transcribe_result();
		}

		EXPECT_TRUE(d_shared_ptr != d_shared_ptr2);
		EXPECT_TRUE(d_intrusive_ptr != d_intrusive_ptr2);

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr, "d_shared_ptr", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr2, "d_shared_ptr2", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr, "d_intrusive_ptr", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr2, "d_intrusive_ptr2", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}

		EXPECT_TRUE(d_shared_ptr == d_shared_ptr2);
		EXPECT_TRUE(d_intrusive_ptr == d_intrusive_ptr2);

		//
		// We can load a smart pointer from a raw pointer (and its pointed-to object).
		//

		if (!GPlatesScribe::TranscribeUtils::load_smart_pointer_from_raw_pointer(
				TRANSCRIBE_SOURCE,
				scribe,
				d_post_derived_object_ptr1,
				"d_derived_object_ptr1",
				false/*track*/))
		{
			return scribe.get_transcribe_result();
		}

		if (!GPlatesScribe::TranscribeUtils::load_smart_pointer_from_raw_pointer(
				TRANSCRIBE_SOURCE,
				scribe,
				d_post_derived_object_ptr1,
				"d_derived_object_ptr1",
				true/*track*/))
		{
			return scribe.get_transcribe_result();
		}

		//
		// We can load an object (and a raw pointer to it) from a smart pointer.
		//

		if (!GPlatesScribe::TranscribeUtils::load_raw_pointer_and_object_from_smart_pointer(
				TRANSCRIBE_SOURCE,
				scribe,
				d_post_derived_object2,
				d_post_derived_object_ptr2,
				"d_derived_object_ptr2",
				false/*track*/))
		{
			return scribe.get_transcribe_result();
		}

		if (!GPlatesScribe::TranscribeUtils::load_raw_pointer_and_object_from_smart_pointer(
				TRANSCRIBE_SOURCE,
				scribe,
				d_post_derived_object2,
				d_post_derived_object_ptr2,
				"d_derived_object_ptr2",
				true/*track*/))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

GPlatesScribe::TranscribeResult
GPlatesUnitTest::transcribe(
		GPlatesScribe::Scribe &scribe,
		TranscribeRawTest::Enum &e,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("ENUM_VALUE_1", TranscribeRawTest::ENUM_VALUE_1),
		GPlatesScribe::EnumValue("ENUM_VALUE_2", TranscribeRawTest::ENUM_VALUE_2),
		GPlatesScribe::EnumValue("ENUM_VALUE_3", TranscribeRawTest::ENUM_VALUE_3)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			e,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::NestedData::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// 'value' was transcribed as construct data.
	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::NestedData::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<NestedData> &nested_data)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, nested_data->value, "value");
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<int> value =
				scribe.load<int>(TRANSCRIBE_SOURCE, "value", GPlatesScribe::TRACK);
		if (!value.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		nested_data.construct_object(value);

		// Inside a raw subtree this is a harmless no-op (objects are not tracked in raw mode).
		scribe.relocated(TRANSCRIBE_SOURCE, nested_data->value, value);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesUnitTest::TranscribeRawTest::Data::Data() :
	b(false),
	c(0),
	s(0),
	i(0),
	ui(0),
	l(0),
	f(0),
	d(0),
	real(0),
	e(ENUM_VALUE_1),
	opt_none(1.0), // Gets overwritten on loading (should end up as none).
	nested(0)
{
}


void
GPlatesUnitTest::TranscribeRawTest::Data::initialise()
{
	b = true;
	c = 'z';
	s = -321;
	i = -123456;
	ui = 4000000000u;
	l = -2000000000L;
	f = 2.5f;
	d = -1234.56789;
	real = GPlatesMaths::Real(3.14159);
	e = ENUM_VALUE_2;
	str1 = "shared string";
	str2 = "shared string";
	str3 = "another string";
	qstr = QString::fromUtf8("qstring \xc3\xa9\xc3\xa0");
	int_vec.clear();
	int_vec.push_back(1);
	int_vec.push_back(-2);
	int_vec.push_back(3);
	str_vec.clear();
	str_vec.push_back("alpha");
	str_vec.push_back("beta");
	str_vec.push_back("shared string");
	int_str_map.clear();
	int_str_map[1] = "one";
	int_str_map[2] = "two";
	opt_some = 42.5;
	opt_none = boost::none;
	nested.value = 1234;
}


void
GPlatesUnitTest::TranscribeRawTest::Data::check_equality(
		const Data &other) const
{
	SCOPED_TRACE("check_equality");

	EXPECT_TRUE(b == other.b);
	EXPECT_TRUE(c == other.c);
	EXPECT_TRUE(s == other.s);
	EXPECT_TRUE(i == other.i);
	EXPECT_TRUE(ui == other.ui);
	EXPECT_TRUE(l == other.l);
	// Note: Using EXPECT_EQ (rather than EXPECT_TRUE) for the floating-point types since
	// transcribing should round-trip them *exactly* (and it avoids a -Wfloat-equal warning).
	EXPECT_EQ(f, other.f);
	EXPECT_EQ(d, other.d);
	EXPECT_TRUE(real == other.real);
	EXPECT_TRUE(e == other.e);
	EXPECT_TRUE(str1 == other.str1);
	EXPECT_TRUE(str2 == other.str2);
	EXPECT_TRUE(str3 == other.str3);
	EXPECT_TRUE(qstr == other.qstr);
	EXPECT_TRUE(int_vec == other.int_vec);
	EXPECT_TRUE(str_vec == other.str_vec);
	EXPECT_TRUE(int_str_map == other.int_str_map);
	EXPECT_TRUE(opt_some == other.opt_some);
	EXPECT_TRUE(opt_none == other.opt_none);
	EXPECT_TRUE(nested.value == other.nested.value);
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::Data::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, b, "b") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, c, "c") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, s, "s") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, i, "i") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, ui, "ui") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, l, "l") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, f, "f") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d, "d") ||
		// Transcribes via the delegate protocol (which streams directly)...
		!scribe.transcribe(TRANSCRIBE_SOURCE, real, "real") ||
		// Transcribes via the enum protocol (as a string)...
		!scribe.transcribe(TRANSCRIBE_SOURCE, e, "e") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, str1, "str1") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, str2, "str2") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, str3, "str3") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, qstr, "qstr") ||
		// A nested RAW option is ignored inside a raw subtree (everything is already raw).
		// When this object is transcribed via the *general* path (see the raw compatibility
		// test case) it instead creates a nested raw stream boundary...
		!scribe.transcribe(TRANSCRIBE_SOURCE, int_vec, "int_vec", GPlatesScribe::RAW) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, str_vec, "str_vec") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, int_str_map, "int_str_map") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, opt_some, "opt_some") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, opt_none, "opt_none"))
	{
		return scribe.get_transcribe_result();
	}

	// Transcribe 'nested' via save/load construction (it has no default constructor).
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, nested, "nested");
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<NestedData> nested_ref =
				scribe.load<NestedData>(TRANSCRIBE_SOURCE, "nested");
		if (!nested_ref.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		nested = nested_ref.get();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesUnitTest::TranscribeRawTest::ArrayData::ArrayData() :
	doubles(),
	floats(),
	ints()
{  }


void
GPlatesUnitTest::TranscribeRawTest::ArrayData::initialise()
{
	doubles[0] = 1.5;
	doubles[1] = -2.25;
	doubles[2] = 0.0;
	doubles[3] = 123456.789;

	floats[0] = 1.5f;
	floats[1] = -2.25f;
	floats[2] = 3.0f;

	ints[0] = 0;
	ints[1] = -1;
	ints[2] = 2147483647;
	ints[3] = -2147483647 - 1;
	ints[4] = 42;
}


void
GPlatesUnitTest::TranscribeRawTest::ArrayData::check_equality(
		const ArrayData &other) const
{
	SCOPED_TRACE("check_equality");

	for (unsigned int n = 0; n < 4; ++n)
	{
		EXPECT_EQ(doubles[n], other.doubles[n]);
	}
	for (unsigned int n = 0; n < 3; ++n)
	{
		EXPECT_EQ(floats[n], other.floats[n]);
	}
	for (unsigned int n = 0; n < 5; ++n)
	{
		EXPECT_EQ(ints[n], other.ints[n]);
	}
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::ArrayData::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe_raw_array(TRANSCRIBE_SOURCE, doubles, 4, "doubles") ||
		!scribe.transcribe_raw_array(TRANSCRIBE_SOURCE, floats, 3, "floats") ||
		!scribe.transcribe_raw_array(TRANSCRIBE_SOURCE, ints, 5, "ints"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesUnitTest::TranscribeRawTest::FixedArrayData::FixedArrayData() :
	nums(),
	matrix(),
	strs()
{  }


void
GPlatesUnitTest::TranscribeRawTest::FixedArrayData::initialise()
{
	nums[0] = 0;
	nums[1] = -7;
	nums[2] = 2147483647;
	nums[3] = 42;

	matrix[0][0] = 1.5;
	matrix[0][1] = -2.25;
	matrix[0][2] = 0.0;
	matrix[1][0] = 123456.789;
	matrix[1][1] = -0.001;
	matrix[1][2] = 3.0;

	strs[0] = "hello";
	strs[1] = "world";
}


void
GPlatesUnitTest::TranscribeRawTest::FixedArrayData::check_equality(
		const FixedArrayData &other) const
{
	SCOPED_TRACE("check_equality");

	for (unsigned int n = 0; n < 4; ++n)
	{
		EXPECT_EQ(nums[n], other.nums[n]);
	}
	for (unsigned int i = 0; i < 2; ++i)
	{
		for (unsigned int j = 0; j < 3; ++j)
		{
			EXPECT_EQ(matrix[i][j], other.matrix[i][j]);
		}
	}
	for (unsigned int n = 0; n < 2; ++n)
	{
		EXPECT_EQ(strs[n], other.strs[n]);
	}
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::FixedArrayData::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, nums, "nums") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, matrix, "matrix") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, strs, "strs"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_1()
{
	Data before_data;
	before_data.initialise();

	try
	{
		//
		// Text archive
		//
		{
			SCOPED_TRACE("text archive");

			std::stringstream text_archive;

			test_case_raw_1_write(
					GPlatesScribe::TextArchiveWriter::create(text_archive),
					before_data);

			text_archive.seekp(0);

			test_case_raw_1_read(
					GPlatesScribe::TextArchiveReader::create(text_archive),
					before_data);
		}

		//
		// Binary archive
		//
		{
			SCOPED_TRACE("binary archive");

			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			test_case_raw_1_write(
					GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer),
					before_data);

			binary_archive.close();

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			test_case_raw_1_read(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader),
					before_data);
		}

		//
		// XML archive
		//
		{
			SCOPED_TRACE("XML archive");

			QBuffer xml_archive;
			xml_archive.open(QBuffer::WriteOnly);

			QXmlStreamWriter xml_stream_writer(&xml_archive);
			xml_stream_writer.writeStartDocument();

			test_case_raw_1_write(
					GPlatesScribe::XmlArchiveWriter::create(xml_stream_writer),
					before_data);

			xml_stream_writer.writeEndDocument();

			xml_archive.close();

			xml_archive.open(QBuffer::ReadOnly);
			xml_archive.seek(0);

			QXmlStreamReader xml_stream_reader(&xml_archive);
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isStartDocument());

			GPlatesScribe::XmlArchiveReader::non_null_ptr_type xml_archive_reader =
					GPlatesScribe::XmlArchiveReader::create(xml_stream_reader);

			test_case_raw_1_read(
					xml_archive_reader,
					before_data);

			xml_archive_reader->close();
			xml_stream_reader.readNext();
			EXPECT_TRUE(xml_stream_reader.isEndDocument());
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_1_write(
		const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
		Data &before_data)
{
	SCOPED_TRACE("test_case_raw_1_write");

	GPlatesScribe::Scribe scribe;

	scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::RAW);

	EXPECT_TRUE(scribe.is_transcription_complete());

	// The entire 'Data' subtree should have been streamed into a *single* raw stream
	// (the nested RAW option on 'int_vec' is ignored inside the raw subtree).
	EXPECT_EQ(scribe.get_transcription()->get_num_raw_stream_objects(), 1u);

	// Strings inside the raw stream are interned in the transcription's unique-string pool:
	// "ENUM_VALUE_2", "shared string" (transcribed three times but interned once),
	// "another string", the QString utf8 bytes, "alpha", "beta", "one" and "two".
	EXPECT_EQ(scribe.get_transcription()->get_num_unique_string_objects(), 8u);

	archive_writer->write_transcription(*scribe.get_transcription());
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_1_read(
		const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
		Data &before_data)
{
	SCOPED_TRACE("test_case_raw_1_read");

	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	Data after_data;

	EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data", GPlatesScribe::RAW));

	EXPECT_TRUE(scribe.is_transcription_complete());

	before_data.check_equality(after_data);
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_64_bit_integers()
{
	// 64-bit integers are encoded full-width in the raw lane, unlike the general path
	// (which encodes 64-bit integer types as 32-bit integers and throws outside their range).
	const long long before_int64 = 0x123456789abcdef0LL;
	const unsigned long long before_uint64 = 0xfedcba9876543210ULL;

	try
	{
		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		{
			GPlatesScribe::Scribe scribe;

			scribe.transcribe(TRANSCRIBE_SOURCE, before_int64, "int64", GPlatesScribe::RAW);
			scribe.transcribe(TRANSCRIBE_SOURCE, before_uint64, "uint64", GPlatesScribe::RAW);

			EXPECT_TRUE(scribe.is_transcription_complete());

			// Each RAW boundary object is its own raw stream.
			EXPECT_EQ(scribe.get_transcription()->get_num_raw_stream_objects(), 2u);

			GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
					*scribe.get_transcription());
		}

		binary_archive.close();

		binary_archive.open(QBuffer::ReadOnly);
		binary_archive.seek(0);

		QDataStream binary_stream_reader(&binary_archive);

		{
			GPlatesScribe::Scribe scribe(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

			long long after_int64 = 0;
			unsigned long long after_uint64 = 0;

			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_int64, "int64", GPlatesScribe::RAW));
			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_uint64, "uint64", GPlatesScribe::RAW));

			EXPECT_EQ(after_int64, before_int64);
			EXPECT_EQ(after_uint64, before_uint64);
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_cross_type_integers()
{
	// The raw lane encodes integers in a canonical, type-independent form (a sign discriminator
	// followed by a varint), so - as on the general path - a value saved through one integral type
	// can be loaded through another. The transcribe handlers rely on this (eg, saving an 'int' and
	// loading a 'GPlatesModel::integer_plate_id_type', which is an 'unsigned long').
	const int before_int = 42;
	const unsigned int before_uint = 300;

	try
	{
		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		{
			GPlatesScribe::Scribe scribe;

			scribe.transcribe(TRANSCRIBE_SOURCE, before_int, "signed", GPlatesScribe::RAW);
			scribe.transcribe(TRANSCRIBE_SOURCE, before_uint, "unsigned", GPlatesScribe::RAW);

			EXPECT_TRUE(scribe.is_transcription_complete());

			GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
					*scribe.get_transcription());
		}

		binary_archive.close();

		binary_archive.open(QBuffer::ReadOnly);
		binary_archive.seek(0);

		QDataStream binary_stream_reader(&binary_archive);

		{
			GPlatesScribe::Scribe scribe(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

			// Load through *different* integral types than were saved.
			unsigned long after_ulong = 0;   // 'int' saved -> 'unsigned long' loaded.
			long long after_llong = 0;        // 'unsigned int' saved -> 'long long' loaded.

			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_ulong, "signed", GPlatesScribe::RAW));
			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_llong, "unsigned", GPlatesScribe::RAW));

			EXPECT_EQ(after_ulong, static_cast<unsigned long>(before_int));
			EXPECT_EQ(after_llong, static_cast<long long>(before_uint));
		}

		//
		// Loading a negative value through an unsigned type is out of range and must fail cleanly
		// (a RawStreamError, mirroring the general path's range check).
		//
		QBuffer negative_archive;
		negative_archive.open(QBuffer::WriteOnly);

		QDataStream negative_stream_writer(&negative_archive);

		{
			GPlatesScribe::Scribe scribe;

			const int before_negative = -1;
			scribe.transcribe(TRANSCRIBE_SOURCE, before_negative, "negative", GPlatesScribe::RAW);

			GPlatesScribe::BinaryArchiveWriter::create(negative_stream_writer)->write_transcription(
					*scribe.get_transcription());
		}

		negative_archive.close();

		negative_archive.open(QBuffer::ReadOnly);
		negative_archive.seek(0);

		QDataStream negative_stream_reader(&negative_archive);

		{
			GPlatesScribe::Scribe scribe(
					GPlatesScribe::BinaryArchiveReader::create(negative_stream_reader)->read_transcription());

			unsigned int after_negative = 0;
			EXPECT_THROW(
					scribe.transcribe(TRANSCRIBE_SOURCE, after_negative, "negative", GPlatesScribe::RAW),
					GPlatesScribe::Exceptions::RawStreamError);
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::BaseA::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "a"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::BaseB::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, b, "b"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::Derived::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Note: In raw mode 'transcribe_base' still registers the base/derived inheritance links
	// (needed to up-cast shared-object backrefs) but streams the base class sub-objects inline.
	if (!scribe.transcribe_base<BaseA>(TRANSCRIBE_SOURCE, *this, "BaseA") ||
		!scribe.transcribe_base<BaseB>(TRANSCRIBE_SOURCE, *this, "BaseB") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d, "d"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::RefCountedData::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// The nested child is an intrusive (shared-owner) pointer - when a child object is itself
	// shared (referenced elsewhere too) this exercises backref ordering for a shared object
	// nested inside another shared object.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, value, "value") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, child, "child"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesUnitTest::TranscribeRawTest::PointerData::PointerData() :
	raw_owned(NULL),
	intrusive_1(new RefCountedData()),
	intrusive_2(intrusive_1),
	intrusive_solo(new RefCountedData())
{
}


GPlatesUnitTest::TranscribeRawTest::PointerData::~PointerData()
{
	delete raw_owned;
}


void
GPlatesUnitTest::TranscribeRawTest::PointerData::initialise()
{
	scoped.reset(new int(123));

	raw_owned = new int(-456);

	// A single Derived object shared (with aliasing) by 'shared_a', 'shared_b' and 'weak_a'.
	const boost::shared_ptr<Derived> derived(new Derived());
	derived->a = 10;
	derived->b = 20;
	derived->d = 30;
	shared_a = derived;
	shared_b = derived;
	weak_a = shared_a;

	// 'shared_null' remains NULL.

	// A single RefCountedData object shared by 'intrusive_1' and 'intrusive_2'
	// (reference count 2 - exercises the deduplicated raw-lane pointer encoding).
	intrusive_1 = RefCountedData::non_null_ptr_type(new RefCountedData(7));
	intrusive_2 = intrusive_1;

	// Sole owner (reference count 1 - exercises the inline raw-lane pointer encoding).
	intrusive_solo = RefCountedData::non_null_ptr_type(new RefCountedData(8));
}


void
GPlatesUnitTest::TranscribeRawTest::PointerData::check_equality(
		const PointerData &other) const
{
	SCOPED_TRACE("check_equality");

	// Note: The aliasing (and reference count) checks are made on *both* objects - notably on
	// 'other' which, in the test cases, is the *loaded* object.

	EXPECT_TRUE(scoped && other.scoped && *scoped == *other.scoped);
	EXPECT_TRUE(raw_owned && other.raw_owned && *raw_owned == *other.raw_owned);

	// 'shared_a' and 'shared_b' should reference the *same* Derived object (aliasing preserved).
	EXPECT_TRUE(shared_a && shared_b && other.shared_a && other.shared_b);
	if (shared_a && shared_b && other.shared_a && other.shared_b)
	{
		const Derived *derived = dynamic_cast<const Derived *>(shared_a.get());
		EXPECT_TRUE(derived);
		EXPECT_TRUE(derived == dynamic_cast<const Derived *>(shared_b.get()));

		const Derived *other_derived = dynamic_cast<const Derived *>(other.shared_a.get());
		EXPECT_TRUE(other_derived);
		EXPECT_TRUE(other_derived == dynamic_cast<const Derived *>(other.shared_b.get()));

		EXPECT_EQ(shared_a->a, other.shared_a->a);
		EXPECT_EQ(shared_b->b, other.shared_b->b);
		if (derived && other_derived)
		{
			EXPECT_EQ(derived->d, other_derived->d);
		}
	}

	// 'weak_a' should reference the same object as 'shared_a' (aliasing preserved).
	EXPECT_TRUE(!weak_a.expired() && weak_a.lock() == shared_a);
	EXPECT_TRUE(!other.weak_a.expired() && other.weak_a.lock() == other.shared_a);

	EXPECT_TRUE(!shared_null && !other.shared_null);

	// 'intrusive_1' and 'intrusive_2' should reference the *same* object (aliasing preserved)
	// and hence its reference count should be exactly 2 (its only owners).
	EXPECT_TRUE(intrusive_1 == intrusive_2);
	EXPECT_TRUE(other.intrusive_1 == other.intrusive_2);
	EXPECT_EQ(intrusive_1->get_reference_count(), 2);
	EXPECT_EQ(other.intrusive_1->get_reference_count(), 2);
	EXPECT_EQ(intrusive_1->value, other.intrusive_1->value);

	EXPECT_EQ(intrusive_solo->get_reference_count(), 1);
	EXPECT_EQ(other.intrusive_solo->get_reference_count(), 1);
	EXPECT_EQ(intrusive_solo->value, other.intrusive_solo->value);
}


GPlatesScribe::TranscribeResult
GPlatesUnitTest::TranscribeRawTest::PointerData::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Note: The weak pointer is transcribed *after* a shared pointer to the same object
	// (a requirement of the general path - see the boost::weak_ptr transcribe overload).
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, scoped, "scoped", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, raw_owned, "raw_owned", GPlatesScribe::EXCLUSIVE_OWNER | GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, shared_a, "shared_a", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, shared_b, "shared_b", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, weak_a, "weak_a", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, shared_null, "shared_null", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, intrusive_1, "intrusive_1", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, intrusive_2, "intrusive_2", GPlatesScribe::TRACK) ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, intrusive_solo, "intrusive_solo", GPlatesScribe::TRACK))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_pointers()
{
	try
	{
		//
		// Raw round trip of the owning pointer graph.
		//
		{
			PointerData before_data;
			before_data.initialise();

			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			{
				GPlatesScribe::Scribe scribe;

				scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::RAW);

				EXPECT_TRUE(scribe.is_transcription_complete());

				// The entire pointer graph (markers, class names, backrefs and pointed-to
				// objects) should be inside a single raw stream.
				EXPECT_EQ(scribe.get_transcription()->get_num_raw_stream_objects(), 1u);

				GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
						*scribe.get_transcription());
			}

			binary_archive.close();

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			PointerData after_data;

			{
				GPlatesScribe::Scribe scribe(
						GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

				EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data", GPlatesScribe::RAW));
			}

			// Note: The equality check is made *after* the load scribe is destroyed so that the
			// scribe's internal boost::shared_ptr map no longer contributes to reference counts.
			before_data.check_equality(after_data);
		}

		//
		// Compatibility: save the same pointer graph via the *general* path (like an older
		// version without the raw lane) and load *with* the RAW option - the boundary falls
		// back to the general path.
		//
		// Note: The boundary object is *tracked* here - on the general path an untracked
		// boundary would untrack the (owned, tracked) pointed-to objects inside it, which
		// throws because pointers still reference them. (The raw lane has no tracking at
		// all, so the first test case above doesn't need TRACK.)
		//
		{
			PointerData before_data;
			before_data.initialise();

			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			{
				GPlatesScribe::Scribe scribe;

				scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::TRACK);

				EXPECT_TRUE(scribe.is_transcription_complete());

				GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
						*scribe.get_transcription());
			}

			binary_archive.close();

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			PointerData after_data;

			{
				GPlatesScribe::Scribe scribe(
						GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

				EXPECT_TRUE(scribe.transcribe(
						TRANSCRIBE_SOURCE, after_data, "data",
						GPlatesScribe::RAW | GPlatesScribe::TRACK));
			}

			before_data.check_equality(after_data);
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_nested_shared_objects()
{
	// A shared object nested inside another shared object.
	//
	// The save path registers a shared object (for backrefs) *before* streaming its contents, so
	// a nested shared object is assigned a *higher* backref index than its parent. The load path
	// must reserve the parent's backref slot before loading its contents (rather than registering
	// it afterwards) so that the indices match - otherwise the nested child would be registered
	// before its parent on load but after it on save, misaligning every subsequent backref.
	try
	{
		RefCountedData::non_null_ptr_type child(new RefCountedData(100));
		RefCountedData::non_null_ptr_type parent(new RefCountedData(200));
		parent->child = child;

		// 'parent' is the first shared object (backref index 0); its nested 'child' is the second
		// (backref index 1). The repeated entries then back-reference both.
		std::vector<RefCountedData::non_null_ptr_type> before_data;
		before_data.push_back(parent);  // Streams 'parent' (and, nested, 'child').
		before_data.push_back(parent);  // Backref to 'parent'.
		before_data.push_back(child);   // Backref to the nested 'child'.

		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		{
			GPlatesScribe::Scribe scribe;

			scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::RAW);

			EXPECT_TRUE(scribe.is_transcription_complete());
			EXPECT_EQ(scribe.get_transcription()->get_num_raw_stream_objects(), 1u);

			GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
					*scribe.get_transcription());
		}

		binary_archive.close();

		binary_archive.open(QBuffer::ReadOnly);
		binary_archive.seek(0);

		QDataStream binary_stream_reader(&binary_archive);

		std::vector<RefCountedData::non_null_ptr_type> after_data;

		{
			GPlatesScribe::Scribe scribe(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data", GPlatesScribe::RAW));
		}

		// Note: The aliasing/reference-count checks are made after the load scribe is destroyed
		// (its internal shared-object bookkeeping no longer contributes to reference counts).
		ASSERT_EQ(after_data.size(), 3u);

		// References (not copies) so they do not contribute to the reference counts checked below.
		const RefCountedData::non_null_ptr_type &after_parent = after_data[0];
		const RefCountedData::non_null_ptr_type &after_child = after_data[2];

		// The repeated 'parent' entry resolved to the same object (aliasing preserved).
		EXPECT_TRUE(after_data[0] == after_data[1]);
		EXPECT_EQ(after_parent->value, 200);

		// The nested child aliases the separately-owned 'child' entry (nested backref resolved).
		ASSERT_TRUE(static_cast<bool>(after_parent->child));
		EXPECT_TRUE(*after_parent->child == after_child);
		EXPECT_EQ(after_child->value, 100);

		// 'parent' is owned by 'after_data[0]' and 'after_data[1]' (reference count 2).
		EXPECT_EQ(after_parent->get_reference_count(), 2);
		// 'child' is owned by 'parent->child' and 'after_data[2]' (reference count 2).
		EXPECT_EQ(after_child->get_reference_count(), 2);
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_compatibility()
{
	Data before_data;
	before_data.initialise();

	try
	{
		//
		// Save via the general path (no RAW option at the boundary - like an older version
		// without the raw lane) and load *with* the RAW option - the boundary dispatches on
		// the transcription kind (COMPOSITE) and falls back to the general path.
		//
		// (Note that the nested RAW option on 'int_vec' *does* apply on the general path,
		// creating a raw stream boundary inside the general path - which is also loadable.)
		//
		{
			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			{
				GPlatesScribe::Scribe scribe;

				scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data");

				EXPECT_TRUE(scribe.is_transcription_complete());

				GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
						*scribe.get_transcription());
			}

			binary_archive.close();

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			{
				GPlatesScribe::Scribe scribe(
						GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

				Data after_data;

				EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data", GPlatesScribe::RAW));

				before_data.check_equality(after_data);
			}
		}

		//
		// Save *with* the RAW option and load *without* it - the general path cannot decode
		// a raw stream so the load fails cleanly (returns false, without throwing).
		//
		{
			QBuffer binary_archive;
			binary_archive.open(QBuffer::WriteOnly);

			QDataStream binary_stream_writer(&binary_archive);

			{
				GPlatesScribe::Scribe scribe;

				scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::RAW);

				EXPECT_TRUE(scribe.is_transcription_complete());

				GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
						*scribe.get_transcription());
			}

			binary_archive.close();

			binary_archive.open(QBuffer::ReadOnly);
			binary_archive.seek(0);

			QDataStream binary_stream_reader(&binary_archive);

			{
				GPlatesScribe::Scribe scribe(
						GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

				Data after_data;

				EXPECT_TRUE(!scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data"));
			}
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}


namespace GPlatesUnitTest
{
	namespace TranscribeRawTestImpl
	{
		//! A transcribe handler that (incorrectly) transcribes an object *reference* in raw mode.
		struct ReferenceData
		{
			int target;
		};

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				ReferenceData &reference_data,
				bool transcribed_construct_data)
		{
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, reference_data.target, "target", GPlatesScribe::TRACK))
			{
				return scribe.get_transcribe_result();
			}

			if (scribe.is_saving())
			{
				scribe.save_reference(TRANSCRIBE_SOURCE, reference_data.target, "target_ref");
			}
			else // loading...
			{
				GPlatesScribe::LoadRef<int> target_ref =
						scribe.load_reference<int>(TRANSCRIBE_SOURCE, "target_ref");
				if (!target_ref.is_valid())
				{
					return scribe.get_transcribe_result();
				}
			}

			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		}

		//! A transcribe handler that (incorrectly) transcribes a *non-owning* pointer in raw mode.
		struct NonOwningPointerData
		{
			int value;
			int *value_ptr;
		};

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				NonOwningPointerData &pointer_data,
				bool transcribed_construct_data)
		{
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, pointer_data.value, "value", GPlatesScribe::TRACK) ||
				// A non-owning pointer (no EXCLUSIVE_OWNER/SHARED_OWNER option)...
				!scribe.transcribe(TRANSCRIBE_SOURCE, pointer_data.value_ptr, "value_ptr", GPlatesScribe::TRACK))
			{
				return scribe.get_transcribe_result();
			}

			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		}

		/**
		 * Adds a single raw stream object (with the specified raw stream data) tagged "data"
		 * to the transcription - as if saved by 'transcribe(..., "data", RAW)'.
		 */
		void
		add_raw_stream_to_transcription(
				GPlatesScribe::Transcription &transcription,
				const std::vector<char> &raw_stream_data)
		{
			GPlatesScribe::Transcription::CompositeObject &root_composite_object =
					transcription.add_composite_object(
							GPlatesScribe::TranscriptionScribeContext::ROOT_OBJECT_ID);

			const GPlatesScribe::Transcription::object_id_type raw_stream_object_id =
					GPlatesScribe::TranscriptionScribeContext::ROOT_OBJECT_ID + 1;

			root_composite_object.set_child(
					transcription.get_or_create_object_key("data", 0),
					raw_stream_object_id);

			transcription.add_raw_stream(raw_stream_object_id, raw_stream_data);
		}

		/**
		 * Creates a transcription containing a single raw stream object (with the specified
		 * raw stream data) tagged "data" - as if saved by 'transcribe(..., "data", RAW)'.
		 */
		GPlatesScribe::Transcription::non_null_ptr_type
		create_transcription_with_raw_stream(
				const std::vector<char> &raw_stream_data)
		{
			GPlatesScribe::Transcription::non_null_ptr_type transcription =
					GPlatesScribe::Transcription::create();

			add_raw_stream_to_transcription(*transcription, raw_stream_data);

			return transcription;
		}
	}
}

void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_errors()
{
	// Skip these tests in debug build because GPlatesGlobal::Assert() aborts instead of
	// throwing an exception and these tests check for exceptions...
#ifndef GPLATES_DEBUG

	//
	// Transcribing an object *reference* inside a raw subtree should throw an exception.
	//
	{
		GPlatesScribe::Scribe scribe;

		TranscribeRawTestImpl::ReferenceData reference_data;
		reference_data.target = 1;

		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, reference_data, "data", GPlatesScribe::RAW),
				GPlatesScribe::Exceptions::InvalidRawTranscribeOperation);
	}

	//
	// Transcribing a *non-owning* pointer inside a raw subtree should throw an exception
	// (owning pointers are supported - see 'test_case_raw_pointers').
	//
	{
		GPlatesScribe::Scribe scribe;

		TranscribeRawTestImpl::NonOwningPointerData pointer_data;
		pointer_data.value = 1;
		pointer_data.value_ptr = &pointer_data.value;

		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, pointer_data, "data", GPlatesScribe::RAW),
				GPlatesScribe::Exceptions::InvalidRawTranscribeOperation);
	}

	//
	// The RAW option directly on a pointer should throw an exception.
	//
	{
		GPlatesScribe::Scribe scribe;

		int value = 1;
		int *value_ptr = &value;

		scribe.transcribe(TRANSCRIBE_SOURCE, value, "value", GPlatesScribe::TRACK);

		EXPECT_THROW(
				scribe.transcribe(
						TRANSCRIBE_SOURCE, value_ptr, "value_ptr",
						GPlatesScribe::RAW | GPlatesScribe::TRACK),
				GPlatesScribe::Exceptions::InvalidRawTranscribeOperation);
	}

	//
	// A raw stream with a future codec version should be rejected cleanly.
	//
	{
		// The raw stream contains only a codec version (127 - some far-future version).
		std::vector<char> raw_stream_data;
		raw_stream_data.push_back(0x7f);

		GPlatesScribe::Scribe scribe(
				TranscribeRawTestImpl::create_transcription_with_raw_stream(raw_stream_data));

		int after_int;
		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, after_int, "data", GPlatesScribe::RAW),
				GPlatesScribe::Exceptions::UnsupportedRawStreamVersion);
	}

	//
	// A raw stream is positional - loading a *smaller* type than was saved leaves the stream
	// not fully consumed, which should throw (rather than silently succeed).
	//
	{
		// Codec version 0 followed by 8 bytes (as if a 'double' was saved).
		std::vector<char> raw_stream_data(1 + 8, 0);

		GPlatesScribe::Scribe scribe(
				TranscribeRawTestImpl::create_transcription_with_raw_stream(raw_stream_data));

		float after_float;
		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, after_float, "data", GPlatesScribe::RAW),
				GPlatesScribe::Exceptions::RawStreamError);
	}

	//
	// ...and loading a *larger* type than was saved reads past the end, which should throw.
	//
	{
		// Codec version 0 followed by 4 bytes (as if a 'float' was saved).
		std::vector<char> raw_stream_data(1 + 4, 0);

		GPlatesScribe::Scribe scribe(
				TranscribeRawTestImpl::create_transcription_with_raw_stream(raw_stream_data));

		double after_double;
		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, after_double, "data", GPlatesScribe::RAW),
				GPlatesScribe::Exceptions::RawStreamError);
	}

	//
	// Sanity check the hand-crafted raw stream: loading the *same* size type succeeds.
	//
	{
		// Codec version 0 followed by 8 bytes (as if a 'double' was saved).
		std::vector<char> raw_stream_data(1 + 8, 0);

		GPlatesScribe::Scribe scribe(
				TranscribeRawTestImpl::create_transcription_with_raw_stream(raw_stream_data));

		double after_double;
		EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_double, "data", GPlatesScribe::RAW));
		EXPECT_EQ(after_double, 0.0);
	}

	//
	// A pointed-to class name that is not export registered (eg, an archive created by a
	// future version) fails the load *softly* (like the general path) - it does not throw.
	//
	{
		GPlatesScribe::Transcription::non_null_ptr_type transcription =
				GPlatesScribe::Transcription::create();

		// Register the unknown class name in the transcription's unique-string pool.
		const unsigned int unknown_class_name_index =
				transcription->get_or_create_unique_string_index(
						"GPlatesUnitTest::TranscribeRawTest::RemovedInThisVersion");
		ASSERT_TRUE(unknown_class_name_index < 128); // Fits in a single varint byte.

		std::vector<char> raw_stream_data;
		raw_stream_data.push_back(0x00); // Codec version 0.
		raw_stream_data.push_back(0x01); // RAW_POINTER_INLINE.
		raw_stream_data.push_back(static_cast<char>(unknown_class_name_index)); // Class name.

		TranscribeRawTestImpl::add_raw_stream_to_transcription(*transcription, raw_stream_data);

		GPlatesScribe::Scribe scribe(transcription);

		boost::shared_ptr<TranscribeRawTest::BaseA> after_ptr;
		EXPECT_TRUE(!scribe.transcribe(TRANSCRIBE_SOURCE, after_ptr, "data", GPlatesScribe::RAW));
		EXPECT_TRUE(scribe.get_transcribe_result() == GPlatesScribe::TRANSCRIBE_UNKNOWN_TYPE);
	}

	//
	// An invalid owning pointer marker byte should throw.
	//
	{
		std::vector<char> raw_stream_data;
		raw_stream_data.push_back(0x00); // Codec version 0.
		raw_stream_data.push_back(static_cast<char>(0xff)); // Invalid marker.

		GPlatesScribe::Scribe scribe(
				TranscribeRawTestImpl::create_transcription_with_raw_stream(raw_stream_data));

		boost::shared_ptr<TranscribeRawTest::BaseA> after_ptr;
		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, after_ptr, "data", GPlatesScribe::RAW),
				GPlatesScribe::Exceptions::RawStreamError);
	}

	//
	// An owning pointer backref referencing a shared object that was never streamed should throw.
	//
	{
		std::vector<char> raw_stream_data;
		raw_stream_data.push_back(0x00); // Codec version 0.
		raw_stream_data.push_back(0x03); // RAW_POINTER_SHARED_BACKREF.
		raw_stream_data.push_back(0x00); // Backref index 0 (but no shared objects streamed).

		GPlatesScribe::Scribe scribe(
				TranscribeRawTestImpl::create_transcription_with_raw_stream(raw_stream_data));

		boost::shared_ptr<TranscribeRawTest::BaseA> after_ptr;
		EXPECT_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, after_ptr, "data", GPlatesScribe::RAW),
				GPlatesScribe::Exceptions::RawStreamError);
	}

#endif // GPLATES_DEBUG
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_raw_array()
{
	ArrayData before_data;
	before_data.initialise();

	// Round trip through the raw lane - exercises 'transcribe_raw_array's bulk fixed-width path.
	try
	{
		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		{
			GPlatesScribe::Scribe scribe;

			scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::RAW);

			EXPECT_TRUE(scribe.is_transcription_complete());

			GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
					*scribe.get_transcription());
		}

		binary_archive.close();

		binary_archive.open(QBuffer::ReadOnly);
		binary_archive.seek(0);

		QDataStream binary_stream_reader(&binary_archive);

		{
			GPlatesScribe::Scribe scribe(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

			ArrayData after_data;

			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data", GPlatesScribe::RAW));

			before_data.check_equality(after_data);
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}

	// Round trip through the general path (no RAW option) - exercises 'transcribe_raw_array's
	// per-element fallback loop (see the method's doc comment in "Scribe.h").
	try
	{
		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		{
			GPlatesScribe::Scribe scribe;

			scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data");

			EXPECT_TRUE(scribe.is_transcription_complete());

			GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
					*scribe.get_transcription());
		}

		binary_archive.close();

		binary_archive.open(QBuffer::ReadOnly);
		binary_archive.seek(0);

		QDataStream binary_stream_reader(&binary_archive);

		{
			GPlatesScribe::Scribe scribe(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

			ArrayData after_data;

			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data"));

			before_data.check_equality(after_data);
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}


void
GPlatesUnitTest::TranscribeRawTest::test_case_fixed_array()
{
	FixedArrayData before_data;
	before_data.initialise();

	// Round trip through the raw lane - exercises "TranscribeArray.h"'s raw-lane fast path (for
	// 'nums' and 'matrix') and its general-path fallback (for 'strs', a non-arithmetic array).
	try
	{
		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		{
			GPlatesScribe::Scribe scribe;

			scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data", GPlatesScribe::RAW);

			EXPECT_TRUE(scribe.is_transcription_complete());

			GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
					*scribe.get_transcription());
		}

		binary_archive.close();

		binary_archive.open(QBuffer::ReadOnly);
		binary_archive.seek(0);

		QDataStream binary_stream_reader(&binary_archive);

		{
			GPlatesScribe::Scribe scribe(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

			FixedArrayData after_data;

			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data", GPlatesScribe::RAW));

			before_data.check_equality(after_data);
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}

	// Round trip through the general path (no RAW option) - exercises the per-element fallback
	// loop for all three members.
	try
	{
		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		{
			GPlatesScribe::Scribe scribe;

			scribe.transcribe(TRANSCRIBE_SOURCE, before_data, "data");

			EXPECT_TRUE(scribe.is_transcription_complete());

			GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer)->write_transcription(
					*scribe.get_transcription());
		}

		binary_archive.close();

		binary_archive.open(QBuffer::ReadOnly);
		binary_archive.seek(0);

		QDataStream binary_stream_reader(&binary_archive);

		{
			GPlatesScribe::Scribe scribe(
					GPlatesScribe::BinaryArchiveReader::create(binary_stream_reader)->read_transcription());

			FixedArrayData after_data;

			EXPECT_TRUE(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data"));

			before_data.check_equality(after_data);
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		ADD_FAILURE() << message.str().c_str();
		return;
	}
}


TEST(TranscribeTest, primitives_1)
{
	GPlatesUnitTest::TranscribePrimitivesTest().test_case_primitives_1();
}

TEST(TranscribeTest, untracked_exception)
{
	GPlatesUnitTest::TranscribeUntrackedTest().test_case_untracked_exception();
}

TEST(TranscribeTest, untracked_1)
{
	GPlatesUnitTest::TranscribeUntrackedTest().test_case_untracked_1();
}

TEST(TranscribeTest, inheritance_1)
{
	GPlatesUnitTest::TranscribeInheritanceTest().test_case_inheritance_1();
}

TEST(TranscribeTest, inheritance_2)
{
	GPlatesUnitTest::TranscribeInheritanceTest().test_case_inheritance_2();
}

TEST(TranscribeTest, compatibility_1)
{
	GPlatesUnitTest::TranscribeCompatibilityTest().test_case_compatibility_1();
}

TEST(TranscribeTest, raw_1)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_raw_1();
}

TEST(TranscribeTest, raw_64_bit_integers)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_raw_64_bit_integers();
}

TEST(TranscribeTest, raw_cross_type_integers)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_raw_cross_type_integers();
}

TEST(TranscribeTest, raw_pointers)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_raw_pointers();
}

TEST(TranscribeTest, raw_nested_shared_objects)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_raw_nested_shared_objects();
}

TEST(TranscribeTest, raw_compatibility)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_raw_compatibility();
}

TEST(TranscribeTest, raw_errors)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_raw_errors();
}

TEST(TranscribeTest, raw_array)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_raw_array();
}

TEST(TranscribeTest, fixed_array)
{
	GPlatesUnitTest::TranscribeRawTest().test_case_fixed_array();
}
