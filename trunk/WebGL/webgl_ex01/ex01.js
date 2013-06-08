var ex01 = function() {

	var container = undefined;
	var gl = undefined;
	var shaders = {};
	var uniforms = {};
	var attribs = {};
	var wvMatrix = undefined;
	var mwMatrix = undefined;
	var pMatrix = undefined;
	var shaderProgram = undefined;
    var meshes = {};
    var isDrawLines = undefined;

	function setColor(r,g,b) {
		gl.uniform3fv(uniforms.color, [r,g,b]);
	}

	function updateMVP() {
		var mvMatrix = mat4.create();
		//mat4.multiply(wvMatrix,mwMatrix,mvMatrix);
		mat4.multiply(mvMatrix, wvMatrix, mwMatrix);
	    gl.uniformMatrix4fv(uniforms.pMatrix, false, pMatrix);
	    gl.uniformMatrix4fv(uniforms.mvMatrix, false, mvMatrix );
	    //gl.uniformMatrix3fv(UD.uniforms.NMatrix, false, mat4.toInverseMat3(UD.mvMatrix));
	}	

	function drawSquare(x,y,s) {
		if(!meshes.square) {

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
			
			meshes.square = cglib.SimpleMesh.extend();
			meshes.square.init(gl, indices)
				.addAttrib('position', 3, vertices);
		}
	
		// Set world position
		mat4.identity(mwMatrix);
		mat4.translate(mwMatrix, mwMatrix, [x,y,0,1]);
		mat4.scale(mwMatrix, mwMatrix, [s,s,s]);
		updateMVP();

		meshes.square.drawMode = isDrawLines?gl.LINE_LOOP:gl.TRIANGLES;
		
		// Render
		meshes.square.setAttribLocs(attribs);
		meshes.square.render();	
	}

	function drawCircle(x,y,s) {

		if(!meshes.circle) {

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
			
			meshes.circle = cglib.SimpleMesh.extend();
			meshes.circle.init(gl, indices)
				.addAttrib('position', 3, vertices);							
			meshes.circle.drawMode = gl.TRIANGLE_FAN;			
		}

		//glPushMatrix();
		mat4.identity(mwMatrix);
		mat4.translate(mwMatrix, mwMatrix, [x,y,0,1]);
		mat4.scale(mwMatrix, mwMatrix, [s,s,s]);
		updateMVP();
		
		// Render
		meshes.circle.setAttribLocs(attribs);
		meshes.circle.render();	

		//glPopMatrix();
	}

	function drawTriangle(x,y,s) {
		if(!meshes.triangle) {

			// init square
			var vertices = [
			                -1.0,0,0,
			                +1.0,0,0,
			                0,+1.0,0
			];
			var indices = [
			               0,1,2
			               ]; 
				
			meshes.triangle = cglib.SimpleMesh.extend();
			meshes.triangle.init(gl, indices)
				.addAttrib('position', 3, vertices);
		}
		
		// Set world position
		mat4.identity(mwMatrix);
		mat4.translate(mwMatrix, mwMatrix, [x,y,0,1]);
		mat4.scale(mwMatrix, mwMatrix, [s,s,s]);
		updateMVP();

		meshes.triangle.drawMode = isDrawLines?gl.LINE_LOOP:gl.TRIANGLES;
		
		// Render
		meshes.triangle.setAttribLocs(attribs);
		meshes.triangle.render();			
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
		
		isDrawLines = true;
		setColor(0, 0, 0);
		drawTriangle(0, 0.5, 0.5);
		isDrawLines = false;

		// Windows

		isDrawLines = true;
		setColor(0.3, 0.3, 0.85);
		drawSquare(-0.5/2, +0.5/4, 0.5/4);					
		drawSquare(+0.5/2, +0.5/4, 0.5/4);
		isDrawLines = false;	
	}

	function animate() {
		mat4.identity(wvMatrix);
		var s = (Math.cos(frame/100.0));
		mat4.rotate(wvMatrix, wvMatrix, s*30 * Math.PI/180.0, [0, 0, 1]);
		s = (s+1)/2;
		s = (1-s)*1/5 + s*1;
		mat4.scale(wvMatrix, wvMatrix, [s, s, s]);
	}

	function drawScene() {
		frame++;
		
		var h = gl.viewportHeight;
		var w = gl.viewportWidth;
		
	    gl.viewport(0, 0, w, h);
	    gl.clear(gl.COLOR_BUFFER_BIT);

	    //mat4.perspective(45, gl.viewportWidth / gl.viewportHeight, 0.1, 100.0, UD.pMatrix);
	    mat4.identity(pMatrix);
	    
		// Maintain aspect ratio
		if (w <= h) {
			mat4.ortho(-2.5, 2.5, -2.5 * h / w,	2.5 * h / w, -10.0, 10.0, pMatrix);
		} else {		
			mat4.ortho(-2.5 * w / h, 2.5 * w / h, -2.5, 2.5, -10.0, 10.0, pMatrix);
		}
	    
	    //mat4.ortho(-5,1,-1,1,-1,1, UD.pMatrix);
	    
	    gl.useProgram(shaderProgram);
	    
	    animate();
	    world_render();

		//mat4.scale(UD.mvMatrix,[0.25,0.25,0.25]);
	}

	function initShaders() {

		// Compile shaders
	    var vertexShader = cglib.WebGLCommon.compileShader(gl, shaders.vs,gl.VERTEX_SHADER);
	    var fragmentShader = cglib.WebGLCommon.compileShader(gl, shaders.fs,gl.FRAGMENT_SHADER);
	    
	    // Link program
	    shaderProgram = cglib.WebGLCommon.linkProgram(gl, vertexShader, fragmentShader);
	    gl.useProgram(shaderProgram);
	   
	    // Store attrib IDs
	    attribs = {};
	    attribs.position = gl.getAttribLocation(shaderProgram, "aVertex");    

	    // Store unfiform IDs
	    uniforms = {};
	    uniforms.pMatrix = gl.getUniformLocation(shaderProgram, "uPMatrix");
	    uniforms.mvMatrix = gl.getUniformLocation(shaderProgram, "uMVMatrix");
	    uniforms.color = gl.getUniformLocation(shaderProgram, "uColor");
	}

	function initWorld() {
	    // Init global user-data    
	    mwMatrix = mat4.create();
	    wvMatrix = mat4.create();
	    pMatrix = mat4.create(); 
	    meshes = {};
	    color = {};    
	    frame = 0;	
	}
	
	function initGL(context) {
		gl = context;
	    initShaders();	   	     
	    
	    gl.clearColor(0.0, 0.5, 0.5, 1);
	    gl.clear(gl.COLOR_BUFFER_BIT);
	}

	function init(_container) {
		container = _container;
	    container.setDisplay(drawScene);	    
	    container.loadResources(['ex01.vert', 'ex01.frag']);
	}

	function start() {
	    // Load resources
	    shaders = {};
	    shaders.fs = container.getShaderText('ex01.frag');
	    shaders.vs = container.getShaderText('ex01.vert');

	    initGL(container.getContext());         
	    
	    initWorld();	    
	}

	function getDebug(val) {
		return eval(val);
	}	

	return {
		init : init,
		start : start,
		getDebug : getDebug,
	};	
} ();
