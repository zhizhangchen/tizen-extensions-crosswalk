// on (successCallback, errorCallback, filter, sortMode, limit, offsetight (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// FIXME: A lot of these methods should throw NOT_SUPPORTED_ERR on desktop.
// There is no easy way to do verify which methods are supported yet.
var postMessage = (function() {
  var callbacks = {};
  var next_reply_id = 0;
  extension.setMessageListener(function(json) {
    var msg = JSON.parse(json);
    var reply_id = msg._reply_id;
    var callback = callbacks[reply_id];
    if (reply_id === undefined) {
      //It's an event. Call listeners
      listeners.forEach(function (listener) {
        listener[msg.eventType](msg.data);
      });
    }
    else if (callback) {
      delete msg._reply_id;
      delete callbacks[reply_id];
      if (msg.error)
        callback.error(msg);
      else
        callback.success(msg.data);
    } else {
      console.log('Invalid reply_id received from node extension: ' + reply_id);
    }
  });
  return function(msg, sucessCallback, errorCallback) {
    var reply_id = next_reply_id;
    next_reply_id += 1;
    callbacks[reply_id] = {error: errorCallback, success: sucessCallback};
    msg._reply_id = reply_id.toString();
    extension.postMessage(JSON.stringify(msg));
  }
})();
exports.find =  function (successCallback, errorCallback, filter, sortMode, limit, offset) {
    postMessage({cmd: "find", filter: filter, sortMode: sortMode, limit: limit, offset: offset}, successCallback, errorCallback);
}
/*exports.find =  function () {
    postMessage({cmd: "find", args: JSON.stringify(arguments)});
    console.log(JSON.stringify(arguments));
}*/
var listeners = [];
exports.addChangeListener =  function (onListenerCB) {
    listeners.push(onListenerCB);
    return Number(extension.internal.sendSyncMessage(JSON.stringify({cmd: "addChangeListener", arguments: Array.prototype.slice.call(arguments)}, function (key, value) {
      if (typeof value === "function")
        return "__function__:" + value.name;
      return value;
    })));
    //return Number(extension.internal.sendSyncMessage(JSON.stringify({cmd: "addChangeListener"})));
}
exports.removeChangeListener =  function (onListenerCB) {
    extension.postMessage(JSON.stringify({cmd: "removeChangeListener"}));
}
exports.remove =  function (entry) {
    extension.postMessage(JSON.stringify({cmd: "remove", entry: entry}));
}
