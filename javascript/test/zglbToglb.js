var fs = require('fs')
var Decoder = require('../index')

var input = fs.readFileSync('./example/test.zglb')
var outputData = Decoder(input, -15)
var buf = Buffer.from(outputData.buffer)
fs.writeFileSync('./example/test.glb', buf, {
  encoding: 'buffer',
  flag: 'w'
})
