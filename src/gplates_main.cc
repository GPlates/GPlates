/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 */

/** 
 * @mainpage gPlates - Interactive Plate Tectonic Reconstructions
 * 
 * @section intro Introduction
 * Welcome to the gPlates Developers' Manual.
 *
 * @section patterns Design Patterns
 * All the references to design patterns in the gPlates documentation refer
 * to the book <i>Design Patterns</i> by Gamma, Helm, Johnson and Vlissides.
 * See the References section below for more details.
 *
 * @section contact Contact
 * <b>Email</b>:
 * - Dr. R. Dietmar M&uuml;ller <dietmar@es.usyd.edu.au>
 * - Stuart Clark <srclark@es.usyd.edu.au>
 * - James Boyden <jboyden@es.usyd.edu.au>
 * - Hamish Law <hlaw@es.usyd.edu.au>
 * - David Symonds <ds@es.usyd.edu.au>
 * 
 * <b>Snail Mail</b>:
 * - University of Sydney Institute of Marine Science<br>
 *   Edgeworth David Building F05<br>
 *   School of Geosciences<br>
 *   The University of Sydney, NSW 2006<br>
 *   AUSTRALIA<br>
 *
 * @section refs References
 * The following books and articles are either cited in the documentation 
 * or played a significant role in the project:
 * - Foley, J., van Dam, A., Feiner, S., and Hughes, J. (1996)
 *   <i>Computer Graphics: Principles and Practice (2nd Ed.)</i>,
 *   Addison-Wesley.
 * - Gahagan, L. (1999) <i>plates4.0: A User's Manual for the Plates
 *   Project's interactive reconstruction software</i>, The
 *   University of Texas Institute for Geophysics.
 * - Gamma, E., Helm, R., Johnson, R., and Vlissides, J. (1995)
 *   <i>Design Patterns: Elements of Reusable Object-Oriented Software</i>,
 *   Addison-Wesley.
 * - Greiner, B., "Euler Rotations in Plate-Tectonic Reconstructions"
 *   in <i>Computers and Geosciences</i> (1999) No. 25, pp209-216.
 * - Josuttis, N. (1999)
 *   <i>The C++ Standard Library: A Tutorial and Reference</i>,
 *   Addison-Wesley.
 * - Stoustrup, B. (2000)
 *   <i>The C++ Programming Language (3rd Ed.)</i>,
 *   Addison-Wesley.
 */

#include <cstdlib>  /* EXIT_SUCCESS */
#include <iostream>

#include "global/config.h"
#include "global/Exception.h"

#include "gui/GLWindow.h"
#include "geo/DataGroup.h"
#include "fileio/GPlatesReader.h"

int
main(int argc, char** argv)
{
	try {
		std::cout << "This is \"" PACKAGE_STRING "\".\n";
	
		GPlatesGui::GLWindow::GetWindow(&argc, argv);

		GPlatesGeo::DataGroup data("Cool Data (tm)", 42, 
			GPlatesGeo::GeologicalData::Attributes_t());

		GPlatesFileIO::GPlatesReader reader(argv[1]);
		reader.Read(data);
		
		glutMainLoop();
	} catch (const GPlatesGlobal::Exception& e) {
		std::cerr << "Caught exception: " << e << std::endl;
	}
	
	return EXIT_SUCCESS;
}
