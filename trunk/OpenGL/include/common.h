
float colorWhite[] = {1,1,1,1};
float colorBlack[] = {0,0,0,0};

// Fixes bug where teapot assumed CCW is default
void glutSolidTeapotFIX(double size) {
	int frontFace;
	glGetIntegerv(GL_FRONT_FACE, &frontFace);
	if(frontFace == GL_CCW)
		glFrontFace(GL_CW);
	else
		glFrontFace(GL_CCW);

	glutSolidTeapot(size);
	if(frontFace == GL_CCW)
		glFrontFace(GL_CCW);
	else
		glFrontFace(GL_CW);
}
