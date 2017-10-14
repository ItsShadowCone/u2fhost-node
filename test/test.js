U2F_DEBUG = false;

var crypto = require('crypto');
var assert = require('assert');

var request = require('request');
var u2f;

// For use on the yubico api
var username;
var password;

describe('libu2f-host', function() {
  this.timeout(60000);

  it('should startup', function(done) {
    u2f = require('../');
    username = crypto.randomBytes(8).toString('hex');
    password = crypto.randomBytes(8).toString('hex');
    done();
  });

  it('should register an u2f device', function(done) {
    request({url: 'https://demo.yubico.com/wsapi/u2f/enroll', qs: {username, password}, json: true}, function(err, res, body) {
      if(err)
        throw err;

      console.log('Now, please touch the button on your U2F device to test registering');
      var response = u2f.u2fHost.register(JSON.stringify(body), 'https://demo.yubico.com');

      request({method: 'POST', url: 'https://demo.yubico.com/wsapi/u2f/bind', form: {username, password, data: response}, json: true}, function(err, res, body) {
        if(err)
          throw err;

        assert.equal(typeof body, typeof {});
        assert.equal(body.username, username);

        done();
      });
    });
  });

  it('should sign with the u2f device', function(done) {
    request({url: 'https://demo.yubico.com/wsapi/u2f/sign', qs: {username, password}, json: true}, function(err, res, body) {
      if(err)
        throw err;

      console.log('Now, please touch the button on your U2F device to test signing');
      var response = u2f.u2fHost.sign(JSON.stringify(body), 'https://demo.yubico.com');

      request({method: 'POST', url: 'https://demo.yubico.com/wsapi/u2f/verify', form: {username, password, data: response}, json: true}, function(err, res, body) {
        if(err)
          throw err;

        assert.equal(typeof body, typeof {});

        done();
      });
    });
  });
});
