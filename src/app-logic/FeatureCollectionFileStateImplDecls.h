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

#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPLDECLS_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPLDECLS_H

//
// This header is to reduce compile-time.
// It contains only those parts of "FeatureCollectionFileStateImpl.h" that
// are needed by other header files.
// The parts remaining in "FeatureCollectionFileStateImpl.h" should only be included
// in ".cc" files.
//


#include <iterator>
#include <list>
#include <map>
#include <vector>
#include <boost/operators.hpp>

#include "ClassifyFeatureCollection.h"
#include "FeatureCollectionFileStateDecls.h"

#include "file-io/File.h"


namespace GPlatesAppLogic
{
	namespace FeatureCollectionFileStateImpl
	{
		//! Typedef for the tag type used to identify a specific active state.
		typedef FeatureCollectionFileStateDecls::workflow_tag_type workflow_tag_type;


		/**
		 * The file node active state.
		 */
		class FileNodeActiveState
		{
		public:
			//! Removes all tags from the active states.
			void
			clear_tags();


			//! Adds @a tag to the list of active states (assertion failure if already exists).
			void
			add_tag(
					const workflow_tag_type &tag,
					bool active);


			//! Removes @a tag from the list of active states (assertion failure if not exist).
			void
			remove_tag(
					const workflow_tag_type &tag);


			bool
			does_tag_exist(
					const workflow_tag_type &tag) const;


			//! Returns true *only* if @a tag exists and its active state is true.
			bool
			get_active(
					const workflow_tag_type &tag) const;


			//! Set active state of @a tag (assertion failure if not exist).
			void
			set_active(
					const workflow_tag_type &tag,
					bool active = true);


			/**
			 * Returns all the tags currently in use (regardless of their active state).
			 */
			std::vector<workflow_tag_type>
			get_tags() const;

		private:
			//! Typedef for looking up a specific active state.
			typedef std::map<workflow_tag_type, bool> active_map_type;

			active_map_type d_active_map;
		};


		/**
		 * The file node state - includes active state and feature collection classification.
		 */
		class FileNodeState
		{
		public:
			/**
			 * Construct with optional feature collection classification and active state.
			 */
			FileNodeState(
					const ClassifyFeatureCollection::classifications_type &classification =
							ClassifyFeatureCollection::classifications_type(),
					const FileNodeActiveState &active_state_ = FileNodeActiveState()) :
				d_classification(classification),
				d_active_state(active_state_)
			{  }


			const ClassifyFeatureCollection::classifications_type &
			feature_collection_classification() const
			{
				return d_classification;
			}

			ClassifyFeatureCollection::classifications_type &
			feature_collection_classification()
			{
				return d_classification;
			}


			const FileNodeActiveState &
			active_state() const
			{
				return d_active_state;
			}

			FileNodeActiveState &
			active_state()
			{
				return d_active_state;
			}

		private:
			ClassifyFeatureCollection::classifications_type d_classification;
			FileNodeActiveState d_active_state;
		};


		/**
		 * Contains a file shared ref and attributes related to it.
		 */
		class FileNode
		{
		public:
			/**
			 * Constructor.
			 *
			 * Default file state has all flags set to false.
			 */
			FileNode(
					const GPlatesFileIO::File::shared_ref &file_,
					const FileNodeState &file_node_state_ = FileNodeState()) :
				d_file(file_),
				d_file_node_state(file_node_state_)
			{  }


			GPlatesFileIO::File::shared_ref &
			file()
			{
				return d_file;
			}


			const FileNodeState &
			file_node_state() const
			{
				return d_file_node_state;
			}

			FileNodeState &
			file_node_state()
			{
				return d_file_node_state;
			}

		private:
			GPlatesFileIO::File::shared_ref d_file;
			FileNodeState d_file_node_state;
		};


