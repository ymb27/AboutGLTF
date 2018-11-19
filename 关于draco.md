# 关于Draco

---

文档描述Draco的调用方式以及原理

draco的源码编写中，类的成员变量基本都是私有的，有两种方式访问这些变量

1. 利用Get/Set函数
2. 提供const函数，返回成员变量的常引用

## API使用方法

### 压缩

![draco压缩分析](diagrams-Draco分析.jpg)

---

#### Encoder

​	压缩启动需要Encoder对象，Encoder对象继承自EncoderBase，相当于一个具体实现。能够通过该对象设置一些可配置属性，包括压缩速度以及压缩质量。最关键的函数为

```c++
Status EncodePointCloudToBuffer(const PointCloud &pc, EncoderBuffer *out_buffer);
Status EncodeMeshToBuffer(const Mesh &m, EncoderBuffer *out_buffer);
```

​	这两个函数将Mesh或者PointCloud进行压缩，并输出压缩后的数据(Mesh继承自PointCloud，Mesh相当于带有点之间关系的点云

​	参考:*draco/compression/encode.h*

#### PointCloud

​	两个关键字metadata attributes，可以利用**PointCloudBuilder**辅助创建点云

#### EntryValue

​	EntryValue的主要作用是作为值的容器。其核心的变量`std::vector<uint8_t>`即是利用`std::vector`依赖连续内存空间实现的特性，将一系列的数据，**不管其类型**，通通存放到连续的内存区域中。同时保证该内存区域能自由伸缩。

```c++
  template <typename DataTypeT>
  explicit EntryValue(const std::vector<DataTypeT> &data) {
    const size_t total_size = sizeof(DataTypeT) * data.size();
    data_.resize(total_size);
    memcpy(&data_[0], &data[0], total_size);
  }
```

​	从模板构造函数就可以看出，EntryValue通用容器的特性。

#### Metadata

​	从Metadata的成员变量可以看出，该类是作为EntryValue的容器。同时每个EntryValue又拥有自己的名称。

#### AttributeMetadata

​	从成员变量看，该类型继承于Metadata，并对其进行了简单的“修饰”。AttributeMetadata拥有自己的ID，注释上描述该ID是独一无二的，但从成员函数看并没有保证ID的独立性。

#### GeometryMetadata

​	虽然该类也继承于Metadata，但目前还没有任何看到它如何使用Metadata类。从成员函数以及成员变量分析，该类仅仅是AttributeMetadata的容器。

#### Mesh

​	利用**triangle_soup_mesh_builder**可以简洁地创建mesh