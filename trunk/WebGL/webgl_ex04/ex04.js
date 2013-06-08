var GameLogic = function() {

	var angle = 0;
	var actions = {
		moveForward: false,
		moveBackward: false,
		turnLeft: false,
		turnRight: false,
	};

	var location = vec2.create();

	var verWallsTable = undefined;
	var horWallsTable = undefined;

	var horWalls = [
		[1,1],
		[2,1],
		[3,1],
		[4,1],

		[5,1],
		[6,1],
		[7,1],
		[8,1],

		[9,1],
		[10,1],
		[11,1],
		[12,1],

		[1,5],
		[4,5],

		[5,5],
		[8,5],

		[9,5],
		[12,5]				
	];

	var verWalls = [
		[1,1],
		[1,2],
		[1,3],
		[1,4],

		[5,1],
		[5,2],
		[5,3],
		[5,4],

		[9,1],
		[9,2],
		[9,3],
		[9,4],

		[13,1],
		[13,2],
		[13,3],
		[13,4]				
	];		

	function init() {
		//TODO: change in original as well!
		angle = Math.PI/2;
		location = [8,8];		
		initCollisionTables();
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

			// Check collision
			var curX = Math.floor(location[0]);
			var curY = Math.floor(location[1]);
			var offX = location[0]-curX;
			var offY = location[1]-curY;

			var pad = 0.2;
			if(offY<pad && horWallsTable[curX][curY])
				location[1] = curY + pad;
			if(offY>1-pad && horWallsTable[curX][curY+1])
				location[1] = curY + 1 - pad;
			if(offX<pad && verWallsTable[curX][curY])
				location[0] = curX + pad;
			if(offX>1-pad && verWallsTable[curX+1][curY])
				location[0] = curX + 1 - pad;							
		}
		if(actions.turnLeft) {
			angle -= 0.075;				
		}
		if(actions.turnRight) {
			angle += 0.075;				
		}		
	}

	function createMultiBoolArray(width, height) {
	 	var table = [];
		table[width] = undefined;		
		for(var i = 0;i<width; ++i) {
			table[i] = new Int8Array(height);
		}
		return table;
	}

	function initCollisionTables() {		

		var boardWidth = getBoardSize()[0];
		var boardHeight = getBoardSize()[1];

		horWallsTable = createMultiBoolArray(boardWidth+1, boardHeight+1);
		verWallsTable = createMultiBoolArray(boardWidth+1, boardHeight+1);
		
		for(var i=0;i<horWalls.length;++i) {
			var cell = horWalls[i];
			horWallsTable[cell[0]][cell[1]] = true;
		}
		for(var i=0;i<verWalls.length;++i) {
			var cell = verWalls[i];		
			verWallsTable[cell[0]][cell[1]] = true;
		}		

		for(var i = 0;i<boardWidth;++i) {
			horWallsTable[i][0] = true;
			horWallsTable[i][boardHeight] = true;
		}
		for(var i = 0;i<boardHeight;++i) {
			verWallsTable[0][i] = true;
			verWallsTable[boardWidth][i] = true;
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
	function getBoardSize() {
		return [14, 20];
	}

	return {
		init: init,
		update: update,
		actions: actions,
		getAngle: getAngle,
		getLocation: getLocation,
		getBoardSize: getBoardSize,
		horWalls: horWalls,
		verWalls: verWalls,
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
		//gl.uniform3fv(uniforms.color, [r,g,b]);
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

	function drawWalls() {		
		meshes.walls.setAttribLocs(attribs);
		meshes.walls.render();
	}

	function drawFloor() {		
		meshes.floor.setAttribLocs(attribs);
		meshes.floor.render();
	}

	function renderWorld() {
		
		// Clear FrameBuffer
	    gl.clear(gl.COLOR_BUFFER_BIT);
	
	    // Apply shaders
		gl.useProgram(shaderProgram);
		
		// Create camera transformation
		setupCamera();

		// Draw world
		mat4.identity(mwMatrix);
		updateMVP();
		drawFloor();
		drawWalls();
		drawObjects();						
	};

	/**
	 * Draw a unit size RGB cube
	 */
	function drawCylinder() {				
		meshes.cylinder.setAttribLocs(attribs);
		meshes.cylinder.render();				
	}
	
	function drawSphere(isSmall) {	
		// Render
		if(isSmall) {
			meshes.sphereSmall.setAttribLocs(attribs);
			meshes.sphereSmall.render();						
		} else {
			meshes.sphereLarge.setAttribLocs(attribs);
			meshes.sphereLarge.render();									
		}
	}

	function initWallsMesh() {
		var vertexPositionData = [];
		for(var i=0;i<GameLogic.horWalls.length;++i) {
			var wall = GameLogic.horWalls[i];
			vertexPositionData.push(wall[0], 0, wall[1]);
			vertexPositionData.push(wall[0], 2, wall[1]);
			vertexPositionData.push(wall[0]+1, 2, wall[1]);

			vertexPositionData.push(wall[0]+1, 2, wall[1]);
			vertexPositionData.push(wall[0]+1, 0, wall[1]);			
			vertexPositionData.push(wall[0], 0, wall[1]);
		}

		for(var i=0;i<GameLogic.verWalls.length;++i) {
			var wall = GameLogic.verWalls[i];
			vertexPositionData.push(wall[0], 0, wall[1]);
			vertexPositionData.push(wall[0], 2, wall[1]);
			vertexPositionData.push(wall[0], 2, wall[1]+1);

			vertexPositionData.push(wall[0], 2, wall[1]+1);
			vertexPositionData.push(wall[0], 0, wall[1]+1);
			vertexPositionData.push(wall[0], 0, wall[1]);
		}		

		mesh = cglib.SimpleMesh.extend()
		mesh.init(gl)
			.addAttrib('position', 3, vertexPositionData);
		return mesh; 		
	}

	function initFloorMesh() {
		// Convertion note: Converting these static meshes from begin/end blocks
		// is rather straightforward. Just dump all the sequence into
		// arrays and create a sequenced mesh (vertices.push instead of glVertex)

		var vertexPositionData = [];
		for(var i=0;i<GameLogic.getBoardSize()[0];++i) {
			for(var j=0;j<GameLogic.getBoardSize()[1];++j) {											
				// Because using triangle strip - must use upside-down N shape
				vertexPositionData.push(0+i, 0, 0+j);
				vertexPositionData.push(0+i, 0, 1+j);				
				vertexPositionData.push(1+i, 0, 1+j);

				vertexPositionData.push(1+i, 0, 1+j);
				vertexPositionData.push(1+i, 0, 0+j);
				vertexPositionData.push(0+i, 0, 0+j);							
			}
		}

		mesh = cglib.SimpleMesh.extend()
		mesh.init(gl)
			.addAttrib('position', 3, vertexPositionData);
		mesh.drawMode = gl.TRIANGLE;
		//mesh.drawMode = gl.LINES;

		return mesh; 
	}

	function initMeshes() {
		meshes.cylinder = cglib.meshGenerator.genCylinderMesh(gl, 0.1, 0.1, 1, 8, 1);
		meshes.sphereSmall = cglib.meshGenerator.genSphereMesh(gl, 0.5, 8, 8);
		meshes.sphereLarge = cglib.meshGenerator.genSphereMesh(gl, 0.75, 32, 32);
		meshes.floor = initFloorMesh();
		meshes.walls = initWallsMesh();	
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

	    gl.enable(gl.DEPTH_TEST);
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
		mat4.perspective(pMatrix, 60.0*Math.PI/180, width/height, 0.1, 1000);		

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

	function getDebug(val) {
		return eval(val);
	}

	return {
		init : init,
		start : start,
		getDebug: getDebug,
	};
	
} ();