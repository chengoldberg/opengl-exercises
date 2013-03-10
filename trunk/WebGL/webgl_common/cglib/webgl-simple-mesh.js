// define namespace
var cglib = cglib || {};

/**
 * Simple mesh
 */
cglib.simpleMesh = {		
	faces : {},
	vertices : {},
	colors : {},
	
	facesBufferId : undefined,
	verticesBufferId : undefined,		
	colorsBufferId : undefined,
	
	verticesAttribLoc : undefined,
	colorsAttribLoc : undefined,
	gl : undefined,
	
	init : function(vertices, faces) {
		this.faces = faces;
		this.vertices = vertices;
		this.gl = cglib.WebGLCommon.getCurrentContext();
	},

	setAttribLocs : function(verticesAttribLoc, colorsAttribLoc) {
		this.verticesAttribLoc = verticesAttribLoc;
		this.colorsAttribLoc = colorsAttribLoc;
	},
	
	render : function() {
		if(!this.verticesBufferId) {
			this.initBuffers();
		}
		if(this.verticesBufferId) {	
			this.renderActually();
		}
	},

	initBuffers : function() {
		var gl = this.gl;
		this.verticesBufferId = gl.createBuffer();
		gl.bindBuffer(gl.ARRAY_BUFFER, this.verticesBufferId);
		gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.vertices), gl.STATIC_DRAW);

		if(this.colors) {
			this.colorsBufferId = gl.createBuffer();
			gl.bindBuffer(gl.ARRAY_BUFFER, this.colorsBufferId);
			gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.colors), gl.STATIC_DRAW);
		}
		
		this.facesBufferId = gl.createBuffer();
		gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.facesBufferId);
		gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(this.faces), gl.STATIC_DRAW);	
	},
	
	renderActually : function() {
		var gl = this.gl;
		gl.bindBuffer(gl.ARRAY_BUFFER, this.verticesBufferId);
		gl.enableVertexAttribArray(this.verticesAttribLoc);
		gl.vertexAttribPointer(this.verticesAttribLoc, 3, gl.FLOAT, false, 0, 0);
		
		if(this.colors) {
			gl.bindBuffer(gl.ARRAY_BUFFER, this.colorsBufferId);
			gl.enableVertexAttribArray(this.colorsAttribLoc);
			gl.vertexAttribPointer(this.colorsAttribLoc, 3, gl.FLOAT, false, 0, 0);		
		}
		
		gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.facesBufferId);
		
		var drawMode = this.drawMode === undefined ? gl.TRIANGLES : this.drawMode;
		
		gl.drawElements(drawMode, this.faces.length, gl.UNSIGNED_SHORT, 0);	
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

cglib.meshGenerator = {		
	genSphereMesh : function(r,lat,long) {		
		var latitudeBands = lat;
		var longitudeBands = long;
		var radius = r;
		
		var vertexPositionData = [];
		var normalData = [];
		var textureCoordData = [];
		
		for (var latNumber = 0; latNumber <= latitudeBands; latNumber++) {
		  var theta = latNumber * Math.PI / latitudeBands;
		  var sinTheta = Math.sin(theta);
		  var cosTheta = Math.cos(theta);
		
		  for (var longNumber = 0; longNumber <= longitudeBands; longNumber++) {
		    var phi = longNumber * 2 * Math.PI / longitudeBands;
		    var sinPhi = Math.sin(phi);
		    var cosPhi = Math.cos(phi);
		
		    var x = cosPhi * sinTheta;
		    var y = cosTheta;
		    var z = sinPhi * sinTheta;
		    var u = 1- (longNumber / longitudeBands);
		    var v = latNumber / latitudeBands;
		
		    normalData.push(x);
		    normalData.push(y);
		    normalData.push(z);
		    textureCoordData.push(u);
		    textureCoordData.push(v);
		    vertexPositionData.push(radius * x);
		    vertexPositionData.push(radius * y);
		    vertexPositionData.push(radius * z);
		  }
		}
		
		var indexData = [];
		for (var latNumber = 0; latNumber < latitudeBands; latNumber++) {
		  for (var longNumber = 0; longNumber < longitudeBands; longNumber++) {
		    var first = (latNumber * (longitudeBands + 1)) + longNumber;
		    var second = first + longitudeBands + 1;
		    indexData.push(first);
		    indexData.push(second);
		    indexData.push(first + 1);
		
		    indexData.push(second);
		    indexData.push(second + 1);
		    indexData.push(first + 1);
		  }
		}
	
		mesh = cglib.simpleMesh.extend()
		mesh.init(vertexPositionData, indexData);
		mesh.colors = normalData;
		return mesh; 
	},
	
	genCylinderMesh : function(baseRadius, topRadius, height, slices, stacks) {		
		
		var vertexPositionData = [];
		var normalData = [];
		var textureCoordData = [];
		
		var latitudeBands = slices;
		longitudeBands = stacks;
		
		for (var latNumber = 0; latNumber <= latitudeBands; latNumber++) {
		  var theta = latNumber * 2 * Math.PI / latitudeBands;
		  var sinTheta = Math.sin(theta);
		  var cosTheta = Math.cos(theta);
		
		  for (var longNumber = 0; longNumber <= longitudeBands; longNumber++) {
			var ratio = longNumber/longitudeBands;
		    var radius = baseRadius*(1-ratio) + topRadius*(ratio);
		    
		    var x = radius * cosTheta;
		    var y = radius * sinTheta;
		    var z = ratio*height;
		    
		    //var u = 1- (longNumber / longitudeBands);
		    //var v = latNumber / latitudeBands;
				    
		    normalData.push(x);
		    normalData.push(y);
		    normalData.push(z);
		    //textureCoordData.push(u);
		    //textureCoordData.push(v);
		    vertexPositionData.push(x);
		    vertexPositionData.push(y);
		    vertexPositionData.push(z);
		  }
		}
		
		var indexData = [];
		for (var latNumber = 0; latNumber < latitudeBands; latNumber++) {
		  for (var longNumber = 0; longNumber < longitudeBands; longNumber++) {
		    var first = (latNumber * (longitudeBands + 1)) + longNumber;
		    var second = first + longitudeBands + 1;
		    indexData.push(first);
		    indexData.push(second);
		    indexData.push(first + 1);
		
		    indexData.push(second);
		    indexData.push(second + 1);
		    indexData.push(first + 1);
		  }
		}
	
		mesh = cglib.simpleMesh.extend()
		mesh.init(vertexPositionData, indexData);
		mesh.colors = normalData;
		return mesh; 
	},
};
