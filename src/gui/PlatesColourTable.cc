/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "PlatesColourTable.h"


GPlatesGui::PlatesColourTable *
GPlatesGui::PlatesColourTable::Instance() {

	if (_instance == NULL) {

		// create a new instance
		_instance = new PlatesColourTable();
	}
	return _instance;
}


GPlatesGui::PlatesColourTable::~PlatesColourTable() {

	delete[] _id_table;
	delete[] _colours;
}


GPlatesGui::PlatesColourTable::const_iterator
GPlatesGui::PlatesColourTable::lookup(const GPlatesGlobal::rid_t &id) const {

	// convert the ID into an index into the 'ID table'.
	size_t i = static_cast< size_t >(id.ival());
	const Colour *colour_ptr = _id_table[i];
	if (colour_ptr == NULL) {

		// There is no entry for this ID in the table.
		return end();

	} else return const_iterator(colour_ptr);
}


namespace
{
	using namespace GPlatesGui;

	struct MappingPair
	{
		GPlatesGlobal::rid_t id;
		Colour colour;
	};

	const MappingPair MappingArray[] = {

		{ 101, Colour::YELLOW },
		{ 102, Colour::RED },
		{ 103, Colour::BLUE },
		{ 104, Colour::RED },
		{ 105, Colour::LIME },
		{ 107, Colour::FUSCHIA },
		{ 108, Colour::WHITE },
		{ 109, Colour::RED },
		{ 110, Colour::LIME },
		{ 111, Colour::YELLOW },
		{ 112, Colour::BLUE },
		{ 113, Colour::NAVY },
		{ 114, Colour::WHITE },
		{ 116, Colour::LIME },
		{ 120, Colour::LIME },
		{ 121, Colour::WHITE },
		{ 122, Colour::RED },
		{ 123, Colour::FUSCHIA },
		{ 124, Colour::NAVY },
		{ 199, Colour::WHITE },
		{ 201, Colour::FUSCHIA },
		{ 202, Colour::RED },
		{ 204, Colour::NAVY },
		{ 205, Colour::WHITE },
		{ 206, Colour::RED },
		{ 207, Colour::WHITE },
		{ 208, Colour::FUSCHIA },
		{ 209, Colour::BLUE },
		{ 210, Colour::LIME },
		{ 211, Colour::FUSCHIA },
		{ 212, Colour::WHITE },
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
		{ 225, Colour::WHITE },
		{ 226, Colour::FUSCHIA },
		{ 227, Colour::BLUE },
		{ 228, Colour::YELLOW },
		{ 229, Colour::NAVY },
		{ 230, Colour::LIME },
		{ 231, Colour::RED },
		{ 232, Colour::WHITE },
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
		{ 256, Colour::WHITE },
		{ 257, Colour::RED },
		{ 258, Colour::NAVY },
		{ 259, Colour::LIME },
		{ 260, Colour::BLUE },
		{ 264, Colour::WHITE },
		{ 265, Colour::RED },
		{ 266, Colour::LIME },
		{ 267, Colour::NAVY },
		{ 268, Colour::YELLOW },
		{ 269, Colour::BLUE },
		{ 270, Colour::FUSCHIA },
		{ 271, Colour::RED },
		{ 272, Colour::NAVY },
		{ 273, Colour::WHITE },
		{ 274, Colour::YELLOW },
		{ 275, Colour::RED },
		{ 277, Colour::BLUE },
		{ 280, Colour::RED },
		{ 281, Colour::LIME },
		{ 282, Colour::BLUE },
		{ 283, Colour::WHITE },
		{ 284, Colour::FUSCHIA },
		{ 285, Colour::NAVY },
		{ 286, Colour::WHITE },
		{ 287, Colour::LIME },
		{ 290, Colour::BLUE },
		{ 291, Colour::FUSCHIA },
		{ 299, Colour::NAVY },
		{ 301, Colour::LIME },
		{ 302, Colour::NAVY },
		{ 303, Colour::RED },
		{ 304, Colour::NAVY },
		{ 305, Colour::BLUE },
		{ 306, Colour::WHITE },
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
		{ 331, Colour::WHITE },
		{ 401, Colour::YELLOW },
		{ 402, Colour::RED },
		{ 403, Colour::LIME },
		{ 405, Colour::NAVY },
		{ 406, Colour::YELLOW },
		{ 407, Colour::BLUE },
		{ 408, Colour::FUSCHIA },
		{ 409, Colour::YELLOW },
		{ 501, Colour::WHITE },
		{ 502, Colour::RED },
		{ 503, Colour::BLUE },
		{ 504, Colour::RED },
		{ 505, Colour::NAVY },
		{ 506, Colour::WHITE },
		{ 507, Colour::LIME },
		{ 508, Colour::NAVY },
		{ 509, Colour::WHITE },
		{ 510, Colour::FUSCHIA },
		{ 511, Colour::NAVY },
		{ 512, Colour::LIME },
		{ 513, Colour::YELLOW },
		{ 601, Colour::WHITE },
		{ 602, Colour::RED },
		{ 603, Colour::LIME },
		{ 604, Colour::NAVY },
		{ 606, Colour::BLUE },
		{ 607, Colour::YELLOW },
		{ 608, Colour::RED },
		{ 609, Colour::LIME },
		{ 610, Colour::NAVY },
		{ 611, Colour::WHITE },
		{ 612, Colour::RED },
		{ 613, Colour::LIME },
		{ 614, Colour::YELLOW },
		{ 615, Colour::YELLOW },
		{ 616, Colour::BLUE },
		{ 617, Colour::FUSCHIA },
		{ 618, Colour::WHITE },
		{ 619, Colour::RED },
		{ 620, Colour::LIME },
		{ 621, Colour::NAVY },
		{ 622, Colour::YELLOW },
		{ 623, Colour::BLUE },
		{ 624, Colour::FUSCHIA },
		{ 625, Colour::WHITE },
		{ 626, Colour::RED },
		{ 627, Colour::LIME },
		{ 628, Colour::NAVY },
		{ 629, Colour::YELLOW },
		{ 630, Colour::BLUE },
		{ 631, Colour::FUSCHIA },
		{ 632, Colour::WHITE },
		{ 633, Colour::RED },
		{ 634, Colour::LIME },
		{ 635, Colour::NAVY },
		{ 636, Colour::YELLOW },
		{ 637, Colour::BLUE },
		{ 638, Colour::FUSCHIA },
		{ 639, Colour::WHITE },
		{ 640, Colour::RED },
		{ 641, Colour::LIME },
		{ 642, Colour::NAVY },
		{ 643, Colour::YELLOW },
		{ 644, Colour::BLUE },
		{ 645, Colour::FUSCHIA },
		{ 646, Colour::WHITE },
		{ 647, Colour::RED },
		{ 648, Colour::LIME },
		{ 649, Colour::NAVY },
		{ 650, Colour::YELLOW },
		{ 651, Colour::BLUE },
		{ 652, Colour::FUSCHIA },
		{ 666, Colour::WHITE },
		{ 667, Colour::RED },
		{ 668, Colour::LIME },
		{ 669, Colour::NAVY },
		{ 670, Colour::YELLOW },
		{ 671, Colour::BLUE },
		{ 672, Colour::FUSCHIA },
		{ 673, Colour::WHITE },
		{ 674, Colour::RED },
		{ 675, Colour::LIME },
		{ 676, Colour::NAVY },
		{ 677, Colour::YELLOW },
		{ 678, Colour::BLUE },
		{ 679, Colour::FUSCHIA },
		{ 680, Colour::WHITE },
		{ 681, Colour::RED },
		{ 682, Colour::LIME },
		{ 683, Colour::NAVY },
		{ 684, Colour::YELLOW },
		{ 685, Colour::BLUE },
		{ 686, Colour::FUSCHIA },
		{ 687, Colour::WHITE },
		{ 688, Colour::RED },
		{ 689, Colour::LIME },
		{ 690, Colour::NAVY },
		{ 691, Colour::YELLOW },
		{ 692, Colour::BLUE },
		{ 693, Colour::FUSCHIA },
		{ 694, Colour::WHITE },
		{ 695, Colour::RED },
		{ 696, Colour::LIME },
		{ 697, Colour::NAVY },
		{ 698, Colour::YELLOW },
		{ 699, Colour::BLUE },
		{ 701, Colour::LIME },
		{ 702, Colour::NAVY },
		{ 704, Colour::RED },
		{ 705, Colour::LIME },
		{ 706, Colour::WHITE },
		{ 707, Colour::RED },
		{ 708, Colour::LIME },
		{ 709, Colour::YELLOW },
		{ 710, Colour::WHITE },
		{ 712, Colour::BLUE },
		{ 713, Colour::LIME },
		{ 714, Colour::FUSCHIA },
		{ 715, Colour::WHITE },
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
		{ 814, Colour::WHITE },
		{ 815, Colour::RED },
		{ 816, Colour::LIME },
		{ 817, Colour::NAVY },
		{ 818, Colour::YELLOW },
		{ 819, Colour::BLUE },
		{ 820, Colour::FUSCHIA },
		{ 821, Colour::WHITE },
		{ 822, Colour::RED },
		{ 823, Colour::LIME },
		{ 824, Colour::NAVY },
		{ 825, Colour::YELLOW },
		{ 826, Colour::BLUE },
		{ 827, Colour::FUSCHIA },
		{ 828, Colour::WHITE },
		{ 829, Colour::RED },
		{ 830, Colour::LIME },
		{ 831, Colour::NAVY },
		{ 832, Colour::WHITE },
		{ 833, Colour::RED },
		{ 834, Colour::LIME },
		{ 835, Colour::NAVY },
		{ 836, Colour::YELLOW },
		{ 837, Colour::BLUE },
		{ 838, Colour::FUSCHIA },
		{ 839, Colour::WHITE },
		{ 840, Colour::LIME },
		{ 842, Colour::RED },
		{ 843, Colour::LIME },
		{ 844, Colour::NAVY },
		{ 845, Colour::YELLOW },
		{ 846, Colour::BLUE },
		{ 847, Colour::FUSCHIA },
		{ 850, Colour::WHITE },
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
		{ 915, Colour::WHITE },
		{ 916, Colour::FUSCHIA },
		{ 917, Colour::YELLOW },
		{ 918, Colour::NAVY },
		{ 919, Colour::LIME },
		{ 920, Colour::NAVY },
		{ 921, Colour::RED },
		{ 922, Colour::BLUE },
		{ 925, Colour::WHITE },
		{ 926, Colour::RED },
		{ 927, Colour::LIME },
	};


