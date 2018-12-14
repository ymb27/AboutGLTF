# Work Note

---

## 当前工作要求

后台需要对数据进行压缩，获得的二进制数据传输到页面，利用js进行解压

![后端工作流程](./%E5%90%8E%E7%AB%AF%E5%B7%A5%E4%BD%9C%E6%B5%81%E7%A8%8B.jpg)

## 工作进展

------

### 任务情况

* [x] 解析gltf，提取模型bin数据
  * [x] 不读入图片数据
    * 当image对象以uri形式保存时，仅读取uri字符串，不进行解析；
    * 当image对象以bufferView的形式保存时，仅读取bufferView属性信息，对应的buffer数据*依然会读入*
  * [x] 不输出图片数据
    * 当image对象以uri形式保存时，输出原uri字符串
    * 当image对象以bufferView形式保存时，输出buffer，默认缓冲区都以base64形式嵌入到gltf json部分中
  * [ ] 读取含有中文路径文件base_dir设置问题
    * [x] 测试windows上中文情况
    * 利用标准库，先将gltf的utf-8数据变成Unicode；再将unicode 变成 ansi迎合windows的api
    * (事实上windows api支持unicode，但是tinygltf使用了ansi，所以只能再转成ansi)
    * [ ] 测试linux上中文情况
* [ ] 生成draco对象并进行压缩
  * [x] 处理不使用索引的模型 
  * [x] 提取gltf中的triangle primitive手动构造draco::mesh
  * [x] 检验压缩前后模型的数据一致性：
    * [x] 索引信息
    * [x] 位置信息
    * [x] 法线信息
    * [x] 贴图坐标信息(第一套)
  * [x] 读取多个triangle primitive进行压缩
  * [x] 处理非triangle primitive
    * 对非triangle primitive不进行处理
  * [x] c++测试从draco::mesh还原gltf
  * [x] 输出glb流(非文件)，需要修改tinygltf
    * [x] 处理图片输出问题，图片存储情况维持原样(uri的保持原uri，data-uri的依然是data-uri)
* [ ] 对glb流的json块进行gz压缩
  * [x] 将zlib(解)压缩代码添加到工程中并运行
  * [ ] 提高zlib的压缩率(不限压缩方式)
* [ ] linux下编译测试

  * [x] draco部分测试完毕
* [ ] 前端包开发

### 现存问题

* [ ] 内存占用大(压缩9M数据，运行时内存占用30M)

  * [x] 外部图片加载占用空间
  * [ ] 未知原因

  解决方式：

  * 禁止加载外部图片文件

* [x] 压缩率低(当前为默认设置，仅80%)

  解决方式：

  * 根据primitive各个attribute的min和max值，设置量化的范围
  * 当前压缩率位20%

* [x] 压缩速度慢(当前为默认设置)

  解决方式：

  * 提高压缩比例之后，速度比之前快

* [ ] 读取含中文路径文件时，无法找到bin文件
    * 利用标准库，先将gltf的utf-8数据变成Unicode；再将unicode 变成 ansi迎合windows的api
    * (事实上windows api支持unicode，但是tinygltf使用了ansi，所以只能再转成ansi)
    * [ ] 测试linux上中文情况
