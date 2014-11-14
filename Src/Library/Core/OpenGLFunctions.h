//==================================================================================================================|
// Created 2014.10.28 by Daniel L. Watkins
//
// Copyright (C) 2013-2014 Daniel L. Watkins
// This file is licensed under the MIT License.
//==================================================================================================================|

#ifndef _t3d_OPENGL_FUNCTIONS_H
#define _t3d_OPENGL_FUNCTIONS_H

#include <QtGui/qopenglfunctions_4_2_core.h>

namespace t3d
{
	/*
	 * Inherit from this class protectedly to access all of the standard OpenGL functions. Call
	 * OpenGLFunctions::initializeOpenGLFunctions() before using any of the actual OpenGL functions such as glEnable().
	 */
	class OpenGLFunctions : protected QOpenGLFunctions_4_2_Core
	{
	};
}

#endif