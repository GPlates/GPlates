/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>
#include <QColor>

#include "BuiltinColourPalettes.h"

#include "ColourPaletteAdapter.h"
#include "ColourPaletteUtils.h"
#include "CptColourPalette.h"
#include "RasterColourPalette.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


namespace GPlatesGui
{
	namespace BuiltinColourPalettes
	{
		namespace
		{
			//! The age CPT file.
			const QString AGE_CPT_FILE_NAME = ":/age.cpt";


			/*

				#
				# Python script to convert ColorBrewer (www.ColorBrewer.org) colour schemes into source code for GPlates (below).
				#
				# The colour values can be copied from https://github.com/axismaps/colorbrewer/blob/master/colorbrewer_schemes.js
				# into the Python dict variables 'sequential_colour_dict' and 'diverging_colour_dict' below with just a few minor changes.
				#

				# Sequential colours.
				#
				# The following shows the dict structure. The '...' parts need to be filled in.
				#
				sequential_colour_dict = {
					'OrRd' : {3: [(254,232,200), (253,187,132), (227,74,51)], 4: [(254,240,217), ... },
					'PuBu' :  {3: [(236,231,242), (166,189,219), (43,140,190)], 4: [(241,238,246), ... },
					...
				}

				# Diverging colours.
				#
				# The following shows the dict structure. The '...' parts need to be filled in.
				#
				diverging_colour_dict = {
					'Spectral' :  {3: [(252,141,89), (255,255,191), (153,213,148)], 4: [(215,25,28), ... },
					'RdYlGn' :  {3: [(252,141,89), (255,255,191), (145,207,96)], 4: [(215,25,28), .. },
					...
				}

				sequential_classes_dict = {
					3 : 'ThreeSequentialClasses',
					4 : 'FourSequentialClasses',
					5 : 'FiveSequentialClasses',
					6 : 'SixSequentialClasses',
					7 : 'SevenSequentialClasses',
					8 : 'EightSequentialClasses',
					9 : 'NineSequentialClasses'
				}

				diverging_classes_dict = {
					3  : 'ThreeDivergingClasses',
					4  : 'FourDivergingClasses',
					5  : 'FiveDivergingClasses',
					6  : 'SixDivergingClasses',
					7  : 'SevenDivergingClasses',
					8  : 'EightDivergingClasses',
					9  : 'NineDivergingClasses',
					10 : 'TenDivergingClasses',
					11 : 'ElevenDivergingClasses'
				}

				# Print out the C++ lines to go into GPlates (below).
				# These are lines that look like:
				#
				#     const QColor coloursReds3[3] = { QColor(254,224,210), QColor(252,146,114), QColor(222,45,38) };
				#     sequential_map[std::make_pair(Reds, ThreeSequentialClasses)] = value_type(coloursReds3, coloursReds3 + 3);
				# 
				# ...and...
				# 
				#     const QColor coloursSpectral3[3] = { QColor(252,141,89), QColor(255,255,191), QColor(153,213,148) };
				#     diverging_map[std::make_pair(Spectral, ThreeDivergingClasses)] = value_type(coloursSpectral3, coloursSpectral3 + 3);
				#
				for colour_dict, classes_dict, palette_type_str in ((sequential_colour_dict, sequential_classes_dict, 'sequential'), (diverging_colour_dict, diverging_classes_dict, 'diverging')):
					for colour_name, colour_name_dict in colour_dict.iteritems():
						for classes, colours in colour_name_dict.iteritems():
							text1 = 'const QColor colours{0}{1}[{1}] = {{ '.format(colour_name, classes)
							for colour_index in range(len(colours)):
								colour = colours[colour_index]
								text1 += 'QColor({0},{1},{2})'.format(*colour)
								if colour_index != len(colours) - 1:
									text1 += ', '
							text1 += ' };'
							print text1
				            
							text2 = '{3}_map[std::make_pair({0}, {2})] = value_type(colours{0}{1}, colours{0}{1} + {1});'.format(colour_name, classes, classes_dict[classes], palette_type_str)
							print text2
						print
					print
					print
					print

			*/

