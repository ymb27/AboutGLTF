# Work Note

---

## 当前工作要求

后台需要对数据进行压缩，获得的二进制数据传输到页面，利用js进行解压

![后端工作流程](G:/AboutGLTF/%E5%90%8E%E7%AB%AF%E5%B7%A5%E4%BD%9C%E6%B5%81%E7%A8%8B.jpg)

## 工作进展

------

### 任务情况

* [ ] 解析gltf，提取模型bin数据
  * [ ] 不读入图片数据，除非源文件已经以data-uri的方式进行存储
* [ ] 生成draco对象并进行压缩

  * [x] 提取gltf中的triangle primitive手动构造draco::mesh
  * [ ] 检验压缩前后模型的数据一致性：
    * [x] 索引信息
    * [x] 位置信息
    * [x] 法线信息
    * [ ] 贴图坐标信息(第一套)
  * [x] 读取多个triangle primitive进行压缩
  * [x] 处理非triangle primitive
    * 对非triangle primitive不进行处理
  * [x] c++测试从draco::mesh还原gltf
  * [ ] 输出glb流(非文件)，需要修改tinygltf
    * [ ] 处理图片输出问题，图片存储情况维持原样(uri的保持原uri，data-uri的依然是data-uri)
  * [ ] 对glb流的json块进行gz压缩
* [ ] linux下编译测试
* [ ] 前端包开发

### 现存问题

* [ ] 内存占用大(压缩9M数据，运行时内存占用30M)

* [x] 压缩率低(当前为默认设置，仅80%)

  * 根据primitive各个attribute的min和max值，设置量化的范围
  * 当前压缩率位20%

* [ ] 压缩速度慢(当前为默认设置)

