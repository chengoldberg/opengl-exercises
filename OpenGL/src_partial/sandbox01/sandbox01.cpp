#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

// ============================================================================
// This project contains a minimal environment to immediately start working
// with OpenGL.
// ============================================================================

//TODO: Initialize OpenGL states here
void init() {
	
}

//TODO: Render OpenGL scene here
void display(void) {

}

//TODO: Handle window resize event here
void reshape(int width, int height) {

}

void keyboardFunc(unsigned char key, int x, int y) {
	switch(key) {
	case 27:	// Quit on 'Escape' key
		exit(0);
	}
}

int main(int argc, char **argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);	//TODO: Add other required surfaces if needed
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-512)/2, (glutGet(GLUT_SCREEN_HEIGHT)-512)/2);
	glutInitWindowSize(512, 512);

	glutCreateWindow("Sandbox01");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutKeyboardFunc(keyboardFunc);

	//glutFullScreen();

	init();

	glutMainLoop();

	return 0;  
}