			/**
			 * Return the ColorBrewer sequential colours of the specified sequential type and number of classes.
			 *
			 * Colors from www.ColorBrewer.org by Cynthia A. Brewer, Geography, Pennsylvania State University.
			 */
			const std::vector<Colour> &
			get_colorbrewer_sequential_colours(
					ColorBrewerSequentialType sequential_type,
					ColorBrewerSequentialClasses sequential_classes)
			{
				typedef std::pair<ColorBrewerSequentialType, ColorBrewerSequentialClasses> key_type;
				typedef std::vector<Colour> value_type;
				typedef std::map<key_type, value_type> map_type;

				static map_type sequential_map;

				// Initialise static map the first time this function is called.
				static bool sequential_map_initialised = false;
				if (!sequential_map_initialised)
				{
					//
					// Colors from www.ColorBrewer.org by Cynthia A. Brewer, Geography, Pennsylvania State University.
					//
					// This source code was obtained using the Python script above.
					//

                    const QColor coloursReds3[3] = { QColor(254,224,210), QColor(252,146,114), QColor(222,45,38) };
                    sequential_map[std::make_pair(Reds, ThreeSequentialClasses)] = value_type(coloursReds3, coloursReds3 + 3);
                    const QColor coloursReds4[4] = { QColor(254,229,217), QColor(252,174,145), QColor(251,106,74), QColor(203,24,29) };
                    sequential_map[std::make_pair(Reds, FourSequentialClasses)] = value_type(coloursReds4, coloursReds4 + 4);
                    const QColor coloursReds5[5] = { QColor(254,229,217), QColor(252,174,145), QColor(251,106,74), QColor(222,45,38), QColor(165,15,21) };
                    sequential_map[std::make_pair(Reds, FiveSequentialClasses)] = value_type(coloursReds5, coloursReds5 + 5);
                    const QColor coloursReds6[6] = { QColor(254,229,217), QColor(252,187,161), QColor(252,146,114), QColor(251,106,74), QColor(222,45,38), QColor(165,15,21) };
                    sequential_map[std::make_pair(Reds, SixSequentialClasses)] = value_type(coloursReds6, coloursReds6 + 6);
                    const QColor coloursReds7[7] = { QColor(254,229,217), QColor(252,187,161), QColor(252,146,114), QColor(251,106,74), QColor(239,59,44), QColor(203,24,29), QColor(153,0,13) };
                    sequential_map[std::make_pair(Reds, SevenSequentialClasses)] = value_type(coloursReds7, coloursReds7 + 7);
                    const QColor coloursReds8[8] = { QColor(255,245,240), QColor(254,224,210), QColor(252,187,161), QColor(252,146,114), QColor(251,106,74), QColor(239,59,44), QColor(203,24,29), QColor(153,0,13) };
                    sequential_map[std::make_pair(Reds, EightSequentialClasses)] = value_type(coloursReds8, coloursReds8 + 8);
                    const QColor coloursReds9[9] = { QColor(255,245,240), QColor(254,224,210), QColor(252,187,161), QColor(252,146,114), QColor(251,106,74), QColor(239,59,44), QColor(203,24,29), QColor(165,15,21), QColor(103,0,13) };
                    sequential_map[std::make_pair(Reds, NineSequentialClasses)] = value_type(coloursReds9, coloursReds9 + 9);

                    const QColor coloursYlOrRd3[3] = { QColor(255,237,160), QColor(254,178,76), QColor(240,59,32) };
                    sequential_map[std::make_pair(YlOrRd, ThreeSequentialClasses)] = value_type(coloursYlOrRd3, coloursYlOrRd3 + 3);
                    const QColor coloursYlOrRd4[4] = { QColor(255,255,178), QColor(254,204,92), QColor(253,141,60), QColor(227,26,28) };
                    sequential_map[std::make_pair(YlOrRd, FourSequentialClasses)] = value_type(coloursYlOrRd4, coloursYlOrRd4 + 4);
                    const QColor coloursYlOrRd5[5] = { QColor(255,255,178), QColor(254,204,92), QColor(253,141,60), QColor(240,59,32), QColor(189,0,38) };
                    sequential_map[std::make_pair(YlOrRd, FiveSequentialClasses)] = value_type(coloursYlOrRd5, coloursYlOrRd5 + 5);
                    const QColor coloursYlOrRd6[6] = { QColor(255,255,178), QColor(254,217,118), QColor(254,178,76), QColor(253,141,60), QColor(240,59,32), QColor(189,0,38) };
                    sequential_map[std::make_pair(YlOrRd, SixSequentialClasses)] = value_type(coloursYlOrRd6, coloursYlOrRd6 + 6);
                    const QColor coloursYlOrRd7[7] = { QColor(255,255,178), QColor(254,217,118), QColor(254,178,76), QColor(253,141,60), QColor(252,78,42), QColor(227,26,28), QColor(177,0,38) };
                    sequential_map[std::make_pair(YlOrRd, SevenSequentialClasses)] = value_type(coloursYlOrRd7, coloursYlOrRd7 + 7);
                    const QColor coloursYlOrRd8[8] = { QColor(255,255,204), QColor(255,237,160), QColor(254,217,118), QColor(254,178,76), QColor(253,141,60), QColor(252,78,42), QColor(227,26,28), QColor(177,0,38) };
                    sequential_map[std::make_pair(YlOrRd, EightSequentialClasses)] = value_type(coloursYlOrRd8, coloursYlOrRd8 + 8);
                    const QColor coloursYlOrRd9[9] = { QColor(255,255,204), QColor(255,237,160), QColor(254,217,118), QColor(254,178,76), QColor(253,141,60), QColor(252,78,42), QColor(227,26,28), QColor(189,0,38), QColor(128,0,38) };
                    sequential_map[std::make_pair(YlOrRd, NineSequentialClasses)] = value_type(coloursYlOrRd9, coloursYlOrRd9 + 9);

                    const QColor coloursRdPu3[3] = { QColor(253,224,221), QColor(250,159,181), QColor(197,27,138) };
                    sequential_map[std::make_pair(RdPu, ThreeSequentialClasses)] = value_type(coloursRdPu3, coloursRdPu3 + 3);
                    const QColor coloursRdPu4[4] = { QColor(254,235,226), QColor(251,180,185), QColor(247,104,161), QColor(174,1,126) };
                    sequential_map[std::make_pair(RdPu, FourSequentialClasses)] = value_type(coloursRdPu4, coloursRdPu4 + 4);
                    const QColor coloursRdPu5[5] = { QColor(254,235,226), QColor(251,180,185), QColor(247,104,161), QColor(197,27,138), QColor(122,1,119) };
                    sequential_map[std::make_pair(RdPu, FiveSequentialClasses)] = value_type(coloursRdPu5, coloursRdPu5 + 5);
                    const QColor coloursRdPu6[6] = { QColor(254,235,226), QColor(252,197,192), QColor(250,159,181), QColor(247,104,161), QColor(197,27,138), QColor(122,1,119) };
                    sequential_map[std::make_pair(RdPu, SixSequentialClasses)] = value_type(coloursRdPu6, coloursRdPu6 + 6);
                    const QColor coloursRdPu7[7] = { QColor(254,235,226), QColor(252,197,192), QColor(250,159,181), QColor(247,104,161), QColor(221,52,151), QColor(174,1,126), QColor(122,1,119) };
                    sequential_map[std::make_pair(RdPu, SevenSequentialClasses)] = value_type(coloursRdPu7, coloursRdPu7 + 7);
                    const QColor coloursRdPu8[8] = { QColor(255,247,243), QColor(253,224,221), QColor(252,197,192), QColor(250,159,181), QColor(247,104,161), QColor(221,52,151), QColor(174,1,126), QColor(122,1,119) };
                    sequential_map[std::make_pair(RdPu, EightSequentialClasses)] = value_type(coloursRdPu8, coloursRdPu8 + 8);
                    const QColor coloursRdPu9[9] = { QColor(255,247,243), QColor(253,224,221), QColor(252,197,192), QColor(250,159,181), QColor(247,104,161), QColor(221,52,151), QColor(174,1,126), QColor(122,1,119), QColor(73,0,106) };
                    sequential_map[std::make_pair(RdPu, NineSequentialClasses)] = value_type(coloursRdPu9, coloursRdPu9 + 9);

                    const QColor coloursYlOrBr3[3] = { QColor(255,247,188), QColor(254,196,79), QColor(217,95,14) };
                    sequential_map[std::make_pair(YlOrBr, ThreeSequentialClasses)] = value_type(coloursYlOrBr3, coloursYlOrBr3 + 3);
                    const QColor coloursYlOrBr4[4] = { QColor(255,255,212), QColor(254,217,142), QColor(254,153,41), QColor(204,76,2) };
                    sequential_map[std::make_pair(YlOrBr, FourSequentialClasses)] = value_type(coloursYlOrBr4, coloursYlOrBr4 + 4);
                    const QColor coloursYlOrBr5[5] = { QColor(255,255,212), QColor(254,217,142), QColor(254,153,41), QColor(217,95,14), QColor(153,52,4) };
                    sequential_map[std::make_pair(YlOrBr, FiveSequentialClasses)] = value_type(coloursYlOrBr5, coloursYlOrBr5 + 5);
                    const QColor coloursYlOrBr6[6] = { QColor(255,255,212), QColor(254,227,145), QColor(254,196,79), QColor(254,153,41), QColor(217,95,14), QColor(153,52,4) };
                    sequential_map[std::make_pair(YlOrBr, SixSequentialClasses)] = value_type(coloursYlOrBr6, coloursYlOrBr6 + 6);
                    const QColor coloursYlOrBr7[7] = { QColor(255,255,212), QColor(254,227,145), QColor(254,196,79), QColor(254,153,41), QColor(236,112,20), QColor(204,76,2), QColor(140,45,4) };
                    sequential_map[std::make_pair(YlOrBr, SevenSequentialClasses)] = value_type(coloursYlOrBr7, coloursYlOrBr7 + 7);
                    const QColor coloursYlOrBr8[8] = { QColor(255,255,229), QColor(255,247,188), QColor(254,227,145), QColor(254,196,79), QColor(254,153,41), QColor(236,112,20), QColor(204,76,2), QColor(140,45,4) };
                    sequential_map[std::make_pair(YlOrBr, EightSequentialClasses)] = value_type(coloursYlOrBr8, coloursYlOrBr8 + 8);
                    const QColor coloursYlOrBr9[9] = { QColor(255,255,229), QColor(255,247,188), QColor(254,227,145), QColor(254,196,79), QColor(254,153,41), QColor(236,112,20), QColor(204,76,2), QColor(153,52,4), QColor(102,37,6) };
                    sequential_map[std::make_pair(YlOrBr, NineSequentialClasses)] = value_type(coloursYlOrBr9, coloursYlOrBr9 + 9);

                    const QColor coloursGreens3[3] = { QColor(229,245,224), QColor(161,217,155), QColor(49,163,84) };
                    sequential_map[std::make_pair(Greens, ThreeSequentialClasses)] = value_type(coloursGreens3, coloursGreens3 + 3);
                    const QColor coloursGreens4[4] = { QColor(237,248,233), QColor(186,228,179), QColor(116,196,118), QColor(35,139,69) };
                    sequential_map[std::make_pair(Greens, FourSequentialClasses)] = value_type(coloursGreens4, coloursGreens4 + 4);
                    const QColor coloursGreens5[5] = { QColor(237,248,233), QColor(186,228,179), QColor(116,196,118), QColor(49,163,84), QColor(0,109,44) };
                    sequential_map[std::make_pair(Greens, FiveSequentialClasses)] = value_type(coloursGreens5, coloursGreens5 + 5);
                    const QColor coloursGreens6[6] = { QColor(237,248,233), QColor(199,233,192), QColor(161,217,155), QColor(116,196,118), QColor(49,163,84), QColor(0,109,44) };
                    sequential_map[std::make_pair(Greens, SixSequentialClasses)] = value_type(coloursGreens6, coloursGreens6 + 6);
                    const QColor coloursGreens7[7] = { QColor(237,248,233), QColor(199,233,192), QColor(161,217,155), QColor(116,196,118), QColor(65,171,93), QColor(35,139,69), QColor(0,90,50) };
                    sequential_map[std::make_pair(Greens, SevenSequentialClasses)] = value_type(coloursGreens7, coloursGreens7 + 7);
                    const QColor coloursGreens8[8] = { QColor(247,252,245), QColor(229,245,224), QColor(199,233,192), QColor(161,217,155), QColor(116,196,118), QColor(65,171,93), QColor(35,139,69), QColor(0,90,50) };
                    sequential_map[std::make_pair(Greens, EightSequentialClasses)] = value_type(coloursGreens8, coloursGreens8 + 8);
                    const QColor coloursGreens9[9] = { QColor(247,252,245), QColor(229,245,224), QColor(199,233,192), QColor(161,217,155), QColor(116,196,118), QColor(65,171,93), QColor(35,139,69), QColor(0,109,44), QColor(0,68,27) };
                    sequential_map[std::make_pair(Greens, NineSequentialClasses)] = value_type(coloursGreens9, coloursGreens9 + 9);

                    const QColor coloursGnBu3[3] = { QColor(224,243,219), QColor(168,221,181), QColor(67,162,202) };
                    sequential_map[std::make_pair(GnBu, ThreeSequentialClasses)] = value_type(coloursGnBu3, coloursGnBu3 + 3);
                    const QColor coloursGnBu4[4] = { QColor(240,249,232), QColor(186,228,188), QColor(123,204,196), QColor(43,140,190) };
                    sequential_map[std::make_pair(GnBu, FourSequentialClasses)] = value_type(coloursGnBu4, coloursGnBu4 + 4);
                    const QColor coloursGnBu5[5] = { QColor(240,249,232), QColor(186,228,188), QColor(123,204,196), QColor(67,162,202), QColor(8,104,172) };
                    sequential_map[std::make_pair(GnBu, FiveSequentialClasses)] = value_type(coloursGnBu5, coloursGnBu5 + 5);
                    const QColor coloursGnBu6[6] = { QColor(240,249,232), QColor(204,235,197), QColor(168,221,181), QColor(123,204,196), QColor(67,162,202), QColor(8,104,172) };
                    sequential_map[std::make_pair(GnBu, SixSequentialClasses)] = value_type(coloursGnBu6, coloursGnBu6 + 6);
                    const QColor coloursGnBu7[7] = { QColor(240,249,232), QColor(204,235,197), QColor(168,221,181), QColor(123,204,196), QColor(78,179,211), QColor(43,140,190), QColor(8,88,158) };
                    sequential_map[std::make_pair(GnBu, SevenSequentialClasses)] = value_type(coloursGnBu7, coloursGnBu7 + 7);
                    const QColor coloursGnBu8[8] = { QColor(247,252,240), QColor(224,243,219), QColor(204,235,197), QColor(168,221,181), QColor(123,204,196), QColor(78,179,211), QColor(43,140,190), QColor(8,88,158) };
                    sequential_map[std::make_pair(GnBu, EightSequentialClasses)] = value_type(coloursGnBu8, coloursGnBu8 + 8);
                    const QColor coloursGnBu9[9] = { QColor(247,252,240), QColor(224,243,219), QColor(204,235,197), QColor(168,221,181), QColor(123,204,196), QColor(78,179,211), QColor(43,140,190), QColor(8,104,172), QColor(8,64,129) };
                    sequential_map[std::make_pair(GnBu, NineSequentialClasses)] = value_type(coloursGnBu9, coloursGnBu9 + 9);

                    const QColor coloursBuPu3[3] = { QColor(224,236,244), QColor(158,188,218), QColor(136,86,167) };
                    sequential_map[std::make_pair(BuPu, ThreeSequentialClasses)] = value_type(coloursBuPu3, coloursBuPu3 + 3);
                    const QColor coloursBuPu4[4] = { QColor(237,248,251), QColor(179,205,227), QColor(140,150,198), QColor(136,65,157) };
                    sequential_map[std::make_pair(BuPu, FourSequentialClasses)] = value_type(coloursBuPu4, coloursBuPu4 + 4);
                    const QColor coloursBuPu5[5] = { QColor(237,248,251), QColor(179,205,227), QColor(140,150,198), QColor(136,86,167), QColor(129,15,124) };
                    sequential_map[std::make_pair(BuPu, FiveSequentialClasses)] = value_type(coloursBuPu5, coloursBuPu5 + 5);
                    const QColor coloursBuPu6[6] = { QColor(237,248,251), QColor(191,211,230), QColor(158,188,218), QColor(140,150,198), QColor(136,86,167), QColor(129,15,124) };
                    sequential_map[std::make_pair(BuPu, SixSequentialClasses)] = value_type(coloursBuPu6, coloursBuPu6 + 6);
                    const QColor coloursBuPu7[7] = { QColor(237,248,251), QColor(191,211,230), QColor(158,188,218), QColor(140,150,198), QColor(140,107,177), QColor(136,65,157), QColor(110,1,107) };
                    sequential_map[std::make_pair(BuPu, SevenSequentialClasses)] = value_type(coloursBuPu7, coloursBuPu7 + 7);
                    const QColor coloursBuPu8[8] = { QColor(247,252,253), QColor(224,236,244), QColor(191,211,230), QColor(158,188,218), QColor(140,150,198), QColor(140,107,177), QColor(136,65,157), QColor(110,1,107) };
                    sequential_map[std::make_pair(BuPu, EightSequentialClasses)] = value_type(coloursBuPu8, coloursBuPu8 + 8);
                    const QColor coloursBuPu9[9] = { QColor(247,252,253), QColor(224,236,244), QColor(191,211,230), QColor(158,188,218), QColor(140,150,198), QColor(140,107,177), QColor(136,65,157), QColor(129,15,124), QColor(77,0,75) };
                    sequential_map[std::make_pair(BuPu, NineSequentialClasses)] = value_type(coloursBuPu9, coloursBuPu9 + 9);

                    const QColor coloursOrRd3[3] = { QColor(254,232,200), QColor(253,187,132), QColor(227,74,51) };
                    sequential_map[std::make_pair(OrRd, ThreeSequentialClasses)] = value_type(coloursOrRd3, coloursOrRd3 + 3);
                    const QColor coloursOrRd4[4] = { QColor(254,240,217), QColor(253,204,138), QColor(252,141,89), QColor(215,48,31) };
                    sequential_map[std::make_pair(OrRd, FourSequentialClasses)] = value_type(coloursOrRd4, coloursOrRd4 + 4);
                    const QColor coloursOrRd5[5] = { QColor(254,240,217), QColor(253,204,138), QColor(252,141,89), QColor(227,74,51), QColor(179,0,0) };
                    sequential_map[std::make_pair(OrRd, FiveSequentialClasses)] = value_type(coloursOrRd5, coloursOrRd5 + 5);
                    const QColor coloursOrRd6[6] = { QColor(254,240,217), QColor(253,212,158), QColor(253,187,132), QColor(252,141,89), QColor(227,74,51), QColor(179,0,0) };
                    sequential_map[std::make_pair(OrRd, SixSequentialClasses)] = value_type(coloursOrRd6, coloursOrRd6 + 6);
                    const QColor coloursOrRd7[7] = { QColor(254,240,217), QColor(253,212,158), QColor(253,187,132), QColor(252,141,89), QColor(239,101,72), QColor(215,48,31), QColor(153,0,0) };
                    sequential_map[std::make_pair(OrRd, SevenSequentialClasses)] = value_type(coloursOrRd7, coloursOrRd7 + 7);
                    const QColor coloursOrRd8[8] = { QColor(255,247,236), QColor(254,232,200), QColor(253,212,158), QColor(253,187,132), QColor(252,141,89), QColor(239,101,72), QColor(215,48,31), QColor(153,0,0) };
                    sequential_map[std::make_pair(OrRd, EightSequentialClasses)] = value_type(coloursOrRd8, coloursOrRd8 + 8);
                    const QColor coloursOrRd9[9] = { QColor(255,247,236), QColor(254,232,200), QColor(253,212,158), QColor(253,187,132), QColor(252,141,89), QColor(239,101,72), QColor(215,48,31), QColor(179,0,0), QColor(127,0,0) };
                    sequential_map[std::make_pair(OrRd, NineSequentialClasses)] = value_type(coloursOrRd9, coloursOrRd9 + 9);

                    const QColor coloursOranges3[3] = { QColor(254,230,206), QColor(253,174,107), QColor(230,85,13) };
                    sequential_map[std::make_pair(Oranges, ThreeSequentialClasses)] = value_type(coloursOranges3, coloursOranges3 + 3);
                    const QColor coloursOranges4[4] = { QColor(254,237,222), QColor(253,190,133), QColor(253,141,60), QColor(217,71,1) };
                    sequential_map[std::make_pair(Oranges, FourSequentialClasses)] = value_type(coloursOranges4, coloursOranges4 + 4);
                    const QColor coloursOranges5[5] = { QColor(254,237,222), QColor(253,190,133), QColor(253,141,60), QColor(230,85,13), QColor(166,54,3) };
                    sequential_map[std::make_pair(Oranges, FiveSequentialClasses)] = value_type(coloursOranges5, coloursOranges5 + 5);
                    const QColor coloursOranges6[6] = { QColor(254,237,222), QColor(253,208,162), QColor(253,174,107), QColor(253,141,60), QColor(230,85,13), QColor(166,54,3) };
                    sequential_map[std::make_pair(Oranges, SixSequentialClasses)] = value_type(coloursOranges6, coloursOranges6 + 6);
                    const QColor coloursOranges7[7] = { QColor(254,237,222), QColor(253,208,162), QColor(253,174,107), QColor(253,141,60), QColor(241,105,19), QColor(217,72,1), QColor(140,45,4) };
                    sequential_map[std::make_pair(Oranges, SevenSequentialClasses)] = value_type(coloursOranges7, coloursOranges7 + 7);
                    const QColor coloursOranges8[8] = { QColor(255,245,235), QColor(254,230,206), QColor(253,208,162), QColor(253,174,107), QColor(253,141,60), QColor(241,105,19), QColor(217,72,1), QColor(140,45,4) };
                    sequential_map[std::make_pair(Oranges, EightSequentialClasses)] = value_type(coloursOranges8, coloursOranges8 + 8);
                    const QColor coloursOranges9[9] = { QColor(255,245,235), QColor(254,230,206), QColor(253,208,162), QColor(253,174,107), QColor(253,141,60), QColor(241,105,19), QColor(217,72,1), QColor(166,54,3), QColor(127,39,4) };
                    sequential_map[std::make_pair(Oranges, NineSequentialClasses)] = value_type(coloursOranges9, coloursOranges9 + 9);

                    const QColor coloursYlGnBu3[3] = { QColor(237,248,177), QColor(127,205,187), QColor(44,127,184) };
                    sequential_map[std::make_pair(YlGnBu, ThreeSequentialClasses)] = value_type(coloursYlGnBu3, coloursYlGnBu3 + 3);
                    const QColor coloursYlGnBu4[4] = { QColor(255,255,204), QColor(161,218,180), QColor(65,182,196), QColor(34,94,168) };
                    sequential_map[std::make_pair(YlGnBu, FourSequentialClasses)] = value_type(coloursYlGnBu4, coloursYlGnBu4 + 4);
                    const QColor coloursYlGnBu5[5] = { QColor(255,255,204), QColor(161,218,180), QColor(65,182,196), QColor(44,127,184), QColor(37,52,148) };
                    sequential_map[std::make_pair(YlGnBu, FiveSequentialClasses)] = value_type(coloursYlGnBu5, coloursYlGnBu5 + 5);
                    const QColor coloursYlGnBu6[6] = { QColor(255,255,204), QColor(199,233,180), QColor(127,205,187), QColor(65,182,196), QColor(44,127,184), QColor(37,52,148) };
                    sequential_map[std::make_pair(YlGnBu, SixSequentialClasses)] = value_type(coloursYlGnBu6, coloursYlGnBu6 + 6);
                    const QColor coloursYlGnBu7[7] = { QColor(255,255,204), QColor(199,233,180), QColor(127,205,187), QColor(65,182,196), QColor(29,145,192), QColor(34,94,168), QColor(12,44,132) };
                    sequential_map[std::make_pair(YlGnBu, SevenSequentialClasses)] = value_type(coloursYlGnBu7, coloursYlGnBu7 + 7);
                    const QColor coloursYlGnBu8[8] = { QColor(255,255,217), QColor(237,248,177), QColor(199,233,180), QColor(127,205,187), QColor(65,182,196), QColor(29,145,192), QColor(34,94,168), QColor(12,44,132) };
                    sequential_map[std::make_pair(YlGnBu, EightSequentialClasses)] = value_type(coloursYlGnBu8, coloursYlGnBu8 + 8);
                    const QColor coloursYlGnBu9[9] = { QColor(255,255,217), QColor(237,248,177), QColor(199,233,180), QColor(127,205,187), QColor(65,182,196), QColor(29,145,192), QColor(34,94,168), QColor(37,52,148), QColor(8,29,88) };
                    sequential_map[std::make_pair(YlGnBu, NineSequentialClasses)] = value_type(coloursYlGnBu9, coloursYlGnBu9 + 9);

                    const QColor coloursBuGn3[3] = { QColor(229,245,249), QColor(153,216,201), QColor(44,162,95) };
                    sequential_map[std::make_pair(BuGn, ThreeSequentialClasses)] = value_type(coloursBuGn3, coloursBuGn3 + 3);
                    const QColor coloursBuGn4[4] = { QColor(237,248,251), QColor(178,226,226), QColor(102,194,164), QColor(35,139,69) };
                    sequential_map[std::make_pair(BuGn, FourSequentialClasses)] = value_type(coloursBuGn4, coloursBuGn4 + 4);
                    const QColor coloursBuGn5[5] = { QColor(237,248,251), QColor(178,226,226), QColor(102,194,164), QColor(44,162,95), QColor(0,109,44) };
                    sequential_map[std::make_pair(BuGn, FiveSequentialClasses)] = value_type(coloursBuGn5, coloursBuGn5 + 5);
                    const QColor coloursBuGn6[6] = { QColor(237,248,251), QColor(204,236,230), QColor(153,216,201), QColor(102,194,164), QColor(44,162,95), QColor(0,109,44) };
                    sequential_map[std::make_pair(BuGn, SixSequentialClasses)] = value_type(coloursBuGn6, coloursBuGn6 + 6);
                    const QColor coloursBuGn7[7] = { QColor(237,248,251), QColor(204,236,230), QColor(153,216,201), QColor(102,194,164), QColor(65,174,118), QColor(35,139,69), QColor(0,88,36) };
                    sequential_map[std::make_pair(BuGn, SevenSequentialClasses)] = value_type(coloursBuGn7, coloursBuGn7 + 7);
                    const QColor coloursBuGn8[8] = { QColor(247,252,253), QColor(229,245,249), QColor(204,236,230), QColor(153,216,201), QColor(102,194,164), QColor(65,174,118), QColor(35,139,69), QColor(0,88,36) };
                    sequential_map[std::make_pair(BuGn, EightSequentialClasses)] = value_type(coloursBuGn8, coloursBuGn8 + 8);
                    const QColor coloursBuGn9[9] = { QColor(247,252,253), QColor(229,245,249), QColor(204,236,230), QColor(153,216,201), QColor(102,194,164), QColor(65,174,118), QColor(35,139,69), QColor(0,109,44), QColor(0,68,27) };
                    sequential_map[std::make_pair(BuGn, NineSequentialClasses)] = value_type(coloursBuGn9, coloursBuGn9 + 9);

                    const QColor coloursPuBu3[3] = { QColor(236,231,242), QColor(166,189,219), QColor(43,140,190) };
                    sequential_map[std::make_pair(PuBu, ThreeSequentialClasses)] = value_type(coloursPuBu3, coloursPuBu3 + 3);
                    const QColor coloursPuBu4[4] = { QColor(241,238,246), QColor(189,201,225), QColor(116,169,207), QColor(5,112,176) };
                    sequential_map[std::make_pair(PuBu, FourSequentialClasses)] = value_type(coloursPuBu4, coloursPuBu4 + 4);
                    const QColor coloursPuBu5[5] = { QColor(241,238,246), QColor(189,201,225), QColor(116,169,207), QColor(43,140,190), QColor(4,90,141) };
                    sequential_map[std::make_pair(PuBu, FiveSequentialClasses)] = value_type(coloursPuBu5, coloursPuBu5 + 5);
                    const QColor coloursPuBu6[6] = { QColor(241,238,246), QColor(208,209,230), QColor(166,189,219), QColor(116,169,207), QColor(43,140,190), QColor(4,90,141) };
                    sequential_map[std::make_pair(PuBu, SixSequentialClasses)] = value_type(coloursPuBu6, coloursPuBu6 + 6);
                    const QColor coloursPuBu7[7] = { QColor(241,238,246), QColor(208,209,230), QColor(166,189,219), QColor(116,169,207), QColor(54,144,192), QColor(5,112,176), QColor(3,78,123) };
                    sequential_map[std::make_pair(PuBu, SevenSequentialClasses)] = value_type(coloursPuBu7, coloursPuBu7 + 7);
                    const QColor coloursPuBu8[8] = { QColor(255,247,251), QColor(236,231,242), QColor(208,209,230), QColor(166,189,219), QColor(116,169,207), QColor(54,144,192), QColor(5,112,176), QColor(3,78,123) };
                    sequential_map[std::make_pair(PuBu, EightSequentialClasses)] = value_type(coloursPuBu8, coloursPuBu8 + 8);
                    const QColor coloursPuBu9[9] = { QColor(255,247,251), QColor(236,231,242), QColor(208,209,230), QColor(166,189,219), QColor(116,169,207), QColor(54,144,192), QColor(5,112,176), QColor(4,90,141), QColor(2,56,88) };
                    sequential_map[std::make_pair(PuBu, NineSequentialClasses)] = value_type(coloursPuBu9, coloursPuBu9 + 9);

                    const QColor coloursPuRd3[3] = { QColor(231,225,239), QColor(201,148,199), QColor(221,28,119) };
                    sequential_map[std::make_pair(PuRd, ThreeSequentialClasses)] = value_type(coloursPuRd3, coloursPuRd3 + 3);
                    const QColor coloursPuRd4[4] = { QColor(241,238,246), QColor(215,181,216), QColor(223,101,176), QColor(206,18,86) };
                    sequential_map[std::make_pair(PuRd, FourSequentialClasses)] = value_type(coloursPuRd4, coloursPuRd4 + 4);
                    const QColor coloursPuRd5[5] = { QColor(241,238,246), QColor(215,181,216), QColor(223,101,176), QColor(221,28,119), QColor(152,0,67) };
                    sequential_map[std::make_pair(PuRd, FiveSequentialClasses)] = value_type(coloursPuRd5, coloursPuRd5 + 5);
                    const QColor coloursPuRd6[6] = { QColor(241,238,246), QColor(212,185,218), QColor(201,148,199), QColor(223,101,176), QColor(221,28,119), QColor(152,0,67) };
                    sequential_map[std::make_pair(PuRd, SixSequentialClasses)] = value_type(coloursPuRd6, coloursPuRd6 + 6);
                    const QColor coloursPuRd7[7] = { QColor(241,238,246), QColor(212,185,218), QColor(201,148,199), QColor(223,101,176), QColor(231,41,138), QColor(206,18,86), QColor(145,0,63) };
                    sequential_map[std::make_pair(PuRd, SevenSequentialClasses)] = value_type(coloursPuRd7, coloursPuRd7 + 7);
                    const QColor coloursPuRd8[8] = { QColor(247,244,249), QColor(231,225,239), QColor(212,185,218), QColor(201,148,199), QColor(223,101,176), QColor(231,41,138), QColor(206,18,86), QColor(145,0,63) };
                    sequential_map[std::make_pair(PuRd, EightSequentialClasses)] = value_type(coloursPuRd8, coloursPuRd8 + 8);
                    const QColor coloursPuRd9[9] = { QColor(247,244,249), QColor(231,225,239), QColor(212,185,218), QColor(201,148,199), QColor(223,101,176), QColor(231,41,138), QColor(206,18,86), QColor(152,0,67), QColor(103,0,31) };
                    sequential_map[std::make_pair(PuRd, NineSequentialClasses)] = value_type(coloursPuRd9, coloursPuRd9 + 9);

                    const QColor coloursPuBuGn3[3] = { QColor(236,226,240), QColor(166,189,219), QColor(28,144,153) };
                    sequential_map[std::make_pair(PuBuGn, ThreeSequentialClasses)] = value_type(coloursPuBuGn3, coloursPuBuGn3 + 3);
                    const QColor coloursPuBuGn4[4] = { QColor(246,239,247), QColor(189,201,225), QColor(103,169,207), QColor(2,129,138) };
                    sequential_map[std::make_pair(PuBuGn, FourSequentialClasses)] = value_type(coloursPuBuGn4, coloursPuBuGn4 + 4);
                    const QColor coloursPuBuGn5[5] = { QColor(246,239,247), QColor(189,201,225), QColor(103,169,207), QColor(28,144,153), QColor(1,108,89) };
                    sequential_map[std::make_pair(PuBuGn, FiveSequentialClasses)] = value_type(coloursPuBuGn5, coloursPuBuGn5 + 5);
                    const QColor coloursPuBuGn6[6] = { QColor(246,239,247), QColor(208,209,230), QColor(166,189,219), QColor(103,169,207), QColor(28,144,153), QColor(1,108,89) };
                    sequential_map[std::make_pair(PuBuGn, SixSequentialClasses)] = value_type(coloursPuBuGn6, coloursPuBuGn6 + 6);
                    const QColor coloursPuBuGn7[7] = { QColor(246,239,247), QColor(208,209,230), QColor(166,189,219), QColor(103,169,207), QColor(54,144,192), QColor(2,129,138), QColor(1,100,80) };
                    sequential_map[std::make_pair(PuBuGn, SevenSequentialClasses)] = value_type(coloursPuBuGn7, coloursPuBuGn7 + 7);
                    const QColor coloursPuBuGn8[8] = { QColor(255,247,251), QColor(236,226,240), QColor(208,209,230), QColor(166,189,219), QColor(103,169,207), QColor(54,144,192), QColor(2,129,138), QColor(1,100,80) };
                    sequential_map[std::make_pair(PuBuGn, EightSequentialClasses)] = value_type(coloursPuBuGn8, coloursPuBuGn8 + 8);
                    const QColor coloursPuBuGn9[9] = { QColor(255,247,251), QColor(236,226,240), QColor(208,209,230), QColor(166,189,219), QColor(103,169,207), QColor(54,144,192), QColor(2,129,138), QColor(1,108,89), QColor(1,70,54) };
                    sequential_map[std::make_pair(PuBuGn, NineSequentialClasses)] = value_type(coloursPuBuGn9, coloursPuBuGn9 + 9);

                    const QColor coloursBlues3[3] = { QColor(222,235,247), QColor(158,202,225), QColor(49,130,189) };
                    sequential_map[std::make_pair(Blues, ThreeSequentialClasses)] = value_type(coloursBlues3, coloursBlues3 + 3);
                    const QColor coloursBlues4[4] = { QColor(239,243,255), QColor(189,215,231), QColor(107,174,214), QColor(33,113,181) };
                    sequential_map[std::make_pair(Blues, FourSequentialClasses)] = value_type(coloursBlues4, coloursBlues4 + 4);
                    const QColor coloursBlues5[5] = { QColor(239,243,255), QColor(189,215,231), QColor(107,174,214), QColor(49,130,189), QColor(8,81,156) };
                    sequential_map[std::make_pair(Blues, FiveSequentialClasses)] = value_type(coloursBlues5, coloursBlues5 + 5);
                    const QColor coloursBlues6[6] = { QColor(239,243,255), QColor(198,219,239), QColor(158,202,225), QColor(107,174,214), QColor(49,130,189), QColor(8,81,156) };
                    sequential_map[std::make_pair(Blues, SixSequentialClasses)] = value_type(coloursBlues6, coloursBlues6 + 6);
                    const QColor coloursBlues7[7] = { QColor(239,243,255), QColor(198,219,239), QColor(158,202,225), QColor(107,174,214), QColor(66,146,198), QColor(33,113,181), QColor(8,69,148) };
                    sequential_map[std::make_pair(Blues, SevenSequentialClasses)] = value_type(coloursBlues7, coloursBlues7 + 7);
                    const QColor coloursBlues8[8] = { QColor(247,251,255), QColor(222,235,247), QColor(198,219,239), QColor(158,202,225), QColor(107,174,214), QColor(66,146,198), QColor(33,113,181), QColor(8,69,148) };
                    sequential_map[std::make_pair(Blues, EightSequentialClasses)] = value_type(coloursBlues8, coloursBlues8 + 8);
                    const QColor coloursBlues9[9] = { QColor(247,251,255), QColor(222,235,247), QColor(198,219,239), QColor(158,202,225), QColor(107,174,214), QColor(66,146,198), QColor(33,113,181), QColor(8,81,156), QColor(8,48,107) };
                    sequential_map[std::make_pair(Blues, NineSequentialClasses)] = value_type(coloursBlues9, coloursBlues9 + 9);

                    const QColor coloursGreys3[3] = { QColor(240,240,240), QColor(189,189,189), QColor(99,99,99) };
                    sequential_map[std::make_pair(Greys, ThreeSequentialClasses)] = value_type(coloursGreys3, coloursGreys3 + 3);
                    const QColor coloursGreys4[4] = { QColor(247,247,247), QColor(204,204,204), QColor(150,150,150), QColor(82,82,82) };
                    sequential_map[std::make_pair(Greys, FourSequentialClasses)] = value_type(coloursGreys4, coloursGreys4 + 4);
                    const QColor coloursGreys5[5] = { QColor(247,247,247), QColor(204,204,204), QColor(150,150,150), QColor(99,99,99), QColor(37,37,37) };
                    sequential_map[std::make_pair(Greys, FiveSequentialClasses)] = value_type(coloursGreys5, coloursGreys5 + 5);
                    const QColor coloursGreys6[6] = { QColor(247,247,247), QColor(217,217,217), QColor(189,189,189), QColor(150,150,150), QColor(99,99,99), QColor(37,37,37) };
                    sequential_map[std::make_pair(Greys, SixSequentialClasses)] = value_type(coloursGreys6, coloursGreys6 + 6);
                    const QColor coloursGreys7[7] = { QColor(247,247,247), QColor(217,217,217), QColor(189,189,189), QColor(150,150,150), QColor(115,115,115), QColor(82,82,82), QColor(37,37,37) };
                    sequential_map[std::make_pair(Greys, SevenSequentialClasses)] = value_type(coloursGreys7, coloursGreys7 + 7);
                    const QColor coloursGreys8[8] = { QColor(255,255,255), QColor(240,240,240), QColor(217,217,217), QColor(189,189,189), QColor(150,150,150), QColor(115,115,115), QColor(82,82,82), QColor(37,37,37) };
                    sequential_map[std::make_pair(Greys, EightSequentialClasses)] = value_type(coloursGreys8, coloursGreys8 + 8);
                    const QColor coloursGreys9[9] = { QColor(255,255,255), QColor(240,240,240), QColor(217,217,217), QColor(189,189,189), QColor(150,150,150), QColor(115,115,115), QColor(82,82,82), QColor(37,37,37), QColor(0,0,0) };
                    sequential_map[std::make_pair(Greys, NineSequentialClasses)] = value_type(coloursGreys9, coloursGreys9 + 9);

                    const QColor coloursYlGn3[3] = { QColor(247,252,185), QColor(173,221,142), QColor(49,163,84) };
                    sequential_map[std::make_pair(YlGn, ThreeSequentialClasses)] = value_type(coloursYlGn3, coloursYlGn3 + 3);
                    const QColor coloursYlGn4[4] = { QColor(255,255,204), QColor(194,230,153), QColor(120,198,121), QColor(35,132,67) };
                    sequential_map[std::make_pair(YlGn, FourSequentialClasses)] = value_type(coloursYlGn4, coloursYlGn4 + 4);
                    const QColor coloursYlGn5[5] = { QColor(255,255,204), QColor(194,230,153), QColor(120,198,121), QColor(49,163,84), QColor(0,104,55) };
                    sequential_map[std::make_pair(YlGn, FiveSequentialClasses)] = value_type(coloursYlGn5, coloursYlGn5 + 5);
                    const QColor coloursYlGn6[6] = { QColor(255,255,204), QColor(217,240,163), QColor(173,221,142), QColor(120,198,121), QColor(49,163,84), QColor(0,104,55) };
                    sequential_map[std::make_pair(YlGn, SixSequentialClasses)] = value_type(coloursYlGn6, coloursYlGn6 + 6);
                    const QColor coloursYlGn7[7] = { QColor(255,255,204), QColor(217,240,163), QColor(173,221,142), QColor(120,198,121), QColor(65,171,93), QColor(35,132,67), QColor(0,90,50) };
                    sequential_map[std::make_pair(YlGn, SevenSequentialClasses)] = value_type(coloursYlGn7, coloursYlGn7 + 7);
                    const QColor coloursYlGn8[8] = { QColor(255,255,229), QColor(247,252,185), QColor(217,240,163), QColor(173,221,142), QColor(120,198,121), QColor(65,171,93), QColor(35,132,67), QColor(0,90,50) };
                    sequential_map[std::make_pair(YlGn, EightSequentialClasses)] = value_type(coloursYlGn8, coloursYlGn8 + 8);
                    const QColor coloursYlGn9[9] = { QColor(255,255,229), QColor(247,252,185), QColor(217,240,163), QColor(173,221,142), QColor(120,198,121), QColor(65,171,93), QColor(35,132,67), QColor(0,104,55), QColor(0,69,41) };
                    sequential_map[std::make_pair(YlGn, NineSequentialClasses)] = value_type(coloursYlGn9, coloursYlGn9 + 9);

                    const QColor coloursPurples3[3] = { QColor(239,237,245), QColor(188,189,220), QColor(117,107,177) };
                    sequential_map[std::make_pair(Purples, ThreeSequentialClasses)] = value_type(coloursPurples3, coloursPurples3 + 3);
                    const QColor coloursPurples4[4] = { QColor(242,240,247), QColor(203,201,226), QColor(158,154,200), QColor(106,81,163) };
                    sequential_map[std::make_pair(Purples, FourSequentialClasses)] = value_type(coloursPurples4, coloursPurples4 + 4);
                    const QColor coloursPurples5[5] = { QColor(242,240,247), QColor(203,201,226), QColor(158,154,200), QColor(117,107,177), QColor(84,39,143) };
                    sequential_map[std::make_pair(Purples, FiveSequentialClasses)] = value_type(coloursPurples5, coloursPurples5 + 5);
                    const QColor coloursPurples6[6] = { QColor(242,240,247), QColor(218,218,235), QColor(188,189,220), QColor(158,154,200), QColor(117,107,177), QColor(84,39,143) };
                    sequential_map[std::make_pair(Purples, SixSequentialClasses)] = value_type(coloursPurples6, coloursPurples6 + 6);
                    const QColor coloursPurples7[7] = { QColor(242,240,247), QColor(218,218,235), QColor(188,189,220), QColor(158,154,200), QColor(128,125,186), QColor(106,81,163), QColor(74,20,134) };
                    sequential_map[std::make_pair(Purples, SevenSequentialClasses)] = value_type(coloursPurples7, coloursPurples7 + 7);
                    const QColor coloursPurples8[8] = { QColor(252,251,253), QColor(239,237,245), QColor(218,218,235), QColor(188,189,220), QColor(158,154,200), QColor(128,125,186), QColor(106,81,163), QColor(74,20,134) };
                    sequential_map[std::make_pair(Purples, EightSequentialClasses)] = value_type(coloursPurples8, coloursPurples8 + 8);
                    const QColor coloursPurples9[9] = { QColor(252,251,253), QColor(239,237,245), QColor(218,218,235), QColor(188,189,220), QColor(158,154,200), QColor(128,125,186), QColor(106,81,163), QColor(84,39,143), QColor(63,0,125) };
                    sequential_map[std::make_pair(Purples, NineSequentialClasses)] = value_type(coloursPurples9, coloursPurples9 + 9);

					sequential_map_initialised = true;
				}

				return sequential_map[std::make_pair(sequential_type, sequential_classes)];
			}


