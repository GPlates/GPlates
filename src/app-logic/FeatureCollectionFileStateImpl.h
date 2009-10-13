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
 
#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPL_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPL_H

#include <iterator>
#include <list>
#include <map>
#include <boost/function.hpp>
#include <boost/operators.hpp>
#include <QString>

#include "app-logic/ClassifyFeatureCollection.h"

#include "file-io/File.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


namespace GPlatesAppLogic
{
	namespace FeatureCollectionFileStateImpl
	{
		//! Typedef for the tag type used to identify a specific active state.
		typedef QString active_state_tag_type;

		//! Tag used to identify active reconstructable files.
		static const active_state_tag_type RECONSTRUCTABLE_TAG = "reconstructable-workflow-tag";

		//! Tag used to identify active reconstruction files.
		static const active_state_tag_type RECONSTRUCTION_TAG = "reconstruction-workflow-tag";


		/**
		 * The file node active state.
		 */
		class FileNodeActiveState
		{
		public:
			//! Removes all tags from the active states.
			void
			clear()
			{
				d_active_map.clear();
			}

			//! Adds @a tag to the list of active states (assertion failure if already exists).
			void
			add(
					const active_state_tag_type &tag,
					bool active)
			{
				const std::pair<active_map_type::iterator, bool> inserted = d_active_map.insert(
						active_map_type::value_type(tag, active));

				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						inserted.second,
						GPLATES_ASSERTION_SOURCE);
			}


			//! Removes @a tag from the list of active states (assertion failure if not exist).
			void
			remove(
					const active_state_tag_type &tag)
			{
				active_map_type::iterator iter = d_active_map.find(tag);
								
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						iter != d_active_map.end(),
						GPLATES_ASSERTION_SOURCE);

				d_active_map.erase(iter);
			}


			bool
			exists(
					const active_state_tag_type &tag) const
			{
				return d_active_map.find(tag) != d_active_map.end();
			}


			//! Returns true *only* if @a tag exists and its active state is true.
			bool
			get_active(
					const active_state_tag_type &tag) const
			{
				active_map_type::const_iterator iter = d_active_map.find(tag);
				
				return iter != d_active_map.end() && iter->second;
			}


			//! Set active state of @a tag (assertion failure if not exist).
			void
			set_active(
					const active_state_tag_type &tag,
					bool active = true)
			{
				active_map_type::iterator iter = d_active_map.find(tag);
				
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						iter != d_active_map.end(),
						GPLATES_ASSERTION_SOURCE);

				iter->second = active;
			}


			/**
			 * Returns all the tags currently in use (regardless of their active state).
			 */
			std::vector<active_state_tag_type>
			get_tags() const
			{
				std::vector<active_state_tag_type> tags;

				for (active_map_type::const_iterator iter = d_active_map.begin();
					iter != d_active_map.end();
					++iter)
				{
					const active_state_tag_type &tag = iter->first;
					tags.push_back(tag);
				}

				return tags;
			}


			friend
			void
			visit_union(
					const FileNodeActiveState &state1,
					const FileNodeActiveState &state2,
					boost::function<void (active_state_tag_type, bool, bool)> visit_function)
			{
				// Iterate through the first active states.
				active_map_type::const_iterator state1_iter = state1.d_active_map.begin();
				active_map_type::const_iterator state1_end = state1.d_active_map.end();
				for ( ; state1_iter != state1_end; ++state1_iter)
				{
					const active_state_tag_type &active_state_tag = state1_iter->first;

					const bool state1_active = state1_iter->second;
					const bool state2_active = state2.get_active(active_state_tag);

					visit_function(active_state_tag, state1_active, state2_active);
				}

				// Iterate through the new active states to see if there are any states not
				// included in the old active states.
				active_map_type::const_iterator state2_iter = state2.d_active_map.begin();
				active_map_type::const_iterator state2_end = state2.d_active_map.end();
				for ( ; state2_iter != state2_end; ++state2_iter)
				{
					const active_state_tag_type &active_state_tag = state2_iter->first;

					// We're only interested in when the old state does not exist because when
					// it exists we've already taken care of it in the previous loop.
					if (!state1.exists(active_state_tag))
					{
						const bool state1_active = false;
						const bool state2_active = state2.get_active(active_state_tag);

						visit_function(active_state_tag, state1_active, state2_active);
					}
				}
			}

		private:
			//! Typedef for looking up a specific active state.
			typedef std::map<active_state_tag_type, bool> active_map_type;

			active_map_type d_active_map;
		};

		/**
		 * For all states in @a state1 and @a state2 call the function @a visit_function
		 * (passing three parameters - tag, state1's active status, state2's active status).
		 * If one of the states doesn't have a particular tag then it's active status is
		 * assumed to be false.
		 */
		void
		visit_union(
				const FileNodeActiveState &state1,
				const FileNodeActiveState &state2,
				boost::function<void (active_state_tag_type, bool, bool)> visit_function);


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
		 * Keeps track of a file node and its previous active state - this is
		 * an implementation detail used by FeatureCollectionFileState::ActiveListsManager.
		 */
		struct FileNodeInfo
		{
			//! Constructor sets prev active state empty/inactive.
			FileNodeInfo(
					const FileNode &file_node_) :
				file_node(file_node_)
			{  }

			FileNode file_node;

			// NOTE: Only to be used by FeatureCollectionFileState::ActiveListsManager.
			// The current active state is stored in 'file_node'.
			FileNodeActiveState prev_active_state;
		};

		/**
		 * Typedef for a sequence of @a FileNode objects.
		 */
		typedef std::list<FileNodeInfo> file_seq_impl_type;


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
				return *iter->file_node.file();
			}

			static
			FileNode *
			get_file_node_ptr(
					const seq_iterator_type &iter)
			{
				return &iter->file_node;
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
				return *(*iter)->file_node.file();
			}

			static
			FileNode *
			get_file_node_ptr(
					const seq_iterator_type &iter)
			{
				return &(*iter)->file_node;
			}
		};

		/**
		 * Typedef for iterator over @a file_iterator_type.
		 */
		typedef Iterator<DereferenceFileNodeIteratorTraits> active_file_iterator_type;
	}
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPL_H
