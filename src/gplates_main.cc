/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

/** 
 * @mainpage GPlates Source Code Documentation
 * 
 * @section introduction Introduction
 *
 * Welcome to the <a href="http://www.gplates.org/">GPlates</a> source code
 * documentation.  We hope you enjoy your stay.
 *
 * @section contact Contact
 *
 *  - James Boyden: jboyden (at) geosci.usyd.edu.au
 *  - Mark Turner: mturner (at) gps.caltech.edu
 *
 * @section cppguidelines C++ Programming Guidelines
 *
 * It is strongly recommended that before you attempt to commence any serious
 * C++ programming work (whether on GPlates or any other project), you read the
 * book <i>Effective C++</i> (Meyers98).  This book provides invaluable,
 * easy-to-remember, practical guidelines for programming in C++ -- ignore its
 * wisdom at your peril!  If you've already read <i>Effective C++</i>, you may
 * be interested in <i>Exceptional C++</i> (Sutter00) -- the sections on
 * exception safety and the Pimpl idiom are particularly useful.  See the
 * Bibliography below for more details.
 * 
 * @section stdlib C++ Standard Library
 * 
 * The canonical reference for the C++ standard library (in particular, the
 * STL) is the book <i>The C++ Standard Library</i> (Josuttis99).  See the
 * Bibliography below for more details.
 *
 * @section patterns Design Patterns
 *
 * All the references to design patterns in the GPlates documentation refer to
 * the book <i>Design Patterns</i> (Gamma95).  See the Bibliography below for
 * more details.
 *
 * @section wxwidgets WxWidgets
 *
 * <a href="http://www.wxwidgets.org/">WxWidgets</a> (previously known as
 * WxWindows) is an open-source, cross-platform C++ GUI framework.  GPlates
 * currently requires version 2.4, release
 * <a href="http://www.wxwidgets.org/manuals/2.4.2/wx.htm">2.4.2</a> or above.
 *
 * @section bibliography Bibliography
 *
 * The following references have played a role in the design or implementation
 * of the project:
 *
 *  - Abrahams02:  David Abrahams, <i>Exception-Safety in Generic
 *     Components</i> [online].
 *     http://www.boost.org/more/generic_exception_safety.html  [Most-recently
 *     accessed 24 October 2005]
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
