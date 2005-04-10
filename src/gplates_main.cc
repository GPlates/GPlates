/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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
 */

/** 
 * @mainpage GPlates Developers' Manual
 * 
 * @section introduction Introduction
 *
 * Welcome to the GPlates Developers' Manual.
 *
 * @section contact Contact
 *
 * <b>Email</b>:
 *  - James Boyden <jboyden (at) geosci.usyd.edu.au>
 *  - Mark Turner <mturner (at) gps.caltech.edu>
 * 
 * <b>Snail Mail</b>:
 * @par
 *   James Boyden<br>
 *   University of Sydney Institute of Marine Science<br>
 *   Edgeworth David Building F05<br>
 *   School of Geosciences<br>
 *   The University of Sydney, NSW 2006<br>
 *   AUSTRALIA<br>
 *
 * @section wxwidgets WxWidgets
 *
 * <a href="http://www.wxwidgets.org/">WxWidgets</a> (previously known as
 * WxWindows) is an open-source, cross-platform C++ GUI framework.  GPlates
 * currently requires version 2.4, release
 * <a href="http://www.wxwidgets.org/manuals/2.4.2/wx.htm">2.4.2</a> or above.
 *
 * @section patterns Design Patterns
 *
 * All the references to design patterns in the GPlates documentation refer to
 * the book <i>Design Patterns</i> (Gamma95).  See the Bibliography below for
 * more details.
 *
 * @section bibliography Bibliography
 *
 * The following references have played a significant role in the design or
 * implementation of the project:
 *
 *  - Benson04:  Calum Benson, Adam Elman, Seth Nickell, and Colin Z Robertson,
 *     <i>GNOME Human Interface Guidelines 2.0</i> [online].
 *     http://developer.gnome.org/projects/gup/hig/2.0/  [Most-recently
 *     accessed 9 April 2005]
 *  - Cox03:  Allan Cox and Robert Brian Hart, <i>Plate Tectonics:  How It
 *     Works</i>.  Blackwell Scientific Publications, 2003 reprint.
 *  - Foley96:  James D. Foley, Andries van Dam, Steven K. Feiner, and John F.
 *     Hughes, <i>Computer Graphics:  Principles and Practice (2nd
 *     Edition)</i>.  Addison-Wesley, 1996.
 *  - Gahagan99:  L. M. Gahagan, <i>plates4.0:  A User's Manual for the Plates
 *     Project's interactive reconstruction software</i>.  The University of
 *     Texas Institute for Geophysics, 1999.
 *  - Gamma95:  Erich Gamma, Richard Helm, Ralph Johnson, and John
 *     Vlissides, <i>Design Patterns:  Elements of Reusable
 *     Object-Oriented Software</i>.  Addison-Wesley, 1995.
 *  - Greiner99:  B. Greiner, "Euler Rotations in Plate-Tectonic
 *     Reconstructions".  <i>Computers and Geosciences</i> No. 25, Pergamon
 *     Press, 1999 (pp.209-216).
 *  - Josuttis99:  Nicolai M. Josuttis, <i>The C++ Standard Library:  A
 *     Tutorial and Reference</i>.  Addison-Wesley, 1999.
 *  - Kuipers02:  Jack B. Kuipers, <i>Quaternions and Rotation Sequences</i>,
 *     Princeton University Press, 2002.
 *  - Meyers98:  Scott Meyers, <i>Effective C++:  50 Specific Ways to Improve
 *     Your Programs and Designs (2nd Edition)</i>.  Addison-Wesley, 1998.
 *  - Schettino99:  Antonio Schettino, "Polygon intersections in spherical
 *     topology: Application to Plate Tectonics".  <i>Computers and
 *     Geosciences</i> No. 25, Pergamon Press, 1999 (pp.61-69).
 *  - Stroustrup97:  Bjarne Stroustrup, <i>The C++ Programming Language (3rd
 *     Edition)</i>.  Addison-Wesley, 1997.
 *  - Sutter00:  Herb Sutter, <i>Exceptional C++:  47 Engineering Puzzles,
 *     Programming Problems, and Solutions</i>.  Addison Wesley Longman, 2000.
 */

#include "gui/GPlatesApp.h"

IMPLEMENT_APP(GPlatesGui::GPlatesApp)
