/* $Id$ */

/**
 * @file 
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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */

#include <iostream>
#include <cstdlib>
#include <cmath>  /* for fabsf() */
#include "OpenGL.h"
#include "GLWindow.h"

using namespace GPlatesGui;

GLWindow* GLWindow::_window = NULL;

GLWindow*
GLWindow::GetWindow(int *argc, char **argv)
{
	if (_window == NULL)
		_window = new GLWindow(argc, argv);
	return _window;
}

GLWindow::GLWindow(int *argc, char **argv)
 // For some reason, if this is removed, the globe is black
 : _globe(Colour(1.0, 1.0, 1.0, 0.0))   // Globe should be transparent wrt data
{
	// Single buffering, RGBA mode, depth => 3D.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	// Set the default position and size of the window.  These may be
	// overridden via the commandline.
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(600, 600);
	
	// Initialise (wrt the commandline arguments).
	glutInit(argc, argv);
	// The window title should be set via a parameter.
	glutCreateWindow("GPlates Ueber Alles!");
	
	// Set the callback functions
	glutDisplayFunc(&GLWindow::Display);
	glutReshapeFunc(&GLWindow::Reshape);
	glutKeyboardFunc(&GLWindow::Keyboard);
	glutSpecialFunc(&GLWindow::Special);

	// Enable depth buffering.
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Enable blending so that the Globe doesn't get in the way
	// of the data.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_DST_ALPHA);

	/**
	 * Disable lighting, since we're only drawing wire-frame at the
	 * moment.
	 */
#if 0
	// Initialise lighting
	GLfloat specular[] = { 0.0, 0.0, 0.0, 0.0 };
	GLfloat shininess[] = { 500.0 };
	GLfloat light_pos[] = { 1.0, 1.0, 1.0, 0.0 };
	glShadeModel(GL_SMOOTH);

	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
#endif
}

void
GLWindow::Clear(const Colour& c)
{
	// Set colour buffer's clearing colour
	glClearColor(c.Red(), c.Green(), c.Blue(), 0.0);
	// Clear window to current clearing colour.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static GLfloat eyex = 0.0, eyey = 0.0, eyez = -5.0;
static GLfloat ORTHO_RATIO = 1.2;

void
GLWindow::Display()
{
	_window->Clear();
	glLoadIdentity();
	glTranslatef(eyex, eyey, eyez);
	_window->_globe.Draw();
	glutSwapBuffers();
}

void
GLWindow::Reshape(int width, int height)
{
	glViewport(0, 0, static_cast<GLsizei>(width),
	 static_cast<GLsizei>(height));
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLfloat fwidth = static_cast<GLfloat>(width);
	GLfloat fheight = static_cast<GLfloat>(height);
	GLfloat eye_dist = static_cast<GLfloat>(fabsf(eyez));
	GLfloat aspect = (width <= height) ? fheight/fwidth : fwidth/fheight;
	
	glOrtho(-ORTHO_RATIO, ORTHO_RATIO, -ORTHO_RATIO*aspect, 
	 ORTHO_RATIO*aspect, 0.1, eye_dist);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void
GLWindow::Keyboard(unsigned char key, int, int)
{
	if (key == 'q')
		exit(0);
	if (key == 'r') {  // reset
		_window->_globe.GetElevation() = 0.0;
		_window->_globe.GetMeridian() = 0.0;
//		eyez = -5.0;
		ORTHO_RATIO = 1.2;
		// XXX: PostRedisplay doesn't appear to work!!
		glutPostRedisplay();
	}
}

void
GLWindow::Special(int key, int, int)
{
	switch (key) {
		case GLUT_KEY_UP:
			if (glutGetModifiers() == GLUT_ACTIVE_ALT) {
				// Move eye closer
//				eyez += 0.1;
				ORTHO_RATIO -= 0.1;
			} else {
				_window->_globe.GetElevation() -= 1.0;
			}
			break;
			
		case GLUT_KEY_DOWN:
			if (glutGetModifiers() == GLUT_ACTIVE_ALT) {
				// Move eye away
//				eyez -= 0.1;
				ORTHO_RATIO += 0.1;
			} else {
				_window->_globe.GetElevation() += 1.0;
			}
			break;
			
		case GLUT_KEY_LEFT:
			_window->_globe.GetMeridian() -= 1.0;
			break;
			
		case GLUT_KEY_RIGHT:
			_window->_globe.GetMeridian() += 1.0;
			break;
	}
	Reshape(600,600);  // FIXME: bad, Bad, BAD!!
	glutPostRedisplay();
}
