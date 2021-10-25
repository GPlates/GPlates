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

#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <boost/optional.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/weak_ptr.hpp>
#include <QBuffer>
#include <QDebug>
#include <QFile>

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


GPlatesUnitTest::TranscribeTestSuite::TranscribeTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"TranscribeTestSuite")
{
	init(level);
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

		//
		// Binary archive
		//

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
		QFile archive_binary_file("archive_primitives_1.bin");
		bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
		BOOST_CHECK(archive_binary_open_for_writing);
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

		//
		// XML archive
		//

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
		QFile archive_xml_file("archive_primitives_1.xml");
		bool archive_xml_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
		BOOST_CHECK(archive_xml_open_for_writing);
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
		BOOST_CHECK(xml_stream_reader.isStartDocument());

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
		BOOST_CHECK(xml_stream_reader.isEndDocument());
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		BOOST_ERROR(message.str().c_str());
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

	BOOST_CHECK(scribe.is_transcription_complete());

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

	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_scoped_ptr, "data_scoped_ptr", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_data, "data", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_string_array, "string_array", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_char_array, "char_array", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_array, "2d", GPlatesScribe::TRACK));
	Data::NonDefaultConstructable relocated_after_non_default_constructable_array[1][2] =
	{
		{ after_non_default_constructable_array[0][0], after_non_default_constructable_array[0][1] }
	};
	scribe.relocated(
			TRANSCRIBE_SOURCE,
			relocated_after_non_default_constructable_array,
			after_non_default_constructable_array);
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_array_ptr, "p2d", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_sub_array_ptr, "ps2d", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_sub_array_ptr_ptr, "pps2d", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_non_default_constructable_array_element_ptr, "pe2d", GPlatesScribe::TRACK));

	BOOST_CHECK(scribe.is_transcription_complete());

	BOOST_CHECK(after_data_scoped_ptr);
	if (after_data_scoped_ptr)
	{
		before_data_scoped_ptr->check_equality(*after_data_scoped_ptr);
	}
	after_data.check_equality(before_data);

	for (unsigned int n = 0; n < 2; ++n)
	{
		BOOST_CHECK(after_string_array[n] == before_string_array[n]);
		for (unsigned int c = 0; c < 6; ++c)
		{
			BOOST_CHECK(after_char_array[0][n][c] == before_char_array[0][n][c]);
		}
	}

	BOOST_CHECK(relocated_after_non_default_constructable_array[0][0] == before_non_default_constructable_array[0][0]);
	BOOST_CHECK(relocated_after_non_default_constructable_array[0][1] == before_non_default_constructable_array[0][1]);
	BOOST_CHECK(after_non_default_constructable_array_element_ptr == &relocated_after_non_default_constructable_array[0][1]);
	BOOST_CHECK(after_non_default_constructable_array_ptr == &relocated_after_non_default_constructable_array);
	BOOST_CHECK(after_non_default_constructable_sub_array_ptr == relocated_after_non_default_constructable_array);
	BOOST_CHECK((*after_non_default_constructable_sub_array_ptr)[1] == (*before_non_default_constructable_sub_array_ptr)[1]);
	BOOST_CHECK((*after_non_default_constructable_sub_array_ptr)[1] == before_non_default_constructable_array[0][1]);
	BOOST_CHECK(*after_non_default_constructable_sub_array_ptr_ptr == after_non_default_constructable_sub_array_ptr);
	BOOST_CHECK(&(*after_non_default_constructable_sub_array_ptr)[1] == after_non_default_constructable_array_element_ptr);
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
	qill.push_back(8); qill.push_back(9);
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
	qRegisterMetaTypeStreamOperators<StringWithEmbeddedZeros>(
			"GPlatesUnitTest::TranscribePrimitivesTest::Data::StringWithEmbeddedZeros");
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
		!scribe.transcribe(TRANSCRIBE_SOURCE, qill, "qill", GPlatesScribe::TRACK) ||
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

		BOOST_CHECK_CLOSE(f_from_d, d, 0.0001);
		BOOST_CHECK(GPlatesMaths::is_negative_infinity(f_from_geo_distant_future));
		BOOST_CHECK_CLOSE(d_from_f, f, 0.0001);
		BOOST_CHECK_CLOSE(d_from_real, real.dval(), 0.000000001);
		BOOST_CHECK(GPlatesMaths::is_positive_infinity(d_from_f_pos_inf));
		BOOST_CHECK_CLOSE(real_from_f.dval(), f, 0.0001);
		BOOST_CHECK(real_from_f_nan.is_nan());
		BOOST_CHECK(real_from_geo_distant_past.is_positive_infinity());
		BOOST_CHECK_CLOSE(real_from_f.dval(), f, 0.0001);
		BOOST_CHECK_CLOSE(geo_from_f->value(), f, 0.0001);
		BOOST_CHECK(geo_from_f_pos_inf->is_distant_past());
		BOOST_CHECK_CLOSE(geo_from_real->value(), real.dval(), 0.000000001);

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

		BOOST_CHECK(vv.size() == 1 &&
				vv[0].size() == 2 &&
				ia_from_vv[0][0] == vv[0][0] &&
				ia_from_vv[0][1] == vv[0][1]);
		BOOST_CHECK(vv_from_ia.size() == 2 &&
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

		BOOST_CHECK(u_from_uw == uw.str);
		BOOST_CHECK(uw_from_u.str == u);

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
	BOOST_CHECK(ia[0][0] == other.ia[0][0]);
	BOOST_CHECK(ia[0][1] == other.ia[0][1]);
	BOOST_CHECK(ia[1][0] == other.ia[1][0]);
	BOOST_CHECK(ia[1][1] == other.ia[1][1]);
	BOOST_CHECK(e == other.e);
	BOOST_CHECK(e2 == other.e2);
	BOOST_CHECK(b == other.b);
	// Cast to 'double' to avoid compiler warning in boost unit test code...
	BOOST_CHECK_CLOSE(double(f), double(other.f), 0.001);
	BOOST_CHECK_CLOSE(d, other.d, 0.000000001);
	BOOST_CHECK(GPlatesMaths::is_positive_infinity(f_pos_inf) && GPlatesMaths::is_positive_infinity(other.f_pos_inf));
	BOOST_CHECK(GPlatesMaths::is_negative_infinity(f_neg_inf) && GPlatesMaths::is_negative_infinity(other.f_neg_inf));
	BOOST_CHECK(GPlatesMaths::is_nan(f_nan) && GPlatesMaths::is_nan(other.f_nan));
	BOOST_CHECK(GPlatesMaths::is_positive_infinity(d_pos_inf) && GPlatesMaths::is_positive_infinity(other.d_pos_inf));
	BOOST_CHECK(GPlatesMaths::is_negative_infinity(d_neg_inf) && GPlatesMaths::is_negative_infinity(other.d_neg_inf));
	BOOST_CHECK(GPlatesMaths::is_nan(d_nan) && GPlatesMaths::is_nan(other.d_nan));
	BOOST_CHECK(real == other.real);
	BOOST_CHECK(geo_real_time == other.geo_real_time);
	BOOST_CHECK(geo_distant_past == other.geo_distant_past);
	BOOST_CHECK(geo_distant_future == other.geo_distant_future);
	BOOST_CHECK(c == other.c);
	BOOST_CHECK(s == other.s);
	BOOST_CHECK(l == other.l);
	BOOST_CHECK(pi && other.pi && (*pi == *other.pi));
	BOOST_CHECK(pi == &i);
	BOOST_CHECK(pj && other.pj && (*pj == *other.pj));
	BOOST_CHECK(pj == &j);
	BOOST_CHECK(pk && other.pk && (*pk == *other.pk));
	BOOST_CHECK(pl && other.pl && (*pl == *other.pl));
	BOOST_CHECK(pl == &vv[0][1]);
	BOOST_CHECK(ps && other.ps && (*ps == *other.ps));
	BOOST_CHECK(ps == &int_str_map_vec[0][4]);
	BOOST_CHECK(pqs && other.pqs && (*pqs == *other.pqs));
	BOOST_CHECK(pqs == &int_qstr_qmap_qvec[0][4]);
	BOOST_CHECK(pqs2 && other.pqs2 && (*pqs2 == *other.pqs2));
	BOOST_CHECK(pqs2 == &const_cast<QString &>(*qstr_list.begin()));
	BOOST_CHECK(ppi && other.ppi && *pi && *other.pi && (**ppi == **other.ppi));
	BOOST_CHECK(ppi == &pi && *ppi == &i);
	BOOST_CHECK(signed_ints == other.signed_ints);
	BOOST_CHECK(j == other.j);
	BOOST_CHECK(i == other.i);
	BOOST_CHECK(u == other.u);
	BOOST_CHECK(uw == other.uw);
	BOOST_CHECK(pr == other.pr);
	BOOST_CHECK(str_deq == other.str_deq);
	BOOST_CHECK(double_stack == other.double_stack);
	BOOST_CHECK(string_stack_queue == other.string_stack_queue);

	// Compare std::priority_queue<int>, but there's no equality operator...
	std::priority_queue<int> int_priority_queue_copy = int_priority_queue;
	std::priority_queue<int> other_int_priority_queue_copy = other.int_priority_queue;
	BOOST_CHECK(int_priority_queue_copy.size() == other_int_priority_queue_copy.size());
	if (int_priority_queue_copy.size() == other_int_priority_queue_copy.size())
	{
		while (!int_priority_queue_copy.empty())
		{
			BOOST_CHECK(int_priority_queue_copy.top() == other_int_priority_queue_copy.top());
			int_priority_queue_copy.pop();
			other_int_priority_queue_copy.pop();
		}
	}

	BOOST_CHECK(v == other.v);
	BOOST_CHECK(vv == other.vv);
	BOOST_CHECK(vu == other.vu);
	BOOST_CHECK(vu.size() == 1 && vu[0].str.length() == 12); // Ensure string wasn't clipped at first embedded zero.
	BOOST_CHECK(ilist == other.ilist);
	BOOST_CHECK(str_set == other.str_set);
	BOOST_CHECK(int_str_map_vec == other.int_str_map_vec);
	BOOST_CHECK(int_qstr_qmap_qvec == other.int_qstr_qmap_qvec);
	BOOST_CHECK(qill == other.qill);
	BOOST_CHECK(qstr_set == other.qstr_set);
	BOOST_CHECK(qstr_list == other.qstr_list);
	BOOST_CHECK(bin == other.bin && bin == boost::none);
	BOOST_CHECK(brin == other.brin && brin == boost::none);
	BOOST_CHECK(bi && other.bi && (*bi == *other.bi));
	BOOST_CHECK(bri == other.bri);
	// boost::optional<const int &> with reference to the integer inside 'bi'...
	BOOST_CHECK(bri && bi && *bi && (&bri.get() == &bi->get()));
	BOOST_CHECK(pbv && other.pbv && (*pbv == *other.pbv));
	BOOST_CHECK(pbv == boost::get<QString>(&bv));
	BOOST_CHECK(pbv2 && other.pbv2 && (*pbv2 == *other.pbv2));
	BOOST_CHECK(pbv2 == boost::get<NonDefaultConstructable>(&bv2));
	BOOST_CHECK(bv == other.bv);
	BOOST_CHECK(bv2 == other.bv2);
	BOOST_CHECK(qv == other.qv);
	BOOST_CHECK(qv_reg.type() == QVariant::UserType && other.qv_reg.type() == QVariant::UserType &&
			qv_reg.userType() == other.qv_reg.userType() &&
			qv_reg.canConvert<StringWithEmbeddedZeros>() && other.qv_reg.canConvert<StringWithEmbeddedZeros>() &&
			qv_reg.value<StringWithEmbeddedZeros>() == other.qv_reg.value<StringWithEmbeddedZeros>());
	BOOST_CHECK(lqv.size() == other.lqv.size());
	// 'qv_list' is just a QVariant wrapped around 'lqv'.
	BOOST_CHECK(qv_list.type() == QVariant::List && other.qv_list.type() == QVariant::List &&
			qv_list.canConvert< QList<QVariant> >() && other.qv_list.canConvert< QList<QVariant> >() &&
			qv_list.value< QList<QVariant> >().size() == lqv.size() &&
			other.qv_list.value< QList<QVariant> >().size() == other.lqv.size());
	for (int n = 0; n < lqv.size(); ++n)
	{
		BOOST_CHECK(lqv[n].type() == other.lqv[n].type());
		// 'qv_list' is just a QVariant wrapped around 'lqv'.
		BOOST_CHECK(qv_list.value< QList<QVariant> >()[n].type() == other.qv_list.value< QList<QVariant> >()[n].type());

		if (lqv[n].type() == QVariant::UserType)
		{
			BOOST_CHECK(
					lqv[n].canConvert<StringWithEmbeddedZeros>() && other.lqv[n].canConvert<StringWithEmbeddedZeros>() &&
					lqv[n].value<StringWithEmbeddedZeros>() == other.lqv[n].value<StringWithEmbeddedZeros>());
			// 'qv_list' is just a QVariant wrapped around 'lqv'.
			BOOST_CHECK(
					qv_list.value< QList<QVariant> >()[n].canConvert<StringWithEmbeddedZeros>() &&
						other.qv_list.value< QList<QVariant> >()[n].canConvert<StringWithEmbeddedZeros>() &&
					qv_list.value< QList<QVariant> >()[n].value<StringWithEmbeddedZeros>() ==
						other.qv_list.value< QList<QVariant> >()[n].value<StringWithEmbeddedZeros>());
		}
		else
		{
			BOOST_CHECK(lqv[n] == other.lqv[n]);
			// 'qv_list' is just a QVariant wrapped around 'lqv'.
			BOOST_CHECK(qv_list.value< QList<QVariant> >()[n] == other.qv_list.value< QList<QVariant> >()[n]);
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

	return transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, str);

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

		BOOST_CHECK_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr, "var_ptr"),
				GPlatesScribe::Exceptions::TranscribedUntrackedPointerBeforeReferencedObject);
	}
