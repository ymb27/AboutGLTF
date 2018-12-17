var fs = require('fs')
var Splitter = require('./lib/Splitter')
var ZlibWrapper = require('./lib/ZlibWrapper')
var k = new Splitter()
var z = new ZlibWrapper()
var output = {}
fs.readFile('./example/test.zglb', function (err, data) {
  if (err) {
    return console.error(err)
  } else {
    k.Analyse(data)
    k.glbJson = z.decompress(k.glbJson)
    output = new Uint8Array(k.glbHeader.length +
      k.glbJson.length + k.glbBin.length)
    var headerView = new DataView(k.glbHeader.buffer)
    var jsonView = new DataView(k.glbJson.buffer)
    var binView = new DataView(k.glbBin.buffer)
    var outputView = new DataView(output.buffer)
    var outputOffset = 0
    var offset = 0
    for (offset = 0; offset < k.glbHeader.length; ++offset) {
      outputView.setUint8(outputOffset, headerView.getUint8(offset))
      ++outputOffset
    }
    for (offset = 0; offset < k.glbJson.length; ++offset) {
      outputView.setUint8(outputOffset, jsonView.setUint8(offset))
      ++outputOffset
    }
    for (offset = 0; offset < k.glbBin.length; ++offset) {
      outputView.setUint8(outputOffset, binView.getUint8(offset))
      ++outputOffset
    }
  }
})
