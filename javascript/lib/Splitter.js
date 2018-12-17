// modular that support split each part of data
/*
compress data outline
compress header -- record each part of data size(offset) refer to Encoder.h
glb header -- header of glb file
glb json data -- json data compressed by zlib
glb bin data -- usually is mesh data compressed by Draco
*/

class Splitter {
  constructor () {
    this.compressHeader = {
      compressDataSize: 0,
      glbHeaderSize: 0,
      glbBinSize: 0,
      glbJsonSize: 0
    }
    this.glbHeader = {}
    this.glbJson = {}
    this.glbBin = {}
  }
  Analyse (src) {
    if (!(src instanceof Uint8Array)) {
      console.error('parameter must be Uint8Array')
    }
    var view = new DataView(src.buffer)
    var offset = 0

    this.compressHeader.compressDataSize = view.getUint32(offset, true)
    offset += 4

    this.compressHeader.glbHeaderSize = view.getUint32(offset, true)
    offset += 4

    this.compressHeader.glbBinSize = view.getUint32(offset, true)
    offset += 4

    this.compressHeader.glbJsonSize = view.getUint32(offset, true)
    offset += 4

    this.glbHeader = src.subarray(offset,
      offset + this.compressHeader.glbHeaderSize)
    offset += this.compressHeader.glbHeaderSize

    this.glbJson = src.subarray(offset,
      offset + this.compressHeader.compressDataSize)
    offset += this.compressHeader.compressDataSize

    this.glbBin = src.subarray(offset,
      offset + this.compressHeader.glbBinSize)
    offset += this.compressHeader.glbBinSize
  }
  toString () {
    return 'compress data size: ' + this.glbJson.length + '\n' +
      'glb header size: ' + this.glbHeader.length + '\n' +
      'glb bin siz: ' + this.glbBin.length + '\n'
  }
}

module.exports = Splitter
