/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_LATLONAREASAMPLING_H
#define GPLATES_UTILS_LATLONAREASAMPLING_H

#include <cmath>
#include <cstddef>
#include <new>
#include <vector>
#include <list>
#include <iterator>  // std::iterator
#include <boost/cstdint.hpp>
#include <boost/operators.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/UnitVector3D.h"
#include "maths/types.h"

//#include "utils/Profile.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesUtils
{
	/**
	 * A roughly uniform area sampling of the sphere into segments aligned
	 * along latitude and longitude.
	 */
	template <typename ElementType>
	class LatLonAreaSampling
	{
	public:
		/**
		 * Creates a lat/lon area sampling where the angular dimension
		 * of each lat/lon area bin is roughly @a sample_bin_angle_spacing_degrees.
		 */
		LatLonAreaSampling(
				const double &sample_bin_angle_spacing_degrees);


		/**
		 * Changes the angular dimension of each lat/lon area bin to be roughly
		 * @a sample_bin_angle_spacing_degrees.
		 * Any elements that have been added prior to this are kept but
		 * arranged into the new area sampling structure built by this call.
		 */
		void
		reset_sample_spacing(
				const double &sample_bin_angle_spacing_degrees);


		/**
		 * Returns the number of sampled elements.
		 *
		 * There is only one sampled element per lat/lon area bin
		 * and that is the element closest to the bin centre.
		 */
		unsigned int
		get_num_sampled_elements() const;


		/**
		 * Returns the sampled element at index @a sampled_element_index.
		 *
		 * The index ranges from zero to @a get_num_sampled_elements - 1
		 * and is only used as a means to iterate over the sampled elements
		 * because the order of sampled elements in this sequence is undetermined.
		 */
		const ElementType &
		get_sampled_element(
				unsigned int sampled_element_index) const;


		/**
		 * Add an element at the location on sphere @a point_on_sphere_location.
		 */
		void
		add_element(
				const ElementType &element,
				const GPlatesMaths::PointOnSphere &point_on_sphere_location);


		/**
		 * Removes all elements added with @a add_element which
		 * also removes all sampled elements.
		 */
		void
		clear_elements();


	private:
		class ElementEntry;

		//! Typedef for global sequence of sample elements.
		typedef std::vector<const ElementEntry *> sample_element_seq_type;


		/**
		 * Stores @a Type objects at a fixed memory address to avoid invalidating pointers to them.
		 * Then we can use in linked lists.
		 * Also reduces memory allocation overhead since potentially tens of thousands of
		 * @a ElementEntry instances can be created.
		 * Objects are destroyed together when @a Storage instance is destroyed.
		 */
		template <typename Type>
		class Storage
		{
		public:
			Storage()
			{
				create_new_chunk();
			}

			void
			clear()
			{
				// Destroy all ElementEntry objects and releases chunk memory.
				d_chunk_seq.clear();
				create_new_chunk();
			}

			/**
			 * Copies @a object to a fixed memory address and returns pointer to it.
			 */
			Type *
			store(
					const Type &object)
			{
				Chunk &chunk = d_chunk_seq.front();
				chunk.d_seq.push_back(object);

				Type *stored_object = &chunk.d_seq.back();

				if (boost::numeric_cast<int>(chunk.d_seq.size()) == NUM_OBJECTS_PER_CHUNK)
				{
					create_new_chunk();
				}

				return stored_object;
			}

		private:
			static const int NUM_OBJECTS_PER_CHUNK = 100;

			struct Chunk
			{
				typedef std::vector<Type> seq_type;
				seq_type d_seq;
			};

			typedef std::list<Chunk> chunk_seq_type;
			chunk_seq_type d_chunk_seq;


			void
			create_new_chunk()
			{
				d_chunk_seq.push_front(Chunk());
				d_chunk_seq.front().d_seq.reserve(NUM_OBJECTS_PER_CHUNK);
			}
		};


		/**
		 * Singly-linked list class of @a NodeType objects.
		 * NOTE: template parameter NodeType must inherit publicly from List<NodeType>::Node.
		 * NOTE: When NodeType is in more than one list (and hence needs to inherit from Node
		 * more than once) use different types for NodeTag to distinguish which Node
		 * is used by which List.
		 */
		template <class NodeType, class NodeTag = void>
		class List
		{
		public:
			//! Template parameter NodeType must inherit publicly from this class.
			class Node
			{
			public:
				Node() :
					d_next_node(NULL)
				{  }

				NodeType *
				get_next_node() const
				{
					return d_next_node;
				}

			private:
				friend class List<NodeType,NodeTag>;

				NodeType *d_next_node;
			};

			//! Iterator over list.
			class Iterator :
					public std::iterator<std::forward_iterator_tag, NodeType>,
					public boost::forward_iteratable<Iterator, NodeType *>
			{
			public:
				Iterator(
						NodeType *node = NULL) :
					d_node(node)
				{  }

				//! 'operator->()' provided by base class boost::forward_iteratable.
				NodeType &
				operator*() const
				{
					return *d_node;
				}

				//! Post-increment operator provided by base class boost::forward_iteratable.
				Iterator &
				operator++()
				{
					// Use "List<NodeType,NodeTag>::Node::" to pick the correct
					// base class in case NodeType inherits from more than once
					// Node bass class (ie, is in more than one list).
					d_node = d_node->List<NodeType,NodeTag>::Node::get_next_node();
					return *this;
				}

				//! Inequality operator provided by base class boost::forward_iteratable.
				friend
				bool
				operator==(
						const Iterator &lhs,
						const Iterator &rhs)
				{
					return lhs.d_node == rhs.d_node;
				}

			private:
				NodeType *d_node;
			};

			//! Typedef for iterator.
			typedef Iterator iterator;


			List() :
				d_list(NULL)
			{  }

			void
			clear()
			{
				d_list = NULL;
			}

			void
			push_front(
					NodeType *node)
			{
				// Use "List<NodeType,NodeTag>::Node::" to pick the correct
				// base class in case NodeType inherits from more than once
				// Node bass class (ie, is in more than one list).
				node->List<NodeType,NodeTag>::Node::d_next_node = d_list;
				d_list = node;
			}

			iterator
			begin()
			{
				return Iterator(d_list);
			}

			iterator
			end()
			{
				return Iterator(NULL);
			}

		private:
			NodeType *d_list;
		};


		/**
		 * Keeps element together with its location on the sphere.
		 */
		class ElementEntry :
				public List<ElementEntry>::Node
		{
		public:
			ElementEntry(
					const ElementType &element,
					const GPlatesMaths::LatLonPoint &lat_lon_location,
					const GPlatesMaths::PointOnSphere &point_on_sphere_location) :
				d_element(element),
				d_lat_lon_location(lat_lon_location),
				d_point_on_sphere_location(point_on_sphere_location.position_vector())
			{  }

			const ElementType &
			get_element() const
			{
				return d_element;
			}

			const GPlatesMaths::LatLonPoint &
			get_lat_lon_location() const
			{
				return d_lat_lon_location;
			}

			const GPlatesMaths::PointOnSphere &
			get_point_on_sphere_location() const
			{
				return d_point_on_sphere_location;
			}

		private:
			ElementType d_element;
			GPlatesMaths::LatLonPoint d_lat_lon_location;
			GPlatesMaths::PointOnSphere d_point_on_sphere_location;
		};


		struct LongitudeLookupFullListTag;
		struct LongitudeLookupInnerListTag;

		/**
		 * Represents a single roughly equal-area sample area on surface of sphere.
		 * Can contains multiple elements but only the element closest to the sample
		 * centre is called the sample element.
		 */
		class SampleBin :
				public List<SampleBin, LongitudeLookupFullListTag>::Node,
				public List<SampleBin, LongitudeLookupInnerListTag>::Node
		{
		public:
			SampleBin(
				const GPlatesMaths::LatLonPoint &sample_centre) :
				d_central_point_on_sphere_location(
						GPlatesMaths::make_point_on_sphere(sample_centre).position_vector()),
				d_sample_element_dot_centre(0),
				d_sample_element(NULL),
				d_sample_element_seq_index(-1)
			{  }


			//! Removes all elements.
			void
			clear_elements()
			{
				d_sample_element_dot_centre = 0;
				d_sample_element = NULL;
				d_sample_element_seq_index = -1;
			}


			/**
			 * Add a new element to this sample bin and if it's the new sample element
			 * (ie, closest element to the sample centre) then replace previous sample
			 * element or push_front (for first time) into @a sample_element_seq.
			 */
			void
			add_element(
					const ElementEntry *new_element_entry,
					sample_element_seq_type &sample_element_seq)
			{
				// See if new element is closer to sample centre than the current
				// sample element.
				const GPlatesMaths::real_t new_element_dot_centre = dot(
						new_element_entry->get_point_on_sphere_location().position_vector(),
						d_central_point_on_sphere_location);

				if (d_sample_element)
				{
					// We already have a sample element - see if the new element is closer.
					if (new_element_dot_centre > d_sample_element_dot_centre)
					{
						d_sample_element = new_element_entry;
						d_sample_element_dot_centre = new_element_dot_centre;

						// Replace our previous sample element with the new one in
						// the global list of sample elements.
						sample_element_seq[d_sample_element_seq_index] = d_sample_element;
					}
				}
				else
				{
					// We don't have a sample element yet so the new element becomes one.
					d_sample_element = new_element_entry;
					d_sample_element_dot_centre = new_element_dot_centre;

					// Add to global list of sample elements.
					d_sample_element_seq_index = sample_element_seq.size();
					sample_element_seq.push_back(d_sample_element);
				}
			}

		private:
			GPlatesMaths::UnitVector3D d_central_point_on_sphere_location;
			GPlatesMaths::real_t d_sample_element_dot_centre;
			const ElementEntry *d_sample_element;

			/**
			 * Index into 'LatLonAreaSampling::d_sample_element_seq' of our sample
			 * element or -1 if we have no elements.
			 */
			int d_sample_element_seq_index;
		};


		/**
		 * Handles lookups using longitude.
		 */
		class LongitudeLookup
		{
		public:
			LongitudeLookup() :
				d_sample_bin_storage(NULL),
				d_use_high_speed_lookup(true),
				// These values are meaningless but better than random values if uninitialised.
				d_latitude_centre(0),
				d_longitude_spacing(0),
				d_inverse_longitude_spacing(0)
			{  }


			void
			initialise(
					const double &latitude_centre,
					const double &longitude_spacing,
					Storage<SampleBin> *sample_bin_storage)
			{
				d_sample_bin_storage = sample_bin_storage;
				d_latitude_centre = latitude_centre;
				d_longitude_spacing = longitude_spacing;
				d_inverse_longitude_spacing = 1.0 / longitude_spacing;

				// Using this epsilon means we don't have to worry about 180 degrees
				// being very close to an exact multiple of the angle spacing and
				// the associated overflow problem converting longitude to an index later.
				static const double EPSILON = 1e-3;

				const unsigned int num_sample_bins = static_cast<unsigned int>(
						360.0 / longitude_spacing + EPSILON) + 1;

				// If the number of potential sample bins exceeds a threshold then
				// use a slower but lower memory lookup method.
				d_use_high_speed_lookup = (num_sample_bins < MAX_SAMPLE_BINS_FOR_HIGH_SPEED_LOOKUP);

				if (d_use_high_speed_lookup)
				{
					// Use high-speed lookup which just involves indexing into
					// an array to get the SampleBin pointer.
					//
					// Initialise our pointer lookup array - sample bins are
					// created as needed during sample bin lookup.
					d_sample_bin_high_speed_lookup.resize(num_sample_bins, NULL);
				}
				else
				{
					// Use the low memory lookup which uses four times less memory for
					// the SampleBin lookup but at a cost of reduced speed.
					//
					// To save memory we have an outer bin that contains up to 8 sample bins
					// which are stored in a singly linked list.
					// The sample bins are populated as needed and the list list.
					// The "+1" is in case 'num_sample_bins' is not a multiple of 8.
					const unsigned int num_outer_bins = (num_sample_bins >> 3) + 1;
					// Contains the list of up to 8 inner bins.
					d_sample_low_memory_lookup.resize(num_outer_bins);
				}
			}


			//! Removes all elements.
			void
			clear_elements()
			{
				typename sample_bin_full_list_type::iterator sample_bin_iter;
				const typename sample_bin_full_list_type::iterator sample_bin_begin =
						d_sample_bin_full_list.begin();
				const typename sample_bin_full_list_type::iterator sample_bin_end =
						d_sample_bin_full_list.end();
				for (sample_bin_iter = sample_bin_begin;
					sample_bin_iter != sample_bin_end;
					++sample_bin_iter)
				{
					sample_bin_iter->clear_elements();
				}
			}


			/**
			 * Retrieve the @a SampleBin at the specified longitude.
			 * @param longitude_full_range is longitude in the range [-360,360].
			 */
			SampleBin &
			operator[](
					const double &longitude_full_range)
			{
				// We know that GPlatesMaths::LatLonPoint asserts its longitude to
				// be in the range [-360,360] so simply narrow the range to [0,360]
				// for our indexing purposes.
				double longitude = longitude_full_range;
				if (longitude < 0)
				{
					longitude += 360;
				}

				const unsigned int sample_bin_index = static_cast<unsigned int>(
						longitude * d_inverse_longitude_spacing);

				return get_sample_bin(sample_bin_index);
			}

		private:
			//! Used by the low memory @a SampleBin lookup to keep track of up to 8 SampleBins.
			class OuterBin
			{
			public:
				OuterBin() :
						d_inner_list_info(INITIAL_INNER_LIST_INFO_VALUE)
				{  }

				SampleBin *
				get_sample_bin(
						const unsigned int inner_index,
						unsigned int &inner_list_length_ref)
				{
					// Get occupancy - 8 bits representing which 8 SampleBins are created.
					const unsigned int occupancy =
							((d_inner_list_info & OCCUPANCY_MASK) >> OCCUPANCY_BIT_OFFSET);

					// Could use a 256 byte lookup table but this arithmetic accesses
					// memory that's in the cpu cache already.
					const unsigned int list_length = inner_list_length_ref =
							(occupancy & 1) + ((occupancy >> 1) & 1) +
							((occupancy >> 2) & 1) + ((occupancy >> 3) & 1) +
							((occupancy >> 4) & 1) + ((occupancy >> 5) & 1) +
							((occupancy >> 6) & 1) + ((occupancy >> 7) & 1);

					if ((occupancy & (1 << inner_index)) == 0)
					{
						// Let caller know that they need to create a SampleBin.
						return NULL;
					}


					const unsigned int list_index = (
							(d_inner_list_info >> (NUM_BITS_PER_LIST_INDEX * inner_index))
									& LIST_INDEX_MASK);

					// The zero index is at the back of the list and highest at the front.
					// Iterate through inner list until we get to the sample bin we want.
					typename inner_list_type::iterator list_iter = d_inner_list.begin();
					for (int index = list_length - list_index; --index > 0; )
					{
						++list_iter;
					}

					return &*list_iter;
				}

				void
				set_sample_bin(
						SampleBin *sample_bin,
						const unsigned int inner_index,
						const unsigned int inner_list_length)
				{
					// Set the occupancy bit to true.
					d_inner_list_info |= ((1 << inner_index) << OCCUPANCY_BIT_OFFSET);
					// No need to clear the 3 bits used for storing list index because
					// should already be zero due to constructor.
					d_inner_list_info |= (inner_list_length << (NUM_BITS_PER_LIST_INDEX * inner_index));
					// Add sample bin to inner list.
					d_inner_list.push_front(sample_bin);
				}

			private:
				typedef boost::uint32_t inner_list_info_type;
				typedef List<SampleBin, LongitudeLookupInnerListTag> inner_list_type;

				// sizeof(OuterBin) has been optimised to 8 bytes on 32-bit systems.
				inner_list_info_type d_inner_list_info;
				inner_list_type d_inner_list;

				static const int INITIAL_INNER_LIST_INFO_VALUE = 0;
				static const int OCCUPANCY_MASK = 0xff000000;
				static const int OCCUPANCY_BIT_OFFSET = 24;
				static const int LIST_INDEX_MASK = 0x7;
				static const int NUM_BITS_PER_LIST_INDEX = 3;

			};

			typedef std::vector<OuterBin> sample_bin_low_memory_lookup_type;
			typedef std::vector<SampleBin *> sample_bin_high_speed_lookup_type;
			typedef List<SampleBin, LongitudeLookupFullListTag> sample_bin_full_list_type;

			static const unsigned int MAX_SAMPLE_BINS_FOR_HIGH_SPEED_LOOKUP = 500;

			Storage<SampleBin> *d_sample_bin_storage;

			bool d_use_high_speed_lookup;
			sample_bin_high_speed_lookup_type d_sample_bin_high_speed_lookup;
			sample_bin_low_memory_lookup_type d_sample_low_memory_lookup;

			sample_bin_full_list_type d_sample_bin_full_list;

			double d_latitude_centre;
			double d_longitude_spacing;
			double d_inverse_longitude_spacing;


			//! Returns @a SampleBin at specified index or creates one if does not exist.
			SampleBin &
			get_sample_bin(
					unsigned int sample_bin_index)
			{
				if (d_use_high_speed_lookup)
				{
					SampleBin *sample_bin = d_sample_bin_high_speed_lookup[sample_bin_index];
					if (sample_bin == NULL)
					{
						sample_bin = create_sample_bin(sample_bin_index);
						d_sample_bin_high_speed_lookup[sample_bin_index] = sample_bin;
					}

					return *sample_bin;
				}

				//
				// use low memory lookup
				//

				const unsigned int sample_bin_outer_index = (sample_bin_index >> 3);
				const unsigned int sample_bin_inner_index = (sample_bin_index & 0x7);

				OuterBin &outer_bin = d_sample_low_memory_lookup[sample_bin_outer_index];

				unsigned int inner_list_length;
				SampleBin *sample_bin = outer_bin.get_sample_bin(
						sample_bin_inner_index, inner_list_length);
				if (sample_bin == NULL)
				{
					sample_bin = create_sample_bin(sample_bin_index);
					outer_bin.set_sample_bin(
							sample_bin, sample_bin_inner_index, inner_list_length);
				}

				return *sample_bin;
			}


			//! Creates a new @a SampleBin at the specified index.
			SampleBin *
			create_sample_bin(
					unsigned int sample_bin_index)
			{
				//
				// This method of delayed creation of sample bins is done to reduce
				// memory usage when the number of sample bins across the sphere is
				// much larger than the total number of elements added which leads
				// to many sample bins not getting used.
				//

				double longitude_centre = (sample_bin_index + 0.5) * d_longitude_spacing;

				// Because we've potentially allocated an extra bin due to 'EPSILON'
				// we need to check if our centre longitude is greater than 360 degrees.
				// This extra bin helps prevent indexing out-of-range in an array.
				if (longitude_centre > 360 - 1e-3)
				{
					longitude_centre = 360 - 1e-3;
				}

				// Store sample bin so we have a fixed address that we can point to.
				SampleBin *const new_sample_bin = d_sample_bin_storage->store(
						SampleBin(GPlatesMaths::LatLonPoint(d_latitude_centre, longitude_centre)));

				// Add to the list of all SampleBins we own.
				d_sample_bin_full_list.push_front(new_sample_bin);

				return new_sample_bin;
			}
		};


		/**
		 * Handles lookups using latitude.
		 */
		class LatitudeLookup
		{
		public:
			LatitudeLookup(
					const double &latitude_spacing)
			{
				reset_spacing(latitude_spacing);
			}


			void
			clear_elements_and_reset_sample_spacing(
					const double &latitude_spacing)
			{
				d_northern_longitude_lookups.clear();
				d_southern_longitude_lookups.clear();

				// Release memory used by all SampleBin objects.
				d_sample_bin_storage.clear();

				reset_spacing(latitude_spacing);
			}


			//! Removes all elements but keeps the current latitude spacing setup.
			void
			clear_elements()
			{
				typename longitude_lookup_sequence_type::iterator longitude_lookup_iter;
				for (longitude_lookup_iter = d_northern_longitude_lookups.begin();
					longitude_lookup_iter != d_northern_longitude_lookups.end();
					++longitude_lookup_iter)
				{
					longitude_lookup_iter->clear_elements();
				}

				for (longitude_lookup_iter = d_southern_longitude_lookups.begin();
					longitude_lookup_iter != d_southern_longitude_lookups.end();
					++longitude_lookup_iter)
				{
					longitude_lookup_iter->clear_elements();
				}
			}


			/**
			 * Retrieve the @a LongitudeLookup at the specified latitude.
			 */
			LongitudeLookup &
			operator[](
					const double &latitude)
			{
				longitude_lookup_sequence_type *longitude_lookup_seq;
				double latitude_abs;
				if (latitude >= 0)
				{
					longitude_lookup_seq = &d_northern_longitude_lookups;
					latitude_abs = latitude;
				}
				else
				{
					longitude_lookup_seq = &d_southern_longitude_lookups;
					latitude_abs = -latitude;
				}

				const unsigned int longitude_lookup_index = static_cast<unsigned int>(
						latitude_abs * d_inverse_latitude_spacing);

#if 0
				GPlatesGlobal::Assert(
						longitude_lookup_index <
								boost::numeric_cast<unsigned int>(longitude_lookup_seq->size()),
						GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));
#endif

				return (*longitude_lookup_seq)[longitude_lookup_index];
			}

		private:
			//! Typedef for sequence of @a LongitudeLookup objects (each at constant latitude).
			typedef std::vector<LongitudeLookup> longitude_lookup_sequence_type;

			Storage<SampleBin> d_sample_bin_storage;

			double d_inverse_latitude_spacing;
			longitude_lookup_sequence_type d_northern_longitude_lookups;
			longitude_lookup_sequence_type d_southern_longitude_lookups;


			void
			reset_spacing(
					const double &latitude_spacing)
			{
				//PROFILE_FUNC();

				d_inverse_latitude_spacing = 1.0 / latitude_spacing;

				// Using this epsilon means we don't have to worry about 90 degrees
				// being very close to an exact multiple of the angle spacing and
				// the associated overflow problem converting latitude to an index later.
				static const double EPSILON = 1e-3;

				const unsigned int num_latitude_spacings_per_hemisphere = static_cast<unsigned int>(
						90.0 / latitude_spacing + EPSILON) + 1;

				d_northern_longitude_lookups.reserve(num_latitude_spacings_per_hemisphere);
				d_southern_longitude_lookups.reserve(num_latitude_spacings_per_hemisphere);

				const double latitude_spacing_radians =
						latitude_spacing * (GPlatesMaths::PI / 180.0);

				for (unsigned int latitude_index = 0;
					latitude_index < num_latitude_spacings_per_hemisphere;
					++latitude_index)
				{
					// Determine the number of longitude bins for the current latitude.
					// The longitude bins occur along the small circle of constant latitude.
					// We want the surface distance of a longitude bin along that circle to
					// be the same as the surface distance along latitude bins.
					// This surface distance is the angular extent of a latitude bin in radians.
					// So the number of longitude bins is the circumference of the small
					// circle of constant latitude divided by this surface distance which is:
					//    num_sample_bins = 2 * pi * cos(latitude) / latitude_spacing_radians
					// The longitude bin spacing in radians is:
					//    2 * pi / num_sample_bins
					// which is:
					//    latitude_spacing_radians / cos(latitude)
					const GPlatesMaths::real_t latitude_radians =
							latitude_index * latitude_spacing_radians;
					// Because we've potentially allocated an extra bin due to 'EPSILON'
					// we need to check if our latitude is greater than 90 degrees.
					const double longitude_spacing_radians =
							(latitude_radians < GPlatesMaths::HALF_PI - EPSILON)
							? latitude_spacing_radians /
									std::cos(latitude_index * latitude_spacing_radians)
							: 2 * GPlatesMaths::PI /* arbitrary value gives us one or two longitude bins */;
					const double longitude_spacing =
							longitude_spacing_radians * (180.0 / GPlatesMaths::PI);

					double latitude_centre = (latitude_index + 0.5) * latitude_spacing;

					// Because we've potentially allocated an extra bin due to 'EPSILON'
					// we need to check if our centre latitude is greater than 90 degrees.
					// This is extra bin helps indexing out-of-range in an array.
					if (latitude_centre > 90 - EPSILON)
					{
						latitude_centre = 90 - EPSILON;
					}

					d_northern_longitude_lookups.push_back(LongitudeLookup());
					d_northern_longitude_lookups.back().initialise(
							latitude_centre, longitude_spacing, &d_sample_bin_storage);

					d_southern_longitude_lookups.push_back(LongitudeLookup());
					d_southern_longitude_lookups.back().initialise(
							-latitude_centre, longitude_spacing, &d_sample_bin_storage);
				}
			}
		};


		Storage<ElementEntry> d_element_entry_storage;

		LatitudeLookup d_latitude_lookup;
		sample_element_seq_type d_sample_element_seq;
		List<ElementEntry> d_element_list;
	};


	template <typename ElementType>
	LatLonAreaSampling<ElementType>::LatLonAreaSampling(
			const double &sample_bin_angle_spacing_degrees) :
		d_latitude_lookup(sample_bin_angle_spacing_degrees)
	{
	}


	template <typename ElementType>
	void
	LatLonAreaSampling<ElementType>::reset_sample_spacing(
			const double &sample_bin_angle_spacing_degrees)
	{
		//PROFILE_FUNC();

		// Clear all the elements and setup a new area sampling.
		d_latitude_lookup.clear_elements_and_reset_sample_spacing(
				sample_bin_angle_spacing_degrees);

		// Clear the sequence of pointers to sample elements.
		// This'll get refilled when we re-add all the element entries below.
		d_sample_element_seq.clear();

		//PROFILE_BLOCK("re-add elements");

		// Iterate through all our current elements.
		typename List<ElementEntry>::iterator element_entry_iter;
		const typename List<ElementEntry>::iterator element_entry_begin = d_element_list.begin();
		const typename List<ElementEntry>::iterator element_entry_end = d_element_list.end();
		for (element_entry_iter = element_entry_begin;
			element_entry_iter != element_entry_end;
			++element_entry_iter)
		{
			ElementEntry &element_entry = *element_entry_iter;

			const GPlatesMaths::LatLonPoint &lat_lon_location =
					element_entry.get_lat_lon_location();

			// Lookup the sample bin using lat/lon coordinates.
			SampleBin &sample_bin = d_latitude_lookup
					[lat_lon_location.latitude()]
					[lat_lon_location.longitude()];

			// Place element entry to the sample bin.
			sample_bin.add_element(&element_entry, d_sample_element_seq);
		}
	}


	template <typename ElementType>
	unsigned int
	LatLonAreaSampling<ElementType>::get_num_sampled_elements() const
	{
		return d_sample_element_seq.size();
	}


	template <typename ElementType>
	const ElementType &
	LatLonAreaSampling<ElementType>::get_sampled_element(
			unsigned int sample_element_index) const
	{
#if 0
 		GPlatesGlobal::Assert(sample_element_index < d_sample_element_seq.size(),
 				GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));
