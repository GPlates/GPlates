/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_ENDIAN_H
#define GPLATES_UTILS_ENDIAN_H

#include <boost/cstdint.hpp>

#include <QSysInfo>


namespace GPlatesUtils
{
	/**
	 * Machine endian conversion functions.
	 *
	 * Qt also has these but only supports integer types and it was also fairly slow when CPU profiling
	 * the conversion of large raster arrays.
	 * So these functions are designed for fast conversion of arrays of basic types or arrays of
	 * structs/classes containing basic types (by specialising the 'swap' template function for the struct/class).
	 * Otherwise the Qt functions might be more useful (cover more non-basic types).
	 */
	namespace Endian
	{
		/**
		 * Converts @a object from @a endian to the endian of the runtime system (or vice versa).
		 *
		 * The object type must have a specialisation of template function @a swap.
		 */
		template <typename T>
		void
		convert(
				T &object,
				QSysInfo::Endian endian);


		/**
		 * Converts a sequence of objects from @a endian to the endian of the runtime system (or vice versa).
		 *
		 * The object type must have a specialisation of template function @a swap.
		 */
		template <typename ForwardIteratorType>
		void
		convert(
				ForwardIteratorType begin,
				ForwardIteratorType end,
				QSysInfo::Endian endian);


		/**
		 * Swaps bytes in each data element in an array to effectively switch/toggle endian-ness.
		 */
		template <typename ForwardIteratorType>
		void
		swap(
				ForwardIteratorType begin,
				ForwardIteratorType end);


		/**
		 * Swaps bytes in the data element to effectively switch/toggle endian-ness.
		 *
		 * In order for this function to work with a specific type it must be specialised for that type.
		 * The specialisation should be placed in the file associated with the type and not here.
		 * For example...
		 *
		 * namespace GPlatesUtils
		 * {
		 * 		namespace Endian
		 * 		{
		 * 			template<>
		 * 			inline
		 * 			void
		 * 			swap<GPlatesFileIO::MyType>(
		 * 					GPlatesFileIO::MyType &object)
		 * 			{
		 * 				swap(object.data_member_1);
		 * 				swap(object.data_member_2);
		 * 			}
		 * 		}
		 * 	}
		 *
		 * ...can be placed at the bottom of "file-io/MyType.h".
		 *
		 * This unspecialised function is intentionally not defined anywhere.
		 */
		template <typename T>
		void
		swap(
				T &object);

		//
		// Specialisations for the basic types.
		//

		template<>
		void
		swap<char>(
				char &object);

		template<>
		void
		swap<unsigned char>(
				unsigned char &object);

		template<>
		void
		swap<short>(
				short &object);

		template<>
		void
		swap<unsigned short>(
				unsigned short &object);

		template<>
		void
		swap<int>(
				int &object);

		template<>
		void
		swap<unsigned int>(
				unsigned int &object);

		template<>
		void
		swap<long>(
				long &object);

		template<>
		void
		swap<unsigned long>(
				unsigned long &object);

		template<>
		void
		swap<float>(
				float &object);

		template<>
		void
		swap<double>(
				double &object);
	}

	//
	// Implementation
	//

	namespace Endian
	{
		namespace Implementation
		{
			//! This unspecialised function is intentionally not defined anywhere.
			template <int size>
			void
			swap(
					void *data);

			template<>
			inline
			void
			swap<1>(
					void *data)
			{
				// Do nothing.
			}

			template<>
			inline
			void
			swap<2>(
					void *data)
			{
				boost::uint16_t &element = *static_cast<boost::uint16_t *>(data);
				element = ((element & 0xff00) >> 8) | ((element & 0x00ff) << 8);
			}

			template<>
			inline
			void
			swap<4>(
					void *data)
			{
				boost::uint32_t &element = *static_cast<boost::uint32_t *>(data);
				element =
							((element & 0xff000000) >> 24) |
							((element & 0x00ff0000) >> 8) |
							((element & 0x0000ff00) << 8) |
							((element & 0x000000ff) << 24);
			}

