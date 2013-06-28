var ex03 = function() {
	
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

	function drawTree(level) {

		setColor(0.5, 0.5, 0.5);
		updateMVP();	
		drawCylinder();	
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(0, 0, 1, 0));

		if(level == properties.maxLevel) {
			setColor(1, 0, 0);
			updateMVP();
			drawSphere();
			return;
		}

		mat4.scale(mwMatrix, mwMatrix, vec3.fromValues(properties.factor, properties.factor, properties.factor));
		for(var i=0; i<properties.splits; ++i) {
			var saveMat = mat4.clone(mwMatrix);	
			mat4.rotate(mwMatrix, mwMatrix, properties.angle*Math.PI/180.0, vec3.fromValues(0,1,0));
			updateMVP();	
			drawTree(level+1);
			mwMatrix = saveMat;

			mat4.rotate(mwMatrix, mwMatrix, (2*Math.PI/properties.splits), vec3.fromValues(0,0,1));
		}
	}

	function renderWorld() {
		
		// Clear FrameBuffer
	    gl.clear(gl.COLOR_BUFFER_BIT);
	
	    // Apply shaders
		gl.useProgram(shaderProgram);
		
		// Create camera transformation
		setupCamera();
		var saveMat = mat4.clone(mwMatrix);
						
		mat4.rotate(mwMatrix, mwMatrix, -90.0, vec4.fromValues(1, 0, 0, 0));
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(0, 0, -1, 0));
		drawTree(0);		
	
		setTextureEnabled(true);
		drawHUD();
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

	/**
	 * Draw a unit size RGB cube
	 */
	function drawCylinder() {	
		//TODO: update VBO positions based on 'factor' for cone-like effect
		meshes.cylinder.drawMode = gl.LINE_LOOP;
		
		// Render
		meshes.cylinder.setAttribLocs(attribs);
		meshes.cylinder.render();				
	}
	
	function drawSphere() {
		meshes.sphere.drawMode = gl.LINE_LOOP;
		
		// Render
		meshes.sphere.setAttribLocs(attribs);
		meshes.sphere.render();						
	}

	function initMeshes() {
		meshes.cylinder = cglib.meshGenerator.genCylinderMesh(gl, 0.1, 0.1, 1, 8, 1);
		meshes.sphere = cglib.meshGenerator.genSphereMesh(gl, 0.25, 8, 8);
		meshes.screenAlignedQuad = cglib.meshGenerator.genScreenAlignedQuad(gl);
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
	    uniforms.textureEnabled = gl.getUniformLocation(shaderProgram, "uTextureEnabled");
	    uniforms.texture0 = gl.getUniformLocation(shaderProgram, "uTexture0");
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

	    canvas2DTexture = cglib.Canvas2DTexture.extend()
	    canvas2DTexture.init(512,512);
	    textures.textTexture = gl.createTexture();
	    invalidatePropertyMenu();
	    
		// Set background color to gray    
	    gl.clearColor(1, 1, 1, 1); 
	}
	
	function animate() {	
		//TODO:
	}
	
	function setupProjectionAndViewport() {
		
		// Setup viewport
		var height = gl.viewportHeight;
		var width = gl.viewportWidth;	
	    gl.viewport(0, 0, width, height);
	    
	    // Create projection transformation   
		w = 5;
		h = 5*(height/width);
		mat4.ortho(pMatrix, -w/2, w/2, -h/2, h/2, -1000, 1000);
	}
	
	/**
	* Create camera transformation such that the model is rotated around the
	* world's X-axis and Y-axis. 
	*/
	function setupCamera() {
	
		// Rotate along temp in world coordinates	
		var yAxisInModelSpace = vec3.fromValues(mwMatrix[1], mwMatrix[5], mwMatrix[9]);
		mat4.rotate(mwMatrix, mwMatrix, -rotX/180*Math.PI, yAxisInModelSpace); 
		rotX = 0;
		
		// Rotate along the (1 0 0) in world coordinates
		var xAxisInModelSpace = vec3.fromValues(mwMatrix[0], mwMatrix[4], mwMatrix[8]);
		mat4.rotate(mwMatrix, mwMatrix, -rotY/180*Math.PI, xAxisInModelSpace);
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
	    container.setMouseMove(mouseMove);
	    container.setMouseDown(mouseDown);
	    container.loadResources(['ex03.vert', 'ex03.frag']);
	}

	function start() {
	    // Load resources
	    shaders = {};
	    shaders.fs = container.getShaderText('ex03.frag');
	    shaders.vs = container.getShaderText('ex03.vert');

	    initGL(container.getContext());         
	    
	    initWorld();
	    
	    renderScene();		
	}
	
	function mouseDown(x, y) {
		prevMouse = {"x":x, "y":y};
		return true;
	}
	
	function mouseMove(x, y, isMouseDown) {
		if(!isMouseDown)
			return false;

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

		return true;
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
		release : release,
		start : start,		
	};
	
} ();