		/**
		 * Bidirectional iterator over files.
		 *
		 * NOTE: the main reason for having this class instead of just using an STL container iterator
		 * is we don't want the user to be able to modify the GPlatesFileIO::shared_ref's stored
		 * inside @a FeatureCollectionFileState because we don't want them to have direct control over when
		 * the feature collections get unloaded. So this iterator interface prevents direct
		 * access to 'GPlatesFileIO::File::shared_ref'.
		 */
		template <class DereferenceTraits>
		class Iterator :
				public std::iterator<
						std::bidirectional_iterator_tag,
						GPlatesFileIO::File>,
				public boost::bidirectional_iteratable<
						Iterator<DereferenceTraits>,
						GPlatesFileIO::File *>
		{
		public:
			//! Typedef for implementation sequence we're iterating over.
			typedef typename DereferenceTraits::seq_type seq_type;

			//! Typedef for implementation sequence iterator.
			typedef typename DereferenceTraits::seq_iterator_type seq_iterator_type;


			//! Create iterator.
			static
			Iterator
			create(
					const seq_iterator_type &file_iter)
			{
				return Iterator(file_iter);
			}


			/**
			 * Dereference operator.
			 * Operator->() provided by class boost::bidirectional_iteratable.
			 *
			 * NOTE: even though the returned reference is non-const and therefore you can
			 * modify the referenced GPlatesFileIO::File object you don't have access to the
			 * GPlatesFileIO::File::shared_ref stored in the application state (which controls
			 * when the file's feature collection is unloaded). So you cannot change when
			 * the file's feature collection is unloaded.
			 */
			GPlatesFileIO::File &
			operator*() const
			{
				return DereferenceTraits::get_file(d_iter);
			}


			/**
			 * Pre-increment operator.
			 * Post-increment operator provided by base class boost::bidirectional_iteratable.
			 */
			Iterator &
			operator++()
			{
				++d_iter;
				return *this;
			}


			/**
			 * Pre-decrement operator.
			 * Post-decrement operator provided by base class boost::bidirectional_iteratable.
			 */
			Iterator &
			operator--()
			{
				--d_iter;
				return *this;
			}


			/**
			 * Equality comparison.
			 * Inequality operator provided by base class boost::bidirectional_iteratable.
			 */
			friend
			bool
			operator==(
					const Iterator &lhs,
					const Iterator &rhs)
			{
				return lhs.d_iter == rhs.d_iter;
			}


			/**
			 * Less then comparison.
			 */
			friend
			bool
			operator<(
					const Iterator &lhs,
					const Iterator &rhs)
			{
				// Compare FileNode pointers.
				return lhs.get_file_node() < rhs.get_file_node();
			}


			/**
			 * Returns the @a iterator implementation.
			 *
			 * NOTE: only to be used by @a FeatureCollectionFileState.
			 */
			seq_iterator_type
			get_iterator_impl() const
			{
				return d_iter;
			}


			/**
			 * Returns the @a FileNode referenced by this iterator.
			 *
			 * NOTE: only to be used by @a FeatureCollectionFileState.
			 */
			FileNode *
			get_file_node() const
			{
				return DereferenceTraits::get_file_node_ptr(d_iter);
			}

		private:
			seq_iterator_type d_iter;


			Iterator(
					const seq_iterator_type &iter) :
				d_iter(iter)
			{  }
		};


		/**
		 * Typedef for a sequence of @a FileNode objects.
		 */
		typedef std::list<FileNode> file_seq_impl_type;
		typedef file_seq_impl_type::iterator file_seq_iterator_impl_type;


		//! Traits for iterator over a sequence of @a FileNode pointers.
		struct DereferenceFileNodePtrTraits
		{
			typedef file_seq_impl_type seq_type;

			typedef seq_type::iterator seq_iterator_type;

			static
			GPlatesFileIO::File &
			get_file(
					const seq_iterator_type &iter)
			{
				return *iter->file();
			}

			static
			FileNode *
			get_file_node_ptr(
					const seq_iterator_type &iter)
			{
				return &*iter;
			}
		};	

		/**
		 * Typedef for iterator over @a FileNode pointers.
		 */
		typedef Iterator<DereferenceFileNodePtrTraits> file_iterator_type;


		/**
		 * Typedef for a sequence of iterators into a @a FileNode sequence.
		 */
		typedef std::list<file_seq_impl_type::iterator> file_iterator_seq_impl_type;
		typedef file_iterator_seq_impl_type::iterator file_iterator_seq_iterator_impl_type;


		//! Traits for iterator over @a file_iterator_type.
		struct DereferenceFileNodeIteratorTraits
		{
			typedef file_iterator_seq_impl_type seq_type;

			typedef seq_type::iterator seq_iterator_type;

			static
			GPlatesFileIO::File &
			get_file(
					const seq_iterator_type &iter)
			{
				return *(*iter)->file();
			}

			static
			FileNode *
			get_file_node_ptr(
					const seq_iterator_type &iter)
			{
				return &**iter;
			}
		};

		/**
		 * Typedef for iterator over @a file_iterator_type.
		 */
		typedef Iterator<DereferenceFileNodeIteratorTraits> active_file_iterator_type;


		/**
		 * A [begin,end) range of bidirectional iteration over sequence of files.
		 * Iterators dereference to 'GPlatesFileIO::File &'.
		 */
		template <class IteratorType>
		struct IteratorRange
		{
			IteratorRange(
					IteratorType begin_,
					IteratorType end_) :
				begin(begin_),
				end(end_)
			{  }

			IteratorType begin; //!< The begin iterator in iterator range [begin,end)
			IteratorType end; //!< The end iterator in iterator range [begin,end)
		};


		/**
		 * Typedef for iterator over loaded files.
		 */
		typedef IteratorRange<file_iterator_type> file_iterator_range_type;

		/**
		 * Typedef for iterator over files used for other purposes
		 * such as active reconstructable files.
		 */
		typedef IteratorRange<active_file_iterator_type> active_file_iterator_range_type;
	}
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPLDECLS_H
