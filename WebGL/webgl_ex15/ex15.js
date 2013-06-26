var ex15 = function() {
	
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
	var targetRotX = undefined;
	var targetRotY = undefined;
	var prevMouse = undefined;
    var meshes = {};
    var textures = {};
    var canvas2DTexture = undefined;
    var frame = 0;
	var resourcesURLs = {
		images : 
		{
			eql1 : '../res/texture/ex15/eql1.png',
			eql2 : '../res/texture/ex15/eql2.png',
			eql12map : '../res/texture/ex15/eql12map_maxmin.png',
		},
	}
		
	function updateMVP() {
		var mvMatrix = mat4.create();
		mat4.multiply(mvMatrix, wvMatrix, mwMatrix);
	    gl.uniformMatrix4fv(uniforms.projectionMatrix, false, pMatrix);
	    gl.uniformMatrix4fv(uniforms.modelViewMatrix, false, mvMatrix );
	}	
	
	function drawPoints(level) {

	    gl.activeTexture(gl.TEXTURE0);
	    gl.bindTexture(gl.TEXTURE_2D, textures.img1to2);
	    gl.uniform1i(uniforms.texImg1to2, 0);
	    
	    gl.activeTexture(gl.TEXTURE1);
	    gl.bindTexture(gl.TEXTURE_2D, textures.img1);
	    gl.uniform1i(uniforms.texImg1, 1);
	    
	    gl.uniform1i(uniforms.frameNum, frame);
	
		meshes.points.setAttribLocs(attribs);
		meshes.points.render();
	}

	function renderWorld() {
		
		// Clear FrameBuffer
	    gl.clear(gl.COLOR_BUFFER_BIT);
	
	    // Apply shaders
		gl.useProgram(shaderProgram);
		
		// Create camera transformation
		setupCamera();
		var saveMat = mat4.clone(mwMatrix);
		
		// Put the pixel cube in the center
		mat4.translate(mwMatrix, mwMatrix, [-127.0, -127.0, +127.0]);
		updateMVP();	
		drawPoints();		

		mwMatrix = saveMat;
	};
	
	function initMeshes() {

		var posData = [];
		for (var x = 0; x < 256; ++x) {
		  for (var y = 0; y < 256; ++y) {
		  	posData.push(x,y,0);
		  }
		}

		mesh = cglib.SimpleMesh.extend();
		mesh.init(gl);
		mesh.addAttrib('position', 3, posData);
		mesh.drawMode = gl.POINTS;

		meshes.points = mesh;
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
	    attribs.texCoord = gl.getAttribLocation(shaderProgram, "aTexCoord");    
	
	    // Store unfiform IDs
	    uniforms = {};
	    uniforms.projectionMatrix = gl.getUniformLocation(shaderProgram, "uProjectionMatrix");
	    uniforms.modelViewMatrix = gl.getUniformLocation(shaderProgram, "uModelViewMatrix");   
	    uniforms.color = gl.getUniformLocation(shaderProgram, "uColor");
	    uniforms.frameNum = gl.getUniformLocation(shaderProgram, "uFrameNum");	    
	    uniforms.texImg1 = gl.getUniformLocation(shaderProgram, "uTexImg1");
	    uniforms.texImg1to2 = gl.getUniformLocation(shaderProgram, "uTexImg1to2");
	}
	
	function initWorld() {
	    // Init global user-data
	    mwMatrix = mat4.create();
	    wvMatrix = mat4.create();
	    pMatrix = mat4.create();           
	}
	
	function initGL(context) {
		gl = context;
	    initShaders();
	    initMeshes();

	    gl.enable(gl.DEPTH_TEST);

	    canvas2DTexture = cglib.Canvas2DTexture.extend()
	    canvas2DTexture.init(512,512);

	    textures.img1 = gl.createTexture();
		gl.bindTexture(gl.TEXTURE_2D, textures.img1);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, container.images.eql1);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.bindTexture(gl.TEXTURE_2D, null);	    

		textures.img2 = gl.createTexture();	    
		gl.bindTexture(gl.TEXTURE_2D, textures.img2);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, container.images.eql2);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.bindTexture(gl.TEXTURE_2D, null);	    

		textures.img1to2 = gl.createTexture();	    
		gl.bindTexture(gl.TEXTURE_2D, textures.img1to2);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, container.images.eql12map);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.bindTexture(gl.TEXTURE_2D, null);	    

		// Set background color to teal
	    gl.clearColor(0, 0.63, 0.9, 1); 
	}
	
	function animate() {
		// Move camera if provided target
		var step = 2;
		if(targetRotY != undefined)	{
			if(Math.abs(targetRotY-rotY)<=step) {
				rotY = targetRotY;
				targetRotY = undefined;
			}
			else	
				rotY += step*(targetRotY-rotY)/Math.abs(targetRotY-rotY)
		}
		if(targetRotX != undefined)	 {
			if(Math.abs(targetRotX-rotX)<=step) {
				rotX = targetRotX;
				targetRotX = undefined;			
			}
			else			
				rotX += step*(targetRotX-rotX)/Math.abs(targetRotX-rotX)		
		}
		frame++;
	}
	
	function setupProjectionAndViewport() {
		
		// Setup viewport
		var height = gl.viewportHeight;
		var width = gl.viewportWidth;	
	    gl.viewport(0, 0, width, height);
	    
	    // Create projection transformation   
		w = width;
		h = height*(height/width);
		mat4.ortho(pMatrix, -w*0.5, w*0.5, -h*0.5, h*0.5, -1000, 1000);				
	}
	
	/**
	* Create camera transformation such that the model is rotated around the
	* world's X-axis and Y-axis. 
	*/
	function setupCamera() {
	
		mat4.identity(mwMatrix);
		mat4.rotateX(mwMatrix, mwMatrix, -rotY/180*Math.PI); 
		mat4.rotateY(mwMatrix, mwMatrix, -rotX/180*Math.PI); 
	}	
	
	function renderScene() {	
	    animate();
	    setupProjectionAndViewport();
	    renderWorld();	    
	}
	
	function keyDown(keyCode) {
		switch(keyCode)
		{			
			case '1'.charCodeAt():
			targetRotX = 0;
			targetRotY = 0;			
			break;
			case '2'.charCodeAt():
			targetRotX = -89.8;
			targetRotY = 0;			
			break;
			case '3'.charCodeAt():
			targetRotX = 0;
			targetRotY = -90;			
			break;
			case '4'.charCodeAt():
			targetRotX = -180;
			targetRotY = 0;						
			break;
			default:
			return false;
		}		
		return true;
	}

	function init(_container) {
		container = _container;
	    container.setDisplay(renderScene);	    
	    container.setKeyDown(keyDown);
	    container.loadResourcesAsync(['ex15.vert', 'ex15.frag'], resourcesURLs.images);
	}

	function start() {
	    // Load resources
	    shaders = {};
	    shaders.fs = container.getShaderText('ex15.frag');
	    shaders.vs = container.getShaderText('ex15.vert');

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

	}
	
	return {
		init : init,
		release : release,
		start : start,		
		mouseFunc : mouseFunc,
		motionFunc : motionFunc
	};
	
} ();