libGK
=====
The purpose of this library is to provide the basic things that a game needs. 
Such as loading of images, fonts and sounds, drawing things on the screen, 
capturing the user input and provide tweening and timing functions. 

A "Hello World" app
-------------------
The code below creates a resizeable OpenGL window, which is ready for action.

	#include <gk.h>

	GK_BOOL init()
	{
		gkSetScreenSize(GK_SIZE(800,600));
		gkSetWindowTitle("Hello World");
		gkSetWindowResizable(GK_TRUE);
			
		/* initialize game */
		
		return GK_TRUE;
	}
	
	void cleanup()
	{
		/* destroy game */
	}
	
	GK_APP(init, cleanup)