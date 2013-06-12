var p = [
	[0,0],
	[20,0],
	[15,25],
	[30,6]	
];

function calcCurveAt(u) {
	var c0 = (1-u)*(1-u)*(1-u)*p[0][0]+ 3*(1-u)*(1-u)*u*p[1][0] + 3*(1-u)*u*u*p[2][0] + u*u*u*p[3][0];
	var c1 = (1-u)*(1-u)*(1-u)*p[0][1]+ 3*(1-u)*(1-u)*u*p[1][1] + 3*(1-u)*u*u*p[2][1] + u*u*u*p[3][1];

	return [c0,c1];
}

var Airplane = {
	startTime : undefined,
	position : undefined,
	angle : undefined,
	pigment : undefined,

	init : function(startTime) {
		this.startTime = startTime;
		this.pigment = [Math.random(),Math.random(),Math.random(),1];
	},

	update : function(time) {
		var u = (time - this.startTime) -Math.floor(time - this.startTime);

		// Calc new position
		var c0 = (1-u)*(1-u)*(1-u)*p[0][0]+ 3*(1-u)*(1-u)*u*p[1][0] + 3*(1-u)*u*u*p[2][0] + u*u*u*p[3][0];
		var c1 = (1-u)*(1-u)*(1-u)*p[0][1]+ 3*(1-u)*(1-u)*u*p[1][1] + 3*(1-u)*u*u*p[2][1] + u*u*u*p[3][1];
		this.position = [c0, c1];

		// Calc new derivative
		var d0 = 3*(p[1][0]-p[0][0])*(1-u)*(1-u) + 
			3*(p[2][0]-p[1][0])*2*u*(1-u) + 
			3*(p[3][0]-p[2][0])*u*u;
		var d1 = 3*(p[1][1]-p[0][1])*(1-u)*(1-u) + 
			3*(p[2][1]-p[1][1])*2*u*(1-u) + 
			3*(p[3][1]-p[2][1])*u*u;

		// Transform derivative to degrees 
		this.angle = Math.PI/2-Math.atan2(d0,d1);			
	},
	
	// Foundation helpers
	extend: function(props) {
        var prop, obj;
        obj = Object.create(this);
        for(prop in props) {
            if(props.hasOwnProperty(prop)) {
                obj[prop] = props[prop];
            }
        }
        return obj;
	},	
}

var GameLogic = function() {
	timeStep = 0.003;
	time = 0;
	airplanes = [];

	function init() {

	}

	function update() {
		// Advance time counter
		time += timeStep;

		for(var i in airplanes) {
			airplanes[i].update(time);
		}
	}

	function addAirplane() {
		var airplane = Airplane.extend();
		airplane.init(time);
		airplanes.push(airplane);
		console.log('Added airplane - now has ' + airplanes.length);
	}

	function removeAirplane() {
		if(airplanes.length > 1) {
			airplanes.pop();
		}
		console.log('Remove airplane - now has ' + airplanes.length);
	}

	function speedUp() {
		timeStep *= 2;
	}

	function speedDown() {
		timeStep /= 2;
	}

	function getAirplanes() {
		return airplanes;
	}

	return {
		init: init,
		update: update,	
		addAirplane : addAirplane,
		removeAirplane : removeAirplane,
		speedUp : speedUp,
		speedDown : speedDown,
		getAirplanes : getAirplanes,
	}
} ();

