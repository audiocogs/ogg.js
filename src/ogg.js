var AV = require('av');
var Ogg = require('../build/libogg');

var OggDemuxer = AV.Demuxer.extend(function() {
  AV.Demuxer.register(this);
  
  this.probe = function(buffer) {
    return buffer.peekString(0, 4) === 'OggS';
  };
  
  this.plugins = [];
  var BUFFER_SIZE = 8192;
  
  this.prototype.init = function() {
    this.ogg = Ogg._AVOggInit();
    this.buf = Ogg._malloc(BUFFER_SIZE);
    
    var self = this;
    var plugin = null;
    var doneHeaders = false;
    
    // copy the stream in case we override it, e.g. flac
    this._stream = this.stream;
    
    this.callback = Ogg.Runtime.addFunction(function(packet, bytes) {
      var data = new Uint8Array(Ogg.HEAPU8.subarray(packet, packet + bytes));      
      
      // find plugin for codec
      if (!plugin) {
        for (var i = 0; i < OggDemuxer.plugins.length; i++) {
          var cur = OggDemuxer.plugins[i];
          var magic = data.subarray(0, cur.magic.length);
          if (String.fromCharCode.apply(String, magic) === cur.magic) {
            plugin = cur;
            break;
          }
        }
        
        if (!plugin)
          throw new Error("Unknown format in Ogg file.");
          
        if (plugin.init)
          plugin.init.call(self);
      }
      
      // send packet to plugin
      if (!doneHeaders)
        doneHeaders = plugin.readHeaders.call(self, data);
      else
        plugin.readPacket.call(self, data);
    });
  };
  
  this.prototype.readChunk = function() {
    while (this._stream.available(BUFFER_SIZE)) {
      Ogg.HEAPU8.set(this._stream.readBuffer(BUFFER_SIZE).data, this.buf);
      Ogg._AVOggRead(this.ogg, this.buf, BUFFER_SIZE, this.callback);
    }
  };
  
  this.prototype.destroy = function() {
    this._super();
    Ogg.Runtime.removeFunction(this.callback);
    Ogg._AVOggDestroy(this.ogg);
    Ogg._free(this.buf);
    
    this.ogg = null;
    this.buf = null;
    this.callback = null;
  };
});

module.exports = OggDemuxer;
AV.OggDemuxer = OggDemuxer; // for browser
