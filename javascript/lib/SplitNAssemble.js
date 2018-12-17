// modular that support split each part of data and assemble a glb after decompression
class Splitter {
  constructor () {
    this.compressHeader = {
      compressDataSize: 0
    }
  }
  Analyse (src) {
    var view = new DataView(src)
    this.compressHeader.compressDataSize = view.getUint32(0, true)
  }
}

module.exports = Splitter
