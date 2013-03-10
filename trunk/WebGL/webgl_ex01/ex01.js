var gl;
var UD = {
	shaders : {}		
};

function setColor(r,g,b) {
	gl.uniform3fv(UD.uniforms.color, [r,g,b]);
}

function updateMVP() {
	var mvMatrix = mat4.create();
	mat4.multiply(UD.wvMatrix,UD.mwMatrix,mvMatrix);
    gl.uniformMatrix4fv(UD.uniforms.pMatrix, false, UD.pMatrix);
    gl.uniformMatrix4fv(UD.uniforms.mvMatrix, false, mvMatrix );
    //gl.uniformMatrix3fv(UD.uniforms.NMatrix, false, mat4.toInverseMat3(UD.mvMatrix));
}	

function drawSquare(x,y,s) {
	if(!UD.meshes.square) {

		// init square
		var vertices = [
		                -1.0,-1.0,0,
		                +1.0,-1.0,0,
		                +1.0,+1.0,0,
		                -1.0,+1.0,0
		];
		var indices = [
		               0,1,2,
		               2,3,0
		               ]; 
			
		UD.meshes.square = mesh_init(vertices,indices,[]);		
	}
	
	// Set world position
	mat4.identity(UD.mwMatrix);
	mat4.translate(UD.mwMatrix, [x,y,0,1]);
	mat4.scale(UD.mwMatrix, [s,s,s]);
	updateMVP();

	UD.meshes.square.drawMode = UD.isDrawLines?gl.LINE_LOOP:gl.TRIANGLES;
	
	// Render
	mesh_render(UD.meshes.square,UD.attribs.vertexPos);	
}

function drawCircle(x,y,s) {

	if(!UD.meshes.circle) {

		// init circle
		var vertices = [0,0,0];
		var indices = [0];
		var cnt = 1;
		for(var alpha = 2*Math.PI/36; alpha<2*Math.PI; alpha+=2*Math.PI/36) {
			vertices.push(Math.cos(alpha));
			vertices.push(Math.sin(alpha));
			vertices.push(0);
			indices.push(cnt);
			cnt++;
		}
		indices.push(1);
		
		UD.meshes.circle = mesh_init(vertices,indices,[]);
		UD.meshes.circle.drawMode = gl.TRIANGLE_FAN;
	}

	//glPushMatrix();
	mat4.identity(UD.mwMatrix);
	mat4.translate(UD.mwMatrix, [x,y,0,1]);
	mat4.scale(UD.mwMatrix, [s,s,s]);
	updateMVP();
	
	// Render
	mesh_render(UD.meshes.circle,UD.attribs.vertexPos);	

	//glPopMatrix();
}

function drawTriangle(x,y,s) {
	if(!UD.meshes.triangle) {

		// init square
		var vertices = [
		                -1.0,0,0,
		                +1.0,0,0,
		                0,+1.0,0
		];
		var indices = [
		               0,1,2
		               ]; 
			
		UD.meshes.triangle = mesh_init(vertices,indices,[]);		
	}
	
	// Set world position
	mat4.identity(UD.mwMatrix);
	mat4.translate(UD.mwMatrix, [x,y,0,1]);
	mat4.scale(UD.mwMatrix, [s,s,s]);
	updateMVP();

	UD.meshes.triangle.drawMode = UD.isDrawLines?gl.LINE_LOOP:gl.TRIANGLES;
	
	// Render
	mesh_render(UD.meshes.triangle,UD.attribs.vertexPos);	
}

function world_render() {
		
	// Sky
	setColor(0, 1, 0);
	drawSquare(0, -10, 10);

	// Ground
	setColor(0, 1, 1);
	drawSquare(0, +10, 10);		

	// Sun
	setColor(1, 0.75, 0);
	drawCircle(-2,2,0.5);

	// House

	setColor(0.85, 0.85, 0.85); //TODO: Color
	drawSquare(0, 0, 0.5);

	// Roof
	setColor(1, 0.5, 0); //TODO: Color
	drawTriangle(0, 0.5, 0.5);
	
	UD.isDrawLines = true;
	setColor(0, 0, 0);
	drawTriangle(0, 0.5, 0.5);
	UD.isDrawLines = false;

	// Windows

	UD.isDrawLines = true;
	setColor(0.3, 0.3, 0.85);					//
	drawSquare(-0.5/2, +0.5/4, 0.5/4);					
	drawSquare(+0.5/2, +0.5/4, 0.5/4);
	UD.isDrawLines = false;	
};

