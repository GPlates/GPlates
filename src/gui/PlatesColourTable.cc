/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include "PlatesColourTable.h"


GPlatesGui::PlatesColourTable *
GPlatesGui::PlatesColourTable::Instance()
{
	if (!d_instance) {

		// create a new instance
		d_instance.reset(new PlatesColourTable());
	}
	return d_instance.get();
}


GPlatesGui::ColourTable::const_iterator
GPlatesGui::PlatesColourTable::lookup(
		const GPlatesModel::ReconstructedFeatureGeometry &feature) const
{
	GPlatesModel::integer_plate_id_type id = *(feature.reconstruction_plate_id());

	// First, ensure that the ID isn't greater than the highest ID in the
	// ID table (which would result in an out-of-bounds index).
	if (id > d_highest_known_rid) {

		// The ID is outside the bounds of this table.
		return end();
	}

	// Now, convert the ID into an index into the 'ID table'.
	size_t idx = static_cast< size_t >(id);
	const Colour *colour_ptr = d_id_table[idx];
	if (colour_ptr == NULL) {

		// There is no entry for this ID in the table.
		return end();

	} else {
		return const_iterator(colour_ptr);
	}
}


GPlatesGui::PlatesColourTable::PlatesColourTable():
		d_highest_known_rid(0) /* no default ctor, so must initialise now */
{
	const MappingPair mapping_array[] = {

		{ 101, Colour::YELLOW },
		{ 102, Colour::RED },
		{ 103, Colour::BLUE },
		{ 104, Colour::LIME},
		{ 105, Colour::PURPLE },
		{ 106, Colour::AQUA },
		{ 107, Colour::FUSCHIA },
		{ 108, Colour::AQUA },
		{ 109, Colour::RED },
		{ 110, Colour::LIME },
		{ 111, Colour::YELLOW },
		{ 112, Colour::BLUE },
		{ 113, Colour::NAVY },
		{ 114, Colour::AQUA },
		{ 116, Colour::LIME },
		{ 120, Colour::LIME },
		{ 121, Colour::AQUA },
		{ 122, Colour::RED },
		{ 123, Colour::FUSCHIA },
		{ 124, Colour::NAVY },
		{ 199, Colour::AQUA },
		{ 201, Colour::FUSCHIA },
		{ 202, Colour::RED },
		{ 204, Colour::NAVY },
		{ 205, Colour::AQUA },
		{ 206, Colour::RED },
		{ 207, Colour::AQUA },
		{ 208, Colour::FUSCHIA },
		{ 209, Colour::BLUE },
		{ 210, Colour::LIME },
		{ 211, Colour::FUSCHIA },
		{ 212, Colour::AQUA },
		{ 213, Colour::NAVY },
		{ 215, Colour::FUSCHIA },
		{ 217, Colour::NAVY },
		{ 218, Colour::LIME },
		{ 219, Colour::YELLOW },
		{ 220, Colour::BLUE },
		{ 221, Colour::FUSCHIA },
		{ 222, Colour::RED },
		{ 223, Colour::LIME },
		{ 224, Colour::NAVY },
		{ 225, Colour::AQUA },
		{ 226, Colour::FUSCHIA },
		{ 227, Colour::BLUE },
		{ 228, Colour::YELLOW },
		{ 229, Colour::NAVY },
		{ 230, Colour::LIME },
		{ 231, Colour::RED },
		{ 232, Colour::AQUA },
		{ 233, Colour::FUSCHIA },
		{ 234, Colour::YELLOW },
		{ 235, Colour::LIME },
		{ 236, Colour::NAVY },
		{ 237, Colour::BLUE },
		{ 238, Colour::RED },
		{ 239, Colour::LIME },
		{ 240, Colour::YELLOW },
		{ 252, Colour::BLUE },
		{ 253, Colour::FUSCHIA },
		{ 254, Colour::NAVY },
		{ 255, Colour::RED },
		{ 256, Colour::AQUA },
		{ 257, Colour::RED },
		{ 258, Colour::NAVY },
		{ 259, Colour::LIME },
		{ 260, Colour::BLUE },
		{ 264, Colour::AQUA },
		{ 265, Colour::RED },
		{ 266, Colour::LIME },
		{ 267, Colour::NAVY },
		{ 268, Colour::YELLOW },
		{ 269, Colour::BLUE },
		{ 270, Colour::FUSCHIA },
		{ 271, Colour::RED },
		{ 272, Colour::NAVY },
		{ 273, Colour::AQUA },
		{ 274, Colour::YELLOW },
		{ 275, Colour::RED },
		{ 277, Colour::BLUE },
		{ 280, Colour::RED },
		{ 281, Colour::LIME },
		{ 282, Colour::BLUE },
		{ 283, Colour::AQUA },
		{ 284, Colour::FUSCHIA },
		{ 285, Colour::NAVY },
		{ 286, Colour::AQUA },
		{ 287, Colour::LIME },
		{ 290, Colour::BLUE },
		{ 291, Colour::FUSCHIA },
		{ 299, Colour::NAVY },
		{ 301, Colour::LIME },
		{ 302, Colour::NAVY },
		{ 303, Colour::RED },
		{ 304, Colour::NAVY },
		{ 305, Colour::BLUE },
		{ 306, Colour::AQUA },
		{ 307, Colour::YELLOW },
		{ 308, Colour::YELLOW },
		{ 309, Colour::BLUE },
		{ 310, Colour::NAVY },
		{ 311, Colour::RED },
		{ 312, Colour::LIME },
		{ 313, Colour::NAVY },
		{ 314, Colour::YELLOW },
		{ 315, Colour::RED },
		{ 317, Colour::YELLOW },
		{ 318, Colour::FUSCHIA },
		{ 319, Colour::LIME },
		{ 320, Colour::RED },
		{ 321, Colour::NAVY },
		{ 322, Colour::YELLOW },
		{ 323, Colour::BLUE },
		{ 324, Colour::FUSCHIA },
		{ 330, Colour::YELLOW },
		{ 331, Colour::AQUA },
		{ 401, Colour::YELLOW },
		{ 402, Colour::RED },
		{ 403, Colour::LIME },
		{ 405, Colour::NAVY },
		{ 406, Colour::YELLOW },
		{ 407, Colour::BLUE },
		{ 408, Colour::FUSCHIA },
		{ 409, Colour::YELLOW },
		{ 501, Colour::AQUA },
		{ 502, Colour::RED },
		{ 503, Colour::BLUE },
		{ 504, Colour::RED },
		{ 505, Colour::NAVY },
		{ 506, Colour::AQUA },
		{ 507, Colour::LIME },
		{ 508, Colour::NAVY },
		{ 509, Colour::AQUA },
		{ 510, Colour::FUSCHIA },
		{ 511, Colour::NAVY },
		{ 512, Colour::LIME },
		{ 513, Colour::YELLOW },
		{ 601, Colour::AQUA },
		{ 602, Colour::RED },
		{ 603, Colour::LIME },
		{ 604, Colour::NAVY },
		{ 606, Colour::BLUE },
		{ 607, Colour::YELLOW },
		{ 608, Colour::RED },
		{ 609, Colour::LIME },
		{ 610, Colour::NAVY },
		{ 611, Colour::AQUA },
		{ 612, Colour::RED },
		{ 613, Colour::LIME },
		{ 614, Colour::YELLOW },
		{ 615, Colour::YELLOW },
		{ 616, Colour::BLUE },
		{ 617, Colour::FUSCHIA },
		{ 618, Colour::AQUA },
		{ 619, Colour::RED },
		{ 620, Colour::LIME },
		{ 621, Colour::NAVY },
		{ 622, Colour::YELLOW },
		{ 623, Colour::BLUE },
		{ 624, Colour::FUSCHIA },
		{ 625, Colour::AQUA },
		{ 626, Colour::RED },
		{ 627, Colour::LIME },
		{ 628, Colour::NAVY },
		{ 629, Colour::YELLOW },
		{ 630, Colour::BLUE },
		{ 631, Colour::FUSCHIA },
		{ 632, Colour::AQUA },
		{ 633, Colour::RED },
		{ 634, Colour::LIME },
		{ 635, Colour::NAVY },
		{ 636, Colour::YELLOW },
		{ 637, Colour::BLUE },
		{ 638, Colour::FUSCHIA },
		{ 639, Colour::AQUA },
		{ 640, Colour::RED },
		{ 641, Colour::LIME },
		{ 642, Colour::NAVY },
		{ 643, Colour::YELLOW },
		{ 644, Colour::BLUE },
		{ 645, Colour::FUSCHIA },
		{ 646, Colour::AQUA },
		{ 647, Colour::RED },
		{ 648, Colour::LIME },
		{ 649, Colour::NAVY },
		{ 650, Colour::YELLOW },
		{ 651, Colour::BLUE },
		{ 652, Colour::FUSCHIA },
		{ 666, Colour::AQUA },
		{ 667, Colour::RED },
		{ 668, Colour::LIME },
		{ 669, Colour::NAVY },
		{ 670, Colour::YELLOW },
		{ 671, Colour::BLUE },
		{ 672, Colour::FUSCHIA },
		{ 673, Colour::AQUA },
		{ 674, Colour::RED },
		{ 675, Colour::LIME },
		{ 676, Colour::NAVY },
		{ 677, Colour::YELLOW },
		{ 678, Colour::BLUE },
		{ 679, Colour::FUSCHIA },
		{ 680, Colour::AQUA },
		{ 681, Colour::RED },
		{ 682, Colour::LIME },
		{ 683, Colour::NAVY },
		{ 684, Colour::YELLOW },
		{ 685, Colour::BLUE },
		{ 686, Colour::FUSCHIA },
		{ 687, Colour::AQUA },
		{ 688, Colour::RED },
		{ 689, Colour::LIME },
		{ 690, Colour::NAVY },
		{ 691, Colour::YELLOW },
		{ 692, Colour::BLUE },
		{ 693, Colour::FUSCHIA },
		{ 694, Colour::AQUA },
		{ 695, Colour::RED },
		{ 696, Colour::LIME },
		{ 697, Colour::NAVY },
		{ 698, Colour::YELLOW },
		{ 699, Colour::BLUE },
		{ 701, Colour::LIME },
		{ 702, Colour::NAVY },
		{ 704, Colour::RED },
		{ 705, Colour::LIME },
		{ 706, Colour::AQUA },
		{ 707, Colour::RED },
		{ 708, Colour::LIME },
		{ 709, Colour::YELLOW },
		{ 710, Colour::AQUA },
		{ 712, Colour::BLUE },
		{ 713, Colour::LIME },
		{ 714, Colour::FUSCHIA },
		{ 715, Colour::AQUA },
		{ 750, Colour::NAVY },
		{ 801, Colour::LIME },
		{ 802, Colour::BLUE },
		{ 803, Colour::NAVY },
		{ 804, Colour::YELLOW },
		{ 805, Colour::LIME },
		{ 806, Colour::NAVY },
		{ 807, Colour::LIME },
		{ 808, Colour::RED },
		{ 809, Colour::LIME },
		{ 810, Colour::RED },
		{ 811, Colour::YELLOW },
		{ 812, Colour::BLUE },
		{ 813, Colour::YELLOW },
		{ 814, Colour::AQUA },
		{ 815, Colour::RED },
		{ 816, Colour::LIME },
		{ 817, Colour::NAVY },
		{ 818, Colour::YELLOW },
		{ 819, Colour::BLUE },
		{ 820, Colour::FUSCHIA },
		{ 821, Colour::AQUA },
		{ 822, Colour::RED },
		{ 823, Colour::LIME },
		{ 824, Colour::NAVY },
		{ 825, Colour::YELLOW },
		{ 826, Colour::BLUE },
		{ 827, Colour::FUSCHIA },
		{ 828, Colour::AQUA },
		{ 829, Colour::RED },
		{ 830, Colour::LIME },
		{ 831, Colour::NAVY },
		{ 832, Colour::AQUA },
		{ 833, Colour::RED },
		{ 834, Colour::LIME },
		{ 835, Colour::NAVY },
		{ 836, Colour::YELLOW },
		{ 837, Colour::BLUE },
		{ 838, Colour::FUSCHIA },
		{ 839, Colour::AQUA },
		{ 840, Colour::LIME },
		{ 842, Colour::RED },
		{ 843, Colour::LIME },
		{ 844, Colour::NAVY },
		{ 845, Colour::YELLOW },
		{ 846, Colour::BLUE },
		{ 847, Colour::FUSCHIA },
		{ 850, Colour::AQUA },
		{ 851, Colour::RED },
		{ 880, Colour::LIME },
		{ 901, Colour::RED },
		{ 902, Colour::LIME },
		{ 903, Colour::FUSCHIA },
		{ 904, Colour::LIME },
		{ 906, Colour::BLUE },
		{ 907, Colour::NAVY },
		{ 909, Colour::RED },
		{ 910, Colour::LIME },
		{ 911, Colour::RED },
		{ 912, Colour::NAVY },
		{ 913, Colour::FUSCHIA },
		{ 914, Colour::NAVY },
		{ 915, Colour::AQUA },
		{ 916, Colour::FUSCHIA },
		{ 917, Colour::YELLOW },
		{ 918, Colour::NAVY },
		{ 919, Colour::LIME },
		{ 920, Colour::NAVY },
		{ 921, Colour::RED },
		{ 922, Colour::BLUE },
		{ 925, Colour::AQUA },
		{ 926, Colour::RED },
		{ 927, Colour::LIME },
	};


	size_t len_mapping_array =
			sizeof(mapping_array) / sizeof(mapping_array[0]);

	// First pass is to discover the highest rotation ID.
	// [We won't assume that the 'mapping_array' is sorted.]
	d_highest_known_rid = getHighestID(mapping_array, len_mapping_array);

	// Allocate the arrays.
	// FIXME: make this code use auto_ptrs to avoid possible mem-leaks.
	// FIXME: (2): even better, use std::vector!
	/*
	 * See the comment at the declaration of 'd_id_table' to understand why
	 * this array is of length (d_highest_known_rid + 1).
	 */
	size_t lend_id_table =
			static_cast< size_t >(d_highest_known_rid + 1);
	d_id_table.resize(lend_id_table);

	d_colours.resize(len_mapping_array);

	// Second pass is to populate the two arrays.
	populate(mapping_array, len_mapping_array);
}


void
GPlatesGui::PlatesColourTable::populate(
		const MappingPair array[],
		size_t array_len)
{
	for (size_t array_idx = 0; array_idx < array_len; ++array_idx) {

		// convert the ID into an index into the 'ID table'.
		size_t id_table_idx =
		 static_cast< size_t >(array[array_idx].id);

		d_colours[array_idx] = array[array_idx].colour;
		d_id_table[id_table_idx] = &d_colours[array_idx];
	}
}

GPlatesModel::integer_plate_id_type
GPlatesGui::PlatesColourTable::getHighestID(
		const MappingPair array[],
		size_t array_len)
{

	GPlatesModel::integer_plate_id_type highest_so_far = 0;
	for (size_t i = 0; i < array_len; i++) {

		if (array[i].id > highest_so_far) {
			
			highest_so_far = array[i].id;
		}
	}
	return highest_so_far;
}


boost::scoped_ptr<GPlatesGui::PlatesColourTable>
GPlatesGui::PlatesColourTable::d_instance;
