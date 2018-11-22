# Work Note

---

## 当前工作要求

后台需要对数据进行压缩，获得的二进制数据传输到页面，利用js进行解压

![后端工作流程](G:/AboutGLTF/%E5%90%8E%E7%AB%AF%E5%B7%A5%E4%BD%9C%E6%B5%81%E7%A8%8B.jpg)

## 技术步骤

------

### 1. 解析gltf生成draco的mesh对象（11.15 ~ ）

* [x] 解析gltf，提取模型bin数据

  * 可以直接利用tinygltf读取其中所有属性的内容

* [ ] 生成draco的mesh对象

  * 当前已经可以利用draco的`triangle_soup_mesh_builder`生成mesh

  * 尚未验证使用的builder生成的内容是否正确——应该将mesh重新生成gltf后加以验证

  * 尚未考虑从gltf中加载多个模型的情况，尚未考虑压缩点，线图元的情况

  * 正在进行从draco::mesh还原gltf的过程