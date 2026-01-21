# Language Manager 数据格式与推理接口规范 1.0

> Language Manager Data Format and Inference Interface Specification 1.0

LangMgr 是一个模块化的语言处理插件系统，支持文本分割、语言标记、语音转换等任务。系统采用插件化设计，支持运行时动态加载和依赖管理。

## 1. 核心层级与组件

1. 启动流程

   void PluginFactory->addPluginPath(const char *iid, const std::filesystem::path &path);

   FactoryPlugin PluginFactory->plugin(const char *iid, const char *key);

   Package PackageManager->open(pacakge);

   TaskFactory FactoryPlugin->create();

   Task TaskFactory->create(Package.ModuleSpec);

   TaskResult Task->start(TaskStartInput);

2. 插件 (Plugin) -> 任务/会话工厂插件 (Task/Session FactoryPlugin)

   1.1 Plugin 系统扩展的基础单元，支持动态加载，通过唯一标识符(iid)和键值(key)注册功能组件，分为任务工厂插件和会话工厂插件两种类型。

   1.2 TaskFactoryPlugin 任务的工厂类，负责根据模块规范创建任务实例。

   1.3 SessionFactoryPlugin 用于创建AI模型推理驱动，如onnxDriver，支持不同硬件后端（CPU、CUDA、DML等）。

3. 插件工厂 (PluginFactory) -> 包管理器 (PackageManager) -> 管理器 (Manager)

   2.1 PluginFactory 插件管理器，负责插件的发现、加载和生命周期管理，维护插件路径和实例缓存。

   2.2 PackageManager 包管理系统，负责包的发现、加载、依赖解析和初始化顺序计算，维护包路径和模块元数据。

   2.3 Manager 提供高层API（如文本分割、标注、G2P转换），协调插件、包、任务的完整工作流程。

4. 包 (Package)

   模块的容器，包含多个模块及其相关资源文件。支持版本管理和依赖声明，通过JSON清单文件定义包的结构。

   模块规范 (ModuleSpec) 模块的元数据和配置描述，包含模块的标识、类别、API级别、配置信息等，是创建任务实例的蓝图。

5. 任务 (Task)
   实际执行处理逻辑的单元基类，定义任务的生命周期（初始化、启动、停止）。SessionTask为特殊类型，作为AI模型驱动的基类。

## 2. Package

### 文件结构

本规范内，可分发的插件包的最小单位是 Package ，是一个以`lmpk`为扩展名的 UTF-8 编码 ZIP 格式的压缩包。

压缩包内基本结构为：
```
+ xxx.lmpk
  - package.json
  - ...
```

Package 内多使用`json`作为声明文件，我们规定，声明文件中使用的相对路径的基路径是这个声明文件的在所目录。

#### 描述文件

`Package.json`是 Package 的描述文件，主要包括以下内容。

```json
{
  "packageId": "eng-official",
  "version": "1.0.0",
  "vendor": "",
  "copyright": "Copyright (C) 小狼",
  "description": "",
  "url": "",
  "modules": {
    "tagger": [
      {
        "moduleId": "tagger-eng",
        "class": "tagger.template.RegexTaggerInference",
        "configuration": "modules/Tagger-Eng/config.json",
        "dependencies": []
      }
    ],
    "g2p": [
      {
        "moduleId": "g2p-lstm-eng",
        "class": "g2p.model.LstmG2pInference",
        "configuration": "modules/lstm-g2p-en/config.json"
      },
      {
        "moduleId": "g2p-eng",
        "class": "g2p.template.TemplateG2pInference",
        "configuration": "modules/template-eng/config.json",
        "dependencies": [
          {
            "packageId": "eng-official",
            "moduleId": "g2p-lstm-eng",
            "level": 1,
            "version": "*"
          }
        ]
      }
    ]
  }
}
```
+ 必选字段
    + `packageId`：唯一标识符，不准出现以下字符`/\[]:;'"`
    + `version`：版本号，格式为`x.y.z`
+ 可选字段
    + `vender`：提供者
    + `copyright`：版权信息，可为多语言
    + `description`：介绍文字，可为多语言
    + `readme`：放置介绍、许可证等信息的文本
    + `url`：网站
    + `contributes`：功能贡献列表，主要包含子模块
        + `tagger`：推理模块
            + `id`：推理模块 ID
            + `class`：推理类型
            + `configuration`：配置文件
            + `dependencies`：[
              {
                + `packageId`：依赖库 ID
                + `moduleId`：依赖模块 ID
                + `version`：依赖库版本
                + `level`：依赖库ApiLevel
               }
            ]

        + `g2p`：歌手模块
            + `id`：歌手 ID
            + `arch`：歌手架构
            + `path`：歌手信息文件
            + `dependencies`：[]
