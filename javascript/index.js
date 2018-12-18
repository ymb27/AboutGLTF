var Splitter = require('./lib/Splitter')
var Assembler = require('./lib/Assembler')

function DecodeZGLB (src, windowsBit = -15) {
  if (!(src instanceof Uint8Array)) {
    console.error('decode failed! src must be Uint8Array')
    return null
  }
  var k = new Splitter()
  k.Analyse(src)
  var a = new Assembler(k.glbHeader, k.glbJson, k.glbBin)
  return a.GetGLB(windowsBit)
}

module.exports = DecodeZGLB
