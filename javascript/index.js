var fs = require('fs')
var Splitter = require('./lib/Splitter')
var ZlibWrapper = require('./lib/ZlibWrapper')
var k = new Splitter()
var z = new ZlibWrapper()

var input = fs.readFileSync('./example/test.zglb')
k.Analyse(input)
var outputData = (function () {
  k.glbJson = z.decompress(k.glbJson)
  var ret = new Uint8Array(k.glbHeader.length +
    k.glbJson.length + k.glbBin.length)
  var outputOffset = 0
  var offset = 0
  for (offset = 0; offset < k.glbHeader.length; ++offset) {
    ret[outputOffset] = k.glbHeader[offset]
    ++outputOffset
  }
  for (offset = 0; offset < k.glbJson.length; ++offset) {
    ret[outputOffset] = k.glbJson[offset]
    ++outputOffset
  }
  for (offset = 0; offset < k.glbBin.length; ++offset) {
    ret[outputOffset] = k.glbBin[offset]
    ++outputOffset
  }
  return ret
})()
var buf = Buffer.from(outputData.buffer)
fs.writeFileSync('./example/test.glb', buf, {
  encoding: 'buffer',
  flag: 'w'
})
