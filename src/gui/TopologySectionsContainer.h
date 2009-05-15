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

#ifndef GPLATES_GUI_TOPOLOGYSECTIONSCONTAINER_H
#define GPLATES_GUI_TOPOLOGYSECTIONSCONTAINER_H

// Some things won't compile on my branch until it gets merged to platepolygon!
// kill this #define after the merge.
// kill all the #ifndef NEEDS_PLATEPOLYGON_BRANCH checks after the merge -works-.

// #define NEEDS_PLATEPOLYGON_BRANCH


#include <QDebug>
#include <QObject>
#include <iterator>
#include <vector>

#include "maths/LatLonPointConversions.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"
#include "maths/GeometryOnSphere.h"

#include "gui/FeatureFocus.h"

#ifndef NEEDS_PLATEPOLYGON_BRANCH
#include "property-values/GpmlTopologicalSection.h"
#endif

namespace GPlatesGui
{
	/**
	 * Class to manage the back-end data containing topology sections (and useful
	 * metadata) while a topology is being built up by the topology tools.
	 * TopologySectionsContainer is a convenient way to manipulate the Topology
	 * Sections table without caring about all the GUI stuff.
	 *
	 * This container includes an "Insertion Point". It indicates where new
	 * topology sections will be added when @a insert() is called.
	 * 
	 * It is also modified by the @a TopologySectionsTable class when the user
	 * clicks on the "Action Buttons" to the side of each table row, which can
	 * delete rows and modify the Insertion Point's location.
	 */
	class TopologySectionsContainer :
			public QObject
	{
		Q_OBJECT
	public:

		/**
		 * A struct holding the internal data that will be tracked per
		 * row of data in the table (not counting the 'insertion point'
		 * special row).
		 *
		 * Note: If there is any other data you want to pack here, it
		 * should be easy to do. The only members which are accessed by
		 * the table are d_feature_id, d_feature_ref, and d_reverse.
		 *
		 * However, if you change things you will have to update (or
		 * discard) the @a insert_test_data() method defined below.
		 */
		struct TableRow
		{
#ifndef NEEDS_PLATEPOLYGON_BRANCH
			/**
			 * The pointer to the gpml:TopologicalSection property-value that
			 * this table row represents.
			 */
			boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> d_section_ptr;
#endif
			/**
			 * The gpml:FeatureId of the topological section.
			 * Remember, we may be in a position where this does not resolve to a
			 * loaded feature!
			 */
			GPlatesModel::FeatureId d_feature_id;

			/**
			 * A resolved (or not) feature reference.
			 * When we construct a TableRow, we should also attempt to resolve the FeatureId
			 * to a FeatureHandle::non_null_ptr_type, to make it easier to fill in the table
			 * data.
			 *
			 * Mental Note: I have put this here in the struct rather than passed to the
			 * render_xxx methods to allow better decoupling of data and Qt-specific stuff
			 * later. -JC
			 */
			GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry;
			
			/**
			 * The point the user clicked on to select the section.
			 * Used as a reference point to aid the intersection algorithm.
			 * Coords are present-day.
			 */
			boost::optional<GPlatesMaths::LatLonPoint> d_click_point;

			/**
			 * Whether this topology section should be used in reverse.
			 */
			bool d_reverse;
		};
		
		typedef std::vector<TableRow> container_type;
		typedef container_type::size_type size_type;
		typedef container_type::iterator iterator;
		typedef container_type::const_iterator const_iterator;
		

		explicit
		TopologySectionsContainer(
			FeatureFocus &feature_focus);

		virtual
		~TopologySectionsContainer()
		{  }


		/**
		 * Returns the number of topology sections in the container.
		 */
		size_type
		size() const;
		
		/**
		 * Const 'begin' iterator of the underlying vector.
		 * No mutable iterator is supplied, because this class must emit signals
		 * to interested parties when data is modified and maintain a special
		 * insertion point.
		 * 
		 * To insert data, use @a move_insertion_point() or @a reset_insertion_point(),
		 * followed by a call to one of the @a insert() methods.
		 */
		const_iterator
		begin() const
		{
			return d_container.begin();
		}

		/**
		 * Const 'end' iterator of the underlying vector.
		 * No mutable iterator is supplied, because this class must emit signals
		 * to interested parties when data is modified and maintain a special
		 * insertion point.
		 * 
		 * To insert data, use @a move_insertion_point() or @a reset_insertion_point(),
		 * followed by a call to one of the @a insert() methods.
		 */
		const_iterator
		end() const
		{
			return d_container.end();
		}
		
		/**
		 * Accesses an entry of the table by index.
		 *
		 * Note there is no non-const @a at(); use @a update_at()
		 * instead.
		 */
		const TableRow &
		at(
				const size_type index) const;

		/**
		 * Inserts a new entry into the container. Note that there is
		 * no 'append' or 'push_back'; new elements are always inserted
		 * wherever the Insertion Point is. This is usually at the end
		 * of the table, but the user is free to move the Insertion
		 * Point themselves prior to adding a new topological section.
		 *
		 * Once an entry has been inserted, the insertion point itself
		 * is moved down one row (naturally), so multiple calls to
		 * insert() will add things in the correct order.
		 *
		 * The supplied TableRow struct is copied.
		 *
		 * The @a entries_inserted() signal is emitted.
		 */
		void
		insert(
				const TableRow &entry);


		/**
		 * Inserts a bunch of new entries into the container. Note that
		 * there is no 'append' or 'push_back'; new elements are always
		 * inserted wherever the Insertion Point is. This is usually at
		 * the end of the table, but the user is free to move the 
		 * Insertion Point themselves prior to adding a new topological
		 * section.
		 *
		 * Once the entries have been inserted, the insertion point itself
		 * is moved down that number of rows.
		 *
		 * This version of @a insert() takes two STL-style iterators,
		 * allowing you to copy a whole range of structs into the container
		 * in one go.
		 * The supplied iterators should be iterators of any STL container
		 * type (or any custom class that can make similar iterators), and
		 * should be dereferencable to a TopologySectionsContainer::TableRow.
		 *
		 * The @a entries_inserted() signal is emitted.
		 */
		template<typename I>
		void
		insert(
				I begin_it,
				I end_it)
		{
			// All new entries get inserted at the insertion point.
			size_type index = insertion_point();
			// Use STL vector::insert with two input iterators for probably-fast
			// insert (depending on iterator capability)
			d_container.insert(d_container.begin() + index, begin_it, end_it);
			// We need to know how many just got added.
			size_type quantity = std::distance(begin_it, end_it);
			// ... because inserting will naturally bump the insertion point down n rows.
			move_insertion_point(insertion_point() + quantity);
			// ... and we need to emit appropriate signals.
			emit entries_inserted(index, quantity);
		}
		
		
		/**
		 * Updates an existing TableRow in the collection.
		 * 
		 * Calling this replaces the TableRow at index with new data. We
		 * use this method rather than a non-const @a at() so that this
		 * collection can emit signals informing other classes that a row
		 * of the table has been modified.
		 *
		 * The supplied TableRow struct is copied.
		 *
		 * The @a entries_modified() signal is emitted.
		 */
		void
		update_at(
				const size_type index,
				const TableRow &entry);


		/**
		 * Removes an existing TableRow in the collection.
		 * 
		 * Calling this completely removes the TableRow at the given index.
		 * Naturally, this will affect all subsequent indexes, and potentially
		 * moves the Insertion Point to a new location.
		 *
		 * The @a entry_removed() signal is emitted.
		 */
		void
		remove_at(
				const size_type index);


		/**
		 * Returns the current index associated with the Insertion Point.
		 */
		size_type
		insertion_point() const;

		/**
		 * Moves the Insertion Point to a new row of the table. The supplied
		 * index indicates the position a new entry would occupy if it were
		 * inserted into the container. Valid range is 0 to @a size() inclusive.
		 *
		 * The @a insertion_point_moved() signal is emitted.
		 */
		void
		move_insertion_point(
				size_type new_index);

		/** 
		 * Focus the feature at row 
		 */
		void
		focus_at(
				size_type index);

	public slots:

		/**
		 * Moves the Insertion Point to the end of the table.
		 *
		 * The @a insertion_point_moved() signal is emitted.
		 */
		void
		reset_insertion_point();


		/**
		 * Clears the container of data and resets the insertion point.
		 *
		 * The @a cleared() signal is emitted.
		 */
		void
		clear();


#if 0	// The following slots were only used to support easier testing before the platepolygon branch merge.
		// they should go away once everything works fine.
		// See also TopologySectionsTable.cc anon namespace functions.
		// TESTING: insert some fake data.
		void
		insert_test_data()
		{
			GPlatesModel::FeatureHandle::weak_ref null_ref;
			TopologySectionsContainer::TableRow trow1 = { GPlatesModel::FeatureId("GPlates-af5b7ca8-0a8a-4ad0-8741-0b81661f6634"), null_ref, GPlatesMaths::LatLonPoint(51.0,52.0), false };
			TopologySectionsContainer::TableRow trow2 = { GPlatesModel::FeatureId("GPlates-80dee8d2-7ce8-4193-b34f-d54975989e78"), null_ref, GPlatesMaths::LatLonPoint(51.0,52.0), true };
			TopologySectionsContainer::TableRow trow3 = { GPlatesModel::FeatureId("GPlates-9a93bb0d-ffa3-4672-880b-842c3cca671a"), null_ref, GPlatesMaths::LatLonPoint(51.0,52.0), false };
			TopologySectionsContainer::TableRow trow4 = { GPlatesModel::FeatureId("GPlates-e50dc893-b535-491b-996c-edaaa45ad108"), null_ref, GPlatesMaths::LatLonPoint(51.0,52.0), true };
			TopologySectionsContainer::TableRow trow5 = { GPlatesModel::FeatureId("GPlates-4d53a360-65cc-45c2-addd-34d057798cde"), null_ref, GPlatesMaths::LatLonPoint(51.0,52.0), true };
			insert(trow1);
			insert(trow2);
			insert(trow3);
			insert(trow4);
			insert(trow5);
		}

		// TESTING: manipulate the Insertion Point.
		void
		move_insertion_point_idx_4()
		{
			move_insertion_point(4);
		}

		// TESTING: remove some data.
		void
		remove_idx_2()
		{
			remove_at(2);
		}
#endif	// testing slots.

	signals:
		
		/**
		 * Emitted when @a clear() is called and all data has been removed.
		 */
		void
		cleared();

		/**
		 * Emitted whenever the insertion point changes location.
		 */
		void
		insertion_point_moved(
				GPlatesGui::TopologySectionsContainer::size_type new_index);

		/**
		 * Emitted whenever a entry has been deleted from the container.
		 * This doesn't leave a hole in the container; all subsequent entries will
		 * be moved up.
		 */
		void
		entry_removed(
				GPlatesGui::TopologySectionsContainer::size_type deleted_index);

		/**
		 * Emitted whenever a number of entries have been inserted into the container.
		 * All subsequent entries will be moved down, as will the Insertion Point
		 * (though this will also cause the @a insertion_point_moved signal to be
		 * emitted).
		 */
		void
		entries_inserted(
				GPlatesGui::TopologySectionsContainer::size_type inserted_index,
				GPlatesGui::TopologySectionsContainer::size_type quantity);

		/**
		 * Emitted whenever the data from a range of entries has been modified.
		 * The table does not change size, and the insertion point has not moved.
		 *
		 * @a modified_index_begin and @a modified_index_end specify the range
		 * of entries affected; it is an inclusive range, and @a begin may equal
		 * @a end.
		 */
		void
		entries_modified(
				GPlatesGui::TopologySectionsContainer::size_type modified_index_begin,
				GPlatesGui::TopologySectionsContainer::size_type modified_index_end);
		
	private:

		/** Feature focus ptr */
		FeatureFocus *d_feature_focus_ptr;

		/**
		 * The vector of TableRow holding the data to be displayed.
		 */
		container_type d_container;

		/**
		 * The index that new data entries will be inserted into.
		 * As it happens, this also corresponds to which visual row of the table
		 * that the special Insertion Point row is in.
		 */
		TopologySectionsContainer::size_type d_insertion_point;

		
	};
}
#endif // GPLATES_GUI_TOPOLOGYSECTIONSCONTAINER_H