#endif
	// Skip this test in debug build because GPlatesGlobal::Assert() aborts instead of
	// throwing an exception and this test checks for exceptions...
#ifndef GPLATES_DEBUG
	{
		GPlatesScribe::Scribe scribe;

		BOOST_CHECK_THROW(
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

		BOOST_CHECK_THROW(
				scribe.transcribe(TRANSCRIBE_SOURCE, var, "var"),
				GPlatesScribe::Exceptions::UntrackingObjectWithReferences);
	}
#endif
	{
		GPlatesScribe::Scribe scribe;

		scribe.transcribe(TRANSCRIBE_SOURCE, var, "var");

		// This won't find 'var'.
		scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr, "var_ptr", GPlatesScribe::TRACK);

		BOOST_CHECK(
				!scribe.is_transcription_complete(false/*emit_warnings*/));
	}
	// Skip this test in debug build because GPlatesGlobal::Assert() aborts instead of
	// throwing an exception and this test checks for exceptions...
#ifndef GPLATES_DEBUG
	{
		GPlatesScribe::Scribe scribe;

		scribe.transcribe(TRANSCRIBE_SOURCE, var, "var", GPlatesScribe::TRACK);
		scribe.transcribe(TRANSCRIBE_SOURCE, var_ptr_ptr, "var_ptr_ptr", GPlatesScribe::TRACK);

		BOOST_CHECK_THROW(
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

		BOOST_CHECK(
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

		std::stringstream text_archive;

		test_case_untracked_1_write(
				GPlatesScribe::TextArchiveWriter::create(text_archive),
				before_var);

		text_archive.seekp(0);

		test_case_untracked_1_read(
				GPlatesScribe::TextArchiveReader::create(text_archive),
				before_var);

		//
		// Binary archive
		//

		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		test_case_untracked_1_write(
				GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer),
				before_var);

		binary_archive.close();

#if 0
		QFile archive_binary_file("archive_untracked_1.bin");
		bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
		BOOST_CHECK(archive_binary_open_for_writing);
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

		//
		// XML archive
		//

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
		QFile archive_xml_file("archive_untracked_1.xml");
		bool archive_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
		BOOST_CHECK(archive_open_for_writing);
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
		BOOST_CHECK(xml_stream_reader.isStartDocument());

		GPlatesScribe::XmlArchiveReader::non_null_ptr_type xml_archive_reader =
				GPlatesScribe::XmlArchiveReader::create(xml_stream_reader);

		test_case_untracked_1_read(
				xml_archive_reader,
				before_var);

		xml_archive_reader->close();
		xml_stream_reader.readNext();
		BOOST_CHECK(xml_stream_reader.isEndDocument());
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		BOOST_ERROR(message.str().c_str());
		return;
	}
}

