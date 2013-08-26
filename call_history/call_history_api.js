// on (successCallback, errorCallback, filter, sortMode, limit, offsetight (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// FIXME: A lot of these methods should throw NOT_SUPPORTED_ERR on desktop.
// There is no easy way to do verify which methods are supported yet.
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
        listener[msg.eventType](msg.data);
      });
    }
    else if (callback) {
      delete msg._reply_id;
      delete callbacks[reply_id];
      callback(msg.data);
    } else {
      console.log('Invalid reply_id received from node extension: ' + reply_id);
    }
  });
  return function(msg, args) {
    var reply_id = next_reply_id, send_msg = extension.internal.sendSyncMessage;
    Array.prototype.forEach.call(args, function (arg) {
      if (arg) {
        for (var prop in arg) {
          if (typeof arg[prop] === "function") {
            console.log("###################################pushing listener");
            _event_listeners.push(arg);
            break;
          }
        }
        if (typeof arg === "function") {
          next_reply_id += 1;
          arg._reply_id = reply_id.toString();
          callbacks[reply_id] = arg;
          send_msg = extension.postMessage;
        }
      }
    });

    return send_msg(JSON.stringify(msg, function (key, value) {
      if (typeof value === "function")
        return "__function__:" + (value._reply_id ? value._reply_id : "");
      return value;
    }));
  }
})();
/*exports.find =  function (successCallback, errorCallback, filter, sortMode, limit, offset) {
    postMessage({cmd: "find", arguments: Array.prototype.slice.call(arguments)}, successCallback, errorCallback);
}
exports.addChangeListener =  function (onListenerCB) {
    _event_listeners.push(onListenerCB);
    return Number(extension.internal.sendSyncMessage(JSON.stringify({cmd: "addChangeListener", arguments: Array.prototype.slice.call(arguments)}, function (key, value) {
      if (typeof value === "function")
        return "__function__:" + value.name;
      return value;
    })));
}
exports.remove =  function () {
    extension.postMessage(JSON.stringify({cmd: "remove", arguments: Array.prototype.slice.call(arguments)}));
}
exports.removeBatch =  function (entries, sucessCallback, errorCallback) {
    postMessage({cmd: "removeBatch", arguments: Array.prototype.slice.call(arguments)}, sucessCallback, errorCallback);
}*/
function general_func(cmd, args) {
    return postMessage({cmd: cmd, arguments: Array.prototype.slice.call(args)}, args);
}
exports.find = function () {
  return general_func("find", arguments);
}
exports.addChangeListener = function () {
  return general_func("addChangeListener", arguments);
}
exports.remove = function () {
  return general_func("remove", arguments);
}
