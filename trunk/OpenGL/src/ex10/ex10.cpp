/*
 * Copyright (C) 2010  Chen Goldberg
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Windows.h>
#include <GL\glew.h>
#include <stdio.h>

#pragma comment (lib, "glu32.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "Opengl32.lib")

bool flag;

HWND g_hWnd[] = {NULL,NULL};
HGLRC g_hRC[] = {NULL,NULL};    // Permanent rendering context
HDC g_hDC[] = {NULL,NULL};      // Private GDI device context

LPCWSTR lpszAppName = (LPCWSTR) "tester";

// ============================== Helper Classes =========================

class GLContextListener {
public:

	GLContextListener() : m_ID(0) {
	}

	virtual void display() = 0;
	virtual void idle() = 0;
	virtual void reshape(WORD width, WORD height) = 0;

	GLuint m_ID;
};

GLContextListener* g_scenes[2];

class Scene1 : public GLContextListener {
public:

	Scene1() { 
	}

	void display() {
		static int frame = 0;
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(1,0,1);
		glPointSize(50);
		glBegin(GL_POINTS);
		glVertex2d(0,0);
		glEnd();

		if(m_ID == 0) {
			m_ID = glGenLists(1);
			glNewList(m_ID, GL_COMPILE);			
			glBegin(GL_TRIANGLES);
			glColor3f(1,0,0);
			glVertex2d(-0.5,-0.5);
			glColor3f(0,1,0);
			glVertex2d(+0.5,-0.5);
			glColor3f(0,0,1);
			glVertex2d(0,+0.5);
			glEnd();
			glEndList();
		}
		glCallList(m_ID);
		frame++;
		if(frame == 33*5) {
			flag ^= true;
			frame = 0;
		}
	}

	void idle() {
	}

	void reshape(WORD width, WORD height) {
		glClearColor(0,0,0,0);
		glViewport(0,0,width,height);
	}

};

class Scene0 : public GLContextListener {
public:
	void display() {
		glRotated(1,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(1,0,0);
		glPointSize(5);
		glBegin(GL_POINTS);
		glVertex2d(1,0);
		glEnd();

		glCallList(g_scenes[1]->m_ID);
	}

	void idle() {
	}

	void reshape(WORD width, WORD height) {
		glClearColor(0,0,0,0);
		glViewport(0,0,width,height);
	}
};

// ============================== Helper Functions =========================

int checkForGLErrors( const char *s ) {
  int errors = 0 ;
  int counter = 0 ;
  while ( counter < 1000 )
  {
    GLenum x = glGetError() ;
    if ( x == GL_NO_ERROR )
      return errors ;
    fprintf( stderr, "%s: OpenGL error: %s [%08x]\n", s ? s : "", gluErrorString ( x ), counter ) ;
    errors++ ;
    counter++ ;
  }
  return errors;
}


//
// Select the pixel format for a given device context
//
void SetDCPixelFormat(HDC hDC)
{
	int nPixelFormat;
	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), // Size of this structure
		1,                             // Version of this structure
		PFD_DRAW_TO_WINDOW |           // Draw to window (not to bitmap)
		PFD_SUPPORT_OPENGL |           // Support OpenGL calls in window
		PFD_DOUBLEBUFFER,              // Double-buffered mode
		PFD_TYPE_RGBA,                 // RGBA color mode
		32,                            // Want 32-bit color
		0,0,0,0,0,0,                   // Not used to select mode
		0,0,                           // Not used to select mode
		0,0,0,0,0,                     // Not used to select mode
		16,                            // Size of depth buffer
		0,                             // Not used here
		0,                             // Not used here
		0,                             // Not used here
		0,                             // Not used here
		0,0,0 };                       // Not used here
		// Choose a pixel format that best matches that described in pfd
		nPixelFormat = ChoosePixelFormat(hDC, &pfd);
		// Set the pixel format for the device context
		SetPixelFormat(hDC, nPixelFormat, &pfd);
}

//
// Window procedure, handles all messages for this program
//
LRESULT CALLBACK WndProc(HWND    hWnd,
	UINT    message,
	WPARAM    wParam,
	LPARAM    lParam) {

	unsigned int wndIndex;
	if(g_hWnd[0] == NULL)
		wndIndex = 0;
	else if(g_hWnd[1] == NULL)
		wndIndex = 1;
	else if(hWnd == g_hWnd[0])
		wndIndex = 0;
	else if(hWnd == g_hWnd[1])
		wndIndex = 1;
	else
		return (DefWindowProc(hWnd, message, wParam, lParam));

	HGLRC hRC = g_hRC[wndIndex];
	HDC hDC = g_hDC[wndIndex];

	GLContextListener* scene = g_scenes[wndIndex];

	switch (message)
	{
		// Window creation, set up for OpenGL
	case WM_CREATE:
		// Store the device context
		hDC = GetDC(hWnd);
		// Select the pixel format
		SetDCPixelFormat(hDC);
		// Create the rendering context and make it current
		hRC = wglCreateContext(hDC);
		g_hRC[wndIndex] = hRC;

		if(wndIndex==1) {
			if(wglShareLists(g_hRC[0],g_hRC[1]))
				puts("wglShareLists successful!");
			else
				puts("wglShareLists failed!");
		}

		wglMakeCurrent(hDC, hRC);
		// Create a timer that fires 30 times a second
		SetTimer(hWnd,33,1,NULL);

		g_hWnd[wndIndex] = hWnd;
		g_hDC[wndIndex] = hDC;		

		break;
		// Window is being destroyed, clean up
	case WM_DESTROY:
		// Kill the timer that we created
		KillTimer(hWnd,101);
		// Deselect the current rendering context and delete it
		wglMakeCurrent(hDC,NULL);
		wglDeleteContext(hRC);
		// Tell the application to terminate after the window
		// is gone.
		PostQuitMessage(0);
		break;
		// Window is resized.
	case WM_SIZE:
		wglMakeCurrent(hDC, hRC);
		// Call our function which modifies the clipping
		// volume and viewport
		scene->reshape(LOWORD(lParam), HIWORD(lParam));
		break;
		// Timer moves and bounces the rectangle, simply calls
		// our previous OnIdle function, then invalidates the
		// window so it will be redrawn.
	case WM_TIMER:
		{
			wglMakeCurrent(hDC, hRC);
			scene->idle();
			InvalidateRect(hWnd,NULL,FALSE);
		}
		break;
		// The painting function. This message is sent by Windows
		// whenever the screen needs updating.
	case WM_PAINT:
		{
			wglMakeCurrent(hDC, hRC);
			// Call OpenGL drawing code
			scene->display();
			// Call function to swap the buffers
			SwapBuffers(hDC);
			// Validate the newly painted client area
			ValidateRect(hWnd,NULL);
		}
		break;

	default:   // Passes it on if unprocessed
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0L);
}

// Entry point of all Windows programs
int APIENTRY WinMain(    HINSTANCE     hInstance,
	HINSTANCE     hPrevInstanc,
	LPSTR         lpCmdLine,
	int           nCmdShow)
{
	MSG        msg;       // Windows message structure
	WNDCLASS   wc;        // Windows class structure
	HWND       hWnd;      // Storage for window handle
	// Register window style
	wc.style        = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc  = (WNDPROC) WndProc;
	wc.cbClsExtra   = 0;
	wc.cbWndExtra   = 0;
	wc.hInstance    = hInstance;
	wc.hIcon        = NULL;
	wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
	// No need for background brush for OpenGL window
	wc.hbrBackground  = NULL;
	wc.lpszMenuName   = NULL;
	wc.lpszClassName  = lpszAppName;

	// Register the window class
	if(RegisterClass(&wc) == 0)
		return FALSE;

	g_scenes[0] = new Scene0();
	g_scenes[1] = new Scene1();


	// Create the main application window
	hWnd = CreateWindow(
		lpszAppName,
		lpszAppName,
		// OpenGL requires WS_CLIPCHILDREN and WS_CLIPSIBLINGS
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		// Window position and size
		100, 100,
		512, 512,
		NULL,
		NULL,
		hInstance,
		NULL);
	g_hWnd[0] = hWnd;

	// If window was not created, quit
	if(hWnd == NULL)
		return FALSE;
	// Display the window
	ShowWindow(hWnd,SW_SHOW);
	UpdateWindow(hWnd);	

	// Create the second application window
	hWnd = CreateWindow(
		lpszAppName,
		lpszAppName,
		// OpenGL requires WS_CLIPCHILDREN and WS_CLIPSIBLINGS
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		// Window position and size
		512+100, 100,
		512, 512,
		NULL,
		NULL,
		hInstance,
		NULL);
	g_hWnd[1] = hWnd;

	// If window was not created, quit
	if(hWnd == NULL)
		return FALSE;
	// Display the window
	ShowWindow(hWnd,SW_SHOW);
	UpdateWindow(hWnd);

	// Process application messages until the application closes
	while( GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}