void
GPlatesUnitTest::TranscribeUntrackedTest::test_case_untracked_1_write(
		const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
		variant_type &before_variant)
{
	GPlatesScribe::Scribe scribe;

	scribe.transcribe(TRANSCRIBE_SOURCE, before_variant, "variant", GPlatesScribe::TRACK);

	BOOST_CHECK(scribe.is_transcription_complete());

	archive_writer->write_transcription(*scribe.get_transcription());
}

void
GPlatesUnitTest::TranscribeUntrackedTest::test_case_untracked_1_read(
		const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
		variant_type &before_variant)
{
	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	variant_type after_variant;

	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_variant, "variant", GPlatesScribe::TRACK));

	BOOST_CHECK(scribe.is_transcription_complete());

	BOOST_CHECK(after_variant == before_variant);
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

		//
		// Binary archive
		//

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
		QFile archive_binary_file("archive_inheritance_1.bin");
		bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
		BOOST_CHECK(archive_binary_open_for_writing);
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

		//
		// XML archive
		//

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
		QFile archive_xml_file("archive_inheritance_1.xml");
		bool archive_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
		BOOST_CHECK(archive_open_for_writing);
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
		BOOST_CHECK(xml_stream_reader.isStartDocument());

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
		BOOST_CHECK(xml_stream_reader.isEndDocument());
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		BOOST_ERROR(message.str().c_str());
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

	BOOST_CHECK(scribe.is_transcription_complete());

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
	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	GPlatesScribe::TranscribeContext<A> transcribe_context_a(untranscribed_object);
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<A> transcribe_context_guard_a(
			scribe,
			transcribe_context_a);

	boost::optional<int> after_d;
	B *after_data_ptr;
	int *after_x_ptr;

	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_x_ptr, "x", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_d, "d", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_ptr, "data_ptr", GPlatesScribe::TRACK));

	// 'after_data2' has reference to internal 'a' object of 'after_data' and
	// 'after_data' gets relocated below so transcribe 'after_data2' first so we can check
	// that its pointer reference points to the relocated 'after_data'.
	GPlatesScribe::LoadRef<D> after_data2_ref = scribe.load<D>(TRANSCRIBE_SOURCE, "data2", GPlatesScribe::TRACK);
	BOOST_CHECK(after_data2_ref.is_valid());
	D after_data2 = after_data2_ref;
	scribe.relocated(TRANSCRIBE_SOURCE, after_data2, after_data2_ref);

	// Test object relocation where object ('D') has a non-empty abstract base class ('A').
	GPlatesScribe::LoadRef<D> after_data = scribe.load<D>(TRANSCRIBE_SOURCE, "data", GPlatesScribe::TRACK);
	BOOST_CHECK(after_data.is_valid());
	D relocated_after_data(after_data);
	scribe.relocated(TRANSCRIBE_SOURCE, relocated_after_data, after_data);

	GPlatesScribe::LoadRef<E> after_e_ref = scribe.load<E>(TRANSCRIBE_SOURCE, "data_e", GPlatesScribe::TRACK);
	BOOST_CHECK(after_e_ref.is_valid());
	E after_e = after_e_ref;
	scribe.relocated(TRANSCRIBE_SOURCE, after_e, after_e_ref);

	BOOST_CHECK(scribe.is_transcription_complete());

	BOOST_CHECK(after_x_ptr && (*after_x_ptr == *before_x_ptr));
	// Check relocation of 'D' and hence its explicit relocation handler properly relocates
	// its pointed-to 'x' integer which should update 'after_x_ptr'.
	BOOST_CHECK(after_x_ptr && (after_x_ptr == relocated_after_data.x.get()));

	BOOST_CHECK(after_d == before_d);
	// Make sure points to relocated object (not original object).
	BOOST_CHECK(after_data_ptr && (after_data_ptr == static_cast<B *>(&relocated_after_data)));
	relocated_after_data.check_equality(before_data);
	after_data2.check_equality(before_data2);
	// Make sure points to relocated object (not original object).
	BOOST_CHECK(after_data2.d == &relocated_after_data.a);
	if (after_data_ptr)
	{
		static_cast<D &>(*after_data_ptr).check_equality(before_data);
	}
	// Make sure points to untranscribed object.
	BOOST_CHECK(&relocated_after_data.untranscribed_object == &untranscribed_object);
	BOOST_CHECK(&after_data2.untranscribed_object == &untranscribed_object);
	// Make sure points to relocated object (not original object).
	BOOST_CHECK(&after_e.b == static_cast<B *>(&relocated_after_data));
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

		//
		// Binary archive
		//

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
		QFile archive_binary_file("archive_inheritance_2.bin");
		bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
		BOOST_CHECK(archive_binary_open_for_writing);
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

		//
		// XML archive
		//

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
		QFile archive_xml_file("archive_inheritance_2.xml");
		bool archive_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
		BOOST_CHECK(archive_open_for_writing);
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
		BOOST_CHECK(xml_stream_reader.isStartDocument());

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
		BOOST_CHECK(xml_stream_reader.isEndDocument());
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		BOOST_ERROR(message.str().c_str());
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

	BOOST_CHECK(scribe.is_transcription_complete());

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
	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	GPlatesScribe::TranscribeContext<A> transcribe_context_a(untranscribed_object);
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<A> transcribe_context_guard_a(
			scribe,
			transcribe_context_a);

	boost::scoped_ptr<int> after_d;
	boost::shared_ptr<B> after_data_ptr;
	boost::weak_ptr<B> after_data_weak_ptr;
	boost::shared_ptr<D> after_data_ptr2;

	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_d, "d", GPlatesScribe::TRACK));
	// Transcribe through base class pointer.
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_weak_ptr, "data_weak_ptr", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_ptr, "data_ptr", GPlatesScribe::TRACK));
	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_data_ptr2, "data_ptr2", GPlatesScribe::TRACK));

	GPlatesScribe::LoadRef< GPlatesUtils::non_null_intrusive_ptr<E> > after_intrusive_ptr_ref =
			scribe.load< GPlatesUtils::non_null_intrusive_ptr<E> >(
					TRANSCRIBE_SOURCE, "data_intrusive_ptr", GPlatesScribe::TRACK);
	BOOST_CHECK(after_intrusive_ptr_ref.is_valid());
	GPlatesUtils::non_null_intrusive_ptr<E> after_intrusive_ptr = after_intrusive_ptr_ref;
	scribe.relocated(TRANSCRIBE_SOURCE, after_intrusive_ptr, after_intrusive_ptr_ref);

	BOOST_CHECK(scribe.is_transcription_complete());

	BOOST_CHECK(after_d);
	BOOST_CHECK(*after_d == *before_d);
	BOOST_CHECK(after_data_ptr);
	BOOST_CHECK(!after_data_weak_ptr.expired());
	BOOST_CHECK(after_data_weak_ptr.lock() == after_data_ptr);
	BOOST_CHECK(after_data_ptr2);
	BOOST_CHECK(after_data_ptr && (typeid(*after_data_ptr) == typeid(D)));
	BOOST_CHECK(after_data_ptr2 && (typeid(*after_data_ptr2) == typeid(D)));
	if (after_data_ptr)
	{
		static_cast<D &>(*after_data_ptr).check_equality(static_cast<const D &>(*before_data_ptr));
		BOOST_CHECK(&static_cast<D &>(*after_data_ptr).untranscribed_object == &untranscribed_object);
	}
	if (after_data_ptr2)
	{
		static_cast<D &>(*after_data_ptr2).check_equality(static_cast<const D &>(*before_data_ptr));
		BOOST_CHECK(&static_cast<D &>(*after_data_ptr2).untranscribed_object == &untranscribed_object);
	}
	BOOST_CHECK(after_intrusive_ptr);
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
	b_object.check_equality(other.b_object);
	BOOST_CHECK(a == other.a);
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
	BOOST_CHECK(b == other.b);
	BOOST_CHECK(int_pair == other.int_pair);
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
	A::check_equality(other);
	B::check_equality(other);

	BOOST_CHECK(d && (*d == *other.d));
	BOOST_CHECK(x && (*x == *other.x));
	BOOST_CHECK(y == other.y);

	if (self.expired())
	{
		BOOST_CHECK(other.self.expired());
	}
	else
	{
		BOOST_CHECK(self.lock() && (self.lock().get() == this));
		BOOST_CHECK(other.self.lock() && (other.self.lock().get() == &other));
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

		std::stringstream text_archive;

		test_case_compatibility_1_write(
				GPlatesScribe::TextArchiveWriter::create(text_archive),
				before_smart_ptr_data);

		text_archive.seekp(0);

		test_case_compatibility_1_read(
				GPlatesScribe::TextArchiveReader::create(text_archive),
				before_smart_ptr_data);

		//
		// Binary archive
		//

		QBuffer binary_archive;
		binary_archive.open(QBuffer::WriteOnly);

		QDataStream binary_stream_writer(&binary_archive);

		test_case_compatibility_1_write(
				GPlatesScribe::BinaryArchiveWriter::create(binary_stream_writer),
				before_smart_ptr_data);

		binary_archive.close();

#if 0
		QFile archive_binary_file("archive_compatibility_1.bin");
		bool archive_binary_open_for_writing = archive_binary_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
		BOOST_CHECK(archive_binary_open_for_writing);
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

		//
		// XML archive
		//

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
		QFile archive_xml_file("archive_compatibility_1.xml");
		bool archive_open_for_writing = archive_xml_file.open(QIODevice::WriteOnly | QIODevice::Text);
		BOOST_CHECK(archive_open_for_writing);
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
		BOOST_CHECK(xml_stream_reader.isStartDocument());

		GPlatesScribe::XmlArchiveReader::non_null_ptr_type xml_archive_reader =
				GPlatesScribe::XmlArchiveReader::create(xml_stream_reader);

		test_case_compatibility_1_read(
				xml_archive_reader,
				before_smart_ptr_data);

		xml_archive_reader->close();
		xml_stream_reader.readNext();
		BOOST_CHECK(xml_stream_reader.isEndDocument());
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		std::ostringstream message;
		message << "Error transcribing: " << scribe_exception;
		BOOST_ERROR(message.str().c_str());
		return;
	}
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::test_case_compatibility_1_write(
		const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
		SmartPtrData &before_smart_ptr_data)
{
	GPlatesScribe::Scribe scribe;

	scribe.transcribe(TRANSCRIBE_SOURCE, before_smart_ptr_data, "smart_ptr_data", GPlatesScribe::TRACK);

	BOOST_CHECK(scribe.is_transcription_complete());

	archive_writer->write_transcription(*scribe.get_transcription());
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::test_case_compatibility_1_read(
		const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
		SmartPtrData &before_smart_ptr_data)
{
	GPlatesScribe::Scribe scribe(archive_reader->read_transcription());

	SmartPtrData after_smart_ptr_data;

	BOOST_CHECK(scribe.transcribe(TRANSCRIBE_SOURCE, after_smart_ptr_data, "smart_ptr_data", GPlatesScribe::TRACK));
	before_smart_ptr_data.check_equality(after_smart_ptr_data);

	BOOST_CHECK(scribe.is_transcription_complete());
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::Derived::check_equality(
		const Derived &other) const
{
	BOOST_CHECK(d_value == other.d_value);
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
	d_auto_ptr.reset(new Derived(value));
	d_non_null_intrusive_ptr = GPlatesUtils::non_null_intrusive_ptr<Base>(new Derived(value));

	d_pre_derived_object1 = Derived(value);
	d_pre_derived_object_ptr2.reset(new Derived(value));
}

void
GPlatesUnitTest::TranscribeCompatibilityTest::SmartPtrData::check_equality(
		const SmartPtrData &other) const
{
	BOOST_CHECK(d_scoped_ptr && other.d_scoped_ptr);
	if (d_scoped_ptr && other.d_scoped_ptr)
	{
		BOOST_CHECK(dynamic_cast<Derived *>(d_scoped_ptr.get()) && dynamic_cast<Derived *>(other.d_scoped_ptr.get()));
		dynamic_cast<Derived *>(d_scoped_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_scoped_ptr));
	}

	BOOST_CHECK(d_shared_ptr && other.d_shared_ptr);
	if (d_shared_ptr && other.d_shared_ptr)
	{
		BOOST_CHECK(dynamic_cast<Derived *>(d_shared_ptr.get()) && dynamic_cast<Derived *>(other.d_shared_ptr.get()));
		dynamic_cast<Derived *>(d_shared_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_shared_ptr));
	}

	BOOST_CHECK(d_shared_ptr2 && other.d_shared_ptr2);
	if (d_shared_ptr2 && other.d_shared_ptr2)
	{
		BOOST_CHECK(dynamic_cast<Derived *>(d_shared_ptr2.get()) && dynamic_cast<Derived *>(other.d_shared_ptr2.get()));
		dynamic_cast<Derived *>(d_shared_ptr2.get())->check_equality(dynamic_cast<Derived &>(*other.d_shared_ptr2));
	}

	BOOST_CHECK(d_intrusive_ptr && other.d_intrusive_ptr);
	if (d_intrusive_ptr && other.d_intrusive_ptr)
	{
		BOOST_CHECK(dynamic_cast<Derived *>(d_intrusive_ptr.get()) && dynamic_cast<Derived *>(other.d_intrusive_ptr.get()));
		dynamic_cast<Derived *>(d_intrusive_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_intrusive_ptr));
	}

	BOOST_CHECK(d_intrusive_ptr2 && other.d_intrusive_ptr2);
	if (d_intrusive_ptr2 && other.d_intrusive_ptr2)
	{
		BOOST_CHECK(dynamic_cast<Derived *>(d_intrusive_ptr2.get()) && dynamic_cast<Derived *>(other.d_intrusive_ptr2.get()));
		dynamic_cast<Derived *>(d_intrusive_ptr2.get())->check_equality(dynamic_cast<Derived &>(*other.d_intrusive_ptr2));
	}

	BOOST_CHECK(d_auto_ptr.get() && other.d_auto_ptr.get());
	if (d_auto_ptr.get() && other.d_auto_ptr.get())
	{
		BOOST_CHECK(dynamic_cast<Derived *>(d_auto_ptr.get()) && dynamic_cast<Derived *>(other.d_auto_ptr.get()));
		dynamic_cast<Derived *>(d_auto_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_auto_ptr));
	}

	BOOST_CHECK(dynamic_cast<Derived *>(d_non_null_intrusive_ptr.get()) && dynamic_cast<Derived *>(other.d_non_null_intrusive_ptr.get()));
	dynamic_cast<Derived *>(d_non_null_intrusive_ptr.get())->check_equality(dynamic_cast<Derived &>(*other.d_non_null_intrusive_ptr));

	BOOST_CHECK(dynamic_cast<Derived *>(other.d_post_derived_object_ptr1.get()));
	d_pre_derived_object1.check_equality(dynamic_cast<Derived &>(*other.d_post_derived_object_ptr1));

	BOOST_CHECK(dynamic_cast<Derived *>(d_pre_derived_object_ptr2.get()));
	dynamic_cast<Derived &>(*d_pre_derived_object_ptr2).check_equality(other.d_post_derived_object2);
	BOOST_CHECK(other.d_post_derived_object_ptr2 == &other.d_post_derived_object2);
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
		scribe.transcribe(TRANSCRIBE_SOURCE, d_auto_ptr, "d_auto_ptr", GPlatesScribe::TRACK);
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
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr, "d_auto_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr2, "d_auto_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr, "d_shared_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr2, "d_shared_ptr") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_auto_ptr, "d_intrusive_ptr") ||
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

		BOOST_CHECK(d_shared_ptr != d_shared_ptr2);
		BOOST_CHECK(d_intrusive_ptr != d_intrusive_ptr2);

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr, "d_shared_ptr", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_shared_ptr2, "d_shared_ptr2", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr, "d_intrusive_ptr", GPlatesScribe::TRACK) ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_intrusive_ptr2, "d_intrusive_ptr2", GPlatesScribe::TRACK))
		{
			return scribe.get_transcribe_result();
		}

		BOOST_CHECK(d_shared_ptr == d_shared_ptr2);
		BOOST_CHECK(d_intrusive_ptr == d_intrusive_ptr2);

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

