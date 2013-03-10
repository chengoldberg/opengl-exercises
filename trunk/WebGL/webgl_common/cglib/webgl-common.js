//////////////////////////////////////////////////////////////////////////////
// Chen's handy WebGL functions
//////////////////////////////////////////////////////////////////////////////

// define namespace
var cglib = cglib || {};

/**
 * WebGL Common Module 
 */
cglib.WebGLCommon = function() {
	
	var gl = undefined;	
	
	function bindContext(_gl) {
		gl = _gl; 
	}
	
	function getCurrentContext() {
		return gl;
	}
	
	function compileShader(str,type) {	
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
	
	function linkProgram(vs,fs) {
	    program = gl.createProgram();
	    gl.attachShader(program, vs);
	    gl.attachShader(program, fs);
	    gl.linkProgram(program);
	
	    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
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
	function getShaderScript(gl, id) {
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
		bindContext : bindContext,
		getCurrentContext : getCurrentContext,
		compileShader : compileShader,
		linkProgram : linkProgram,
		getShaderScript : getShaderScript,
		loadFile : loadFile
		};
} ();