var ex05 = function() {
	
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
	var frame = 0;
	var resourcesURLs = {
		airplaneMesh : '../res/mesh/bomber1.off',
		lightingShader : '../res/shader/lighting.vert'
	}

	function updateMVP() {
		var mvMatrix = mat4.create();
		mat4.multiply(mvMatrix, wvMatrix, mwMatrix);
	    gl.uniformMatrix4fv(uniforms.projectionMatrix, false, pMatrix);
	    gl.uniformMatrix4fv(uniforms.modelViewMatrix, false, mvMatrix );

	    var normalMatrix = mat3.create();
	    mat3.normalFromMat4(normalMatrix, mvMatrix);
	    gl.uniformMatrix3fv(uniforms.normalMatrix, false, normalMatrix);
	}	

	function setLighting(status) {
		gl.uniform1i(uniforms.lightingEnabled, status);
	}

	function setLightStatus(lightIndex, status) {
		gl.uniform1i(uniforms.lights[lightIndex].isEnabled, status);
	}

	function setLightPosition(lightIndex, pos) {
		var lightPosition = vec4.create();
		
		var mvMatrix = mat4.create();
		mat4.multiply(mvMatrix, wvMatrix, mwMatrix);		

		vec4.transformMat4(lightPosition, pos, mvMatrix);
		gl.uniform4fv(uniforms.lights[lightIndex].position, lightPosition);
	}	

	function setLightIntensity(lightIndex, ambient, diffuse, specular) {
		if(ambient)	
			gl.uniform4fv(uniforms.lights[lightIndex].ambient, ambient);
		if(diffuse)
			gl.uniform4fv(uniforms.lights[lightIndex].diffuse, diffuse);
		if(specular)
			gl.uniform4fv(uniforms.lights[lightIndex].specular, specular);
	}

	function setLightAttenuation(lightIndex, K0, K1, K2) {
		if(K0)	
			gl.uniform1f(uniforms.lights[lightIndex].constantAttenuation, K0);
		if(K1)
			gl.uniform1f(uniforms.lights[lightIndex].linearAttenuation, K1);
		if(K2)
			gl.uniform1f(uniforms.lights[lightIndex].quadraticAttenuation, K2);
	}

	function setLightSpot(lightIndex, spotDirection, spotExponent, spotCutoff) {
		if(spotDirection)	
			gl.uniform3fv(uniforms.lights[lightIndex].spotDirection, spotDirection);
		if(spotExponent)
			gl.uniform1f(uniforms.lights[lightIndex].spotExponent, spotExponent);
		if(spotCutoff)
			gl.uniform1f(uniforms.lights[lightIndex].spotCutoff, spotCutoff);
	}

	function setMaterial(ambient, diffuse, specular, specularPower, emission) {
		if(ambient)	
			gl.uniform4fv(uniforms.ambient, ambient);
		if(diffuse) {
			gl.uniform4fv(uniforms.diffuse, diffuse);
		}
		if(specular)
			gl.uniform4fv(uniforms.specular, specular);
		if(specularPower)
			gl.uniform1f(uniforms.specularPower, specularPower);
		if(emission)
			gl.uniform4fv(uniforms.emission, emission);		
	}

	function setColor(r,g,b) {
		//gl.uniform3fv(uniforms.color, [r,g,b]);
	}

	function drawCurve() {				
		meshes.curve.setAttribLocs(attribs);
		meshes.curve.setDrawMode(gl.LINE_STRIP);
		meshes.curve.render();
	}

	function drawFloor() {		
		setMaterial(
			[0.65,0.65,0.65,1],
			[0.65,0.65,0.65,1],
			[1,1,1,1],
			100,
			[0,0,0,1]);

		meshes.floor.setAttribLocs(attribs);
		meshes.floor.render();
	}

	function renderWorld() {
		
	    // Apply shaders
		gl.useProgram(shaderProgram);
		
		// Sky light
		mat4.identity(mwMatrix);
		setLightStatus(0, true);		
		setLightPosition(0, [0.5,1,1,0]);		

		//setLightPosition(1, [4,4,4,1]);		

		// Draw world		
		mat4.identity(mwMatrix);
		updateMVP();
		//drawFloor();
		setLighting(false);
		drawCurve();
		setLighting(true);
		drawAirplanes();
	};

	/**
	 * Draw a unit size RGB cube
	 */
	function drawAirplane(airplane) {		
		mat4.identity(mwMatrix);
		mat4.translate(mwMatrix, mwMatrix, [airplane.position[0], airplane.position[1],0,0]);
		mat4.rotate(mwMatrix, mwMatrix, airplane.angle, [0,0,1,0]);

		// Object transfromations (specific to "bomber1")
		mat4.rotate(mwMatrix, mwMatrix, Math.PI/2, [0,1,0,0]);
		mat4.rotate(mwMatrix, mwMatrix, -Math.PI/2, [1,0,0,0]);
		mat4.scale(mwMatrix, mwMatrix, [0.1,0.1,0.1,1]);
		updateMVP();

		setMaterial(
			airplane.pigment,
			airplane.pigment,
			[1,1,1,1],
			100,
			[0,0,0,1]);

		meshes.airplane.setAttribLocs(attribs);
		meshes.airplane.render();				
	}

	function drawAirplanes() {
		var airplanes = GameLogic.getAirplanes();
		for(var airplane in airplanes) {
			drawAirplane(airplanes[airplane]);
		}
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

	function initCurveMesh() {		
		var vertexPositionData = [];

		for(var i=0;i<=1;i+=0.01) {
		 	var point = calcCurveAt(i);
			vertexPositionData.push(point[0], point[1]);
		}

		mesh = cglib.SimpleMesh.extend()
		mesh.init(gl)
			.addAttrib('position', 2, vertexPositionData);
		mesh.drawMode = gl.GL_LINE_STRIP;

		return mesh; 		
	}

	function initFloorMesh() {
		// Convertion note: Converting these static meshes from begin/end blocks
		// is rather straightforward. Just dump all the sequence into
		// arrays and create a sequenced mesh (vertices.push instead of glVertex)
		/*
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
			.addAttrib('position', 3, vertexPositionData)
			.addAttrib('normal', 3, [0,1,0]);
		mesh.drawMode = gl.TRIANGLE;
		//mesh.drawMode = gl.LINES;

		return mesh; 
		*/
	}

	function initMeshes() {
		//meshes.floor = initFloorMesh();
		meshes.curve = initCurveMesh();

		meshes.airplane = cglib.meshLoader.fromText(gl, container.getShaderText(resourcesURLs.airplaneMesh));
		cglib.meshGenerator.genNormals(meshes.airplane);
		meshes.airplane.drawMode = gl.GL_LINE_STRIP;
	}

	function initShaders() {
	
		// Compile shaders
		console.log(shaders.vs + shaders.vs2);
	    var vertexShader = cglib.WebGLCommon.compileShader(gl, shaders.vs + shaders.vs2, gl.VERTEX_SHADER);
	    var fragmentShader = cglib.WebGLCommon.compileShader(gl, shaders.fs, gl.FRAGMENT_SHADER);
	    
	    // Link program
	    shaderProgram = cglib.WebGLCommon.linkProgram(gl, [vertexShader, fragmentShader]);
	    gl.useProgram(shaderProgram);
	   
	    // Store attrib IDs
	    attribs = {};
	    attribs.position = gl.getAttribLocation(shaderProgram, "aPosition");    
	    attribs.normal = gl.getAttribLocation(shaderProgram, "aNormal");   
	
	    // Store unfiform IDs
	    uniforms = {};
	    uniforms.normalMatrix = gl.getUniformLocation(shaderProgram, "uNormalMatrix");
	    uniforms.projectionMatrix = gl.getUniformLocation(shaderProgram, "uProjectionMatrix");
	    uniforms.modelViewMatrix = gl.getUniformLocation(shaderProgram, "uModelViewMatrix");   
	    uniforms.lightingEnabled = gl.getUniformLocation(shaderProgram, "uLightingEnabled");

		uniforms.lights = [];
	    for(var i=0;i<4;++i) {
	    	uniforms.lights[i] = {
	    		position: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].position"),
	    		ambient: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].ambient"),
	    		diffuse: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].diffuse"),
	    		specular: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].specular"),    			
	    		spotDirection: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].spotDirection"),    			
	    		spotExponent: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].spotExponent"),    			
	    		spotCutoff: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].spotCutoff"),    			
	    		constantAttenuation: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].constantAttenuation"),    			
	    		linearAttenuation: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].linearAttenuation"),    			
	    		quadraticAttenuation: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].quadraticAttenuation"),    			    		
	    		isEnabled: gl.getUniformLocation(shaderProgram, "uLightSource[" + i + "].isEnabled"),    			
	    	};
	    	// Set default values
	    	gl.uniform1f(uniforms.lights[i].constantAttenuation, 1);
	    	gl.uniform3fv(uniforms.lights[i].spotDirection, [0,0,-1]);
	    	gl.uniform1f(uniforms.lights[i].spotCutoff, 180);
	    }

	    uniforms.ambient = gl.getUniformLocation(shaderProgram, "uMaterial.ambient");
	    uniforms.diffuse = gl.getUniformLocation(shaderProgram, "uMaterial.diffuse");
	    uniforms.specular = gl.getUniformLocation(shaderProgram, "uMaterial.specular");
	    uniforms.specularPower = gl.getUniformLocation(shaderProgram, "uMaterial.shininess");
	    uniforms.emission = gl.getUniformLocation(shaderProgram, "uMaterial.emission");	    
	}
	
	function initWorld() {
	    // Init global user-data	    
	    mwMatrix = mat4.create();
	    wvMatrix = mat4.create();
	    pMatrix = mat4.create(); 

	    GameLogic.init();               

	    GameLogic.addAirplane();
	}
	
	function initGL(context) {
		gl = context;
	    initShaders();
	    initMeshes();

		// Setup sky light	
		setLightIntensity(0, 
			[0.2,0.2,0.2,1],
			[0.8, 0.8, 0.8, 1],
			[1,1,1,1]);

		// Setup flashlight
		setLightIntensity(1, 
			[0,0,0,1],
			[1, 0, 0, 1],
			[1,1,1,1]);
		setLightSpot(1, [0,0,-1], 100, 10);

		// Setup sphere light
		setLightIntensity(2, 
			[0, 0, 0, 1],
			[1, 1, 0, 1],
			[1, 1, 0, 1]);		
		setLightAttenuation(2,1,0,0.5);

		// Set background color to gray    
	    gl.clearColor(0, 0, 0, 1); 

	    gl.enable(gl.DEPTH_TEST);
	}
	
	function animate() {	
		frame++;
		GameLogic.update();		
	}
	
	function setupProjectionAndViewport() {
		
	}
	
	function renderScene() {	
	    animate();

		// Clear FrameBuffer
	    gl.clear(gl.COLOR_BUFFER_BIT);

		var height = gl.viewportHeight;
		var width = gl.viewportWidth;	
		var x = GameLogic.getAirplanes()[0].position[0];
		var y = GameLogic.getAirplanes()[0].position[1];
		var angle = GameLogic.getAirplanes()[0].angle;

		//
		// Viewport4 - Side view
		//		
	    gl.viewport(0, 0, width/2, height/2);	    
		mat4.ortho(pMatrix, -1, 31, 0, 15, -100, 100);
		mat4.identity(wvMatrix);
	    renderWorld();	    

		//
		// Viewport2 - Isometric
		//		
	    gl.viewport(width/2+1, 0, width/2, height/2);	    	    
	    mat4.perspective(pMatrix, 60.0*Math.PI/180, width/height, 0.1, 1000);		
	    mat4.lookAt(wvMatrix, [x+0.7, y+0.7, +0.7], [x,y,0], [0,1,0]);
	    renderWorld();

		//
		// Viewport3 - Over the head camera
		//		
		gl.viewport(width/2+1, height/2+1, width/2, height/2);	    	    			
		mat4.identity(wvMatrix);
		mat4.translate(wvMatrix, wvMatrix, [0, -0.35, -1.4, 0]);
		mat4.rotate(wvMatrix, wvMatrix, Math.PI/2, [0,1,0,0]);
		mat4.rotate(wvMatrix, wvMatrix, -angle, [0,0,1,0]);
		mat4.translate(wvMatrix, wvMatrix, [-x, -y, 0, 0]);
		renderWorld();

		//
		// Viewport 4 - Wide view 
		//
	    gl.viewport(0, height/2, width/2, height/2);	    	    
	    mat4.perspective(pMatrix, 90.0*Math.PI/180, width/height, 0.1, 1000);		
	    mat4.identity(wvMatrix);
		mat4.rotate(wvMatrix, wvMatrix, -60.0*Math.PI/180, [0,1,0,0]);
		mat4.translate(wvMatrix, wvMatrix, [-29, -8, -1, 0]);
		renderWorld();
	}
	
	function keyDown(keyCode) {
		switch(keyCode)
		{
			case 37:
			break;
			case 39:
			break;
			case 38:
			break;
			case 40:
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
			GameLogic.speedDown();
			break;
			case 39:
			GameLogic.speedUp();
			break;
			case 38:
			GameLogic.addAirplane();			
			break;
			case 40:
			GameLogic.removeAirplane();
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
	    container.loadResources(['ex05.vert', resourcesURLs.lightingShader, 'ex05.frag',resourcesURLs.airplaneMesh]);
	}

	function start() {
	    // Load resources
	    shaders = {};
	    shaders.fs = container.getShaderText('ex05.frag');
	    shaders.vs = container.getShaderText('ex05.vert');
	    shaders.vs2 = container.getShaderText(resourcesURLs.lightingShader);
	    
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