# Language Manager 文档索引

> 版本：v3.0 | 更新日期：2026-07-01

## 核心文档

- [PRD-v2.0.md](PRD-v2.0.md) — 产品需求文档（核心概念、架构、API、错误处理、配置、依赖系统、测试、命名规范、设计评审记录）
- [Architecture-Overview.md](Architecture-Overview.md) — 架构概览（分层结构、继承层次、目录结构、初始化流程、依赖解析）
- [Conventions-and-Standards.md](Conventions-and-Standards.md) — 编码规范与标准（代码格式、命名、错误处理、日志、配置、插件导出、设计原则）

## 设计准则与决策

- [decisions/human-decisions.md](decisions/human-decisions.md) — 人工决策记录（ARCH/ROBUST/INFRA/PACK 系列设计准则，框架演进的权威依据）

## 宿主集成

- [host-integration/](host-integration/README.md) — LangCore 宿主集成指南（面向 ds-editor-lite / synthrt 等宿主工程接入）
  - [01-framework-capabilities.md](host-integration/01-framework-capabilities.md) — 框架能力审计（Manager API、PackageManager、ContextState 状态机、G2pRes、插件系统、幂等守卫）
  - [02-context-isolation-mechanism.md](host-integration/02-context-isolation-mechanism.md) — 上下文隔离机制（三维路由模型、默认/私有上下文、S5 漏洞修复、跨上下文依赖、命名校验）
  - [03-host-integration-contract.md](host-integration/03-host-integration-contract.md) — 宿主集成契约（必须做 / 禁止做、加载约束 L-1~L-4、初始化顺序、ONNX 注册模式、错误处理）
  - [04-test-design.md](host-integration/04-test-design.md) — 测试设计（双层测试策略、三大测试领域、L1 已完成实现计划）

## 开发指南

- [Plugin-Development-Guide.md](Plugin-Development-Guide.md) — 插件开发指南（环境、项目结构、最小示例、多版本支持、错误处理、打包）
- [ChainG2p-Design-Document.md](ChainG2p-Design-Document.md) — ChainG2p 设计文档（责任链G2p框架、Pipeline、5种Step、配置格式）

## 设计文档

- [design/VoiceBank-Scoped-Package-Design.md](design/VoiceBank-Scoped-Package-Design.md) — Voice Bank Scoped Package 设计（Context 隔离、FQID、去重、API设计）— 状态：v3.1 确认版（v4.0 简化方案已撤销）

## 参考资料

- [Module-Reference.md](Module-Reference.md) — 模块参考手册（核心库头文件、插件、资源包、测试目录）
- [Issues-Tracker.md](Issues-Tracker.md) — 问题追踪（已修复问题、待修复Bug、性能优化建议、未完成功能、长期目标）
- [reference/examples/](reference/examples/) — 示例文件（ChainG2p宏示例、package.json示例）
- [reference/iso-639-3.tab](reference/iso-639-3.tab) — ISO 639-3 语言代码参考表
- [reference/image/](reference/image/) — 文档配图（g2p.png 调用流程图）

## 测试

- [Test-Design-Document.md](Test-Design-Document.md) — 测试设计文档（四层测试体系、catch2/ 单一目标 LangMgrTests、tst_langCore/ 集成测试）
