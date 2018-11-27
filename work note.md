# Work Note

---

## 当前工作要求

后台需要对数据进行压缩，获得的二进制数据传输到页面，利用js进行解压

![后端工作流程](G:/AboutGLTF/%E5%90%8E%E7%AB%AF%E5%B7%A5%E4%BD%9C%E6%B5%81%E7%A8%8B.jpg)

## 工作进展

------

### 任务情况

* [x] 解析gltf，提取模型bin数据

  * 可以直接利用tinygltf读取其中所有属性的内容
* [ ] 生成draco对象并进行压缩

  * [x] 提取gltf中的triangle primitive手动构造draco::mesh
  * [ ] 检验压缩前后模型的数据一致性：
    * [x] 索引信息
    * [x] 位置信息
    * [x] 法线信息
    * [ ] 贴图坐标信息(第一套)
  * [x] 读取多个triangle primitive进行压缩，并合并到一个buffer中
  * [ ] 处理非triangle primitive
  * [ ] c++测试从draco::mesh还原gltf
* [ ] linux下编译测试
* [ ] 前端包开发

### 现存问题

* [ ] 内存占用大(压缩9M数据，运行时内存占用30M)

* [ ] 压缩率低(当前为默认设置，仅80%)

  优化可能——分析模型数据，设置合适的AttributeQuantization；

  去除重复的属性数据，但会降低解压时的效率，增加解压过程的内存占用

* [ ] 压缩速度慢(当前为默认设置)