function animate() {
	mat4.identity(UD.wvMatrix);
	var s = (Math.cos(UD.frame/100.0));
	mat4.rotate(UD.wvMatrix,s*30 * Math.PI/180.0, [0, 0, 1]);
	s = (s+1)/2;
	s = (1-s)*1/5 + s*1;
	mat4.scale(UD.wvMatrix,[s, s, s]);
}


function drawScene() {
	UD.frame++;
	
	var h = gl.viewportHeight;
	var w = gl.viewportWidth;
	
    gl.viewport(0, 0, w, h);
    //gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    //mat4.perspective(45, gl.viewportWidth / gl.viewportHeight, 0.1, 100.0, UD.pMatrix);
    mat4.identity(UD.pMatrix);
    
	// Maintain aspect ratio
	if (w <= h) {
		mat4.ortho(-2.5, 2.5, -2.5 * h / w,	2.5 * h / w, -10.0, 10.0, UD.pMatrix);
	} else {		
		mat4.ortho(-2.5 * w / h, 2.5 * w / h, -2.5, 2.5, -10.0, 10.0, UD.pMatrix);
	}
    
    //mat4.ortho(-5,1,-1,1,-1,1, UD.pMatrix);
    
    gl.useProgram(UD.shaderProgram);
    
    animate();
    world_render();

	//mat4.scale(UD.mvMatrix,[0.25,0.25,0.25]);
    requestAnimFrame(drawScene);
}

function initShaders() {

	// Compile shaders
    var vertexShader = compileShader(UD.shaders.vs,gl.VERTEX_SHADER);
    var fragmentShader = compileShader(UD.shaders.fs,gl.FRAGMENT_SHADER);
    
    // Link program
    UD.shaderProgram = linkProgram(vertexShader, fragmentShader);
    gl.useProgram(UD.shaderProgram);
   
    // Store attrib IDs
    UD.attribs = {};
    UD.attribs.vertexPos = gl.getAttribLocation(UD.shaderProgram, "aVertex");    
    //UD.attribs.vertexNormal = gl.getAttribLocation(UD.shaderProgram, "aNormal");

    // Store unfiform IDs
    UD.uniforms = {};
    UD.uniforms.pMatrix = gl.getUniformLocation(UD.shaderProgram, "uPMatrix");
    UD.uniforms.mvMatrix = gl.getUniformLocation(UD.shaderProgram, "uMVMatrix");
    UD.uniforms.color = gl.getUniformLocation(UD.shaderProgram, "uColor");
    /*
    UD.uniforms.NMatrix = gl.getUniformLocation(UD.shaderProgram, "uNMatrix");
    UD.uniforms.lightPosition = gl.getUniformLocation(UD.shaderProgram, "uLightPosition");
    UD.uniforms.eyePosition = gl.getUniformLocation(UD.shaderProgram, "uEyePosition");    
    UD.uniforms.diffuse = gl.getUniformLocation(UD.shaderProgram, "uDiffuse");
    UD.uniforms.ambient = gl.getUniformLocation(UD.shaderProgram, "uAmbient");
    UD.uniforms.specular = gl.getUniformLocation(UD.shaderProgram, "uSpecular");
    UD.uniforms.specularPower = gl.getUniformLocation(UD.shaderProgram, "uSpecularPower");
    */      
}

function initGL(canvas) {
    try {
        gl = canvas.getContext("experimental-webgl");
        gl.viewportWidth = canvas.width;
        gl.viewportHeight = canvas.height;
    } catch (e) {
    }
    if (!gl) {
        alert("Could not initialise WebGL, sorry :-(");
    }
}

function webGLStart() {
    var canvas = document.getElementById("mycanvas");
    initGL(canvas);
    
    UD.shaders = {};
    UD.shaders.fs = getShaderScript(gl,"shader-fs");
    UD.shaders.vs = getShaderScript(gl,"shader-vs");
    
    initShaders();   
       
    //world_init();
    
    gl.clearColor(0.0, 0.5, 0.5, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
    //gl.enable(gl.DEPTH_TEST);
    
    UD.mwMatrix = mat4.create();
    UD.wvMatrix = mat4.create();
    UD.pMatrix = mat4.create(); 
    UD.meshes = {};
    UD.color = {};    
    UD.frame = 0;
    
    drawScene();
}
