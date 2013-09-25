function createP2Image(image, imageA) {
    if (!isPowerOfTwo(image.width) || !isPowerOfTwo(image.height)) {
        // Scale up the texture to the next highest power of two dimensions.
        var canvas = document.createElement("canvas");
        canvas.width = nextHighestPowerOfTwo(image.width);
        canvas.height = nextHighestPowerOfTwo(image.height);
        var ctx = canvas.getContext("2d");
        ctx.drawImage(image, 0, 0, image.width, image.height);
        if(imageA) {
        	ctx.globalCompositeOperation = 'destination-in';
        	ctx.drawImage(imageA, 0, 0);
        }

        image = canvas;
    }
    return image;
}
 
function isPowerOfTwo(x) {
    return (x & (x - 1)) == 0;
}
 
function nextHighestPowerOfTwo(x) {
    --x;
    for (var i = 1; i < 32; i <<= 1) {
        x = x | x >> i;
    }
    return x + 1;
}

var KEY_CODE_ARROW_DOWN = 40;
var KEY_CODE_ARROW_UP = 38;


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
    var startTime = undefined;
    var timePreviousFrame = undefined;
    var zoom = 1;
    var zoomDelta = 0;
    var autoAnimate = true;

	var resourcesURLs = {
		images : 
		{
			0 : '../res/texture/ex19/newimage000_RGB.jpg',
			1 : '../res/texture/ex19/newimage001_RGB.jpg',
			'A1' : '../res/texture/ex19/newimage001_A.png',
			2 : '../res/texture/ex19/newimage002_RGB.jpg',
			'A2' : '../res/texture/ex19/newimage002_A.png',
			3 : '../res/texture/ex19/newimage003_RGB.jpg',
			'A3' : '../res/texture/ex19/newimage003_A.png',
			4 : '../res/texture/ex19/newimage004_RGB.jpg',
			'A4' : '../res/texture/ex19/newimage004_A.png',
			5 : '../res/texture/ex19/newimage005_RGB.jpg',
			'A5' : '../res/texture/ex19/newimage005_A.png',
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
		
		for(var Hi in textures.img) {
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

		// This trick is use NPO2 textures
		var fx = params.origWidth/nextHighestPowerOfTwo(params.origWidth)
		var fy = params.origHeight/nextHighestPowerOfTwo(params.origHeight)

		var vertexPositionData = [
				0,0,0, //0
				params.origWidth,0,0, //1
				params.origWidth,params.origHeight,0, //2
				0,params.origHeight,0];//3

		var texCoordData = [
				0,0, //0
				fx,0, //1
				fx,fy, //2
				0,fy];//3

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
			var compositedImage = createP2Image(container.images[i], container.images['A'+i]);
			gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, compositedImage);
			// Must do these for non-power-of-two support:
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);			
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);			

			gl.generateMipmap(gl.TEXTURE_2D);

			gl.bindTexture(gl.TEXTURE_2D, null);	    
		}

		// Set background color to teal
	    gl.clearColor(0, 0.63, 0.9, 1); 
	}
	
	function animate() {
		
		if(autoAnimate) {
			if(!startTime)
				startTime = new Date().getTime();
			var miliDelta = new Date().getTime() - startTime;
			var alpha = (Math.cos(Math.PI*miliDelta/4000)+1)/2;
			zoom = (1-alpha)*1 + (alpha)*7;

		} else {

			var timeCurrentFrame = new Date().getTime();
			if(!timePreviousFrame)
				timePreviousFrame = timeCurrentFrame;		
			var timeDeltaFrame = timeCurrentFrame - timePreviousFrame;			
			if(timeDeltaFrame>100)
				timeDeltaFrame=100;
			if(timeDeltaFrame<16)
				timeDeltaFrame=16;
			// A good time is from 1 to 7 in 4000 milli - that is 6/4000 zoom per ms
			zoom += zoomDelta*timeDeltaFrame*6/4000;			
			timePreviousFrame = timeCurrentFrame;
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
		//var alpha = Math.min(Math.log(scaleFactor)/Math.log(lastZoom),1)
		//var alpha = zoom/lastZoom;
		var alpha = 1;
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
		autoAnimate = false;
		switch(keyCode)
		{			
			case KEY_CODE_ARROW_UP:
			zoomDelta = 1;
			break;
			case KEY_CODE_ARROW_DOWN:
			zoomDelta = -1;			
			break;
		}		
		return true;
	}

	function keyUp(keyCode) {
		
		switch(keyCode)
		{			
			case KEY_CODE_ARROW_UP:
			zoomDelta = 0;
			break;
			case KEY_CODE_ARROW_DOWN:
			zoomDelta = 0;			
			break;
		}		
		return true;
	}

	function init(_container) {
		container = _container;
	    container.setDisplay(renderScene);	  
	    container.setKeyDown(keyDown);
	    container.setKeyUp(keyUp);
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