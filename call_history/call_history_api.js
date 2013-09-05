var _event_listeners = [];
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
      if (typeof value === "function")
        return "__function__:" + (value._reply_id ? value._reply_id : "");
      else if  (value === undefined) {
        return "__undefined__";
      }
      else if  (Number.isNaN(value)) {
        return "__NaN__";
      }
      else return value;
    });
    resp = send_msg(request);
    //throw request;
    respObj = ((resp === undefined || resp === "" || resp == "undefined") ? undefined : JSON.parse(resp, function(key, value) {
      if (value === "__function__")
        return function () {
          postMessage(this, key, arguments);
        }
      return value;
    }));
    if (respObj && respObj.exception) {
      throw respObj.exception;
    }
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
