//////////////////////////////////////////////////////////////////////////////
// Chen's handy WebGL functions
//////////////////////////////////////////////////////////////////////////////

// define namespace
var cglib = cglib || {};

/**
 * WebGL Common Module 
 */
cglib.WebGLCommon = function() {
		
	function compileShader(gl, str,type) {	
	    var shader;
	    shader = gl.createShader(type);
	
	    gl.shaderSource(shader, str);
	    gl.compileShader(shader);
	
	    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
	        alert(gl.getShaderInfoLog(shader));
	        return null;
	    }
	
	    return shader;
	}
	
	function linkProgram(gl, vs,fs) {
	    program = gl.createProgram();
	    gl.attachShader(program, vs);
	    gl.attachShader(program, fs);
	    gl.linkProgram(program);
	
	    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
	    	alert(gl.getProgramInfoLog(program));
	        alert("Could not initialise shaders");
	    }	
	    return program;
	}
	
	/**
	 * Returns a shader's script
	 * @param gl
	 * @param id
	 * @returns
	 */
	function getShaderScript(id) {
	    var shaderScript = document.getElementById(id);
	    if (!shaderScript) {
	        return null;
	    }
	
	    var str = "";
	    var k = shaderScript.firstChild;
	    while (k) {
	        if (k.nodeType == 3)
	            str += k.textContent;
	        k = k.nextSibling;
	    }
	    return str;
	}
	
	function loadFile(file, callback, noCache, isJson) {
		var request = new XMLHttpRequest();
		request.onreadystatechange = function() {
			if (request.readyState == 1) {
				if (isJson) {
					request.overrideMimeType('application/json');
				}
				request.send();
			} else if (request.readyState == 4) {
				if (request.status == 200) {
					callback(request.responseText);
				} else if (request.status == 404) {
					throw 'File "' + file + '" does not exist.';
				} else {
					throw 'XHR error ' + request.status + '.';
				}
			}
		};
		var url = file;
		if (noCache) {
			url += '?' + (new Date()).getTime();
		}
		request.open('GET', url, true);
	}
	
	// Define public interface
	return {
		compileShader : compileShader,
		linkProgram : linkProgram,
		getShaderScript : getShaderScript,
		loadFile : loadFile
		};
} ();

cglib.Canvas2DTexture = {
	textureCanvas: undefined,
	ctx: undefined, 

	init: function(width, height) {
		jQuery('<canvas id="auxCanvas" width="512" height="512" style="display:none"></canvas>').appendTo("body");
		this.textureCanvas = jQuery("#auxCanvas")[0];
		this.ctx = this.textureCanvas.getContext('2d');		
	},

	drawToTexture: function(gl, drawFunc, auxTex) {			
		this.ctx.clearRect(0, 0, this.textureCanvas.width, this.textureCanvas.height);
		
		if(drawFunc)
			drawFunc(this.ctx);

		if(!auxTex)
			auxTex = gl.createTexture();						

		gl.bindTexture(gl.TEXTURE_2D, auxTex);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, this.textureCanvas);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.bindTexture(gl.TEXTURE_2D, null);

		return auxTex;
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