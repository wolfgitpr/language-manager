# Language Manager 重构方案 — 总览

> **日期**: 2026-07-01
> **状态**: 设计稿，待评审
> **设计版本**: 基于 v3.1（`ContextKey{context, version}` 双维度），v4.0 简化方案已撤销
> **参考来源**: [synthrt/dspk-g2p-design](file:///D:/projects/synthrt/docs/dspk-g2p-design/README.md)（跨模块设计 + 框架优化建议）+ 本项目实际代码审计
> **取代**: 原 `docs/lang-framework-plan/` 与 `docs/refactoring/`（内容已整合到本目录）

---

## 1. 背景

Language Manager（LangCore）为宿主（ds-editor-lite / synthrt）提供 G2P（字素到音素）转换能力，核心是 **Context 隔离路由**：每个声库的自定义 G2P 包注册在独立 context，按 `(context, version, g2pId)` 路由，实现多声库 G2P 隔离调用。

当前框架（v3.1）核心机制稳定可用，但存在以下问题：

1. **关键漏洞**：ChainG2p 的 `ModelStep` 用裸 id 查找 task（[ModelStep.cpp:52](file:///D:/projects/language-manager/plugins/G2ps/ChainG2p/internal/Steps/ModelStep.cpp)），不感知当前 context，导致"声库私有 ChainG2p + 声库私有 LstmG2p"组合失败（S5 场景）。
2. **架构债**：三层 public 继承（`PluginFactory→PackageManager→Manager`）暴露 deprecated 的扁平化方法（`checkDependencies` 等），宿主易误用。
3. **欠设计**：`contextStates` 私有，宿主无法主动发现失败 context；`convert()` 失败语义用 `mode="copy"` 表达，需检查 `errorType` 才能区分。
4. **跨模块契约不显式**：ONNX driver 注册时序、启动期加载边界、三层依赖校验等约束散落在宿主侧实现中，框架侧未显式声明。
5. **文档与代码不一致**：`PackageManager::open()` 传递依赖未实现但公共 API 仍暴露。

技术债清理（12 项 P1/P2/P3）与架构改进（AI-01/02/03）均已完成，详见 git 提交历史 `f17ac89..c810e82`。

本方案与 [synthrt/dspk-g2p-design](file:///D:/projects/synthrt/docs/dspk-g2p-design/README.md) 配套：synthrt 侧定义 dspk 格式与跨模块初始化设计，本方案定义 language-manager 框架侧的优化与修复。

---

## 2. 设计目标

| 目标 | 说明 |
|------|------|
| **接口稳定** | 保持 v3.1 公共 API 签名稳定（[ARCH-02](../decisions/human-decisions.md)）；新增为 additive，破坏性变更须递增 Level |
| **不过度设计** | 不引入 `ILangCore` facade、不重构三层继承为组合（风险高且宿主不感知）；只做必要的漏洞修复与可观测性增强 |
| **抛弃技术债** | 当前 Level（v3.x）对 deprecated 扁平化方法追加 `[[deprecated]]` 警告，私有化推迟到 v4.x 并递增 Level；清理 goto 控制流；修复成员遮蔽；统一 include 守卫 |
| **隔离可靠** | 修复 ModelStep context 漏洞，确保 S5 场景（声库私有 ChainG2p + LstmG2p）正常工作 |
| **可观测性** | 新增失败 context 查询 API，让宿主能主动发现初始化失败的声库 context |
| **跨模块契约明确** | 显式声明 ONNX driver 边界、启动期加载窗口、三层依赖校验、稳定 API 白名单 |

---

## 3. 文档结构

| 文档 | 内容 | 来源 |
|------|------|------|
| [README.md](README.md) | 总览、目标、原则、文档结构、与 synthrt 参考的关系 | 新写 |
| [01-current-state-audit.md](01-current-state-audit.md) | 现状审计：框架实际能力、关键漏洞、架构债、文档不一致 | 整合自 `lang-framework-plan/01` |
| [02-interface-design.md](02-interface-design.md) | 接口与修复设计：稳定公共面、ModuleSpec::contextKey、ModelStep FQID 修复、可观测性 API、G2pRes::isOk | 整合自 `lang-framework-plan/02` |
| [03-host-integration-contract.md](03-host-integration-contract.md) | 宿主集成契约（参考 synthrt 01/05）：框架与宿主边界、稳定 API 白名单、ONNX driver 边界、三层校验、启动期加载边界 | 新写，对标 synthrt `05-cross-module-initialization-design` |
| [04-implementation-tasks.md](04-implementation-tasks.md) | 实施任务清单（含验证步骤）、执行顺序、风险控制 | 整合自 `lang-framework-plan/03` |
| [05-design-principles-check.md](05-design-principles-check.md) | 设计原则核对：与 [human-decisions.md](../decisions/human-decisions.md) ARCH/ROBUST/INFRA/PACK 系列逐项核对 | 新写 |

---

## 4. 与 synthrt 参考方案的对齐

synthrt 的 [dspk-g2p-design/06-language-manager-optimization-suggestions.md](file:///D:/projects/synthrt/docs/dspk-g2p-design/06-language-manager-optimization-suggestions.md) 提出五项框架优化建议，本方案的对应关系：

| synthrt 建议 | 优先级 | 本方案对应 | 状态 |
|--------------|--------|-----------|------|
| **O-1** `failedContexts()` API | 必需 | [02 §4](02-interface-design.md) 可观测性 API | ⬜ 待实施 |
| **O-2** ModelStep FQID 两级查找 | 建议 | [02 §3](02-interface-design.md) VULN-1 修复 | ⬜ 待实施 |
| **O-3** ONNX driver 可用性校验前移 | 建议 | [03 §4](03-host-integration-contract.md) ONNX driver 边界 | ⬜ 待实施（宿主侧断言 + 框架侧 failedContexts 覆盖） |
| **O-4** `DependencyResolver` 支持 `level` 精确匹配 | 建议 | [04 任务 1.10](04-implementation-tasks.md) | ⬜ 待核实 |
| **O-5** `initialize()` 幂等性防护 | 建议 | [04 任务 1.11](04-implementation-tasks.md) | ⬜ 待实施 |

synthrt 的 [05-cross-module-initialization-design.md](file:///D:/projects/synthrt/docs/dspk-g2p-design/05-cross-module-initialization-design.md) 定义跨模块初始化依赖，本方案在 [03-host-integration-contract.md](03-host-integration-contract.md) 中从框架侧视角对齐这些契约（ONNX driver 归属、启动期加载边界、三层校验、稳定 API 白名单）。

---

## 5. 不做的事情（避免过度设计）

| 不做 | 理由 |
|------|------|
| 不引入 `ILangCore` facade 接口 | `Manager` 公共 API 已稳定且被宿主使用；facade 增加间接层无收益 |
| 不重构三层继承为组合 | 风险高（涉及 PluginFactory/PackageManager/Manager 全部重写），宿主侧不感知继承结构；通过处理 deprecated 方法（v3.x 追加 `[[deprecated]]` 警告，v4.x 私有化）即可达到目标 |
| 不实现 LangCore reload | reload 涉及 task 销毁重建、依赖图重建，复杂度过高；宿主侧重启 UX 可接受（synthrt D3 硬约束 L-1） |
| 不实现 `PackageManager::open` 传递依赖 | 宿主不依赖此功能（使用 `addPackagePath + initialize` 全流程）；实现涉及依赖图子图加载，复杂度高 |
| 不移除 version 维度（v4.0） | v4.0 已撤销；version 维度经调用方实施 + 12 测试用例验证为必需 |
| 不重构 VersionedTaskManager 多版本机制 | 仅 LstmG2p 使用 V1/V2，机制本身合理；统一为单版本反而破坏扩展性 |
| 不引入运行期 G2P 热加载 | 框架无 reload 架构；运行期新装声库提示重启（synthrt D3/L-1） |
| 不引入 `g2pContext` 显式字段 | synthrt v3 已删除；G2P 严格跟随声库，路由两级判定 |

---

## 6. 执行策略

```
阶段 1（框架侧独立）：漏洞修复 + 架构债清理
  ├─ 修复 ModelStep context 漏洞（ModuleSpec::contextKey + FQID 查找）
  ├─ 追加 deprecated 扁平化方法编译期警告（v3.x；私有化推迟到 v4.x 并递增 Level）
  ├─ 新增 contextState() / failedContexts() 可观测性 API（ContextState 四态：Pending/Ready/Failed/NotRegistered）
  ├─ 新增 G2pRes::isOk() / isFailed() 便利方法
  ├─ initialize() 幂等性防护
  └─ 清理文档不一致（PackageManager::open 说明）

阶段 2（宿主侧对接）：宿主升级框架版本
  └─ 见 synthrt/dspk-g2p-design 方案阶段 2

阶段 3（持续）：依赖解析增强
  └─ DependencyResolver 支持 level 精确匹配（待核实当前实现）

阶段 4（v4.x，未来版本）：破坏性变更
  └─ 私有化 deprecated 扁平化方法并递增 Level
```

每步单独提交、不推送，完成后更新 [04-implementation-tasks.md](04-implementation-tasks.md) 对应任务状态。

---

## 7. 验证准则

1. 现有 `tests/catch2/` 14 个测试文件全部通过（context 隔离、版本化、依赖解析等）
2. 现有 `tests/tst_langCore/` 集成测试通过
3. **新增测试**：S5 场景（声库私有 ChainG2p + 声库私有 LstmG2p）端到端 convert 成功
4. **新增测试**：`failedContexts()` 返回失败的 context
5. deprecated 方法追加 `[[deprecated]]` 警告后，宿主侧编译通过且无警告触发（确认未调用；私有化推迟到 v4.x）
6. `initialize()` 二次调用返回 `AlreadyInitialized` 错误
7. 完整构建通过

---

**关联文档**: [synthrt/dspk-g2p-design](file:///D:/projects/synthrt/docs/dspk-g2p-design/README.md) · [VoiceBank-Scoped-Package-Design.md](../design/VoiceBank-Scoped-Package-Design.md) · [decisions/human-decisions.md](../decisions/human-decisions.md) · [01-current-state-audit.md](01-current-state-audit.md) · [02-interface-design.md](02-interface-design.md) · [03-host-integration-contract.md](03-host-integration-contract.md) · [04-implementation-tasks.md](04-implementation-tasks.md) · [05-design-principles-check.md](05-design-principles-check.md)
