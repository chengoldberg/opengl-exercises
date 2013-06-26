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
		
	var properties = {
		angle:45,
		maxLevel:4,		
		factor:0.6,
		splits:2,		
	};
	var propertiesSteps = {
		angle:1,
		maxLevel:1,		
		factor:0.01,
		splits:1,		
	};	
	var propertiesRanges = {
		angle:[0,180],
		maxLevel:[0,8],		
		factor:[0.1,1],
		splits:[0,8],		
	};	
	var propertiesNames	= ['angle','maxLevel','factor','splits'];	
	var currentProperty = 0;

	function changeProperty(delta) {
		currentProperty += delta;
		currentProperty = Math.max(currentProperty,0);
		currentProperty = Math.min(currentProperty,3);
		invalidatePropertyMenu();
	}

	function changePropertyValue(delta) {
		var name = propertiesNames[currentProperty];
		properties[name] += delta*propertiesSteps[name];
		properties[name] = Math.max(properties[name], propertiesRanges[name][0]);
		properties[name] = Math.min(properties[name], propertiesRanges[name][1]);
		invalidatePropertyMenu();
	}

	function invalidatePropertyMenu() {

		canvas2DTexture.drawToTexture(gl, function(ctx) {
			var fontSize = 24;
			ctx.font = fontSize + "px monospace";
			ctx.textBaseline = "top";

			lines = [				
				"angle " + properties.angle,
				"maxLevel " + properties.maxLevel,				
				"factor " + properties.factor,
				"splits " + properties.splits];

			for(var i=0; i<4; ++i) {
				if(i == currentProperty) 
					ctx.fillStyle = "#FF0000";		
				else
					ctx.fillStyle = "#000000";		
				ctx.fillText(lines[i], 0, fontSize*i);		
			}
		}, textures.textTexture);
	}

	function setTextureEnabled(val) {
		gl.uniform1i(uniforms.textureEnabled, val);
	}

	function updateMVP() {
		var mvMatrix = mat4.create();
		mat4.multiply(mvMatrix, wvMatrix, mwMatrix);
	    gl.uniformMatrix4fv(uniforms.projectionMatrix, false, pMatrix);
	    gl.uniformMatrix4fv(uniforms.modelViewMatrix, false, mvMatrix );
	}	
	
	function setColor(r,g,b) {
		gl.uniform3fv(uniforms.color, [r,g,b]);
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
	
		setTextureEnabled(true);
		//drawHUD();
		setTextureEnabled(false);

		mwMatrix = saveMat;
	};
	
	function drawHUD() {
		var projectionMatrix = mat4.create();
		var modelViewMatrix = mat4.create();	

	    gl.uniformMatrix4fv(uniforms.projectionMatrix, false, projectionMatrix);
	    gl.uniformMatrix4fv(uniforms.modelViewMatrix, false, modelViewMatrix);
	    gl.activeTexture(gl.TEXTURE0);
	    gl.bindTexture(gl.TEXTURE_2D, textures.textTexture);
	    gl.uniform1i(uniforms.texture0, 0);

	    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
	    gl.enable(gl.BLEND);
		meshes.screenAlignedQuad.drawMode = gl.TRIANGLE_STRIP;
		meshes.screenAlignedQuad.setAttribLocs(attribs);		
		meshes.screenAlignedQuad.render();						
	}

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
		console.log('1')
		textures.img2 = gl.createTexture();	    
		gl.bindTexture(gl.TEXTURE_2D, textures.img2);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, container.images.eql2);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.bindTexture(gl.TEXTURE_2D, null);	    
		console.log('2')
		textures.img1to2 = gl.createTexture();	    
		gl.bindTexture(gl.TEXTURE_2D, textures.img1to2);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, container.images.eql12map);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.bindTexture(gl.TEXTURE_2D, null);	    
		console.log('3')
		// Set background color to gray    
	    gl.clearColor(0, 0.63, 0.9, 1); 
	}
	
	function animate() {	
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
		//mat4.ortho(pMatrix, -w*0.25, w*0.75, -h*0.25, h*0.75, -1000, 1000);		
		mat4.ortho(pMatrix, -w*0.5, w*0.5, -h*0.5, h*0.5, -1000, 1000);				

		//console.log(pMatrix);
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
	
	function keyDown(keyCode) {
		switch(keyCode)
		{
			case 37:
			changePropertyValue(-1);
			break;
			case 39:
			changePropertyValue(+1);
			break;
			case 38:
			changeProperty(-1);
			break;
			case 40:
			changeProperty(+1);
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