			/**
			 * Return the ColorBrewer diverging colours of the specified sequential type and number of classes.
			 *
			 * Colors from www.ColorBrewer.org by Cynthia A. Brewer, Geography, Pennsylvania State University.
			 */
			const std::vector<Colour> &
			get_colorbrewer_diverging_colours(
					ColorBrewerDivergingType diverging_type,
					ColorBrewerDivergingClasses diverging_classes)
			{
				typedef std::pair<ColorBrewerDivergingType, ColorBrewerDivergingClasses> key_type;
				typedef std::vector<Colour> value_type;
				typedef std::map<key_type, value_type> map_type;

				static map_type diverging_map;

				// Initialise static map the first time this function is called.
				static bool diverging_map_initialised = false;
				if (!diverging_map_initialised)
				{
					//
					// Colors from www.ColorBrewer.org by Cynthia A. Brewer, Geography, Pennsylvania State University.
					//
					// This source code was obtained using the Python script above.
					//

					const QColor coloursSpectral3[3] = { QColor(252,141,89), QColor(255,255,191), QColor(153,213,148) };
					diverging_map[std::make_pair(Spectral, ThreeDivergingClasses)] = value_type(coloursSpectral3, coloursSpectral3 + 3);
					const QColor coloursSpectral4[4] = { QColor(215,25,28), QColor(253,174,97), QColor(171,221,164), QColor(43,131,186) };
					diverging_map[std::make_pair(Spectral, FourDivergingClasses)] = value_type(coloursSpectral4, coloursSpectral4 + 4);
					const QColor coloursSpectral5[5] = { QColor(215,25,28), QColor(253,174,97), QColor(255,255,191), QColor(171,221,164), QColor(43,131,186) };
					diverging_map[std::make_pair(Spectral, FiveDivergingClasses)] = value_type(coloursSpectral5, coloursSpectral5 + 5);
					const QColor coloursSpectral6[6] = { QColor(213,62,79), QColor(252,141,89), QColor(254,224,139), QColor(230,245,152), QColor(153,213,148), QColor(50,136,189) };
					diverging_map[std::make_pair(Spectral, SixDivergingClasses)] = value_type(coloursSpectral6, coloursSpectral6 + 6);
					const QColor coloursSpectral7[7] = { QColor(213,62,79), QColor(252,141,89), QColor(254,224,139), QColor(255,255,191), QColor(230,245,152), QColor(153,213,148), QColor(50,136,189) };
					diverging_map[std::make_pair(Spectral, SevenDivergingClasses)] = value_type(coloursSpectral7, coloursSpectral7 + 7);
					const QColor coloursSpectral8[8] = { QColor(213,62,79), QColor(244,109,67), QColor(253,174,97), QColor(254,224,139), QColor(230,245,152), QColor(171,221,164), QColor(102,194,165), QColor(50,136,189) };
					diverging_map[std::make_pair(Spectral, EightDivergingClasses)] = value_type(coloursSpectral8, coloursSpectral8 + 8);
					const QColor coloursSpectral9[9] = { QColor(213,62,79), QColor(244,109,67), QColor(253,174,97), QColor(254,224,139), QColor(255,255,191), QColor(230,245,152), QColor(171,221,164), QColor(102,194,165), QColor(50,136,189) };
					diverging_map[std::make_pair(Spectral, NineDivergingClasses)] = value_type(coloursSpectral9, coloursSpectral9 + 9);
					const QColor coloursSpectral10[10] = { QColor(158,1,66), QColor(213,62,79), QColor(244,109,67), QColor(253,174,97), QColor(254,224,139), QColor(230,245,152), QColor(171,221,164), QColor(102,194,165), QColor(50,136,189), QColor(94,79,162) };
					diverging_map[std::make_pair(Spectral, TenDivergingClasses)] = value_type(coloursSpectral10, coloursSpectral10 + 10);
					const QColor coloursSpectral11[11] = { QColor(158,1,66), QColor(213,62,79), QColor(244,109,67), QColor(253,174,97), QColor(254,224,139), QColor(255,255,191), QColor(230,245,152), QColor(171,221,164), QColor(102,194,165), QColor(50,136,189), QColor(94,79,162) };
					diverging_map[std::make_pair(Spectral, ElevenDivergingClasses)] = value_type(coloursSpectral11, coloursSpectral11 + 11);

					const QColor coloursRdYlBu3[3] = { QColor(252,141,89), QColor(255,255,191), QColor(145,191,219) };
					diverging_map[std::make_pair(RdYlBu, ThreeDivergingClasses)] = value_type(coloursRdYlBu3, coloursRdYlBu3 + 3);
					const QColor coloursRdYlBu4[4] = { QColor(215,25,28), QColor(253,174,97), QColor(171,217,233), QColor(44,123,182) };
					diverging_map[std::make_pair(RdYlBu, FourDivergingClasses)] = value_type(coloursRdYlBu4, coloursRdYlBu4 + 4);
					const QColor coloursRdYlBu5[5] = { QColor(215,25,28), QColor(253,174,97), QColor(255,255,191), QColor(171,217,233), QColor(44,123,182) };
					diverging_map[std::make_pair(RdYlBu, FiveDivergingClasses)] = value_type(coloursRdYlBu5, coloursRdYlBu5 + 5);
					const QColor coloursRdYlBu6[6] = { QColor(215,48,39), QColor(252,141,89), QColor(254,224,144), QColor(224,243,248), QColor(145,191,219), QColor(69,117,180) };
					diverging_map[std::make_pair(RdYlBu, SixDivergingClasses)] = value_type(coloursRdYlBu6, coloursRdYlBu6 + 6);
					const QColor coloursRdYlBu7[7] = { QColor(215,48,39), QColor(252,141,89), QColor(254,224,144), QColor(255,255,191), QColor(224,243,248), QColor(145,191,219), QColor(69,117,180) };
					diverging_map[std::make_pair(RdYlBu, SevenDivergingClasses)] = value_type(coloursRdYlBu7, coloursRdYlBu7 + 7);
					const QColor coloursRdYlBu8[8] = { QColor(215,48,39), QColor(244,109,67), QColor(253,174,97), QColor(254,224,144), QColor(224,243,248), QColor(171,217,233), QColor(116,173,209), QColor(69,117,180) };
					diverging_map[std::make_pair(RdYlBu, EightDivergingClasses)] = value_type(coloursRdYlBu8, coloursRdYlBu8 + 8);
					const QColor coloursRdYlBu9[9] = { QColor(215,48,39), QColor(244,109,67), QColor(253,174,97), QColor(254,224,144), QColor(255,255,191), QColor(224,243,248), QColor(171,217,233), QColor(116,173,209), QColor(69,117,180) };
					diverging_map[std::make_pair(RdYlBu, NineDivergingClasses)] = value_type(coloursRdYlBu9, coloursRdYlBu9 + 9);
					const QColor coloursRdYlBu10[10] = { QColor(165,0,38), QColor(215,48,39), QColor(244,109,67), QColor(253,174,97), QColor(254,224,144), QColor(224,243,248), QColor(171,217,233), QColor(116,173,209), QColor(69,117,180), QColor(49,54,149) };
					diverging_map[std::make_pair(RdYlBu, TenDivergingClasses)] = value_type(coloursRdYlBu10, coloursRdYlBu10 + 10);
					const QColor coloursRdYlBu11[11] = { QColor(165,0,38), QColor(215,48,39), QColor(244,109,67), QColor(253,174,97), QColor(254,224,144), QColor(255,255,191), QColor(224,243,248), QColor(171,217,233), QColor(116,173,209), QColor(69,117,180), QColor(49,54,149) };
					diverging_map[std::make_pair(RdYlBu, ElevenDivergingClasses)] = value_type(coloursRdYlBu11, coloursRdYlBu11 + 11);

					const QColor coloursRdYlGn3[3] = { QColor(252,141,89), QColor(255,255,191), QColor(145,207,96) };
					diverging_map[std::make_pair(RdYlGn, ThreeDivergingClasses)] = value_type(coloursRdYlGn3, coloursRdYlGn3 + 3);
					const QColor coloursRdYlGn4[4] = { QColor(215,25,28), QColor(253,174,97), QColor(166,217,106), QColor(26,150,65) };
					diverging_map[std::make_pair(RdYlGn, FourDivergingClasses)] = value_type(coloursRdYlGn4, coloursRdYlGn4 + 4);
					const QColor coloursRdYlGn5[5] = { QColor(215,25,28), QColor(253,174,97), QColor(255,255,191), QColor(166,217,106), QColor(26,150,65) };
					diverging_map[std::make_pair(RdYlGn, FiveDivergingClasses)] = value_type(coloursRdYlGn5, coloursRdYlGn5 + 5);
					const QColor coloursRdYlGn6[6] = { QColor(215,48,39), QColor(252,141,89), QColor(254,224,139), QColor(217,239,139), QColor(145,207,96), QColor(26,152,80) };
					diverging_map[std::make_pair(RdYlGn, SixDivergingClasses)] = value_type(coloursRdYlGn6, coloursRdYlGn6 + 6);
					const QColor coloursRdYlGn7[7] = { QColor(215,48,39), QColor(252,141,89), QColor(254,224,139), QColor(255,255,191), QColor(217,239,139), QColor(145,207,96), QColor(26,152,80) };
					diverging_map[std::make_pair(RdYlGn, SevenDivergingClasses)] = value_type(coloursRdYlGn7, coloursRdYlGn7 + 7);
					const QColor coloursRdYlGn8[8] = { QColor(215,48,39), QColor(244,109,67), QColor(253,174,97), QColor(254,224,139), QColor(217,239,139), QColor(166,217,106), QColor(102,189,99), QColor(26,152,80) };
					diverging_map[std::make_pair(RdYlGn, EightDivergingClasses)] = value_type(coloursRdYlGn8, coloursRdYlGn8 + 8);
					const QColor coloursRdYlGn9[9] = { QColor(215,48,39), QColor(244,109,67), QColor(253,174,97), QColor(254,224,139), QColor(255,255,191), QColor(217,239,139), QColor(166,217,106), QColor(102,189,99), QColor(26,152,80) };
					diverging_map[std::make_pair(RdYlGn, NineDivergingClasses)] = value_type(coloursRdYlGn9, coloursRdYlGn9 + 9);
					const QColor coloursRdYlGn10[10] = { QColor(165,0,38), QColor(215,48,39), QColor(244,109,67), QColor(253,174,97), QColor(254,224,139), QColor(217,239,139), QColor(166,217,106), QColor(102,189,99), QColor(26,152,80), QColor(0,104,55) };
					diverging_map[std::make_pair(RdYlGn, TenDivergingClasses)] = value_type(coloursRdYlGn10, coloursRdYlGn10 + 10);
					const QColor coloursRdYlGn11[11] = { QColor(165,0,38), QColor(215,48,39), QColor(244,109,67), QColor(253,174,97), QColor(254,224,139), QColor(255,255,191), QColor(217,239,139), QColor(166,217,106), QColor(102,189,99), QColor(26,152,80), QColor(0,104,55) };
					diverging_map[std::make_pair(RdYlGn, ElevenDivergingClasses)] = value_type(coloursRdYlGn11, coloursRdYlGn11 + 11);

					const QColor coloursPiYG3[3] = { QColor(233,163,201), QColor(247,247,247), QColor(161,215,106) };
					diverging_map[std::make_pair(PiYG, ThreeDivergingClasses)] = value_type(coloursPiYG3, coloursPiYG3 + 3);
					const QColor coloursPiYG4[4] = { QColor(208,28,139), QColor(241,182,218), QColor(184,225,134), QColor(77,172,38) };
					diverging_map[std::make_pair(PiYG, FourDivergingClasses)] = value_type(coloursPiYG4, coloursPiYG4 + 4);
					const QColor coloursPiYG5[5] = { QColor(208,28,139), QColor(241,182,218), QColor(247,247,247), QColor(184,225,134), QColor(77,172,38) };
					diverging_map[std::make_pair(PiYG, FiveDivergingClasses)] = value_type(coloursPiYG5, coloursPiYG5 + 5);
					const QColor coloursPiYG6[6] = { QColor(197,27,125), QColor(233,163,201), QColor(253,224,239), QColor(230,245,208), QColor(161,215,106), QColor(77,146,33) };
					diverging_map[std::make_pair(PiYG, SixDivergingClasses)] = value_type(coloursPiYG6, coloursPiYG6 + 6);
					const QColor coloursPiYG7[7] = { QColor(197,27,125), QColor(233,163,201), QColor(253,224,239), QColor(247,247,247), QColor(230,245,208), QColor(161,215,106), QColor(77,146,33) };
					diverging_map[std::make_pair(PiYG, SevenDivergingClasses)] = value_type(coloursPiYG7, coloursPiYG7 + 7);
					const QColor coloursPiYG8[8] = { QColor(197,27,125), QColor(222,119,174), QColor(241,182,218), QColor(253,224,239), QColor(230,245,208), QColor(184,225,134), QColor(127,188,65), QColor(77,146,33) };
					diverging_map[std::make_pair(PiYG, EightDivergingClasses)] = value_type(coloursPiYG8, coloursPiYG8 + 8);
					const QColor coloursPiYG9[9] = { QColor(197,27,125), QColor(222,119,174), QColor(241,182,218), QColor(253,224,239), QColor(247,247,247), QColor(230,245,208), QColor(184,225,134), QColor(127,188,65), QColor(77,146,33) };
					diverging_map[std::make_pair(PiYG, NineDivergingClasses)] = value_type(coloursPiYG9, coloursPiYG9 + 9);
					const QColor coloursPiYG10[10] = { QColor(142,1,82), QColor(197,27,125), QColor(222,119,174), QColor(241,182,218), QColor(253,224,239), QColor(230,245,208), QColor(184,225,134), QColor(127,188,65), QColor(77,146,33), QColor(39,100,25) };
					diverging_map[std::make_pair(PiYG, TenDivergingClasses)] = value_type(coloursPiYG10, coloursPiYG10 + 10);
					const QColor coloursPiYG11[11] = { QColor(142,1,82), QColor(197,27,125), QColor(222,119,174), QColor(241,182,218), QColor(253,224,239), QColor(247,247,247), QColor(230,245,208), QColor(184,225,134), QColor(127,188,65), QColor(77,146,33), QColor(39,100,25) };
					diverging_map[std::make_pair(PiYG, ElevenDivergingClasses)] = value_type(coloursPiYG11, coloursPiYG11 + 11);

					const QColor coloursPuOr3[3] = { QColor(241,163,64), QColor(247,247,247), QColor(153,142,195) };
					diverging_map[std::make_pair(PuOr, ThreeDivergingClasses)] = value_type(coloursPuOr3, coloursPuOr3 + 3);
					const QColor coloursPuOr4[4] = { QColor(230,97,1), QColor(253,184,99), QColor(178,171,210), QColor(94,60,153) };
					diverging_map[std::make_pair(PuOr, FourDivergingClasses)] = value_type(coloursPuOr4, coloursPuOr4 + 4);
					const QColor coloursPuOr5[5] = { QColor(230,97,1), QColor(253,184,99), QColor(247,247,247), QColor(178,171,210), QColor(94,60,153) };
					diverging_map[std::make_pair(PuOr, FiveDivergingClasses)] = value_type(coloursPuOr5, coloursPuOr5 + 5);
					const QColor coloursPuOr6[6] = { QColor(179,88,6), QColor(241,163,64), QColor(254,224,182), QColor(216,218,235), QColor(153,142,195), QColor(84,39,136) };
					diverging_map[std::make_pair(PuOr, SixDivergingClasses)] = value_type(coloursPuOr6, coloursPuOr6 + 6);
					const QColor coloursPuOr7[7] = { QColor(179,88,6), QColor(241,163,64), QColor(254,224,182), QColor(247,247,247), QColor(216,218,235), QColor(153,142,195), QColor(84,39,136) };
					diverging_map[std::make_pair(PuOr, SevenDivergingClasses)] = value_type(coloursPuOr7, coloursPuOr7 + 7);
					const QColor coloursPuOr8[8] = { QColor(179,88,6), QColor(224,130,20), QColor(253,184,99), QColor(254,224,182), QColor(216,218,235), QColor(178,171,210), QColor(128,115,172), QColor(84,39,136) };
					diverging_map[std::make_pair(PuOr, EightDivergingClasses)] = value_type(coloursPuOr8, coloursPuOr8 + 8);
					const QColor coloursPuOr9[9] = { QColor(179,88,6), QColor(224,130,20), QColor(253,184,99), QColor(254,224,182), QColor(247,247,247), QColor(216,218,235), QColor(178,171,210), QColor(128,115,172), QColor(84,39,136) };
					diverging_map[std::make_pair(PuOr, NineDivergingClasses)] = value_type(coloursPuOr9, coloursPuOr9 + 9);
					const QColor coloursPuOr10[10] = { QColor(127,59,8), QColor(179,88,6), QColor(224,130,20), QColor(253,184,99), QColor(254,224,182), QColor(216,218,235), QColor(178,171,210), QColor(128,115,172), QColor(84,39,136), QColor(45,0,75) };
					diverging_map[std::make_pair(PuOr, TenDivergingClasses)] = value_type(coloursPuOr10, coloursPuOr10 + 10);
					const QColor coloursPuOr11[11] = { QColor(127,59,8), QColor(179,88,6), QColor(224,130,20), QColor(253,184,99), QColor(254,224,182), QColor(247,247,247), QColor(216,218,235), QColor(178,171,210), QColor(128,115,172), QColor(84,39,136), QColor(45,0,75) };
					diverging_map[std::make_pair(PuOr, ElevenDivergingClasses)] = value_type(coloursPuOr11, coloursPuOr11 + 11);

					const QColor coloursBrBG3[3] = { QColor(216,179,101), QColor(245,245,245), QColor(90,180,172) };
					diverging_map[std::make_pair(BrBG, ThreeDivergingClasses)] = value_type(coloursBrBG3, coloursBrBG3 + 3);
					const QColor coloursBrBG4[4] = { QColor(166,97,26), QColor(223,194,125), QColor(128,205,193), QColor(1,133,113) };
					diverging_map[std::make_pair(BrBG, FourDivergingClasses)] = value_type(coloursBrBG4, coloursBrBG4 + 4);
					const QColor coloursBrBG5[5] = { QColor(166,97,26), QColor(223,194,125), QColor(245,245,245), QColor(128,205,193), QColor(1,133,113) };
					diverging_map[std::make_pair(BrBG, FiveDivergingClasses)] = value_type(coloursBrBG5, coloursBrBG5 + 5);
					const QColor coloursBrBG6[6] = { QColor(140,81,10), QColor(216,179,101), QColor(246,232,195), QColor(199,234,229), QColor(90,180,172), QColor(1,102,94) };
					diverging_map[std::make_pair(BrBG, SixDivergingClasses)] = value_type(coloursBrBG6, coloursBrBG6 + 6);
					const QColor coloursBrBG7[7] = { QColor(140,81,10), QColor(216,179,101), QColor(246,232,195), QColor(245,245,245), QColor(199,234,229), QColor(90,180,172), QColor(1,102,94) };
					diverging_map[std::make_pair(BrBG, SevenDivergingClasses)] = value_type(coloursBrBG7, coloursBrBG7 + 7);
					const QColor coloursBrBG8[8] = { QColor(140,81,10), QColor(191,129,45), QColor(223,194,125), QColor(246,232,195), QColor(199,234,229), QColor(128,205,193), QColor(53,151,143), QColor(1,102,94) };
					diverging_map[std::make_pair(BrBG, EightDivergingClasses)] = value_type(coloursBrBG8, coloursBrBG8 + 8);
					const QColor coloursBrBG9[9] = { QColor(140,81,10), QColor(191,129,45), QColor(223,194,125), QColor(246,232,195), QColor(245,245,245), QColor(199,234,229), QColor(128,205,193), QColor(53,151,143), QColor(1,102,94) };
					diverging_map[std::make_pair(BrBG, NineDivergingClasses)] = value_type(coloursBrBG9, coloursBrBG9 + 9);
					const QColor coloursBrBG10[10] = { QColor(84,48,5), QColor(140,81,10), QColor(191,129,45), QColor(223,194,125), QColor(246,232,195), QColor(199,234,229), QColor(128,205,193), QColor(53,151,143), QColor(1,102,94), QColor(0,60,48) };
					diverging_map[std::make_pair(BrBG, TenDivergingClasses)] = value_type(coloursBrBG10, coloursBrBG10 + 10);
					const QColor coloursBrBG11[11] = { QColor(84,48,5), QColor(140,81,10), QColor(191,129,45), QColor(223,194,125), QColor(246,232,195), QColor(245,245,245), QColor(199,234,229), QColor(128,205,193), QColor(53,151,143), QColor(1,102,94), QColor(0,60,48) };
					diverging_map[std::make_pair(BrBG, ElevenDivergingClasses)] = value_type(coloursBrBG11, coloursBrBG11 + 11);

					const QColor coloursPRGn3[3] = { QColor(175,141,195), QColor(247,247,247), QColor(127,191,123) };
					diverging_map[std::make_pair(PRGn, ThreeDivergingClasses)] = value_type(coloursPRGn3, coloursPRGn3 + 3);
					const QColor coloursPRGn4[4] = { QColor(123,50,148), QColor(194,165,207), QColor(166,219,160), QColor(0,136,55) };
					diverging_map[std::make_pair(PRGn, FourDivergingClasses)] = value_type(coloursPRGn4, coloursPRGn4 + 4);
					const QColor coloursPRGn5[5] = { QColor(123,50,148), QColor(194,165,207), QColor(247,247,247), QColor(166,219,160), QColor(0,136,55) };
					diverging_map[std::make_pair(PRGn, FiveDivergingClasses)] = value_type(coloursPRGn5, coloursPRGn5 + 5);
					const QColor coloursPRGn6[6] = { QColor(118,42,131), QColor(175,141,195), QColor(231,212,232), QColor(217,240,211), QColor(127,191,123), QColor(27,120,55) };
					diverging_map[std::make_pair(PRGn, SixDivergingClasses)] = value_type(coloursPRGn6, coloursPRGn6 + 6);
					const QColor coloursPRGn7[7] = { QColor(118,42,131), QColor(175,141,195), QColor(231,212,232), QColor(247,247,247), QColor(217,240,211), QColor(127,191,123), QColor(27,120,55) };
					diverging_map[std::make_pair(PRGn, SevenDivergingClasses)] = value_type(coloursPRGn7, coloursPRGn7 + 7);
					const QColor coloursPRGn8[8] = { QColor(118,42,131), QColor(153,112,171), QColor(194,165,207), QColor(231,212,232), QColor(217,240,211), QColor(166,219,160), QColor(90,174,97), QColor(27,120,55) };
					diverging_map[std::make_pair(PRGn, EightDivergingClasses)] = value_type(coloursPRGn8, coloursPRGn8 + 8);
					const QColor coloursPRGn9[9] = { QColor(118,42,131), QColor(153,112,171), QColor(194,165,207), QColor(231,212,232), QColor(247,247,247), QColor(217,240,211), QColor(166,219,160), QColor(90,174,97), QColor(27,120,55) };
					diverging_map[std::make_pair(PRGn, NineDivergingClasses)] = value_type(coloursPRGn9, coloursPRGn9 + 9);
					const QColor coloursPRGn10[10] = { QColor(64,0,75), QColor(118,42,131), QColor(153,112,171), QColor(194,165,207), QColor(231,212,232), QColor(217,240,211), QColor(166,219,160), QColor(90,174,97), QColor(27,120,55), QColor(0,68,27) };
					diverging_map[std::make_pair(PRGn, TenDivergingClasses)] = value_type(coloursPRGn10, coloursPRGn10 + 10);
					const QColor coloursPRGn11[11] = { QColor(64,0,75), QColor(118,42,131), QColor(153,112,171), QColor(194,165,207), QColor(231,212,232), QColor(247,247,247), QColor(217,240,211), QColor(166,219,160), QColor(90,174,97), QColor(27,120,55), QColor(0,68,27) };
					diverging_map[std::make_pair(PRGn, ElevenDivergingClasses)] = value_type(coloursPRGn11, coloursPRGn11 + 11);

					const QColor coloursRdBu3[3] = { QColor(239,138,98), QColor(247,247,247), QColor(103,169,207) };
					diverging_map[std::make_pair(RdBu, ThreeDivergingClasses)] = value_type(coloursRdBu3, coloursRdBu3 + 3);
					const QColor coloursRdBu4[4] = { QColor(202,0,32), QColor(244,165,130), QColor(146,197,222), QColor(5,113,176) };
					diverging_map[std::make_pair(RdBu, FourDivergingClasses)] = value_type(coloursRdBu4, coloursRdBu4 + 4);
					const QColor coloursRdBu5[5] = { QColor(202,0,32), QColor(244,165,130), QColor(247,247,247), QColor(146,197,222), QColor(5,113,176) };
					diverging_map[std::make_pair(RdBu, FiveDivergingClasses)] = value_type(coloursRdBu5, coloursRdBu5 + 5);
					const QColor coloursRdBu6[6] = { QColor(178,24,43), QColor(239,138,98), QColor(253,219,199), QColor(209,229,240), QColor(103,169,207), QColor(33,102,172) };
					diverging_map[std::make_pair(RdBu, SixDivergingClasses)] = value_type(coloursRdBu6, coloursRdBu6 + 6);
					const QColor coloursRdBu7[7] = { QColor(178,24,43), QColor(239,138,98), QColor(253,219,199), QColor(247,247,247), QColor(209,229,240), QColor(103,169,207), QColor(33,102,172) };
					diverging_map[std::make_pair(RdBu, SevenDivergingClasses)] = value_type(coloursRdBu7, coloursRdBu7 + 7);
					const QColor coloursRdBu8[8] = { QColor(178,24,43), QColor(214,96,77), QColor(244,165,130), QColor(253,219,199), QColor(209,229,240), QColor(146,197,222), QColor(67,147,195), QColor(33,102,172) };
					diverging_map[std::make_pair(RdBu, EightDivergingClasses)] = value_type(coloursRdBu8, coloursRdBu8 + 8);
					const QColor coloursRdBu9[9] = { QColor(178,24,43), QColor(214,96,77), QColor(244,165,130), QColor(253,219,199), QColor(247,247,247), QColor(209,229,240), QColor(146,197,222), QColor(67,147,195), QColor(33,102,172) };
					diverging_map[std::make_pair(RdBu, NineDivergingClasses)] = value_type(coloursRdBu9, coloursRdBu9 + 9);
					const QColor coloursRdBu10[10] = { QColor(103,0,31), QColor(178,24,43), QColor(214,96,77), QColor(244,165,130), QColor(253,219,199), QColor(209,229,240), QColor(146,197,222), QColor(67,147,195), QColor(33,102,172), QColor(5,48,97) };
					diverging_map[std::make_pair(RdBu, TenDivergingClasses)] = value_type(coloursRdBu10, coloursRdBu10 + 10);
					const QColor coloursRdBu11[11] = { QColor(103,0,31), QColor(178,24,43), QColor(214,96,77), QColor(244,165,130), QColor(253,219,199), QColor(247,247,247), QColor(209,229,240), QColor(146,197,222), QColor(67,147,195), QColor(33,102,172), QColor(5,48,97) };
					diverging_map[std::make_pair(RdBu, ElevenDivergingClasses)] = value_type(coloursRdBu11, coloursRdBu11 + 11);

					const QColor coloursRdGy3[3] = { QColor(239,138,98), QColor(255,255,255), QColor(153,153,153) };
					diverging_map[std::make_pair(RdGy, ThreeDivergingClasses)] = value_type(coloursRdGy3, coloursRdGy3 + 3);
					const QColor coloursRdGy4[4] = { QColor(202,0,32), QColor(244,165,130), QColor(186,186,186), QColor(64,64,64) };
					diverging_map[std::make_pair(RdGy, FourDivergingClasses)] = value_type(coloursRdGy4, coloursRdGy4 + 4);
					const QColor coloursRdGy5[5] = { QColor(202,0,32), QColor(244,165,130), QColor(255,255,255), QColor(186,186,186), QColor(64,64,64) };
					diverging_map[std::make_pair(RdGy, FiveDivergingClasses)] = value_type(coloursRdGy5, coloursRdGy5 + 5);
					const QColor coloursRdGy6[6] = { QColor(178,24,43), QColor(239,138,98), QColor(253,219,199), QColor(224,224,224), QColor(153,153,153), QColor(77,77,77) };
					diverging_map[std::make_pair(RdGy, SixDivergingClasses)] = value_type(coloursRdGy6, coloursRdGy6 + 6);
					const QColor coloursRdGy7[7] = { QColor(178,24,43), QColor(239,138,98), QColor(253,219,199), QColor(255,255,255), QColor(224,224,224), QColor(153,153,153), QColor(77,77,77) };
					diverging_map[std::make_pair(RdGy, SevenDivergingClasses)] = value_type(coloursRdGy7, coloursRdGy7 + 7);
					const QColor coloursRdGy8[8] = { QColor(178,24,43), QColor(214,96,77), QColor(244,165,130), QColor(253,219,199), QColor(224,224,224), QColor(186,186,186), QColor(135,135,135), QColor(77,77,77) };
					diverging_map[std::make_pair(RdGy, EightDivergingClasses)] = value_type(coloursRdGy8, coloursRdGy8 + 8);
					const QColor coloursRdGy9[9] = { QColor(178,24,43), QColor(214,96,77), QColor(244,165,130), QColor(253,219,199), QColor(255,255,255), QColor(224,224,224), QColor(186,186,186), QColor(135,135,135), QColor(77,77,77) };
					diverging_map[std::make_pair(RdGy, NineDivergingClasses)] = value_type(coloursRdGy9, coloursRdGy9 + 9);
					const QColor coloursRdGy10[10] = { QColor(103,0,31), QColor(178,24,43), QColor(214,96,77), QColor(244,165,130), QColor(253,219,199), QColor(224,224,224), QColor(186,186,186), QColor(135,135,135), QColor(77,77,77), QColor(26,26,26) };
					diverging_map[std::make_pair(RdGy, TenDivergingClasses)] = value_type(coloursRdGy10, coloursRdGy10 + 10);
					const QColor coloursRdGy11[11] = { QColor(103,0,31), QColor(178,24,43), QColor(214,96,77), QColor(244,165,130), QColor(253,219,199), QColor(255,255,255), QColor(224,224,224), QColor(186,186,186), QColor(135,135,135), QColor(77,77,77), QColor(26,26,26) };
					diverging_map[std::make_pair(RdGy, ElevenDivergingClasses)] = value_type(coloursRdGy11, coloursRdGy11 + 11);

					diverging_map_initialised = true;
				}

				return diverging_map[std::make_pair(diverging_type, diverging_classes)];
			}


