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
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/weak_ptr.hpp>
#include <QDataStream>
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
					Enum2 &enum2,
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
						enum2,
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
			std::unique_ptr<Base> d_unique_ptr;
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


	/**
	 * Test transcribing via the raw stream ("raw lane") - the RAW option.
	 */
	class TranscribeRawTest
	{
	public:
		TranscribeRawTest()
		{ }

		//! Raw round trip of primitives/strings/containers through all archive types.
		void
		test_case_raw_1();

		//! Raw round trip of full-width 64-bit integers (general path restricts to 32-bit range).
		void
		test_case_raw_64_bit_integers();

		//! Raw round trip of an integer saved through one integral type and loaded through another.
		void
		test_case_raw_cross_type_integers();

		//! Raw round trip of owning pointers (inline, NULL, polymorphic, shared-owner backrefs).
		void
		test_case_raw_pointers();

		//! Raw round trip of a shared object nested inside another shared object (backref ordering).
		void
		test_case_raw_nested_shared_objects();

		//! Loading between transcriptions saved with and without the RAW option.
		void
		test_case_raw_compatibility();

		//! Errors: unsupported operations in raw mode, codec version rejection, positional mismatches.
		void
		test_case_raw_errors();

		//! Raw round trip of 'Scribe::transcribe_raw_array' - bulk fixed-size arithmetic arrays.
		void
		test_case_raw_array();

		//! Raw round trip of native C arrays through "TranscribeArray.h" (1D, multidimensional and
		//! non-arithmetic element types).
		void
		test_case_fixed_array();


		//! Tests save/load construction (non-default constructor) inside a raw subtree.
		struct NestedData
		{
			explicit
			NestedData(
					int value_) :
				value(value_)
			{  }

			int value;

		private:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			static
			GPlatesScribe::TranscribeResult
			transcribe_construct_data(
					GPlatesScribe::Scribe &scribe,
					GPlatesScribe::ConstructObject<NestedData> &nested_data);

			friend class GPlatesScribe::Access;
		};

		enum Enum
		{
			ENUM_VALUE_1,
			ENUM_VALUE_2,
			ENUM_VALUE_3
		};

		struct Data
		{
			Data();

			void
			initialise();

			void
			check_equality(
					const Data &other) const;

			bool b;
			char c;
			short s;
			int i;
			unsigned int ui;
			long l;
			float f;
			double d;
			GPlatesMaths::Real real; // Transcribes via the delegate protocol.
			Enum e;
			std::string str1;
			std::string str2; // Same value as 'str1' (tests string interning).
			std::string str3;
			QString qstr;
			std::vector<int> int_vec; // Transcribed with a nested RAW option (which is ignored).
			std::vector<std::string> str_vec;
			std::map<int, std::string> int_str_map;
			boost::optional<double> opt_some;
			boost::optional<double> opt_none;
			NestedData nested; // Transcribed via save/load construction.

		private:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};


		//! Fixed-size arithmetic arrays transcribed with 'Scribe::transcribe_raw_array'.
		struct ArrayData
		{
			ArrayData();

			void
			initialise();

			void
			check_equality(
					const ArrayData &other) const;

			double doubles[4];
			float floats[3];
			int ints[5];

		private:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};


		/**
		 * Fixed-size native C arrays transcribed via 'scribe.transcribe()' (ie, through
		 * "TranscribeArray.h" rather than calling 'Scribe::transcribe_raw_array' directly like
		 * @a ArrayData does). Covers a 1D arithmetic array (raw-lane fast path), a multidimensional
		 * arithmetic array (recurses down to the same fast path) and a non-arithmetic array
		 * (general per-element path only - 'transcribe_raw_array' doesn't support class types).
		 */
		struct FixedArrayData
		{
			FixedArrayData();

			void
			initialise();

			void
			check_equality(
					const FixedArrayData &other) const;

			int nums[4];
			double matrix[2][3];
			std::string strs[2];

		private:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};


		//! First polymorphic base of @a Derived (raw-lane owning pointer tests).
		class BaseA
		{
		public:
			virtual
			~BaseA()
			{  }

			BaseA() :
				a(0)
			{  }

			int a;

		protected:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};

		/**
		 * Second polymorphic base of @a Derived (raw-lane owning pointer tests).
		 *
		 * Being the *second* base gives it a non-zero pointer offset within @a Derived, which
		 * exercises the multiple-inheritance pointer fix-ups of shared-object backrefs.
		 */
		class BaseB
		{
		public:
			virtual
			~BaseB()
			{  }

			BaseB() :
				b(0)
			{  }

			int b;

		protected:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};

		/**
		 * A multiply-inherited polymorphic class transcribed via (owning) base class pointers
		 * in the raw lane - its class name is transcribed as a unique-string-pool index
		 * (it is export registered in "ScribeExportUnitTest.h").
		 */
		class Derived :
				public BaseA,
				public BaseB
		{
		public:

			Derived() :
				d(0)
			{  }

			int d;

		private:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};

		/**
		 * An intrusively reference-counted class (raw-lane owning pointer tests) - its reference
		 * count is passed as a use-count hint that selects between the deduplicated (shared) and
		 * inline (sole owner) raw-lane pointer encodings.
		 */
		class RefCountedData :
				public GPlatesUtils::ReferenceCount<RefCountedData>
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<RefCountedData> non_null_ptr_type;

			explicit
			RefCountedData(
					int value_ = 0) :
				value(value_)
			{  }

			int value;

			// An optional nested (shared) child - used to test backref ordering when a shared
			// object is nested inside another shared object.
			boost::optional<non_null_ptr_type> child;

		private:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};

		//! An object graph of owning pointers (raw-lane owning pointer tests).
		struct PointerData
		{
			PointerData();

			~PointerData();

			void
			initialise();

			void
			check_equality(
					const PointerData &other) const;

			boost::scoped_ptr<int> scoped;         // Exclusive owner - streamed inline.
			int *raw_owned;                        // EXCLUSIVE_OWNER raw pointer - streamed inline.
			boost::shared_ptr<BaseA> shared_a;     // A Derived object shared with 'shared_b' and 'weak_a'.
			boost::shared_ptr<BaseB> shared_b;     // Same Derived object - backref (with pointer offset).
			boost::weak_ptr<BaseA> weak_a;         // Weak pointer to the same Derived object - backref.
			boost::shared_ptr<BaseA> shared_null;  // NULL pointer.
			RefCountedData::non_null_ptr_type intrusive_1;    // Shared with 'intrusive_2' (use count 2 - deduplicated).
			RefCountedData::non_null_ptr_type intrusive_2;    // Same object - backref.
			RefCountedData::non_null_ptr_type intrusive_solo; // Sole owner (use count 1 - streamed inline).

		private:

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			friend class GPlatesScribe::Access;
		};

	private:

		void
		test_case_raw_1_write(
				const GPlatesScribe::ArchiveWriter::non_null_ptr_type &archive_writer,
				Data &before_data);

		void
		test_case_raw_1_read(
				const GPlatesScribe::ArchiveReader::non_null_ptr_type &archive_reader,
				Data &before_data);
	};

	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			TranscribeRawTest::Enum &e,
			bool transcribed_construct_data);
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

