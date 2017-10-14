const u2fhostnode = require('bindings')('u2fhostnode');

function U2FError(message, code) {
  this.name = "U2FError";
  this.message = message;
  this.code = code;
  this.stack = (new Error()).stack;
}
U2FError.prototype = new Error;

module.exports = {
  register(appId, registerRequests, registeredKeys, callback, opt_timeoutSeconds) {

  },

  sign(appId, challenge, registeredKeys, callback, opt_timeoutSeconds) {

  },

  u2fHost: {
    register(challenge, origin, callback) {
      if(callback) {

      } else {
        try {
          return u2fhostnode.register(challenge, origin);
        } catch(e) {
          throw new U2FError(e.message, e.code);
        }
      }
    },

    sign(challenge, origin, callback) {
      if(callback) {

      } else {
        try {
          return u2fhostnode.sign(challenge, origin);
        } catch(e) {
          throw new U2FError(e.message, e.code);
        }
      }
    }
  }
}