			// These colours are arbitrary.
			const Colour DEFAULT_SCALAR_COLOURS[] = {
					Colour(0, 0, 1) /* blue - low */,
					Colour(0, 1, 1) /* cyan */,
					Colour(0, 1, 0) /* green - middle */,
					Colour(1, 1, 0) /* yellow */,
					Colour(1, 0, 0) /* red - high */
			};

			const unsigned int NUM_DEFAULT_RASTER_COLOURS =
					sizeof(DEFAULT_SCALAR_COLOURS) / sizeof(Colour);

			const double DEFAULT_SCALAR_LOWER_BOUND = 0;
			const double DEFAULT_SCALAR_UPPER_BOUND = 1;
		}
	}
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::BuiltinColourPalettes::create_age_palette()
{
	// Don't need to report any read errors - the age CPT file is embedded and should just work.
	GPlatesFileIO::ReadErrorAccumulation read_errors;

	RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
			ColourPaletteUtils::read_cpt_raster_colour_palette(
					AGE_CPT_FILE_NAME,
					false/*allow_integer_colour_palette*/,
					read_errors);

	boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette =
			RasterColourPaletteExtract::get_colour_palette<double>(*raster_colour_palette);

	// Should be a real-valued palette.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			colour_palette,
			GPLATES_ASSERTION_SOURCE);

