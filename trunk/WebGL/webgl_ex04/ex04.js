var GameLogic = function() {

	var angle = 0;
	var actions = {
		moveForward: false,
		moveBackward: false,
		turnLeft: false,
		turnRight: false,
	};

	var location = vec2.create();

	function init() {
		angle = 0;
		location = [8,8];		
	}

	function update() {
		if(actions.moveForward || actions.moveBackward) {
			dx = Math.cos(angle);
			dy = Math.sin(angle);

			if(actions.moveForward) {
				location[0] -= dx*0.1;
				location[1] -= dy*0.1;				
			}
			if(actions.moveBackward) {
				location[0] += dx*0.1;
				location[1] += dy*0.1;				
			}
		}
		if(actions.turnLeft) {
			angle += 0.075;				
			console.log(angle)
		}
		if(actions.turnRight) {
			angle -= 0.075;				
			console.log(angle)
		}		
	}

	// JS note: when doing a module-like closure - you can't "expose" primitive values (e.g. can't return angle)
	// Therefore must use getters.
	function getAngle() {
		return angle;
	}
	function getLocation() {
		return location;
	}

	return {
		init: init,
		update: update,
		actions: actions,
		getAngle: getAngle,
		getLocation: getLocation,
	}
} ();

var ex04 = function() {
	
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
		
	function updateMVP() {
		var mvMatrix = mat4.create();
		mat4.multiply(mvMatrix, wvMatrix, mwMatrix);
	    gl.uniformMatrix4fv(uniforms.projectionMatrix, false, pMatrix);
	    gl.uniformMatrix4fv(uniforms.modelViewMatrix, false, mvMatrix );
	}	
	
	function setColor(r,g,b) {
		gl.uniform3fv(uniforms.color, [r,g,b]);
	}

	function drawObjects() {

		// It's ok to overide mw matrix here
		setColor(1, 0.5, 0.5);
		mat4.identity(mwMatrix);
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(3, 0.75, 3, 0));
		updateMVP();	
		drawSphere(true);

		mat4.identity(mwMatrix);
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(7, 1, 3, 0));		
		updateMVP();
		drawSphere(false)

		mat4.identity(mwMatrix);
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(11, 1, 3, 0));		
		updateMVP();
		drawCylinder();		
	}

	function renderWorld() {
		
		// Clear FrameBuffer
	    gl.clear(gl.COLOR_BUFFER_BIT);
	
	    // Apply shaders
		gl.useProgram(shaderProgram);
		
		// Create camera transformation
		setupCamera();
		mat4.identity(mwMatrix);

		drawObjects();				

		mat4.identity(mwMatrix);
		//mat4.identity(wvMatrix);
		mat4.translate(mwMatrix, mwMatrix, vec4.fromValues(0,0,-5,0));
		//mat4.identity(pMatrix);
		updateMVP();
		drawSphere(false);		
	};

	/**
	 * Draw a unit size RGB cube
	 */
	function drawCylinder() {	
			
		meshes.cylinder.drawMode = gl.LINE_LOOP;
		
		// Render
		meshes.cylinder.setAttribLocs(attribs.position);
		meshes.cylinder.render();				
	}
	
	function drawSphere(isSmall) {
		meshes.sphereSmall.drawMode = gl.LINE_LOOP;
		meshes.sphereLarge.drawMode = gl.LINE_LOOP;
		
		// Render
		if(isSmall) {
			meshes.sphereSmall.setAttribLocs(attribs.position);
			meshes.sphereSmall.render();						
		} else {
			meshes.sphereLarge.setAttribLocs(attribs.position);
			meshes.sphereLarge.render();									
		}
	}

	function initMeshes() {
		meshes.cylinder = cglib.meshGenerator.genCylinderMesh(gl, 0.1, 0.1, 1, 8, 1);
		meshes.sphereSmall = cglib.meshGenerator.genSphereMesh(gl, 0.5, 8, 8);
		meshes.sphereLarge = cglib.meshGenerator.genSphereMesh(gl, 0.75, 32, 32);
	}

	function initShaders() {
	
		// Compile shaders
	    var vertexShader = cglib.WebGLCommon.compileShader(gl, shaders.vs, gl.VERTEX_SHADER);
	    var fragmentShader = cglib.WebGLCommon.compileShader(gl, shaders.fs, gl.FRAGMENT_SHADER);
	    
	    // Link program
	    shaderProgram = cglib.WebGLCommon.linkProgram(gl, vertexShader, fragmentShader);
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

	    GameLogic.init();          
	}
	
	function initGL(context) {
		gl = context;
	    initShaders();
	    initMeshes();
    
		// Set background color to gray    
	    gl.clearColor(0, 0, 0, 1); 
	}
	
	function animate() {	
		GameLogic.update();		
	}
	
	function setupProjectionAndViewport() {
		
		// Setup viewport
		var height = gl.viewportHeight;
		var width = gl.viewportWidth;	
	    gl.viewport(0, 0, width, height);
	    
	    // Create projection transformation   
		mat4.perspective(pMatrix, 30.0, width/height, 0.1, 1000);		

		//console.log(pMatrix);	
	}
	
	/**
	* Create camera transformation such that the model is rotated around the
	* world's X-axis and Y-axis. 
	*/
	function setupCamera() {
		//console.log(GameLogic.angle);
		mat4.identity(wvMatrix);		
		mat4.rotate(wvMatrix, wvMatrix, GameLogic.getAngle()-Math.PI/2, [0,1,0,0]); 
		mat4.translate(wvMatrix, wvMatrix, vec4.fromValues(-GameLogic.getLocation()[0],-1, -GameLogic.getLocation()[1], 0));
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
			GameLogic.actions.turnLeft = true;
			break;
			case 39:
			GameLogic.actions.turnRight = true;
			break;
			case 38:
			GameLogic.actions.moveForward = true;
			break;
			case 40:
			GameLogic.actions.moveBackward = true;
			break;
			default:
			return false;
		}		
		return true;
	}

	function keyUp(keyCode) {
		switch(keyCode)
		{
			case 37:
			GameLogic.actions.turnLeft = false;
			break;
			case 39:
			GameLogic.actions.turnRight = false;
			break;
			case 38:
			GameLogic.actions.moveForward = false;
			break;
			case 40:
			GameLogic.actions.moveBackward = false;
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
	    container.setKeyUp(keyUp);
	    container.loadResources(['ex04.vert', 'ex04.frag']);
	}

	function start() {
	    // Load resources
	    shaders = {};
	    shaders.fs = container.getShaderText('ex04.frag');
	    shaders.vs = container.getShaderText('ex04.vert');

	    initGL(container.getContext());         
	    
	    initWorld();
	    
	    renderScene();		
	}
	
	return {
		init : init,
		start : start,
	};
	
} ();