void
GPlatesUnitTest::TranscribeTestSuite::construct_maps()
{
	construct_transcribe_primitives_test();
	construct_transcribe_untracked_test();
	construct_transcribe_inheritance_test();
	construct_transcribe_compatibility_test();
}

void
GPlatesUnitTest::TranscribeTestSuite::construct_transcribe_primitives_test()
{
	boost::shared_ptr<TranscribePrimitivesTest> instance(new TranscribePrimitivesTest());
	ADD_TESTCASE(TranscribePrimitivesTest,test_case_primitives_1);
}
void
GPlatesUnitTest::TranscribeTestSuite::construct_transcribe_untracked_test()
{
	boost::shared_ptr<TranscribeUntrackedTest> instance(new TranscribeUntrackedTest());
	ADD_TESTCASE(TranscribeUntrackedTest,test_case_untracked_exception);
	ADD_TESTCASE(TranscribeUntrackedTest,test_case_untracked_1);
}

void
GPlatesUnitTest::TranscribeTestSuite::construct_transcribe_inheritance_test()
{
	boost::shared_ptr<TranscribeInheritanceTest> instance(new TranscribeInheritanceTest());
	ADD_TESTCASE(TranscribeInheritanceTest,test_case_inheritance_1);
	ADD_TESTCASE(TranscribeInheritanceTest,test_case_inheritance_2);
}

void
GPlatesUnitTest::TranscribeTestSuite::construct_transcribe_compatibility_test()
{
	boost::shared_ptr<TranscribeCompatibilityTest> instance(new TranscribeCompatibilityTest());
	ADD_TESTCASE(TranscribeCompatibilityTest,test_case_compatibility_1);
}
