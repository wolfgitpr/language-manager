# Language Manager 文档索引

> 版本：v3.0 | 更新日期：2026-07-01

## 核心文档

- [PRD-v2.0.md](PRD-v2.0.md) — 产品需求文档（核心概念、架构、API、错误处理、配置、依赖系统、测试、命名规范、设计评审记录）
- [Architecture-Overview.md](Architecture-Overview.md) — 架构概览（分层结构、继承层次、目录结构、初始化流程、依赖解析）
- [Conventions-and-Standards.md](Conventions-and-Standards.md) — 编码规范与标准（代码格式、命名、错误处理、日志、配置、插件导出、设计原则）

## 设计准则与决策

- [decisions/human-decisions.md](decisions/human-decisions.md) — 人工决策记录（ARCH/ROBUST/INFRA/PACK 系列设计准则，框架演进的权威依据）

## 重构方案

- [refactor-plan/](refactor-plan/README.md) — 重构方案总览（基于 v3.1，对齐 synthrt/dspk-g2p-design 优化建议 O-1~O-5）— 状态：设计稿，待评审
  - [01-current-state-audit.md](refactor-plan/01-current-state-audit.md) — 现状审计（框架能力、VULN-1 漏洞、架构债、文档不一致）
  - [02-interface-design.md](refactor-plan/02-interface-design.md) — 接口与修复设计（稳定公共面、ModelStep FQID 修复、可观测性 API）
  - [03-host-integration-contract.md](refactor-plan/03-host-integration-contract.md) — 宿主集成契约（框架与宿主边界、ONNX driver 边界、三层校验、启动期加载边界）
  - [04-implementation-tasks.md](refactor-plan/04-implementation-tasks.md) — 实施任务清单（含验证步骤、执行顺序、风险控制）
  - [05-design-principles-check.md](refactor-plan/05-design-principles-check.md) — 设计原则核对（与 human-decisions.md 17 条原则逐项核对）

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
