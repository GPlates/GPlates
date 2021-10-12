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

#include <QDebug>
#include <QObject>
#include <iterator>
#include <vector>

#include "maths/LatLonPointConversions.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"
#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"

#include "property-values/GpmlTopologicalSection.h"

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
		 * This is the minimum information needed to display in the topological
		 * sections table GUI and to allow the topology tools to identify the
		 * type of topology sections it's dealing with.
		 */
		class TableRow
		{
		public:
			/**
			 * Construct using information from a GpmlTopologicalSection.
			 *
			 * For point sections this will include a geometry property delegate.
			 * For line sections this will include a geometry property delegate,
			 *    possibly start/end intersections and a reverse geometry flag.
			 */
			TableRow(
					const GPlatesModel::FeatureId &feature_id,
					const GPlatesModel::PropertyName &geometry_property_name,
					bool reverse_order = false,
					const boost::optional<GPlatesMaths::PointOnSphere> &click_point = boost::none);


			/**
			 * Construct using information from a clicked geometry.
			 *
			 * This is used when the user clicks on a geometry and wants to add it as a
			 * topological section - the reverse flag is usually always false in this case.
			 */
			TableRow(
					const GPlatesModel::FeatureHandle::properties_iterator &geometry_property,
					const boost::optional<GPlatesMaths::PointOnSphere> &click_point = boost::none,
					bool reverse_order = false);


			//! Get the feature id.
			const GPlatesModel::FeatureId &
			get_feature_id() const
			{
				return d_feature_id;
			}

			//! Get the feature reference.
			const GPlatesModel::FeatureHandle::weak_ref &
			get_feature_ref() const
			{
				return d_feature_ref;
			}

			//! Get the geometry property iterator.
			const GPlatesModel::FeatureHandle::properties_iterator &
			get_geometry_property() const
			{
				return d_geometry_property;
			}

			//! Set the present day click point.
			const boost::optional<GPlatesMaths::PointOnSphere> &
			get_present_day_click_point() const
			{
				return d_present_day_click_point;
			}

			//! Set the present day click point.
			void
			set_present_day_click_point(
					const boost::optional<GPlatesMaths::PointOnSphere> &present_day_click_point)
			{
				d_present_day_click_point = present_day_click_point;
			}

			//! Get the reverse order of section points.
			bool
			get_reverse() const
			{
				return d_reverse;
			}

			//! Set the reverse order of section points.
			void
			set_reverse(
					bool reverse)
			{
				d_reverse = reverse;
			}

		private:
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
			 * NOTE: remember to check 'is_valid()'.
			 *
			 * Mental Note: I have put this here in the struct rather than passed to the
			 * render_xxx methods to allow better decoupling of data and Qt-specific stuff
			 * later. -JC
			 */
			GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

			/**
			 * As well as the feature reference, another thing to keep track of in the
			 * table is the geometric property which is to be used for intersections, etc.
			 * This is particularly important to track if we want to let the user click
			 * on the table and highlight bits of geometry while building a topology.
			 *
			 * NOTE: remember to check 'is_valid()'.
			 *
			 * NOTE: 'd_geometry_property' must be declared/initialised after 'd_feature_ref'
			 * since it uses it when constructed.
			 */
			GPlatesModel::FeatureHandle::properties_iterator d_geometry_property;

			/**
			 * The point the user clicked on to select the section.
			 * Used as a reference point to aid the intersection algorithm.
			 * Coords are present-day.
			 */
			boost::optional<GPlatesMaths::PointOnSphere> d_present_day_click_point;

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
		TopologySectionsContainer();

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
			const size_type index = insertion_point();
			// Use STL vector::insert with two input iterators for probably-fast
			// insert (depending on iterator capability)
			d_container.insert(d_container.begin() + index, begin_it, end_it);
			// We need to know how many just got added.
			size_type quantity = std::distance(begin_it, end_it);
			// ... because inserting will naturally bump the insertion point down n rows.
			move_insertion_point(insertion_point() + quantity);
			// ... and we need to emit appropriate signals.
			// Adding iterator range for caller's convenience since caller may want
			// an iterator but should not assume a random access iterator (we can however
			// since this is our implementation).
			emit entries_inserted(
					index,
					quantity,
					d_container.begin() + index/*original insertion point*/,
					d_container.begin() + index + quantity);
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
		 * The @a entry_modified() signal is emitted.
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
		 *
		 * The insertion points always refers to an existing @a TableRow and
		 * can be used in @a at.
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
		 * The @a focus_feature_at_index(int) signal is emitted.
		 */
		void
		set_focus_feature_at_index( 
				size_type index );

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
			TopologySectionsContainer::TableRow trow1 = { GPlatesModel::FeatureId("GPlates-af5b7ca8-0a8a-4ad0-8741-0b81661f6634"), null_ref, GPlatesMaths::PointOnSphere::north_pole, false };
			TopologySectionsContainer::TableRow trow2 = { GPlatesModel::FeatureId("GPlates-80dee8d2-7ce8-4193-b34f-d54975989e78"), null_ref, GPlatesMaths::PointOnSphere::north_pole, true };
			TopologySectionsContainer::TableRow trow3 = { GPlatesModel::FeatureId("GPlates-9a93bb0d-ffa3-4672-880b-842c3cca671a"), null_ref, GPlatesMaths::PointOnSphere::north_pole, false };
			TopologySectionsContainer::TableRow trow4 = { GPlatesModel::FeatureId("GPlates-e50dc893-b535-491b-996c-edaaa45ad108"), null_ref, GPlatesMaths::PointOnSphere::north_pole, true };
			TopologySectionsContainer::TableRow trow5 = { GPlatesModel::FeatureId("GPlates-4d53a360-65cc-45c2-addd-34d057798cde"), null_ref, GPlatesMaths::PointOnSphere::north_pole, true };
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
		 *
		 * The new index always refers to an existing @a TableRow and can be used
		 * in @a at.
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
				GPlatesGui::TopologySectionsContainer::size_type quantity,
				GPlatesGui::TopologySectionsContainer::const_iterator inserted_begin,
				GPlatesGui::TopologySectionsContainer::const_iterator inserted_end);

		/**
		 * Emitted whenever the data in an entry has been modified.
		 * The table does not change size, and the insertion point has not moved.
		 */
		void
		entry_modified(
				GPlatesGui::TopologySectionsContainer::size_type modified_index);

		/**
		 * Emitted whenever a feature is focused 
		 */
		void
		focus_feature_at_index(
				GPlatesGui::TopologySectionsContainer::size_type index);
		
	private:

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

