/* $Id$ */

/**
 * @file 
 * Contains the implementation of the HTMLColourNames class.
 *
 * Most recent change:
 *   $Date$
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

#include "HTMLColourNames.h"

GPlatesGui::HTMLColourNames::HTMLColourNames()
{
	insert_colour("aliceblue", 240, 248, 255);
	insert_colour("antiquewhite", 250, 235, 215);
	insert_colour("aqua", 0, 255, 255);
	insert_colour("aquamarine", 127, 255, 212);
	insert_colour("azure", 240, 255, 255);
	insert_colour("beige", 245, 245, 220);
	insert_colour("bisque", 255, 228, 196);
	insert_colour("black", 0, 0, 0);
	insert_colour("blanchedalmond", 255, 235, 205);
	insert_colour("blue", 0, 0, 255);
	insert_colour("blueviolet", 138, 43, 226);
	insert_colour("brown", 165, 42, 42);
	insert_colour("burlywood", 222, 184, 135);
	insert_colour("cadetblue", 95, 158, 160);
	insert_colour("chartreuse", 127, 255, 0);
	insert_colour("chocolate", 210, 105, 30);
	insert_colour("coral", 255, 127, 80);
	insert_colour("cornflowerblue", 100, 149, 237);
	insert_colour("cornsilk", 255, 248, 220);
	insert_colour("crimson", 220, 20, 60);
	insert_colour("cyan", 0, 255, 255);
	insert_colour("darkblue", 0, 0, 139);
	insert_colour("darkcyan", 0, 139, 139);
	insert_colour("darkgoldenrod", 184, 134, 11);
	insert_colour("darkgray", 169, 169, 169);
	insert_colour("darkgreen", 0, 100, 0);
	insert_colour("darkkhaki", 189, 183, 107);
	insert_colour("darkmagenta", 139, 0, 139);
	insert_colour("darkolivegreen", 85, 107, 47);
	insert_colour("darkorange", 255, 140, 0);
	insert_colour("darkorchid", 153, 50, 204);
	insert_colour("darkred", 139, 0, 0);
	insert_colour("darksalmon", 233, 150, 122);
	insert_colour("darkseagreen", 143, 188, 143);
	insert_colour("darkslateblue", 72, 61, 139);
	insert_colour("darkslategray", 47, 79, 79);
	insert_colour("darkturquoise", 0, 206, 209);
	insert_colour("darkviolet", 148, 0, 211);
	insert_colour("deeppink", 255, 20, 147);
	insert_colour("deepskyblue", 0, 191, 255);
	insert_colour("dimgray", 105, 105, 105);
	insert_colour("dodgerblue", 30, 144, 255);
	insert_colour("firebrick", 178, 34, 34);
	insert_colour("floralwhite", 255, 250, 240);
	insert_colour("forestgreen", 34, 139, 34);
	insert_colour("fuchsia", 255, 0, 255);
	insert_colour("gainsboro", 220, 220, 220);
	insert_colour("ghostwhite", 248, 248, 255);
	insert_colour("gold", 255, 215, 0);
	insert_colour("goldenrod", 218, 165, 32);
	insert_colour("gray", 128, 128, 128);
	insert_colour("green", 0, 128, 0);
	insert_colour("greenyellow", 173, 255, 47);
	insert_colour("honeydew", 240, 255, 240);
	insert_colour("hotpink", 255, 105, 180);
	insert_colour("indianred", 205, 92, 92);
	insert_colour("indigo", 75, 0, 130);
	insert_colour("ivory", 255, 255, 240);
	insert_colour("khaki", 240, 230, 140);
	insert_colour("lavender", 230, 230, 250);
	insert_colour("lavenderblush", 255, 240, 245);
	insert_colour("lawngreen", 124, 252, 0);
	insert_colour("lemonchiffon", 255, 250, 205);
	insert_colour("lightblue", 173, 216, 230);
	insert_colour("lightcoral", 240, 128, 128);
	insert_colour("lightcyan", 224, 255, 255);
	insert_colour("lightgoldenrodyellow", 250, 250, 210);
	insert_colour("lightgreen", 144, 238, 144);
	insert_colour("lightgrey", 211, 211, 211);
	insert_colour("lightpink", 255, 182, 193);
	insert_colour("lightsalmon", 255, 160, 122);
	insert_colour("lightseagreen", 32, 178, 170);
	insert_colour("lightskyblue", 135, 206, 250);
	insert_colour("lightslategray", 119, 136, 153);
	insert_colour("lightsteelblue", 176, 196, 222);
	insert_colour("lightyellow", 255, 255, 224);
	insert_colour("lime", 0, 255, 0);
	insert_colour("limegreen", 50, 205, 50);
	insert_colour("linen", 250, 240, 230);
	insert_colour("magenta", 255, 0, 255);
	insert_colour("maroon", 128, 0, 0);
	insert_colour("mediumaquamarine", 102, 205, 170);
	insert_colour("mediumblue", 0, 0, 205);
	insert_colour("mediumorchid", 186, 85, 211);
	insert_colour("mediumpurple", 147, 112, 219);
	insert_colour("mediumseagreen", 60, 179, 113);
	insert_colour("mediumslateblue", 123, 104, 238);
	insert_colour("mediumspringgreen", 0, 250, 154);
	insert_colour("mediumturquoise", 72, 209, 204);
	insert_colour("mediumvioletred", 199, 21, 133);
	insert_colour("midnightblue", 25, 25, 112);
	insert_colour("mintcream", 245, 255, 250);
	insert_colour("mistyrose", 255, 228, 225);
	insert_colour("moccasin", 255, 228, 181);
	insert_colour("navajowhite", 255, 222, 173);
	insert_colour("navy", 0, 0, 128);
	insert_colour("oldlace", 253, 245, 230);
	insert_colour("olive", 128, 128, 0);
	insert_colour("olivedrab", 107, 142, 35);
	insert_colour("orange", 255, 165, 0);
	insert_colour("orangered", 255, 69, 0);
	insert_colour("orchid", 218, 112, 214);
	insert_colour("palegoldenrod", 238, 232, 170);
	insert_colour("palegreen", 152, 251, 152);
	insert_colour("palevioletred", 219, 112, 147);
	insert_colour("papayawhip", 255, 239, 213);
	insert_colour("peachpuff", 255, 218, 185);
	insert_colour("peru", 205, 133, 63);
	insert_colour("pink", 255, 192, 203);
	insert_colour("plum", 221, 160, 221);
	insert_colour("powderblue", 176, 224, 230);
	insert_colour("purple", 128, 0, 128);
	insert_colour("red", 255, 0, 0);
	insert_colour("rosybrown", 188, 143, 143);
	insert_colour("royalblue", 65, 105, 225);
	insert_colour("saddlebrown", 139, 69, 19);
	insert_colour("salmon", 250, 128, 114);
	insert_colour("sandybrown", 250, 164, 96);
	insert_colour("seagreen", 46, 139, 87);
	insert_colour("seashell", 255, 245, 238);
	insert_colour("sienna", 160, 82, 45);
	insert_colour("silver", 192, 192, 192);
	insert_colour("skyblue", 135, 206, 235);
	insert_colour("slateblue", 106, 90, 205);
	insert_colour("slategray", 112, 128, 144);
	insert_colour("snow", 255, 250, 250);
	insert_colour("springgreen", 0, 255, 127);
	insert_colour("steelblue", 70, 130, 180);
	insert_colour("tan", 210, 180, 140);
	insert_colour("teal", 0, 128, 128);
	insert_colour("thistle", 216, 191, 216);
	insert_colour("tomato", 255, 99, 71);
	insert_colour("turquoise", 64, 224, 208);
	insert_colour("violet", 238, 130, 238);
	insert_colour("wheat", 245, 222, 179);
	insert_colour("white", 255, 255, 255);
	insert_colour("whitesmoke", 245, 245, 245);
	insert_colour("yellow", 255, 255, 0);
	insert_colour("yellowgreen", 154, 205, 50);
}