			template<>
			inline
			void
			swap<8>(
					void *data)
			{
#ifndef BOOST_NO_INT64_T
				boost::uint64_t &element = *static_cast<boost::uint64_t *>(data);
				element =
							((element & UINT64_C(0xff00000000000000)) >> 56) |
							((element & UINT64_C(0x00ff000000000000)) >> 40) |
							((element & UINT64_C(0x0000ff0000000000)) >> 24) |
							((element & UINT64_C(0x000000ff00000000)) >> 8) |
							((element & UINT64_C(0x00000000ff000000)) << 8) |
							((element & UINT64_C(0x0000000000ff0000)) << 24) |
							((element & UINT64_C(0x000000000000ff00)) << 40) |
							((element & UINT64_C(0x00000000000000ff)) << 56);
#else
				// Simulate 64-bit with 32-bit since 64-bit arithmetic is not available.
				// The high and low 32-bit words of the 64-bit word.
				boost::uint8_t *element_bytes = static_cast<boost::uint8_t *>(data);
				const boost::uint32_t element_hi = *static_cast<const boost::uint32_t *>(static_cast<void *>(element_bytes));
				const boost::uint32_t element_lo = *static_cast<const boost::uint32_t *>(static_cast<void *>(element_bytes + 4));
				*static_cast<boost::uint32_t *>(static_cast<void *>(element_bytes)) =
						((element_lo & 0xff000000) >> 24) |
						((element_lo & 0x00ff0000) >> 8) |
						((element_lo & 0x0000ff00) << 8) |
						((element_lo & 0x000000ff) << 24);
				*static_cast<boost::uint32_t *>(static_cast<void *>(element_bytes + 4)) =
						((element_hi & 0xff000000) >> 24) |
						((element_hi & 0x00ff0000) >> 8) |
						((element_hi & 0x0000ff00) << 8) |
						((element_hi & 0x000000ff) << 24);
#endif
			}
		}

		template<>
		inline
		void
		swap<char>(
				char &object)
		{
			Implementation::swap<sizeof(char)>(&object);
		}

		template<>
		inline
		void
		swap<unsigned char>(
				unsigned char &object)
		{
			Implementation::swap<sizeof(unsigned char)>(&object);
		}

		template<>
		inline
		void
		swap<short>(
				short &object)
		{
			Implementation::swap<sizeof(short)>(&object);
		}

		template<>
		inline
		void
		swap<unsigned short>(
				unsigned short &object)
		{
			Implementation::swap<sizeof(unsigned short)>(&object);
		}

		template<>
		inline
		void
		swap<int>(
				int &object)
		{
			Implementation::swap<sizeof(int)>(&object);
		}

		template<>
		inline
		void
		swap<unsigned int>(
				unsigned int &object)
		{
			Implementation::swap<sizeof(unsigned int)>(&object);
		}

		template<>
		inline
		void
		swap<long>(
				long &object)
		{
			Implementation::swap<sizeof(long)>(&object);
		}

		template<>
		inline
		void
		swap<unsigned long>(
				unsigned long &object)
		{
			Implementation::swap<sizeof(unsigned long)>(&object);
		}

		template<>
		inline
		void
		swap<float>(
				float &object)
		{
			Implementation::swap<sizeof(float)>(&object);
		}

		template<>
		inline
		void
		swap<double>(
				double &object)
		{
			Implementation::swap<sizeof(double)>(&object);
		}

		template <typename ForwardIteratorType>
		void
		swap(
				ForwardIteratorType begin,
				ForwardIteratorType end)
		{
			for (ForwardIteratorType iter = begin; iter != end; ++iter)
			{
				swap(*iter);
			}
		}

		template <typename T>
		void
		convert(
				T &object,
				QSysInfo::Endian endian)
		{
			if (QSysInfo::ByteOrder != endian)
			{
				swap(object);
			}
		}

		template <typename ForwardIteratorType>
		void
		convert(
				ForwardIteratorType begin,
				ForwardIteratorType end,
				QSysInfo::Endian endian)
		{
			if (QSysInfo::ByteOrder != endian)
			{
				swap(begin, end);
			}
		}
	}
}

#endif // GPLATES_UTILS_ENDIAN_H
