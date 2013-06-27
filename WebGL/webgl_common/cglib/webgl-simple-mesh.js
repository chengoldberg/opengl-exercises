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
	elementsPerFace : 3,

	setDrawMode : function(val) {
		this.drawMode = val;
	},

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

		var curElementsNum = clientBuffer.length/componentsNum;
		var isConstant = false;
		if(curElementsNum == 1) {
			isConstant = true
		} else if(curElementsNum != this.elementsNum) {
			console.log('error - number of elements different than first attribute');
		}

		this.attribs[attribName] = {
			'clientBuffer': clientBuffer,
			'name': attribName,
			'componentsNum': componentsNum,
			'hostBufferId': undefined,
			'isConstant': isConstant,
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
			if(loc == undefined)
				continue;
			if(attrib.isConstant) {
			    gl.vertexAttrib3fv(loc, attrib.clientBuffer);
			} else {
				gl.bindBuffer(gl.ARRAY_BUFFER, attrib.hostBufferId);
				gl.enableVertexAttribArray(loc);
				gl.vertexAttribPointer(loc, attrib.componentsNum, gl.FLOAT, false, 0, 0);					
				gl.bindBuffer(gl.ARRAY_BUFFER, null);
			}
		}		

		var drawMode = this.drawMode == undefined ? gl.TRIANGLES : this.drawMode;

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

	release : function(gl) {
		for(var key in this.attribs) {
			var attrib = this.attribs[key];
			gl.deleteBuffer(attrib.hostBufferId);			
		}		
		if(this.faces)
			gl.deleteBuffer(this.facesBufferId)
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

cglib.meshLoader = {
	fromText : function(gl, text) {
    	var buffer = text.split(/\s+/);
		var cnt = 0;

		if(buffer[cnt++] != 'OFF')
			throw "Unsupported file-format"
		var verticesAmount = parseInt(buffer[cnt++]);
		var facesAmount = parseInt(buffer[cnt++]);
		cnt++;

		var vertices = [];
		var faces = [];

		for(var i=0; i<verticesAmount; ++i) {
			vertices.push(
				parseFloat(buffer[cnt++]),
				parseFloat(buffer[cnt++]),
				parseFloat(buffer[cnt++]));
		}

		for(var i=0; i<facesAmount; ++i) {
			cnt++;
			faces.push(
				parseFloat(buffer[cnt++]),
				parseFloat(buffer[cnt++]),
				parseFloat(buffer[cnt++]));
		}		
	
		var mesh = cglib.SimpleMesh.extend()
		mesh.init(gl, faces)
			.addAttrib('position', 3, vertices);
		return mesh; 
	}
};

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
		mesh.init(gl, indexData)
			.addAttrib('position', 3, vertexPositionData)
			.addAttrib('normal', 3, normalData);	
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
		mesh.init(gl, indexData)
			.addAttrib('position', 3, vertexPositionData)
			.addAttrib('normal', 3, normalData);
		return mesh; 
	},

	genNormals : function(mesh) {

		var normalsCount = [];
		var normals = [];
		var verticesAmount = mesh.elementsNum;

		for(var i = 0;i<verticesAmount;++i) {
			normalsCount[i] = 0;
			normals[i] = vec3.create();
		}
		
		var vertices = mesh.attribs['position'].clientBuffer;
		var faces = mesh.faces;
		var facesAmount = faces.length/mesh.elementsPerFace;
		var vertexIndex = -1;

		for (var i=0; i < facesAmount; ++i){

			vertexIndex = faces[3*i + 0];
			var x1 = vertices[3*vertexIndex + 0];
			var y1 = vertices[3*vertexIndex + 1];
			var z1 = vertices[3*vertexIndex + 2];

			vertexIndex = faces[3*i + 1];
			var x2 = vertices[3*vertexIndex + 0];
			var y2 = vertices[3*vertexIndex + 1];
			var z2 = vertices[3*vertexIndex + 2];

			vertexIndex = faces[3*i + 2];
			var x3 = vertices[3*vertexIndex + 0];
			var y3 = vertices[3*vertexIndex + 1];
			var z3 = vertices[3*vertexIndex + 2];

			var pen = [
				y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2),
				z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2),
				x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)]

			vec3.normalize(pen, pen);

			for(var j=0; j < 3;++j){
				vertexIndex = faces[3*i + j];			
				normalsCount[vertexIndex]++;			
				vec3.add(normals[vertexIndex], normals[vertexIndex], pen);
			}
		}

		normalData = [];
		for(var i=0;i<verticesAmount;++i) {
			var N = normals[i];
			var d = normalsCount[i];
			N = [N[0]/d, N[1]/d, N[2]/d];
			vec3.normalize(N, N);
			normalData.push(N[0], N[1], N[2]);
		}

		mesh.addAttrib('normal', 3, normalData);
	}
};
