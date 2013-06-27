var ex02 = function() {
	
	var container = undefined;
	var gl = undefined;
	var shaders = {};
	var uniforms = {};
	var attribs = {};
	var wvMatrix = undefined;
	var mwMatrix = undefined;
	var pMatrix = undefined;
	var shaderProgram = undefined;
	var rotX = 0;
	var rotY = 0;
	var prevMouse = undefined;
    var meshes = {};
		
	function updateMVP() {
		var mvMatrix = mat4.create();
		mat4.multiply(mvMatrix, wvMatrix, mwMatrix);
	    gl.uniformMatrix4fv(uniforms.projectionMatrix, false, pMatrix);
	    gl.uniformMatrix4fv(uniforms.modelViewMatrix, false, mvMatrix );
	}	
	
	function loadRGBCube() {
		
		var vertices = [
				-1,-1,-1, //0
				-1,-1,+1, //1
				-1,+1,-1, //2
				-1,+1,+1, //3
				+1,-1,-1, //4
				+1,-1,+1, //5
				+1,+1,-1, //6
				+1,+1,+1];//7
	
		var colors = [
				0,0,0, //0
				0,0,1, //1
				0,1,0, //2
				0,1,1, //3
				1,0,0, //4
				1,0,1, //5
				1,1,0, //6
				1,1,1];//7
			
		var faces = [
			0,4,5,	5,1,0,
			0,1,3,	3,2,0,
			0,2,6,	6,4,0,
			7,6,2,	2,3,7,
			7,5,4,	4,6,7,
			7,3,1,	1,5,7];
		
		meshes.cube = cglib.SimpleMesh.extend();
		meshes.cube.init(gl, faces)
			.addAttrib('position', 3, vertices)
			.addAttrib('color', 3, colors);
	}
	
	/**
	 * Draw a unit size RGB cube
	 */
	function drawRGBCube(drawMode) {
	
		if(!meshes.cube) {
			loadRGBCube();
		}
			
		meshes.cube.drawMode = drawMode;
		
		// Render
		meshes.cube.setAttribLocs(attribs);
		meshes.cube.render();				
	}
	
	function initShaders() {
	
		// Compile shaders
	    var vertexShader = cglib.WebGLCommon.compileShader(gl, shaders.vs, gl.VERTEX_SHADER);
	    var fragmentShader = cglib.WebGLCommon.compileShader(gl, shaders.fs, gl.FRAGMENT_SHADER);
	    
	    // Link program
	    shaderProgram = cglib.WebGLCommon.linkProgram(gl, [vertexShader, fragmentShader]);
	    gl.useProgram(shaderProgram);
	   
	    // Store attrib IDs
	    attribs = {};
	    attribs.position = gl.getAttribLocation(shaderProgram, "aPosition");    
	    attribs.color = gl.getAttribLocation(shaderProgram, "aColor");
	
	    // Store unfiform IDs
	    uniforms = {};
	    uniforms.projectionMatrix = gl.getUniformLocation(shaderProgram, "uProjectionMatrix");
	    uniforms.modelViewMatrix = gl.getUniformLocation(shaderProgram, "uModelViewMatrix");   
	}
	
	function initWorld() {
	    // Init global user-data
	    mwMatrix = mat4.create();
	    wvMatrix = mat4.create();
	    pMatrix = mat4.create();       
	    
		// Place camera at (0,0,10)
		mat4.translate(wvMatrix, wvMatrix, vec4.fromValues(0, 0, -10, 0));	
	}
	
	function initGL(context) {
		gl = context;
	    initShaders();
	    
		// Set background color to gray    
	    gl.clearColor(0.5, 0.5, 0.5, 1); 
	}
	
	function renderWorld() {
		
		// Clear FrameBuffer
	    gl.clear(gl.COLOR_BUFFER_BIT);
	    gl.enable(gl.CULL_FACE);
	
	    // Apply shaders
		gl.useProgram(shaderProgram);
		
		// Create camera transformation
		setupCamera();
		var saveMat = mat4.clone(mwMatrix);
		
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(-3, 0, 0, 0));
		updateMVP();	
		isDrawLines = true;
		drawRGBCube(gl.TRIANGLES);
		
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(+3, 0, 0, 0));
		updateMVP();
		drawRGBCube(gl.LINE_LOOP);
	
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(+3, 0, 0, 0));
		updateMVP();
		drawRGBCube(gl.POINTS);
	
		mwMatrix = saveMat;
	};
	
	function animate() {	
		//TODO:
	}
	
	function setupProjectionAndViewport() {
		
		// Setup viewport
		var height = gl.viewportHeight;
		var width = gl.viewportWidth;	
	    gl.viewport(0, 0, width, height);
	    
	    // Create projection transformation   
		w = 10;
		h = 10*(height/width);
		mat4.ortho(pMatrix, -w/2, w/2, -h/2, h/2, -1, 1000);
	}
	
	/**
	* Create camera transformation such that the model is rotated around the
	* world's X-axis and Y-axis. 
	*/
	function setupCamera() {
	
		// Rotate along temp in world coordinates	
		var yAxisInModelSpace = vec3.fromValues(mwMatrix[1], mwMatrix[5], mwMatrix[9]);
		mat4.rotate(mwMatrix, mwMatrix, -rotX/180*3.14, yAxisInModelSpace); 
		rotX = 0;
		
		// Rotate along the (1 0 0) in world coordinates
		var xAxisInModelSpace = vec3.fromValues(mwMatrix[0], mwMatrix[4], mwMatrix[8]);
		mat4.rotate(mwMatrix, mwMatrix, -rotY/180*3.14, xAxisInModelSpace);
		rotY = 0;				
	}	
	
	function renderScene() {	
	    animate();
	    setupProjectionAndViewport();
	    renderWorld();	    
	}
	
	function init(_container) {
		container = _container;
	    container.setDisplay(renderScene);	    
	    container.loadResources(['ex02.vert', 'ex02.frag']);
	}

	function start() {
	    // Load resources
	    shaders = {};
	    shaders.fs = container.getShaderText('ex02.frag');
	    shaders.vs = container.getShaderText('ex02.vert');

	    initGL(container.getContext());         
	    
	    initWorld();
	    
	    renderScene();		
	}
	
	function mouseFunc(x, y) {
		prevMouse = {"x":x, "y":y};
	}
	
	function motionFunc(x, y) {
	
		// Calc difference from previous mouse location
		if(!prevMouse)
			prevMouse = {"x":x, "y":y};
		
		prev = prevMouse;
		dx = prev.x - x;
		dy = prev.y - y;
	
		// Rotate model
		rotate(dx, dy);
	
		// Remember mouse location 
		prevMouse = {"x":x, "y":y};	
	}
	
	function rotate(x, y) {
		rotX += x;
		rotY += y;
	}
	
	function release() {
		for(var mesh in meshes) {
			meshes[mesh].release(gl);
		}
	}
	
	return {
		init : init,
		start : start,
		release : release,
		mouseFunc : mouseFunc,
		motionFunc : motionFunc
	};
	
} ();