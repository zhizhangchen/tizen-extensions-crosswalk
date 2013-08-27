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
      callback.apply(null, msg.arguments);
    } else {
      console.log('Invalid reply_id received from node extension: ' + reply_id);
    }
  });
  return function(api, args) {
    var send_msg = extension.internal.sendSyncMessage;
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
          send_msg = extension.postMessage;
        }
      }
    });

    var result = send_msg(JSON.stringify({cmd: api, arguments: Array.prototype.slice.call(args)}, function (key, value) {
      if (typeof value === "function")
        return "__function__:" + (value._reply_id ? value._reply_id : "");
      return value;
    }));
    return  (result === undefined || result === "")? undefined: JSON.parse(result);
  }
})();

apis.forEach(function(api) {
  exports[api] = function () {
    return postMessage(api, arguments);
  }
})
