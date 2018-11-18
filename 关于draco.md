# 关于Draco

---

文档描述Draco的调用方式以及原理

draco的源码编写中，类的成员变量基本都是私有的，有两种方式访问这些变量

1. 利用Get/Set函数
2. 提供const函数，返回成员变量的常引用

## API使用方法

### 压缩

---

#### Encoder

​	压缩启动需要Encoder对象，Encoder对象继承自EncoderBase，相当于一个具体实现。能够通过该对象设置一些可配置属性，包括压缩速度以及压缩质量。最关键的函数为

```c++
Status EncodePointCloudToBuffer(const PointCloud &pc, EncoderBuffer *out_buffer);
Status EncodeMeshToBuffer(const Mesh &m, EncoderBuffer *out_buffer);
```

​	这两个函数将Mesh或者PointCloud进行压缩，并输出压缩后的数据(Mesh继承自PointCloud，Mesh相当于带有点之间关系的点云)

​	参考:*draco/compression/encode.h*

#### PointCloud

​	两个关键字metadata attributes

