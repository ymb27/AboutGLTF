# 关于gzip与zlib

---

## 基本概念

* LZ77：一种基于字典的无损数据压缩算法
* deflate：一种基于LZ77，并加以霍夫曼编码的压缩算法
* gzip：一种文件结构，也是一种压缩格式，利用deflate对数据进行压缩后，加上头文件和CRC校验
* zlib：一个提供了LZ77，deflate等压缩方法的库；利用该库压缩文件，并添加上相应的文件头以及校验码，可以产生gzip，zlib等格式的文件

### gzip文件结构

* 10字节的文件头，包括magic number，版本号以及时间戳
* 可选的扩展文件头，内容可包括原文件名
* 文件体，包含了Deflate压缩的数据
* 8字节的尾注，包含CRC-32校验和，以及未压缩的原始数据长度

## zlib的使用

### 基本数据流结构

数据流是整个压缩过程的核心数据结构，是用户向zlib输入输出数据的接口。zlib不区分输入的数据类型，所有需要压缩的数据都以字节流的形式输入，以下为zlib提供的流声明

```c
typedef Bytef unsigned char;
typedef struct z_stream_s {
    z_const Bytef *next_in; // next input byte
    unsigned int avail_in; // number of bytes available at next_in
    unsigned long total_in; // total number of input bytes read so far
    
    Bytef *next_out; // next output byte will go here
    unsigned int avail_out; // remaining free space at next_out
    unsigned long total_out; // total number of bytes output so far
    
    z_const char* msg; // last error message, NULL if no error
    struct internal_state FAR *state; // not visible by applications
    
    alloc_func zalloc; // used to allocate the internal state
    free_func zfree; // used to free the internal state
    voidpf opaque; // private data object passed to zalloc and zfree
    
    int data_type; /* best guess about the data type: binary or text
    				for deflate, or the decoding state for inflate*/
    unsigned long adler; // adler-32 or CRC-32 value of the uncompressed data
    unsigned long reserved; // reserved for future use
} z_stream;
```

我们只需要处理:

1. `next_in`指向待压缩数据内存块，`avail_in`表明当前剩余输入数据的量(单位为字节)，`total_in`表明当前已经输入zlib的数据量(单位为字节)。当`avail_in`为0时，代表当前`next_in`所指向的数据流已经全部(指定部分)读入到zlib中，需要更新`next_in`和`avail_in`
2. 和1相反，`next_out`，`avail_out`以及`total_out`代表从流中输出数据的情况
3. `zalloc`以及`zfree`是用于指定zlib执行算法过程中申请内存的函数，通过设置`Z_NULL`(zlib中定义的空值)表明使用zlib预设的默认内存申请函数

除以上变量外，其余变量为zlib自身进行维护

### zlib进行deflate的基本过程

设置完基本数据流之后，需要结合三个函数进行deflate。三个函数包括`deflateInit`，`deflate`，`deflateEnd`。其功能分别为初始化流(设置剩余变量)，对当前流进行压缩以及结束压缩

