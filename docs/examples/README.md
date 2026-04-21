# Examples

本目录包含 Language Manager 的示例文件，帮助开发者理解和使用各种系统功能。

## 文件说明

### package-example.json

包的本地化文件示例，展示了：

- 包级别的本地化（vendor、copyright、description）
- 模块级别的本地化（name、description）
- ChainG2p 步骤描述的本地化（stepDescriptions）

**适用场景**：
- 包创建者创建 package.json
- 理解 ChainG2p 的特殊本地化处理

**位置**：`res/G2pPackages/{PackageName}/package.json`

### ChainG2pMacros.h.example

ChainG2p 专用宏的定义示例，展示了：

- ChainG2p Schema 宏定义
- Group 宏定义（用于包装每个 step）
- ChainG2p 特殊 Section 宏
- 常用步骤的简化宏（clean、tag_validate、dict、model、fallback）

**适用场景**：
- 理解 ChainG2p 的特殊处理
- 学习如何声明 ChainG2p 步骤
- 参考实现自定义步骤宏

## 更多信息

详细的设计文档请参阅：
- [PRD-v2.0.md](../PRD-v2.0.md) — 产品需求文档
- [Plugin-Development-Guide.md](../Plugin-Development-Guide.md) — 插件开发指南
- [ChainG2p-Design-Document.md](../ChainG2p-Design-Document.md) — ChainG2p 设计文档
