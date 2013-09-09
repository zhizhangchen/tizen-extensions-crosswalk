var _event_listeners = [];
var jQuery = jQuery || {},
	  class2type = class2type || {};
jQuery.fn = jQuery.fn || {};
//Copy from jQuery
jQuery.extend = jQuery.fn.extend = function() {
	var options, name, src, copy, copyIsArray, clone,
		target = arguments[0] || {},
		i = 1,
		length = arguments.length,
		deep = false;

	// Handle a deep copy situation
	if ( typeof target === "boolean" ) {
		deep = target;
		target = arguments[1] || {};
		// skip the boolean and the target
		i = 2;
	}

	// Handle case when target is a string or something (possible in deep copy)
	if ( typeof target !== "object" && !jQuery.isFunction(target) ) {
		target = {};
	}

	// extend jQuery itself if only one argument is passed
	if ( length === i ) {
		target = this;
		--i;
	}

	for ( ; i < length; i++ ) {
		// Only deal with non-null/undefined values
		if ( (options = arguments[ i ]) != null ) {
			// Extend the base object
			for ( name in options ) {
				src = target[ name ];
				copy = options[ name ];

				// Prevent never-ending loop
				if ( target === copy ) {
					continue;
				}

				// Recurse if we're merging plain objects or arrays
				if ( deep && copy && ( jQuery.isPlainObject(copy) || (copyIsArray = jQuery.isArray(copy)) ) ) {
					if ( copyIsArray ) {
						copyIsArray = false;
						clone = src && jQuery.isArray(src) ? src : [];

					} else {
						clone = src && jQuery.isPlainObject(src) ? src : {};
					}

					// Never move original objects, clone them
					target[ name ] = jQuery.extend( deep, clone, copy );

				// Don't bring in undefined values
				} else if ( copy !== undefined ) {
					target[ name ] = copy;
				}
			}
		}
	}

	// Return the modified object
	return target;
};

jQuery.extend({
	// See test/unit/core.js for details concerning isFunction.
	// Since version 1.3, DOM methods and functions like alert
	// aren't supported. They return false on IE (#2968).
	isFunction: function( obj ) {
		return jQuery.type(obj) === "function";
	},
	isArray: Array.isArray || function( obj ) {
		return jQuery.type(obj) === "array";
	},

	// A crude way of determining if an object is a window
	isWindow: function( obj ) {
		return obj && typeof obj === "object" && "setInterval" in obj;
	},

	type: function( obj ) {
		return obj == null ?
			String( obj ) :
			class2type[ toString.call(obj) ] || "object";
	},

	isPlainObject: function( obj ) {
		// Must be an Object.
		// Because of IE, we also have to check the presence of the constructor property.
		// Make sure that DOM nodes and window objects don't pass through, as well
		if ( !obj || jQuery.type(obj) !== "object" || obj.nodeType || jQuery.isWindow( obj ) ) {
			return false;
		}

		try {
			// Not own constructor property must be Object
			if ( obj.constructor &&
				!hasOwn.call(obj, "constructor") &&
				!hasOwn.call(obj.constructor.prototype, "isPrototypeOf") ) {
				return false;
			}
		} catch ( e ) {
			// IE8,9 Will throw exceptions on certain host objects #9897
			return false;
		}

		// Own properties are enumerated firstly, so to speed up,
		// if last one is own, then all properties are own.

		var key;
		for ( key in obj ) {}

		return key === undefined || hasOwn.call( obj, key );
	},

	each: function( object, callback, args ) {
		var name, i = 0,
			length = object.length,
			isObj = length === undefined || jQuery.isFunction( object );

		if ( args ) {
			if ( isObj ) {
				for ( name in object ) {
					if ( callback.apply( object[ name ], args ) === false ) {
						break;
					}
				}
			} else {
				for ( ; i < length; ) {
					if ( callback.apply( object[ i++ ], args ) === false ) {
						break;
					}
				}
			}

		// A special, fast, case for the most common use of each
		} else {
			if ( isObj ) {
				for ( name in object ) {
					if ( callback.call( object[ name ], name, object[ name ] ) === false ) {
						break;
					}
				}
			} else {
				for ( ; i < length; ) {
					if ( callback.call( object[ i ], i, object[ i++ ] ) === false ) {
						break;
					}
				}
			}
		}

		return object;
	},
});
// Populate the class2type map
jQuery.each("Boolean Number String Function Array Date RegExp Object".split(" "), function(i, name) {
	class2type[ "[object " + name + "]" ] = name.toLowerCase();
});


var postMessage = (function() {
  var callbacks = {};
  var next_reply_id = 0;
  extension.setMessageListener(function(json) {
    var msg = JSON.parse(json);
    var reply_id = msg._reply_id;
    var callback = callbacks[reply_id];
    if (reply_id === undefined) {
      //It's an event. Call listeners
      _event_listeners.forEach(function (listener) {
        listener[msg.eventType].apply(null, msg.arguments);
      });
    }
    else if (callback) {
      delete msg._reply_id;
      delete callbacks[reply_id];
      if (msg.exception) {
        throw exception;
      }
      callback.apply(null, msg.arguments);
    } else {
      console.log('Invalid reply_id received from node extension: ' + reply_id);
    }
  });
  return function(api, func, args) {
    var send_msg = extension.internal.sendSyncMessage;
    var request, resp, respObj;
    Array.prototype.forEach.call(args, function (arg) {
      if (arg) {
        for (var prop in arg) {
          if (typeof arg[prop] === "function") {
            _event_listeners.push(arg);
            break;
          }
        }
        if (typeof arg === "function") {
          var reply_id = next_reply_id;
          next_reply_id += 1;
          arg._reply_id = reply_id.toString();
          callbacks[reply_id] = arg;
          //send_msg = extension.postMessage;
        }
      }
    });

    request = JSON.stringify({api: api, cmd: func, arguments: Array.prototype.slice.call(args, 0)}, function (key, value) {
      if (typeof value === "function") {
        return "__function__:" + (value._reply_id ? value._reply_id : "");
      }
      else if  (value === undefined) {
        return "__undefined__";
      }
      else if  (Number.isNaN(value)) {
        return "__NaN__";
      }
      else if  (value && value.__obj_ref) {
        return {__obj_ref: value.__obj_ref};
      }
      else return value;
    });
    resp = send_msg(request);
    respObj = ((resp === undefined || resp === "" || resp == "undefined") ? undefined : JSON.parse(resp, function(key, value) {
      if (value === "__function__")
        return function () {
          return postMessage(this, key, arguments);
        }
      return value;
    }));
    if (respObj && respObj.exception) {
      throw respObj.exception;
    }
    respObj && respObj.arguments && respObj.arguments.forEach(function (arg, i) {
      jQuery.extend(true, args[i], arg);
    });
    return respObj && respObj.result;
  }
})();
window.tizen = window.tizen || {};
for (var api in apis) {
  window.tizen[api] = window.tizen[api] || {};
  if (apis[api] === "__constructor__") {
    var constructor =  function () {
      var obj = postMessage(this.api, "__constructor__" , arguments);
      for (var prop in obj) {
        this[prop] = obj[prop];
      }
    }
    constructor.prototype.api = api;
    window.tizen[api] = constructor;
    continue;
  }

  apis[api].forEach(function(func) {
    window.tizen[api][func] = function () {
      return postMessage(this.api, this.func, arguments);
    }.bind({api:api, func:func});
  })
}
postMessage("__frame_loaded__",  "", {});
