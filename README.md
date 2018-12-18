# glTF文件压缩模块

---

## 压缩过程

使用Draco对glTF文件中三角形类型网格进行压缩，再利用flate(zlib)，对json部分字符串进行压缩，输出为glb。

## 模块说明

模块分为前后端两部分，后端为c++静态库，用于压缩glTF模型；前端为js模块，用于解压生成glb

## 第三方

* Draco
* zlib
* tinygltf

## 安装

### 压缩部分

在./cpp路径下cmake，需要手动设置zlib以及dracoenc两个静态库

注意cygwin编译zlib时需要处理wopen问题

### 解压部分

./javascript路径下

```shell
npm install
```

## 使用

### 压缩部分

```c++
#include <Encoder.h>
GLTF_ENCODER::Encoder ec;
GLTF_ENCODER::GE_STATE state;
std::unique_ptr<std::vector<uint8_t> > compressedData;
std::string glTFData;
/* parse gltf data and store in glTFData */
state = ec.EncodeFromMemory(glTFData);
compressedData =ec.Buffer();
/* get error message from ec.ErrorMsg() */
```

### 解压部分

```js
var DecodeZGLB = require('decode_zglb')
var zglbData = new Uint8Array()
/* set zglbData */
var glbData = DecodeZGLB(zglbData)
```

