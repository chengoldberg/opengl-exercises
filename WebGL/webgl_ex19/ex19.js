var ex19 = function() {
	
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
    var zoom = 1;
	var resourcesURLs = {
		images : 
		{
			0 : '../res/texture/ex19/newimage000.png',
			1 : '../res/texture/ex19/newimage001.png',
			2 : '../res/texture/ex19/newimage002.png',
			3 : '../res/texture/ex19/newimage003.png',
			4 : '../res/texture/ex19/newimage004.png',
			5 : '../res/texture/ex19/newimage005.png',
		},
	}
	var params = undefined;
		
	function updateMVP() {
		var mvMatrix = mat4.create();
		mat4.multiply(mvMatrix, wvMatrix, mwMatrix);
	    gl.uniformMatrix4fv(uniforms.projectionMatrix, false, pMatrix);
	    gl.uniformMatrix4fv(uniforms.modelViewMatrix, false, mvMatrix );
	}	
	
	function drawPoints(level) {
	
		meshes.rect.setAttribLocs(attribs);
		meshes.rect.render();
	}

	function renderWorld() {
		
		// Clear FrameBuffer
	    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
	
	    // Apply shaders
		gl.useProgram(shaderProgram);
		
		// Create camera transformation
		setupCamera();

		gl.uniform1i(uniforms.frameNum, frame);
		gl.enable(gl.BLEND);
		gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);	

		mat4.identity(mwMatrix);
		
		for(var Hi in params.Hs) {
			if(Hi>0) {
				mat4.multiply(mwMatrix, mwMatrix, params.Hs[Hi-1]);
			}
			updateMVP();	
		    gl.activeTexture(gl.TEXTURE0);
		    gl.bindTexture(gl.TEXTURE_2D, textures.img[Hi]);
		    gl.uniform1i(uniforms.texImg1, 0);	        	    	

			drawPoints();		

			gl.bindTexture(gl.TEXTURE_2D, null);
		}

		// Put the pixel cube in the center
		//mat4.translate(mwMatrix, mwMatrix, [-127.0, -127.0, +127.0]);

	};
	
	function initMeshes() {

		var vertexPositionData = [
				0,0,0, //0
				params.origWidth,0,0, //1
				params.origWidth,params.origHeight,0, //2
				0,params.origHeight,0];//3

		var texCoordData = [
				0,0, //0
				1,0, //1
				1,1, //2
				0,1];//3

		var indexData = [0,1,2,2,3,0];
		//var indexData = [0,1,1,2,2,3,3,0];

		mesh = cglib.SimpleMesh.extend()		
		mesh.init(gl, indexData);
		mesh.addAttrib('position', 3, vertexPositionData);
		mesh.addAttrib('texCoord', 2, texCoordData);		
		//mesh.drawMode = gl.LINES

		meshes.rect = mesh;
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
	    textures.img = []

	    for(var i=0; i<5;++i) {
		    textures.img[i] = gl.createTexture();
			gl.bindTexture(gl.TEXTURE_2D, textures.img[i]);
			//gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
			gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, container.images[i]);
			// Must do these for non-power-of-two support:
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);			
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);			

			gl.bindTexture(gl.TEXTURE_2D, null);	    
		}

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
	    gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
	    var aspectRatio = gl.viewportHeight/gl.viewportWidth;

	    // Create projection transformation   
        var width = params.origWidth;
        var height = params.origHeight;    
		var fixedHeight = width*aspectRatio;
		
		mat4.ortho(pMatrix, 0, width, (height+fixedHeight)/2, (height-fixedHeight)/2, -1, 1)		
	}
	
	/**
	* Create camera transformation such that the model is rotated around the
	* world's X-axis and Y-axis. 
	*/
	function setupCamera() {
		
		mat4.identity(wvMatrix);

		scaleFactor = Math.pow(2, zoom-1);

		var lastZoom = 32;
		var alpha = Math.min(Math.log(scaleFactor)/Math.log(lastZoom),1)
		var centroid = [
			params.startCentroid[0]*(1-alpha) + params.endCentroid[0]*(alpha),
			params.startCentroid[1]*(1-alpha) + params.endCentroid[1]*(alpha)];

		mat4.translate(wvMatrix, wvMatrix, [params.startCentroid[0]-centroid[0], params.startCentroid[1]-centroid[1], 0]);
		mat4.translate(wvMatrix, wvMatrix, [+centroid[0], +centroid[1], 0]);
		mat4.scale(wvMatrix, wvMatrix, [scaleFactor, scaleFactor, scaleFactor]);
		mat4.translate(wvMatrix, wvMatrix, [-centroid[0], -centroid[1], 0]);
		//mat4.rotateX(mwMatrix, mwMatrix, -rotY/180*Math.PI); 
		//mat4.rotateY(mwMatrix, mwMatrix, -rotX/180*Math.PI); 		
	}	
	
	function renderScene() {	
	    animate();
	    setupProjectionAndViewport();
	    renderWorld();	    
	}
	
	function keyDown(keyCode) {
		
		switch(keyCode)
		{			
			case 'W'.charCodeAt():
			zoom += 0.02;
			break;
			case 'S'.charCodeAt():
			zoom -= 0.02;			
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
	    container.setMouseMove(mouseMove);
	    container.setMouseDown(mouseDown);
	    container.loadResourcesAsync(['ex19.vert', 'ex19.frag','params.json'], resourcesURLs.images);
	}

	function mat3to4(M) {
		A = mat4.create();
		var indices = [0,1,3, 4,5,7, 12,13,15];
		for(i in indices) {
			A[indices[i]] = M[i]
		}
		return A;
	}

	function start() {
	    // Load resources
	    shaders = {};
	    shaders.fs = container.getShaderText('ex19.frag');
	    shaders.vs = container.getShaderText('ex19.vert');

	    paramsString = container.getShaderText('params.json');
	    params = JSON.parse(paramsString);    
	    for(var Hi in params.Hs) {
	    	params.Hs[Hi] = mat3to4(params.Hs[Hi]);
	    }
	    params.origWidth = 2000;
	    params.origHeight = 1333;

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
			for(texture in textures) {
				gl.deleteTexture(textures[texture]);
			}
		}
	}
	
	return {
		init : init,
		release : release,
		start : start,		
	};
	
} ();