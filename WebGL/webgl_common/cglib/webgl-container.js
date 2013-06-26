// define namespace
var cglib = cglib || {};

Object.size = function(obj) {
    var size = 0, key;
    for (key in obj) {
        if (obj.hasOwnProperty(key)) size++;
    }
    return size;
};

/**
 * Simple mesh
 */
cglib.container = {		
	canvas : undefined,
	jsURL : undefined,
	app : undefined,
	displayFunc : undefined,
	keyPressedFunc : undefined,	
	keyDownFunc : undefined,
	keyUpFunc : undefined,
	resourcesURL : undefined,
	contextGL : undefined,
	wrappedDisplayLoop : undefined,
	isReset : false,
	isUsingAsyncLoad : false,

	init : function(canvas) {		
		this.canvas = canvas;
		
		this.wrappedDisplayLoop = this.wrapFunc(this.displayLoop);

		var that = this;
		var isMouseDown = false;

		mousePosRelative = function (canvas, evt){
		  var rect = canvas.getBoundingClientRect();
		  return {
		    x: evt.clientX - rect.left,
		    y: evt.clientY - rect.top
		  };
		}

		canvas.addEventListener('mousemove', function(evt) {		  
		  var mousePos = mousePosRelative(canvas, evt);
		  if(isMouseDown) {
		  	if(that.app) {
		  		that.app.motionFunc(mousePos.x, mousePos.y);	
		  	}
		  }
		  
		});

		canvas.addEventListener('mousedown', function(evt) {
			evt.preventDefault();
		  isMouseDown = true;
		  var mousePos = mousePosRelative(canvas, evt);
		  if(that.app) {
		  	that.app.mouseFunc(mousePos.x, mousePos.y);
		  }		  		  
		});

		canvas.addEventListener('mouseup', function(evt) {
		  isMouseDown = false;
		  //alert('mouseup');
		});

		canvas.addEventListener('resize', function(evt) {
			alert('resize');
			that.reshape();
		});

		this.getContext();
		this.reshape();
	},

	loadApp : function(appName, jsURL, resourcesURL) {
		this.jsURL = jsURL;
		this.resourcesURL = resourcesURL;
		var that = this;
		if(this.app) {
			this.app.release();			
			eval(this.app.name + ' = undefined');
		}

		jQuery.getScript(jsURL)
			.done(function(script, status, jqXHR) {
				that.app = eval(appName);
				that.app.name = appName;
				console.log('loaded!');
				that.app.init(that);
				if(!that.isUsingAsyncLoad) {
					that.start();
				}

			})
			.fail(function(jqxhr, settings, exception) {
				alert('Failed! to load ' + jsURL + exception);
			});
	},

	getContext : function() {
		if(this.contextGL == undefined) {
		    try {	    	
		    	console.log('Asking for context');
		        this.contextGL = this.canvas.getContext("experimental-webgl");
		    } catch (e) {
		    	alert("Can't get context" + e);
		    }
		    if (!this.contextGL) {
		        alert("Could not initialise WebGL, sorry :-(");
		        return;
		    }
		}
	    return this.contextGL;
	},

	reshape : function() {	    
	    this.contextGL.viewportWidth = this.canvas.width;
	    this.contextGL.viewportHeight = this.canvas.height;
	    console.log('Reshape - new sizes: ' + this.canvas.width + "," + this.canvas.height);
	},

	start : function() {
		this.isReset = false;
		this.app.start();
		if(this.displayFunc == undefined) {
			console.log('Can\'t start because displayFunc is not set');	
			return;
		}
		console.log('start');
		this.displayLoop();
	},

	displayLoop : function() {				
		if(this.isReset) {
			console.error("this actually shouldn't happen");
			return;
		}
		this.displayFunc();
		requestAnimFrame(this.wrappedDisplayLoop);
	},

	setDisplay : function(callback) {
		this.displayFunc = callback;
	},

	setKeyPressed : function(callback) {
		this.keyPressedFunc = callback;
		var that = this;
		jQuery(document).keypress(function(event) {
			that.keyPressedFunc(event.which);	
		});
	},

	setKeyDown : function(callback) {
		this.keyDownFunc = callback;
		var that = this;
		jQuery(document).keydown(function(event) {
			if(that.keyDownFunc(event.which))
				event.preventDefault();	
		});
	},

	setKeyUp : function(callback) {
		this.keyUpFunc = callback;
		var that = this;
		jQuery(document).keyup(function(event) {
			if(that.keyUpFunc(event.which))
				event.preventDefault();	
		});
	},

	loadResources : function(shaders) {
		this.shaders = {}
		var that = this;
		for(var shaderIndex in shaders) {
			var shader = shaders[shaderIndex];
			jQuery.ajax({
				async: false,
				url: this.resourcesURL + shader,
				dataType: 'text',
				cache: false,
				success: function(data) {
					that.shaders[shader] = data;
				}
			});
		}
	},

	loadResourcesAsync : function(shaders, images) {
		this.shaders = {}
		this.images = {}

		var that = this;
		this.isUsingAsyncLoad = true;

		var acceptedResponses = 0;
		var expectedResponses = shaders.length + (images?Object.size(images):0);
		var failedResponses = 0;
		
		if(expectedResponses == 0)
			return;

		var acceptResponse = function(isSuccess) {
			console.log(isSuccess);
			if(!isSuccess) {
				debugger;
			}
			failedResponses += !isSuccess;
			// Hope this is atomic...
			acceptedResponses++;	
	
			if(acceptedResponses == expectedResponses) {
				if(failedResponses) {
					throw "Can't start because some resources failed to load";
				} else {
					// Start!
					that.start();
				}
			}
		};

		//
		// Load shaders
		//
		for(var shaderIndex in shaders) {
			// We use closure here so that the inner functions reference a 
			// different 'shader' variable
			(function(shader){
				jQuery.ajax({
					async: true,
					url: that.resourcesURL + shader,
					dataType: 'text',
					cache: false,
					success: function(data) {					
						that.shaders[shader] = data;					
						console.log(data);
						acceptResponse(true);					
					},
					error: function() {
						acceptResponse(false);
					},
			});
			}(shaders[shaderIndex]));
		}

		//
		// Load images
		//
		for(var imageName in images) {
			var img = new Image();
			$(img)
				.load(function(){acceptResponse(true);})
				.error(function(){acceptResponse(false);})
				.attr('src', that.resourcesURL + images[imageName]);
			this.images[imageName] = img;
		}
	},

	getShaderText : function(name) {
		return this.shaders[name];
	},

	releaseBinds : function () {
		if(this.keyDownFunc) {
			jQuery(document).unbind('keydown');
			this.keyDownFunc = undefined;
		}
		if(this.keyUpFunc) {
			jQuery(document).unbind('keyup');
			this.keyUpFunc = undefined;
		}
		if(this.keyPressedFunc) {
			jQuery(document).unbind('keypress');
			this.keyPressedFunc = undefined;
		}		
	},

	reset : function() {
		this.isReset = true;
		this.isUsingAsyncLoad = false;

		// Attempt to stop further renderings
		cancelRequestAnimFrame(this.wrappedDisplayLoop);
		
		// Stop previous key/mouse from being called again
		this.releaseBinds();

		// Reset context
		WebGLDebugUtils.resetToInitialState(this.contextGL);

		if(this.contextGL) {
			var gl = this.contextGL;
			gl.clearColor(1, 1, 1, 0); 
		   	gl.clear(gl.COLOR_BUFFER_BIT);
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

	/* 
	If we want to pass a callback to a method then we are in a problem, because 
	'this' will point somewhere else. Therefore, we need to wrap any method 
	with a function that explicitly sets the right this
	*/
	wrapFunc : function(func) {
		var that = this;
		return function() {
			func.call(that);
		};
	}
}