	GPlatesGlobal::rid_t
	getHighestID(const MappingPair array[], size_t array_len) {

		GPlatesGlobal::rid_t highest = 0;
		for (size_t i = 0; i < array_len; i++) {

			if (array[i].id > highest) highest = array[i].id;
		}
		return highest;
	}


	void
	populate(Colour *id_table[], Colour colours[],
	 const MappingPair array[], size_t array_len) {

		for (size_t i = 0; i < array_len; i++) {

			// convert the ID into an index into the 'ID table'.
			size_t j = static_cast< size_t >(array[i].id.ival());

			colours[i] = array[i].colour;
			id_table[j] = &(colours[i]);
		}
	}
}


GPlatesGui::PlatesColourTable::PlatesColourTable() {

	size_t len_mapping_array =
	 sizeof(MappingArray) / sizeof(MappingArray[0]);

	// First pass is to discover the highest rotation ID.
	// [We won't assume that the 'MappingArray' is sorted.]
	GPlatesGlobal::rid_t highest =
	 getHighestID(MappingArray, len_mapping_array);

	// Allocate the arrays.
	// FIXME: make this code use auto_ptrs to avoid possible mem-leaks.
	size_t len_id_table = static_cast< size_t >(highest.ival()) + 1;
	_id_table = new Colour *[len_id_table];
	for (size_t j = 0; j < len_id_table; j++) {

		// set the pointer to NULL.  [C++ does not do this for us.]
		_id_table[j] = NULL;
	}

	_colours = new Colour[len_mapping_array];

	// Second pass is to populate the two arrays.
	populate(_id_table, _colours, MappingArray, len_mapping_array);
}


GPlatesGui::PlatesColourTable *
GPlatesGui::PlatesColourTable::_instance = NULL;
