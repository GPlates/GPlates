/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 The University of Sydney, Australia
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
#include "model/ReconstructedFeatureGeometry.h"


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

		{ 101, Colour::get_yellow() },
		{ 102, Colour::get_red() },
		{ 103, Colour::get_blue() },
		{ 104, Colour::get_red() },
		{ 105, Colour::get_lime() },
		{ 107, Colour::get_fuschia() },
		{ 108, Colour::get_aqua() },
		{ 109, Colour::get_red() },
		{ 110, Colour::get_lime() },
		{ 111, Colour::get_yellow() },
		{ 112, Colour::get_blue() },
		{ 113, Colour::get_navy() },
		{ 114, Colour::get_aqua() },
		{ 116, Colour::get_lime() },
		{ 120, Colour::get_lime() },
		{ 121, Colour::get_aqua() },
		{ 122, Colour::get_red() },
		{ 123, Colour::get_fuschia() },
		{ 124, Colour::get_navy() },
		{ 199, Colour::get_aqua() },
		{ 201, Colour::get_fuschia() },
		{ 202, Colour::get_red() },
		{ 204, Colour::get_navy() },
		{ 205, Colour::get_aqua() },
		{ 206, Colour::get_red() },
		{ 207, Colour::get_aqua() },
		{ 208, Colour::get_fuschia() },
		{ 209, Colour::get_blue() },
		{ 210, Colour::get_lime() },
		{ 211, Colour::get_fuschia() },
		{ 212, Colour::get_aqua() },
		{ 213, Colour::get_navy() },
		{ 215, Colour::get_fuschia() },
		{ 217, Colour::get_navy() },
		{ 218, Colour::get_lime() },
		{ 219, Colour::get_yellow() },
		{ 220, Colour::get_blue() },
		{ 221, Colour::get_fuschia() },
		{ 222, Colour::get_red() },
		{ 223, Colour::get_lime() },
		{ 224, Colour::get_navy() },
		{ 225, Colour::get_aqua() },
		{ 226, Colour::get_fuschia() },
		{ 227, Colour::get_blue() },
		{ 228, Colour::get_yellow() },
		{ 229, Colour::get_navy() },
		{ 230, Colour::get_lime() },
		{ 231, Colour::get_red() },
		{ 232, Colour::get_aqua() },
		{ 233, Colour::get_fuschia() },
		{ 234, Colour::get_yellow() },
		{ 235, Colour::get_lime() },
		{ 236, Colour::get_navy() },
		{ 237, Colour::get_blue() },
		{ 238, Colour::get_red() },
		{ 239, Colour::get_lime() },
		{ 240, Colour::get_yellow() },
		{ 252, Colour::get_blue() },
		{ 253, Colour::get_fuschia() },
		{ 254, Colour::get_navy() },
		{ 255, Colour::get_red() },
		{ 256, Colour::get_aqua() },
		{ 257, Colour::get_red() },
		{ 258, Colour::get_navy() },
		{ 259, Colour::get_lime() },
		{ 260, Colour::get_blue() },
		{ 264, Colour::get_aqua() },
		{ 265, Colour::get_red() },
		{ 266, Colour::get_lime() },
		{ 267, Colour::get_navy() },
		{ 268, Colour::get_yellow() },
		{ 269, Colour::get_blue() },
		{ 270, Colour::get_fuschia() },
		{ 271, Colour::get_red() },
		{ 272, Colour::get_navy() },
		{ 273, Colour::get_aqua() },
		{ 274, Colour::get_yellow() },
		{ 275, Colour::get_red() },
		{ 277, Colour::get_blue() },
		{ 280, Colour::get_red() },
		{ 281, Colour::get_lime() },
		{ 282, Colour::get_blue() },
		{ 283, Colour::get_aqua() },
		{ 284, Colour::get_fuschia() },
		{ 285, Colour::get_navy() },
		{ 286, Colour::get_aqua() },
		{ 287, Colour::get_lime() },
		{ 290, Colour::get_blue() },
		{ 291, Colour::get_fuschia() },
		{ 299, Colour::get_navy() },
		{ 301, Colour::get_lime() },
		{ 302, Colour::get_navy() },
		{ 303, Colour::get_red() },
		{ 304, Colour::get_navy() },
		{ 305, Colour::get_blue() },
		{ 306, Colour::get_aqua() },
		{ 307, Colour::get_yellow() },
		{ 308, Colour::get_yellow() },
		{ 309, Colour::get_blue() },
		{ 310, Colour::get_navy() },
		{ 311, Colour::get_red() },
		{ 312, Colour::get_lime() },
		{ 313, Colour::get_navy() },
		{ 314, Colour::get_yellow() },
		{ 315, Colour::get_red() },
		{ 317, Colour::get_yellow() },
		{ 318, Colour::get_fuschia() },
		{ 319, Colour::get_lime() },
		{ 320, Colour::get_red() },
		{ 321, Colour::get_navy() },
		{ 322, Colour::get_yellow() },
		{ 323, Colour::get_blue() },
		{ 324, Colour::get_fuschia() },
		{ 330, Colour::get_yellow() },
		{ 331, Colour::get_aqua() },
		{ 401, Colour::get_yellow() },
		{ 402, Colour::get_red() },
		{ 403, Colour::get_lime() },
		{ 405, Colour::get_navy() },
		{ 406, Colour::get_yellow() },
		{ 407, Colour::get_blue() },
		{ 408, Colour::get_fuschia() },
		{ 409, Colour::get_yellow() },
		{ 501, Colour::get_aqua() },
		{ 502, Colour::get_red() },
		{ 503, Colour::get_blue() },
		{ 504, Colour::get_red() },
		{ 505, Colour::get_navy() },
		{ 506, Colour::get_aqua() },
		{ 507, Colour::get_lime() },
		{ 508, Colour::get_navy() },
		{ 509, Colour::get_aqua() },
		{ 510, Colour::get_fuschia() },
		{ 511, Colour::get_navy() },
		{ 512, Colour::get_lime() },
		{ 513, Colour::get_yellow() },
		{ 601, Colour::get_aqua() },
		{ 602, Colour::get_red() },
		{ 603, Colour::get_lime() },
		{ 604, Colour::get_navy() },
		{ 606, Colour::get_blue() },
		{ 607, Colour::get_yellow() },
		{ 608, Colour::get_red() },
		{ 609, Colour::get_lime() },
		{ 610, Colour::get_navy() },
		{ 611, Colour::get_aqua() },
		{ 612, Colour::get_red() },
		{ 613, Colour::get_lime() },
		{ 614, Colour::get_yellow() },
		{ 615, Colour::get_yellow() },
		{ 616, Colour::get_blue() },
		{ 617, Colour::get_fuschia() },
		{ 618, Colour::get_aqua() },
		{ 619, Colour::get_red() },
		{ 620, Colour::get_lime() },
		{ 621, Colour::get_navy() },
		{ 622, Colour::get_yellow() },
		{ 623, Colour::get_blue() },
		{ 624, Colour::get_fuschia() },
		{ 625, Colour::get_aqua() },
		{ 626, Colour::get_red() },
		{ 627, Colour::get_lime() },
		{ 628, Colour::get_navy() },
		{ 629, Colour::get_yellow() },
		{ 630, Colour::get_blue() },
		{ 631, Colour::get_fuschia() },
		{ 632, Colour::get_aqua() },
		{ 633, Colour::get_red() },
		{ 634, Colour::get_lime() },
		{ 635, Colour::get_navy() },
		{ 636, Colour::get_yellow() },
		{ 637, Colour::get_blue() },
		{ 638, Colour::get_fuschia() },
		{ 639, Colour::get_aqua() },
		{ 640, Colour::get_red() },
		{ 641, Colour::get_lime() },
		{ 642, Colour::get_navy() },
		{ 643, Colour::get_yellow() },
		{ 644, Colour::get_blue() },
		{ 645, Colour::get_fuschia() },
		{ 646, Colour::get_aqua() },
		{ 647, Colour::get_red() },
		{ 648, Colour::get_lime() },
		{ 649, Colour::get_navy() },
		{ 650, Colour::get_yellow() },
		{ 651, Colour::get_blue() },
		{ 652, Colour::get_fuschia() },
		{ 666, Colour::get_aqua() },
		{ 667, Colour::get_red() },
		{ 668, Colour::get_lime() },
		{ 669, Colour::get_navy() },
		{ 670, Colour::get_yellow() },
		{ 671, Colour::get_blue() },
		{ 672, Colour::get_fuschia() },
		{ 673, Colour::get_aqua() },
		{ 674, Colour::get_red() },
		{ 675, Colour::get_lime() },
		{ 676, Colour::get_navy() },
		{ 677, Colour::get_yellow() },
		{ 678, Colour::get_blue() },
		{ 679, Colour::get_fuschia() },
		{ 680, Colour::get_aqua() },
		{ 681, Colour::get_red() },
		{ 682, Colour::get_lime() },
		{ 683, Colour::get_navy() },
		{ 684, Colour::get_yellow() },
		{ 685, Colour::get_blue() },
		{ 686, Colour::get_fuschia() },
		{ 687, Colour::get_aqua() },
		{ 688, Colour::get_red() },
		{ 689, Colour::get_lime() },
		{ 690, Colour::get_navy() },
		{ 691, Colour::get_yellow() },
		{ 692, Colour::get_blue() },
		{ 693, Colour::get_fuschia() },
		{ 694, Colour::get_aqua() },
		{ 695, Colour::get_red() },
		{ 696, Colour::get_lime() },
		{ 697, Colour::get_navy() },
		{ 698, Colour::get_yellow() },
		{ 699, Colour::get_blue() },
		{ 701, Colour::get_lime() },
		{ 702, Colour::get_navy() },
		{ 704, Colour::get_red() },
		{ 705, Colour::get_lime() },
		{ 706, Colour::get_aqua() },
		{ 707, Colour::get_red() },
		{ 708, Colour::get_lime() },
		{ 709, Colour::get_yellow() },
		{ 710, Colour::get_aqua() },
		{ 712, Colour::get_blue() },
		{ 713, Colour::get_lime() },
		{ 714, Colour::get_fuschia() },
		{ 715, Colour::get_aqua() },
		{ 750, Colour::get_navy() },
		{ 801, Colour::get_lime() },
		{ 802, Colour::get_blue() },
		{ 803, Colour::get_navy() },
		{ 804, Colour::get_yellow() },
		{ 805, Colour::get_lime() },
		{ 806, Colour::get_navy() },
		{ 807, Colour::get_lime() },
		{ 808, Colour::get_red() },
		{ 809, Colour::get_lime() },
		{ 810, Colour::get_red() },
		{ 811, Colour::get_yellow() },
		{ 812, Colour::get_blue() },
		{ 813, Colour::get_yellow() },
		{ 814, Colour::get_aqua() },
		{ 815, Colour::get_red() },
		{ 816, Colour::get_lime() },
		{ 817, Colour::get_navy() },
		{ 818, Colour::get_yellow() },
		{ 819, Colour::get_blue() },
		{ 820, Colour::get_fuschia() },
		{ 821, Colour::get_aqua() },
		{ 822, Colour::get_red() },
		{ 823, Colour::get_lime() },
		{ 824, Colour::get_navy() },
		{ 825, Colour::get_yellow() },
		{ 826, Colour::get_blue() },
		{ 827, Colour::get_fuschia() },
		{ 828, Colour::get_aqua() },
		{ 829, Colour::get_red() },
		{ 830, Colour::get_lime() },
		{ 831, Colour::get_navy() },
		{ 832, Colour::get_aqua() },
		{ 833, Colour::get_red() },
		{ 834, Colour::get_lime() },
		{ 835, Colour::get_navy() },
		{ 836, Colour::get_yellow() },
		{ 837, Colour::get_blue() },
		{ 838, Colour::get_fuschia() },
		{ 839, Colour::get_aqua() },
		{ 840, Colour::get_lime() },
		{ 842, Colour::get_red() },
		{ 843, Colour::get_lime() },
		{ 844, Colour::get_navy() },
		{ 845, Colour::get_yellow() },
		{ 846, Colour::get_blue() },
		{ 847, Colour::get_fuschia() },
		{ 850, Colour::get_aqua() },
		{ 851, Colour::get_red() },
		{ 880, Colour::get_lime() },
		{ 901, Colour::get_red() },
		{ 902, Colour::get_lime() },
		{ 903, Colour::get_fuschia() },
		{ 904, Colour::get_lime() },
		{ 906, Colour::get_blue() },
		{ 907, Colour::get_navy() },
		{ 909, Colour::get_red() },
		{ 910, Colour::get_lime() },
		{ 911, Colour::get_red() },
		{ 912, Colour::get_navy() },
		{ 913, Colour::get_fuschia() },
		{ 914, Colour::get_navy() },
		{ 915, Colour::get_aqua() },
		{ 916, Colour::get_fuschia() },
		{ 917, Colour::get_yellow() },
		{ 918, Colour::get_navy() },
		{ 919, Colour::get_lime() },
		{ 920, Colour::get_navy() },
		{ 921, Colour::get_red() },
		{ 922, Colour::get_blue() },
		{ 925, Colour::get_aqua() },
		{ 926, Colour::get_red() },
		{ 927, Colour::get_lime() },
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
