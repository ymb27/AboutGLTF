// decompress json data and return a glb file
var ZlibWrapper = require('./ZlibWrapper')

function validateUint8Array (data) {
  return data instanceof Uint8Array
}

class Assembler {
  constructor (header = null, compressedJson = null, bin = null) {
    if (header && !validateUint8Array(header)) {
      console.error('header parameter must be Uint8Array')
      this.glbHeader = null
    } else {
      this.glbHeader = header
    }
    if (compressedJson && !validateUint8Array(compressedJson)) {
      console.error('compressed json data must be Uint8Array')
      this.glbJson = null
    } else {
      this.glbJson = compressedJson
    }
    if (bin && !validateUint8Array(bin)) {
      console.error('bin data must be Uint8Array')
      this.glbBin = null
    } else {
      this.glbBin = bin
    }
  }
  setHeader (header) {
    if (!validateUint8Array(header)) {
      console.error('header data must be Uint8Array')
    } else {
      this.glbHeader = header
    }
  }
  setCompressedJson (json) {
    if (!validateUint8Array(json)) {
      console.error('compressed json data must be Uint8Array')
    } else {
      this.glbJson = json
    }
  }
  setBin (bin) {
    if (!validateUint8Array(bin)) {
      console.error('bin data must be Uint8Array')
    } else {
      this.glbBin = bin
    }
  }
  GetGLB (windowsBit = -15) {
    if (this.glbHeader && this.glbJson && this.glbBin) {
      var z = new ZlibWrapper()
      this.glbJson = z.decompress(this.glbJson, windowsBit)
      var ret = new Uint8Array(this.glbHeader.length +
        this.glbJson.length + this.glbBin.length)
      var retOffset = 0
      var subOffset = 0
      for (subOffset = 0; subOffset < this.glbHeader.length; ++subOffset, ++retOffset) {
        ret[retOffset] = this.glbHeader[subOffset]
      }
      for (subOffset = 0; subOffset < this.glbJson.length; ++subOffset, ++retOffset) {
        ret[retOffset] = this.glbJson[subOffset]
      }
      for (subOffset = 0; subOffset < this.glbBin.length; ++subOffset, ++retOffset) {
        ret[retOffset] = this.glbBin[subOffset]
      }
      return ret
    }
  }
}

module.exports = Assembler