#endif

		return d_sample_element_seq[sample_element_index]->get_element();
	}


	template <typename ElementType>
	void
	LatLonAreaSampling<ElementType>::add_element(
			const ElementType &element,
			const GPlatesMaths::PointOnSphere &point_on_sphere_location)
	{
		// Convert point on sphere to lat/lon coordinates.
		const GPlatesMaths::LatLonPoint lat_lon_location =
				GPlatesMaths::make_lat_lon_point(point_on_sphere_location);

		// Lookup the sample bin using lat/lon coordinates.
		SampleBin &sample_bin = d_latitude_lookup
				[lat_lon_location.latitude()]
				[lat_lon_location.longitude()];

		// Create a new entry - memory is owned by 'd_element_entry_storage'.
		ElementEntry *element_entry = d_element_entry_storage.store(
				ElementEntry(element, lat_lon_location, point_on_sphere_location));

		// Add new entry to the sample bin and replace/append the sequence of
		// sample elements (ie, each sample bin has one sample element that is the closest
		// to the sample centre).
		sample_bin.add_element(element_entry, d_sample_element_seq);

		// Add to our list of all elements added.
		d_element_list.push_front(element_entry);
	}


	template <typename ElementType>
	void
	LatLonAreaSampling<ElementType>::clear_elements()
	{
		//PROFILE_FUNC();

		d_latitude_lookup.clear_elements();
		d_sample_element_seq.clear();
		d_element_list.clear();
		d_element_entry_storage.clear();
	}
}

#endif // GPLATES_UTILS_LATLONAREASAMPLING_H
