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
#ifndef GPLATES_UNIT_TEST_TRANSCRIBE_TEST_H
#define GPLATES_UNIT_TEST_TRANSCRIBE_TEST_H

#include <deque>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/variant.hpp>
#include <boost/weak_ptr.hpp>
#include <QDataStream>
#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include "maths/Real.h"

#include "property-values/GeoTimeInstant.h"

#include "scribe/ScribeArchiveReader.h"
#include "scribe/ScribeArchiveWriter.h"
#include "scribe/Transcribe.h"
#include "scribe/TranscribeContext.h"
#include "scribe/TranscribeEnumProtocol.h"

#include "unit-test/GPlatesTestSuite.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesUnitTest
{
	/**
	 * Test transcribing of primitives and pointers to them.
	 */
	class TranscribePrimitivesTest
	{
	public:
		TranscribePrimitivesTest()
		{ }

		void 
		test_case_primitives_1();


		class Data :
				private boost::noncopyable
		{
		public:

			struct NonDefaultConstructable;

			explicit
			Data(
					int bv2_);

			explicit
			Data(
					const boost::variant<NonDefaultConstructable, char, QString, double> &bv2_);

			~Data();

			void
			initialise();

			void
			check_equality(
					const Data &other);

			struct NonDefaultConstructable
			{
				NonDefaultConstructable(
						int i_) :
					i(i_)
				{  }

				operator int() const
				{
					return i;
				}

				int i;
			};

			enum Enum
			{
				ENUM_VALUE_1,
				ENUM_VALUE_2,
				ENUM_VALUE_3
			};

		private: // Test private enum...

			enum Enum2
			{
				ENUM2_VALUE_1,
				ENUM2_VALUE_2,
				ENUM2_VALUE_3
			};

			//
			// Use friend function (injection) so can access private enum.
			// And implement in class body otherwise some compilers will complain
			// that the enum argument is not accessible (since enum is private).
			//
			friend
			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					Enum2 &e,
					bool transcribed_construct_data)
			{
				// WARNING: Changing the string ids will break backward/forward compatibility.
				//          So don't change the string ids even if the enum name changes.
				static const GPlatesScribe::EnumValue enum_values[] =
				{
					GPlatesScribe::EnumValue("ENUM2_VALUE_1", ENUM2_VALUE_1),
					GPlatesScribe::EnumValue("ENUM2_VALUE_2", ENUM2_VALUE_2),
					GPlatesScribe::EnumValue("ENUM2_VALUE_3", ENUM2_VALUE_3)
				};

				return GPlatesScribe::transcribe_enum_protocol(
						TRANSCRIBE_SOURCE,
						scribe,
						e,
						enum_values,
						enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
			}

		public:

			struct StringWithEmbeddedZeros
			{
				QString str;

				bool
				operator==(
						const StringWithEmbeddedZeros &other) const
				{
					return str == other.str;
				}

			private:
				GPlatesScribe::TranscribeResult
				transcribe(
						GPlatesScribe::Scribe &scribe,
						bool transcribed_construct_data);

				friend class GPlatesScribe::Access;
			};

			struct QStringWrapper
			{
				QStringWrapper()
				{}


				QString str;

				bool
				operator==(
						const QStringWrapper &other) const
				{
					return str == other.str;
				}

			private:
				GPlatesScribe::TranscribeResult
				transcribe(
						GPlatesScribe::Scribe &scribe,
						bool transcribed_construct_data);

				friend class GPlatesScribe::Access;
			};

		private:

			int ia[2][2];
			Enum e;
			Enum2 e2;
			bool b;
			float f;
			const double d;
			float f_pos_inf, f_neg_inf, f_nan;
			double d_pos_inf, d_neg_inf, d_nan;
			GPlatesMaths::Real real;
			GPlatesPropertyValues::GeoTimeInstant geo_real_time;
			GPlatesPropertyValues::GeoTimeInstant geo_distant_past;
			GPlatesPropertyValues::GeoTimeInstant geo_distant_future;
			char c;
			short s;
			long l;
			int i;
			int j;
			std::vector<int> signed_ints;
			QString u;
			QStringWrapper uw;
			int *pi;
			int *pj;
			int *pk;
			int *pl;
			std::string *ps;
			QString *pqs;
			QString *pqs2;
			int **ppi;
			std::pair<int, std::string> pr;
			std::deque<std::string> str_deq;
			std::queue< std::stack<std::string> > string_stack_queue;
			std::priority_queue<int> int_priority_queue;
			std::stack<double> double_stack;
			std::vector<int> v;
			std::vector< std::vector<int> > vv;
			std::vector<StringWithEmbeddedZeros> vu;
			std::list<int> ilist;
			std::set<std::string> str_set;
			std::vector< std::map<int, std::string> > int_str_map_vec;
			QVector< QMap<int, QString> > int_qstr_qmap_qvec;
			QLinkedList<int> qill;
			QSet<QString> qstr_set;
			QStringList qstr_list;
			boost::optional<int> bin;
			boost::optional<int &> brin;
			boost::scoped_ptr< boost::optional<int> > bi;
			boost::optional<const int &> bri;
			QString *pbv;
			boost::variant<int, char, QString, double> bv;
			NonDefaultConstructable *pbv2;
			boost::variant<NonDefaultConstructable, char, QString, double> bv2;
			QVariant qv;
			QVariant qv_reg;
			QList<QVariant> lqv;
			QVariant qv_list;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			static
			GPlatesScribe::TranscribeResult
			transcribe_construct_data(
					GPlatesScribe::Scribe &scribe,
					GPlatesScribe::ConstructObject<Data> &data);

			friend class GPlatesScribe::Access;
		};

	private:

		void
		test_case_1_write(
				const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
				boost::scoped_ptr<Data> &before_data_scoped_ptr,
				Data &before_data,
				const std::string (&before_string_array)[2],
				const char (&before_char_array)[1][2][6],
				const Data::NonDefaultConstructable (&before_non_default_constructable_array)[1][2],
				const Data::NonDefaultConstructable (*&before_non_default_constructable_array_ptr)[1][2],
				const Data::NonDefaultConstructable (*const &before_non_default_constructable_sub_array_ptr)[2],
				const Data::NonDefaultConstructable (*const *const before_non_default_constructable_sub_array_ptr_ptr)[2],
				const Data::NonDefaultConstructable *&before_non_default_constructable_array_element_ptr);

		void
		test_case_1_read(
				const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
				boost::scoped_ptr<Data> &before_data_scoped_ptr,
				Data &before_data,
				const std::string (&before_string_array)[2],
				const char (&before_char_array)[1][2][6],
				const Data::NonDefaultConstructable (&before_non_default_constructable_array)[1][2],
				const Data::NonDefaultConstructable (*&before_non_default_constructable_array_ptr)[1][2],
				const Data::NonDefaultConstructable (*const &before_non_default_constructable_sub_array_ptr)[2],
				const Data::NonDefaultConstructable (*const *const before_non_default_constructable_sub_array_ptr_ptr)[2],
				const Data::NonDefaultConstructable *&before_non_default_constructable_array_element_ptr);
	};

	inline
	QDataStream &
	operator<<(
			QDataStream &out,
			const TranscribePrimitivesTest::Data::StringWithEmbeddedZeros &obj)
	{
		out << obj.str;
		return out;
	}

	inline
	QDataStream &
	operator>>(
			QDataStream &in,
			TranscribePrimitivesTest::Data::StringWithEmbeddedZeros &obj)
	{
		in >> obj.str;
		return in;
	}

	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			TranscribePrimitivesTest::Data::Enum &e,
			bool transcribed_construct_data);

	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			TranscribePrimitivesTest::Data::NonDefaultConstructable &ndc,
			bool transcribed_construct_data);

	GPlatesScribe::TranscribeResult
	transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<TranscribePrimitivesTest::Data::NonDefaultConstructable> &ndc);


	/**
	 * Test transcribing untracked objects.
	 */
	class TranscribeUntrackedTest
	{
	public:

		typedef boost::variant<int, std::string> variant_type;

		void
		test_case_untracked_exception();

		void 
		test_case_untracked_1();

	private:

		void
		test_case_untracked_1_write(
				const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
				variant_type &before_variant);

		void
		test_case_untracked_1_read(
				const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
				variant_type &before_variant);
	};


	/**
	 * Test transcribing of base class pointers to derived class objects.
	 */
	class TranscribeInheritanceTest
	{
	public:
		TranscribeInheritanceTest()
		{ }

		void 
		test_case_inheritance_1();

		void 
		test_case_inheritance_2();


		// A class that is not transcribed but will be referenced by a transcribed class.
		struct UntranscribedClass
		{
		};

		typedef std::pair<TranscribePrimitivesTest::Data::NonDefaultConstructable, int> int_pair_type;

		class B
		{
		public:
			virtual
			~B()
			{  }

			explicit
			B(
					const int_pair_type &int_pair_) :
				b(0),
				int_pair(int_pair_)
			{  }

			void
			initialise(
					int b_);

			void
			check_equality(
					const B &other) const;

			int b;
			int_pair_type int_pair;

		protected:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};

		class A
		{
		public:
			virtual
			~A()
			{  }

			explicit
			A(
					int a_,
					const int_pair_type &b_int_pair,
					const UntranscribedClass &untranscribed_object_) :
				b_object(b_int_pair),
				a(a_),
				untranscribed_object(untranscribed_object_)
			{  }

			void
			initialise(
					int b_);

			void
			check_equality(
					const A &other) const;

			virtual
			void
			test_pure_virtual() = 0;

			B b_object;
			int a;
			const UntranscribedClass &untranscribed_object;

		protected:

			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);
		};

		class D :
				public A,
				public B
		{
		public:

			explicit
			D(
					int &d_,
					int a_,
					const int_pair_type &a_int_pair,
					const int_pair_type &b_int_pair,
					const UntranscribedClass &untranscribed_object_) :
				A(a_, a_int_pair, untranscribed_object_),
				B(b_int_pair),
				d(&d_),
				x(new int(0))
			{  }

			D(
					const D &other) :
				A(other),
				B(other),
				d(other.d),
				x(other.x ? new int(*other.x) : NULL),
				y(other.y)
			{  }

			void
			initialise(
					int b_for_a,
					int b_for_b,
					boost::weak_ptr<D> self_ = boost::weak_ptr<D>());

			void
			check_equality(
					const D &other) const;

			virtual
			void
			test_pure_virtual()
			{  }

			int *d;

			boost::scoped_ptr<int> x;
			boost::optional<int> y; // Test relocation
			boost::weak_ptr<D> self; // Only non-null if client has boost::shared_ptr to this.

		private:

			// Not assignable.
			D &
			operator=(
					const D &other);

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			static
			void
			relocated(
					GPlatesScribe::Scribe &scribe,
					const D &relocated_d,
					const D &transcribed_d);

			friend class GPlatesScribe::Access;
		};

		class E :
				public GPlatesUtils::ReferenceCount<E>
		{
		public:

			explicit
			E(
					B &b_) :
				b(b_)
			{  }

			E(
					const E &other) :
				GPlatesUtils::ReferenceCount<E>(),
				b(other.b)
			{  }

			void
			check_equality(
					const E &other) const;

			B &b;
		};

	private:

		void
		test_case_inheritance_1_write(
				const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
				UntranscribedClass &untranscribed_object,
				boost::optional<int> &before_d,
				D &before_data,
				B *&before_data_ptr,
				int *&before_x_ptr,
				D &before_data2,
				E &before_e);

		void
		test_case_inheritance_1_read(
				const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
				UntranscribedClass &untranscribed_object,
				boost::optional<int> &before_d,
				D &before_data,
				B *&before_data_ptr,
				int *&before_x_ptr,
				D &before_data2,
				E &before_e);

		void
		test_case_inheritance_2_write(
				const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
				UntranscribedClass &untranscribed_object,
				boost::scoped_ptr<int> &before_d,
				boost::shared_ptr<B> &before_data_ptr,
				boost::weak_ptr<B> &before_data_weak_ptr,
				boost::shared_ptr<D> &before_data_ptr2,
				GPlatesUtils::non_null_intrusive_ptr<E> &before_intrusive_ptr);

		void
		test_case_inheritance_2_read(
				const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
				UntranscribedClass &untranscribed_object,
				boost::scoped_ptr<int> &before_d,
				boost::shared_ptr<B> &before_data_ptr,
				boost::weak_ptr<B> &before_data_weak_ptr,
				boost::shared_ptr<D> &before_data_ptr2,
				GPlatesUtils::non_null_intrusive_ptr<E> &before_intrusive_ptr);
	};

	GPlatesScribe::TranscribeResult
	transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<TranscribeInheritanceTest::B> &b);

	GPlatesScribe::TranscribeResult
	transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<TranscribeInheritanceTest::D> &d);

	inline
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			TranscribeInheritanceTest::E &e,
			bool transcribed_construct_data)
	{
		// Do nothing.
		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}

	GPlatesScribe::TranscribeResult
	transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<TranscribeInheritanceTest::E> &e);


	/**
	 * Test backward/forward compatibility.
	 */
	class TranscribeCompatibilityTest
	{
	public:
		TranscribeCompatibilityTest()
		{ }

		void 
		test_case_compatibility_1();


		// A class that is not transcribed but will be referenced by a transcribed class.
		struct UntranscribedClass
		{
		};

		typedef std::pair<TranscribePrimitivesTest::Data::NonDefaultConstructable, int> int_pair_type;

		class Base :
				public GPlatesUtils::ReferenceCount<Base>
		{
		public:

			Base() :
				GPlatesUtils::ReferenceCount<Base>()
			{  }

			Base(
					const Base &other) :
				GPlatesUtils::ReferenceCount<Base>()
			{  }

			Base &
			operator=(
					const Base &other)
			{
				// Ignore reference count.
				return *this;
			}

			virtual
			~Base()
			{  }

			virtual
			void
			func() = 0;
		};

		class Derived :
				public Base
		{
		public:

			explicit
			Derived(
					const std::string &value) :
				d_value(value)
			{  }

			void
			check_equality(
					const Derived &other) const;

			virtual
			void
			func()
			{  }

			std::string d_value;

		protected:

			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			static
			GPlatesScribe::TranscribeResult
			transcribe_construct_data(
					GPlatesScribe::Scribe &scribe,
					GPlatesScribe::ConstructObject<Derived> &derived);
		};

		class SmartPtrData
		{
		public:

			SmartPtrData() :
				d_non_null_intrusive_ptr(new Derived("")),
				d_pre_derived_object1(""),
				d_pre_derived_object_ptr1(&d_pre_derived_object1),
				d_post_derived_object2(""),
				d_post_derived_object_ptr2(NULL)
			{  }

			void
			initialise(
					const std::string &value);

			void
			check_equality(
					const SmartPtrData &other) const;

		private:

			boost::scoped_ptr<Base> d_scoped_ptr;
			boost::shared_ptr<Base> d_shared_ptr, d_shared_ptr2;
			boost::intrusive_ptr<Base> d_intrusive_ptr, d_intrusive_ptr2;
			std::auto_ptr<Base> d_auto_ptr;
			GPlatesUtils::non_null_intrusive_ptr<Base> d_non_null_intrusive_ptr;

			Derived d_pre_derived_object1;
			Derived *d_pre_derived_object_ptr1;
			boost::shared_ptr<Base> d_post_derived_object_ptr1;

			boost::intrusive_ptr<Base> d_pre_derived_object_ptr2;
			Derived d_post_derived_object2;
			Base *d_post_derived_object_ptr2;


			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);
		};

	private:

		void
		test_case_compatibility_1_write(
				const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
				SmartPtrData &before_smart_ptr_data);

		void
		test_case_compatibility_1_read(
				const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
				SmartPtrData &before_smart_ptr_data);
	};



	//
	// To run only Transcribe test suite:
	//
	// gplates-unit-test.exe --G_test_to_run=*/Transcribe
	//
	class TranscribeTestSuite : 
		public GPlatesUnitTest::GPlatesTestSuite
	{
	public:
		TranscribeTestSuite(
				unsigned depth);

	protected:
		void 
		construct_maps();

	private:
		void
		construct_transcribe_primitives_test();

		void
		construct_transcribe_untracked_test();

		void
		construct_transcribe_inheritance_test();

		void
		construct_transcribe_compatibility_test();
	};
}

namespace GPlatesScribe
{
	template <>
	class TranscribeContext<GPlatesUnitTest::TranscribeInheritanceTest::A>
	{
	public:
		explicit
		TranscribeContext(
				const GPlatesUnitTest::TranscribeInheritanceTest::UntranscribedClass &untranscribed_object_) :
			untranscribed_object(untranscribed_object_)
		{  }

		const GPlatesUnitTest::TranscribeInheritanceTest::UntranscribedClass &untranscribed_object;
	};
}

#endif //GPLATES_UNIT_TEST_TRANSCRIBE_TEST_H 

