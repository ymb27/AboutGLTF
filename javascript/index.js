var fs = require('fs')
var Splitter = require('./lib/SplitNAssemble')
var k = new Splitter()
fs.readFile('./test.zglb', function (err, data) {
  if (err) {
    return console.error(err)
  } else {
    k.Analyse(data.buffer)
    console.log(k.compressHeader.compressDataSize)
  }
})
