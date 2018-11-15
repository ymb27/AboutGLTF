# Work Note

---

## 当前工作要求

后台需要对数据进行压缩，获得的二进制数据传输到页面，利用js进行解压

![后端工作流程](G:/AboutGLTF/%E5%90%8E%E7%AB%AF%E5%B7%A5%E4%BD%9C%E6%B5%81%E7%A8%8B.jpg)

## 技术步骤

------

### 1. 解析gltf生成draco的mesh对象（11.15 ~ ）

- 解析gltf，提取模型bin数据
- 生成draco的mesh对象