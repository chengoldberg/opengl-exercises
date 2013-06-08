// define namespace
var cglib = cglib || {};

/**
  * Simple mesh - maintains vertices information with custom attribtues and 
  * possibly their order, and handles their rendering (without regards to 
  * specific program).
 */
cglib.SimpleMesh = {		

	isInitBuffers : false,
	attribs : undefined,
	attribLocs : undefined,
	faces : undefined,
	facesBufferId : undefined,	
	gl : undefined,
	drawMode : undefined,
	elementsNum : undefined,

	/**
	 * Faces is optional - otherwise uses sequential rendering
	 */
	init : function(gl, faces) {
		this.gl = gl;		
		this.faces = faces;
		return this;
	},

	addAttrib : function(attribName, componentsNum, clientBuffer) {		
		if(!this.attribs) {
			this.attribs = {};
			// Obtain number of elements from first attribute, assuming
			// it's the most important.
			this.elementsNum = clientBuffer.length/componentsNum;
		}		
		this.attribs[attribName] = {
			'clientBuffer': clientBuffer,
			'name': attribName,
			'componentsNum': componentsNum,
			'hostBufferId': undefined,
		};
		return this;
	},

	setAttribLocs : function(attribLocs) {
		this.attribLocs = attribLocs;
	},
	
	render : function() {		
		if(!this.isInitBuffers) {
			this.initBuffers();
		}
		if(this.isInitBuffers) {	
			this.renderActually();
		}
	},

	initBuffers : function() {
		var gl = this.gl;

		for(var key in this.attribs) {
			var attrib = this.attribs[key];
			attrib.hostBufferId = gl.createBuffer();
			gl.bindBuffer(gl.ARRAY_BUFFER, attrib.hostBufferId);
			gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(attrib.clientBuffer), gl.STATIC_DRAW);
			gl.bindBuffer(gl.ARRAY_BUFFER, null);
		}
		
		if(this.faces) {
			this.facesBufferId = gl.createBuffer();
			gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.facesBufferId);
			gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(this.faces), gl.STATIC_DRAW);	
			gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);
		}

		this.isInitBuffers = true;
	},
	
	renderActually : function() {
		var gl = this.gl;
		if(!this.attribLocs) {
			console.log('Error rendering - attribute locations not specified');
			return;			
		}

		for(var key in this.attribs) {
			var attrib = this.attribs[key];
			var loc = this.attribLocs[key];
			gl.bindBuffer(gl.ARRAY_BUFFER, attrib.hostBufferId);
			gl.enableVertexAttribArray(loc);
			gl.vertexAttribPointer(loc, attrib.componentsNum, gl.FLOAT, false, 0, 0);					
			gl.bindBuffer(gl.ARRAY_BUFFER, null);
		}		

		var drawMode = this.drawMode === undefined ? gl.TRIANGLES : this.drawMode;

		if(this.faces) {
			gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.facesBufferId);			
			gl.drawElements(drawMode, this.faces.length, gl.UNSIGNED_SHORT, 0);	
			gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);			
		} else {
			gl.drawArrays(drawMode, 0, this.elementsNum);
		}

		for(var key in this.attribs) {
			var attrib = this.attribs[key];
			var loc = this.attribLocs[key];
			gl.disableVertexAttribArray(loc);
		}		
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
	genScreenAlignedQuad : function(gl) {

		var vertexPositionData = [
				+1,-1,0, //0
				+1,+1,0, //1
				-1,-1,0, //2
				-1,+1,0];//3

		var texCoordData = [
				1,0, //0
				1,1, //1
				0,0, //2
				0,1];//3

		var indexData = [0,1,2,3];

		mesh = cglib.SimpleMesh.extend()		
		mesh.init(gl, indexData);
		mesh.addAttrib('position', 3, vertexPositionData);
		mesh.addAttrib('texCoord', 2, texCoordData);
		return mesh; 		
	},

	genSphereMesh : function(gl, r,lat,long) {		
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
		
		    normalData.push(x, y, z);
		    textureCoordData.push(u,v);
		    vertexPositionData.push(radius * x, radius * y, radius * z);
		  }
		}
		
		var indexData = [];
		for (var latNumber = 0; latNumber < latitudeBands; latNumber++) {
		  for (var longNumber = 0; longNumber < longitudeBands; longNumber++) {
		    var first = (latNumber * (longitudeBands + 1)) + longNumber;
		    var second = first + longitudeBands + 1;
		    //indexData.push(first, second, first + 1);		
		    //indexData.push(second, second + 1, first + 1);
		    indexData.push(first, first + 1, second + 1);		
		    indexData.push(second + 1, second, first);
		  }
		}
	
		mesh = cglib.SimpleMesh.extend()
		mesh.init(gl, indexData);		
		mesh.addAttrib('position', 3, vertexPositionData);	
		return mesh; 
	},
	
	genCylinderMesh : function(gl, baseRadius, topRadius, height, slices, stacks) {		
		
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
	
		mesh = cglib.SimpleMesh.extend()
		mesh.init(gl, indexData);		
		mesh.addAttrib('position', 3, vertexPositionData);
		return mesh; 
	},
};
