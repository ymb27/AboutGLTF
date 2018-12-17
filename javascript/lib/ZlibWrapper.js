var pako = require('pako')

class ZlibWrapper {
  // constructor () {
  //   this.inflate = new pako.Inflate({
  //     windowBits: -15,
  //     raw: true
  //   })
  // }
  decompress (src) {
    if (!(src instanceof Uint8Array)) {
      console.error('parameter is not uint8Array')
    }
    return pako.inflateRaw(src, { windowBits: -15 })
  }
}

module.exports = ZlibWrapper