	return colour_palette.get();
}


QString
GPlatesGui::BuiltinColourPalettes::get_colorbrewer_sequential_palette_name(
		ColorBrewerSequentialType sequential_type)
{
	switch (sequential_type)
	{
	case BuiltinColourPalettes::OrRd:
		return "OrRd";
	case BuiltinColourPalettes::PuBu:
		return "PuBu";
	case BuiltinColourPalettes::BuPu:
		return "BuPu";
	case BuiltinColourPalettes::Oranges:
		return "Oranges";
	case BuiltinColourPalettes::BuGn:
		return "BuGn";
	case BuiltinColourPalettes::YlOrBr:
		return "YlOrBr";
	case BuiltinColourPalettes::YlGn:
		return "YlGn";
	case BuiltinColourPalettes::Reds:
		return "Reds";
	case BuiltinColourPalettes::RdPu:
		return "RdPu";
	case BuiltinColourPalettes::Greens:
		return "Greens";
	case BuiltinColourPalettes::YlGnBu:
		return "YlGnBu";
	case BuiltinColourPalettes::Purples:
		return "Purples";
	case BuiltinColourPalettes::GnBu:
		return "GnBu";
	case BuiltinColourPalettes::Greys:
		return "Greys";
	case BuiltinColourPalettes::YlOrRd:
		return "YlOrRd";
	case BuiltinColourPalettes::PuRd:
		return "PuRd";
	case BuiltinColourPalettes::Blues:
		return "Blues";
	case BuiltinColourPalettes::PuBuGn:
		return "PuBuGn";

	default:
		break;
	}

	// Shouldn't be able to get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	return QString();
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::BuiltinColourPalettes::create_colorbrewer_sequential_palette(
		ColorBrewerSequentialType sequential_type,
		ColorBrewerSequentialClasses sequential_classes,
		bool continuous,
		bool invert,
		const boost::optional<Colour> &nan_colour)
{
	RegularCptColourPalette::non_null_ptr_type colour_palette = RegularCptColourPalette::create();

	std::vector<Colour> colours = get_colorbrewer_sequential_colours(sequential_type, sequential_classes);
	if (invert)
	{
		std::reverse(colours.begin(), colours.end());
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			colours.size() == sequential_classes,
			GPLATES_ASSERTION_SOURCE);

	// Background colour, for values before min value.
	colour_palette->set_background_colour(colours.front());

	// Foreground colour, for values after max value.
	colour_palette->set_foreground_colour(colours.back());

	// Add the colour slices for everything in between.
	// The range is [0,1].
	// Each colour slice linearly blends between two colours.
	const unsigned int num_intervals = continuous ? (sequential_classes - 1) : sequential_classes;
	const double interval = 1.0 / num_intervals;
	for (unsigned int i = 0; i != num_intervals; ++i)
	{
		const ColourSlice colour_slice(
				i * interval,
				colours[i],
				// Make it land exactly on 1.0 for the last interval...
				(i + 1 == num_intervals) ? 1.0 : ((i + 1) * interval),
				colours[i + (continuous ? 1 : 0)]);
		colour_palette->add_entry(colour_slice);
	}

	// Set NaN colour.
	if (nan_colour)
	{
		colour_palette->set_nan_colour(nan_colour.get());
	}

	// Convert/adapt Real to double.
	return convert_colour_palette<
			RegularCptColourPalette::key_type,
			double,
			RealToBuiltInConverter<double> >(
					colour_palette,
					RealToBuiltInConverter<double>());
}


QString
GPlatesGui::BuiltinColourPalettes::get_colorbrewer_diverging_palette_name(
		ColorBrewerDivergingType diverging_type)
{
	switch (diverging_type)
	{
	case BuiltinColourPalettes::Spectral:
		return "Spectral";
	case BuiltinColourPalettes::RdYlGn:
		return "RdYlGn";
	case BuiltinColourPalettes::RdBu:
		return "RdBu";
	case BuiltinColourPalettes::PiYG:
		return "PiYG";
	case BuiltinColourPalettes::PRGn:
		return "PRGn";
	case BuiltinColourPalettes::RdYlBu:
		return "RdYlBu";
	case BuiltinColourPalettes::BrBG:
		return "BrBG";
	case BuiltinColourPalettes::RdGy:
		return "RdGy";
	case BuiltinColourPalettes::PuOr:
		return "PuOr";

	default:
		break;
	}

	// Shouldn't be able to get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	return QString();
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::BuiltinColourPalettes::create_colorbrewer_diverging_palette(
		ColorBrewerDivergingType diverging_type,
		ColorBrewerDivergingClasses diverging_classes,
		bool continuous,
		bool invert,
		const boost::optional<Colour> &nan_colour)
{
	RegularCptColourPalette::non_null_ptr_type colour_palette = RegularCptColourPalette::create();

	std::vector<Colour> colours = get_colorbrewer_diverging_colours(diverging_type, diverging_classes);
	if (invert)
	{
		std::reverse(colours.begin(), colours.end());
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			colours.size() == diverging_classes,
			GPLATES_ASSERTION_SOURCE);

	// Background colour, for values before min value.
	colour_palette->set_background_colour(colours.front());

	// Foreground colour, for values after max value.
	colour_palette->set_foreground_colour(colours.back());

	// Add the colour slices for everything in between.
	// The range is [-1,1].
	if (continuous)
	{
		// We need to handle odd and even classes differently since we need to have a colour sample
		// at zero (the middle of the [-1,1] range) otherwise the colours are linearly interpolated
		// at zero - but, for ColorBrewer diverging colours, the colours should only be linearly
		// interpolated within the ranges [-1,0] and [0,1] to avoid colours that are deviate too much
		// from colour scheme designed by ColorBrewer.
		if ((diverging_classes & 1) == 1) // odd number of classes...
		{
			// Each colour slice linearly blends between two colours.
			const unsigned int num_intervals = diverging_classes - 1;
			const double interval = 2.0 / num_intervals;
			for (unsigned int i = 0; i != num_intervals; ++i)
			{
				const ColourSlice colour_slice(
						-1.0 + i * interval,
						colours[i],
						// Make it land exactly on 1.0 for the last interval...
						(i + 1 == num_intervals) ? 1.0 : (-1.0 + (i + 1) * interval),
						colours[i + 1]);
				colour_palette->add_entry(colour_slice);
			}
		}
		else // even number of classes...
		{
			// We need to introduce a colour discontinuity at zero to avoid linear interpolation at zero.
			// Half the colours will cover the range [-1,0] and the other half [0,1].
			// Note that zero is included by two colour slices (one from each range) each using a
			// different colour. This is the colour discontinuity at zero.
			const unsigned int half_num_intervals = (diverging_classes - 2) / 2;
			const double interval = 1.0 / half_num_intervals;
			for (unsigned int i = 0; i != half_num_intervals; ++i)
			{
				const ColourSlice colour_slice(
						-1.0 + i * interval,
						colours[i],
						// Make it land exactly on 0.0 for the last interval...
						(i + 1 == half_num_intervals) ? 0.0 : (-1.0 + (i + 1) * interval),
						colours[i + 1]);
				colour_palette->add_entry(colour_slice);
			}
			for (unsigned int i = 0; i != half_num_intervals; ++i)
			{
				const ColourSlice colour_slice(
						0.0 + i * interval,
						colours[i + half_num_intervals + 1],
						// Make it land exactly on 1.0 for the last interval...
						(i + 1 == half_num_intervals) ? 1.0 : (0.0 + (i + 1) * interval),
						colours[i + half_num_intervals + 2]);
				colour_palette->add_entry(colour_slice);
			}
		}
	}
	else // discrete...
	{
		// Each colour slice has a constant colour.
		const unsigned int num_intervals = diverging_classes;
		const double interval = 2.0 / num_intervals;
		for (unsigned int i = 0; i != num_intervals; ++i)
		{
			const ColourSlice colour_slice(
					-1.0 + i * interval,
					colours[i],
					// Make it land exactly on 1.0 for the last interval...
					(i + 1 == num_intervals) ? 1.0 : (-1.0 + (i + 1) * interval),
					colours[i]);
			colour_palette->add_entry(colour_slice);
		}
	}

	// Set NaN colour.
	if (nan_colour)
	{
		colour_palette->set_nan_colour(nan_colour.get());
	}

	// Convert/adapt Real to double.
	return convert_colour_palette<
			RegularCptColourPalette::key_type,
			double,
			RealToBuiltInConverter<double> >(
					colour_palette,
					RealToBuiltInConverter<double>());
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::BuiltinColourPalettes::create_scalar_colour_palette()
{
	RegularCptColourPalette::non_null_ptr_type colour_palette = RegularCptColourPalette::create();

	// [min, max] is the range [0, 1].
	const double min = DEFAULT_SCALAR_LOWER_BOUND;
	const double max = DEFAULT_SCALAR_UPPER_BOUND;
	const double range = max - min;

	// Background colour, for values before min value.
	colour_palette->set_background_colour(DEFAULT_SCALAR_COLOURS[0]);

	// Foreground colour, for values after max value.
	colour_palette->set_foreground_colour(DEFAULT_SCALAR_COLOURS[NUM_DEFAULT_RASTER_COLOURS - 1]);

	// Add the colour slices for everything in between.
	for (unsigned int i = 0; i != NUM_DEFAULT_RASTER_COLOURS - 1; ++i)
	{
		ColourSlice colour_slice(
				min + i * range / (NUM_DEFAULT_RASTER_COLOURS - 1),
				DEFAULT_SCALAR_COLOURS[i],
				min + (i + 1) * range / (NUM_DEFAULT_RASTER_COLOURS - 1),
				DEFAULT_SCALAR_COLOURS[i + 1]);
		colour_palette->add_entry(colour_slice);
	}

	// Convert/adapt Real to double.
	return convert_colour_palette<
			RegularCptColourPalette::key_type,
			double,
			RealToBuiltInConverter<double> >(
					colour_palette,
					RealToBuiltInConverter<double>());
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::BuiltinColourPalettes::create_gradient_colour_palette()
{
	RegularCptColourPalette::non_null_ptr_type colour_palette = RegularCptColourPalette::create();

	// Background colour, for values before -1.
	colour_palette->set_background_colour(Colour(0, 0, 1) /* blue - high */);

	// Foreground colour, for values after +1.
	colour_palette->set_foreground_colour(Colour(1, 0, 1) /* magenta - high */);

	// Add the colour slices for the range [-1,1].
	colour_palette->add_entry(
			ColourSlice(
					-1, Colour(0, 0, 1) /* blue - high gradient magnitude */,
					-0.5, Colour(0, 1, 1) /* cyan - mid gradient magnitude */));
	colour_palette->add_entry(
			ColourSlice(
					-0.5, Colour(0, 1, 1) /* cyan - mid gradient magnitude */,
					 0, Colour(0, 1, 0) /* green - low gradient magnitude */));
	colour_palette->add_entry(
			ColourSlice(
					0, Colour(1, 1, 0) /* yellow - low gradient magnitude */,
					0.5, Colour(1, 0, 0) /* red - mid gradient magnitude */));
	colour_palette->add_entry(
			ColourSlice(
					0.5, Colour(1, 0, 0) /* red - mid gradient magnitude */,
					1, Colour(1, 0, 1) /* magenta - high gradient magnitude */));

	// Convert/adapt Real to double.
	return convert_colour_palette<
			RegularCptColourPalette::key_type,
			double,
			RealToBuiltInConverter<double> >(
					colour_palette,
					RealToBuiltInConverter<double>());
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::BuiltinColourPalettes::create_strain_colour_palette(
		double min_abs_strain,
		double max_abs_strain,
		const double &max_log_spacing)
{
	// Color symbols: ColorBrewer.org
	const unsigned int num_blends = 5;
	const Colour zero_colour = QColor("#f7f7f7");
	const Colour contraction_colours[num_blends + 1] =
	{
		zero_colour,
		QColor("#d1e5f0"),
		QColor("#92c5de"),
		QColor("#4393c3"),
		QColor("#2166ac"),
		QColor("#053061")
	};
	const Colour extension_colours[num_blends + 1] =
	{
		zero_colour,
		QColor("#fddbc7"),
		QColor("#f4a582"),
		QColor("#d6604d"),
		QColor("#b2182b"),
		QColor("#67001f")
	};

	if (min_abs_strain < 1e-40)
	{
		min_abs_strain = 1e-40;
	}
	if (max_abs_strain < min_abs_strain)
	{
		max_abs_strain = min_abs_strain;
	}

	const double log_min_abs_strain = std::log10(min_abs_strain);
	const double log_max_abs_strain = std::log10(max_abs_strain);

	const double log_spacing_per_blend = (log_max_abs_strain - log_min_abs_strain) / num_blends;
	const unsigned int num_slices_per_blend = max_log_spacing > 1e-6
			? static_cast<unsigned int>(std::ceil(log_spacing_per_blend / max_log_spacing))
			: 0;
	double inv_num_slices_per_blend = 0;
	double log_spacing = 0;
	if (num_slices_per_blend > 0)
	{
		inv_num_slices_per_blend = 1.0 / num_slices_per_blend;
		log_spacing = inv_num_slices_per_blend * log_spacing_per_blend;
	}

	RegularCptColourPalette::non_null_ptr_type colour_palette = RegularCptColourPalette::create();

	// Note Add the lowest values first, that is, from contraction:

	// Background colour for min contraction value.
	colour_palette->set_background_colour(contraction_colours[num_blends]);

	// Add the contraction slices.
	for (unsigned int bc = num_blends; bc > 0; --bc)
	{
		for (unsigned int c = num_slices_per_blend; c > 0; --c)
		{
			const ColourSlice colour_slice_contraction(
					-std::pow(10.0, log_min_abs_strain + (bc - 1) * log_spacing_per_blend + c * log_spacing),
					Colour::linearly_interpolate(
							contraction_colours[bc - 1],
							contraction_colours[bc],
							c * inv_num_slices_per_blend),
					-std::pow(10.0, log_min_abs_strain + (bc - 1) * log_spacing_per_blend + (c - 1) * log_spacing),
					Colour::linearly_interpolate(
							contraction_colours[bc - 1],
							contraction_colours[bc],
							(c - 1) * inv_num_slices_per_blend));
			colour_palette->add_entry(colour_slice_contraction);
		}
	}

	// Add the middle to the spectrum (around zero).
	const ColourSlice colour_slice_zero(
			-min_abs_strain,
			zero_colour,
			min_abs_strain,
			zero_colour);
	colour_palette->add_entry(colour_slice_zero);

	// Add the extension slices.
	for (unsigned int be = 0; be < num_blends; ++be)
	{
		for (unsigned int e = 0; e < num_slices_per_blend; ++e)
		{
			const ColourSlice colour_slice_extension(
					std::pow(10.0, log_min_abs_strain + be * log_spacing_per_blend + e * log_spacing),
					Colour::linearly_interpolate(
							extension_colours[be],
							extension_colours[be + 1],
							e * inv_num_slices_per_blend),
					std::pow(10.0, log_min_abs_strain + be * log_spacing_per_blend + (e + 1) * log_spacing),
					Colour::linearly_interpolate(
							extension_colours[be],
							extension_colours[be + 1],
							(e + 1) * inv_num_slices_per_blend));
			colour_palette->add_entry(colour_slice_extension);
		}
	}

	// Foreground colour for max extension value.
	colour_palette->set_foreground_colour(extension_colours[num_blends]);

	// Set NaN colour.
	colour_palette->set_nan_colour( Colour(0.5, 0.5, 0.5) );

	// Convert/adapt Real to double.
	return convert_colour_palette<
			RegularCptColourPalette::key_type,
			double,
			RealToBuiltInConverter<double> >(
					colour_palette,
					RealToBuiltInConverter<double>());
}


GPlatesScribe::TranscribeResult
GPlatesGui::BuiltinColourPalettes::transcribe(
		GPlatesScribe::Scribe &scribe,
		ColorBrewerSequentialType &colorbrewer_sequential_type,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("OrRd", OrRd),
		GPlatesScribe::EnumValue("PuBu", PuBu),
		GPlatesScribe::EnumValue("BuPu", BuPu),
		GPlatesScribe::EnumValue("Oranges", Oranges),
		GPlatesScribe::EnumValue("BuGn", BuGn),
		GPlatesScribe::EnumValue("YlOrBr", YlOrBr),
		GPlatesScribe::EnumValue("YlGn", YlGn),
		GPlatesScribe::EnumValue("Reds", Reds),
		GPlatesScribe::EnumValue("RdPu", RdPu),
		GPlatesScribe::EnumValue("Greens", Greens),
		GPlatesScribe::EnumValue("YlGnBu", YlGnBu),
		GPlatesScribe::EnumValue("Purples", Purples),
		GPlatesScribe::EnumValue("GnBu", GnBu),
		GPlatesScribe::EnumValue("Greys", Greys),
		GPlatesScribe::EnumValue("YlOrRd", YlOrRd),
		GPlatesScribe::EnumValue("PuRd", PuRd),
		GPlatesScribe::EnumValue("Blues", Blues),
		GPlatesScribe::EnumValue("PuBuGn", PuBuGn)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			colorbrewer_sequential_type,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}


GPlatesScribe::TranscribeResult
GPlatesGui::BuiltinColourPalettes::transcribe(
		GPlatesScribe::Scribe &scribe,
		ColorBrewerSequentialClasses &colorbrewer_sequential_classes,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("ThreeSequentialClasses", ThreeSequentialClasses),
		GPlatesScribe::EnumValue("FourSequentialClasses", FourSequentialClasses),
		GPlatesScribe::EnumValue("FiveSequentialClasses", FiveSequentialClasses),
		GPlatesScribe::EnumValue("SixSequentialClasses", SixSequentialClasses),
		GPlatesScribe::EnumValue("SevenSequentialClasses", SevenSequentialClasses),
		GPlatesScribe::EnumValue("EightSequentialClasses", EightSequentialClasses),
		GPlatesScribe::EnumValue("NineSequentialClasses", NineSequentialClasses)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			colorbrewer_sequential_classes,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}


GPlatesScribe::TranscribeResult
GPlatesGui::BuiltinColourPalettes::transcribe(
		GPlatesScribe::Scribe &scribe,
		ColorBrewerDivergingType &colorbrewer_diverging_type,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("Spectral", Spectral),
		GPlatesScribe::EnumValue("RdYlGn", RdYlGn),
		GPlatesScribe::EnumValue("RdBu", RdBu),
		GPlatesScribe::EnumValue("PiYG", PiYG),
		GPlatesScribe::EnumValue("PRGn", PRGn),
		GPlatesScribe::EnumValue("RdYlBu", RdYlBu),
		GPlatesScribe::EnumValue("BrBG", BrBG),
		GPlatesScribe::EnumValue("RdGy", RdGy),
		GPlatesScribe::EnumValue("PuOr", PuOr)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			colorbrewer_diverging_type,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}


GPlatesScribe::TranscribeResult
GPlatesGui::BuiltinColourPalettes::transcribe(
		GPlatesScribe::Scribe &scribe,
		ColorBrewerDivergingClasses &colorbrewer_diverging_classes,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("ThreeDivergingClasses", ThreeDivergingClasses),
		GPlatesScribe::EnumValue("FourDivergingClasses", FourDivergingClasses),
		GPlatesScribe::EnumValue("FiveDivergingClasses", FiveDivergingClasses),
		GPlatesScribe::EnumValue("SixDivergingClasses", SixDivergingClasses),
		GPlatesScribe::EnumValue("SevenDivergingClasses", SevenDivergingClasses),
		GPlatesScribe::EnumValue("EightDivergingClasses", EightDivergingClasses),
		GPlatesScribe::EnumValue("NineDivergingClasses", NineDivergingClasses),
		GPlatesScribe::EnumValue("TenDivergingClasses", TenDivergingClasses),
		GPlatesScribe::EnumValue("ElevenDivergingClasses", ElevenDivergingClasses)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			colorbrewer_diverging_classes,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
