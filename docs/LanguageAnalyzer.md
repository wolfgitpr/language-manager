# 语言分析器

## 简介

语言分析器用于分割混合语言文本、识别音符语种等，每个语种可能需要多个分析器（文本、常用注音符号）。

## 原理

分析器的输入输出是一个结构体列表，每个分析器仅处理g2pId为"unknown"
的结构体，捕获相应字符、标记语言，切分后未识别的字符串的g2pId仍标记为"unknown"。

分析器由G2pFactory调用，依次处理文本，每个分析器的输出可作为下一个分析器的输入。

## 创建一个语言分析器

### id命名规范

参考[iso-639-3.tab](./iso-639-3.tab)中的三位语言代码, 若为注音符号(pinyin等)、以"-"分隔加上注音符号名称，如 "eng", "
cmn", "cmn-pinyin", "jpn-romaji"。

若某语种G2p准确率不足，使用时需用户手动输入注音符号，建议同时开发该符号的分析器。

### 1、创建分析器

输入输出结构体

[LangCommon.h](../include/language-manager/LangCommon.h)

```c++
struct LangNote {
    QString lyric;
    QString syllable = QString();
    QString syllableRevised = QString();
    QStringList candidates = QStringList();
    QString standard = "unknown";
    QString g2pId = "unknown";
    bool revised = false;
    bool error = false;
};
```

基类： [ILanguageFactory.h](../include/language-manager/IG2pFactory.h)

派生类共有四种： [SingleCharFactory.h](../src/LangAnalyzer/BaseFactory/SingleCharFactory.h)、[MultiCharFactory.h](../src/LangAnalyzer/BaseFactory/MultiCharFactory.h)、[CharSetFactory.h](../src/LangAnalyzer/BaseFactory/CharSetFactory.h)、[DictFactory.h](../src/LangAnalyzer/BaseFactory/DictFactory.h)

根据语言特性，创建一个公开继承于以上五类之一的分析器类：

继承基类需重写以下方法，实现细节参照四个派生类及实现类。

```c++
// 分割出可识别字符，余下未识别字符贪婪匹配、标记为unknown
[[nodiscard]] QList<LangNote> split(const QString &input) const override;
// 是否为当前语种的字符
[[nodiscard]] bool contains(const QChar &c) const override;
[[nodiscard]] bool contains(const QString &input) const override;
// 生成当前语种的随机字符串（一个字/词）
[[nodiscard]] QString randString() const override;
```

中文等象形字符、每单位仅包含一个字，可继承SingleCharFactory；英文等由连续字母构成单词的语言，可继承MultiCharFactory、贪婪匹配符合标注的字符串；

单字有限字符集的语言，可继承CharSetFactory；靠固定字符集识别的语言，可继承DictFactory(DictFactory使用字典树)。

继承CharSetFactory、DictFactory，需额外重写loadDict方法。

```c++
virtual void loadDict();
```

公共方法

```c++
Q_OBJECT // 用于本地化的tr()函数
// 继承自五种类之一，类名建议使用语言/注音体系名+Analyzer、驼峰命名，如"MandarinAnalyzer"、"PinyinAnalyzer"、"EnglishAnalyzer"。
// id命名规范见此小节首
explicit XxxAnalyzer(const QString &id = "Xxx", QObject *parent = nullptr): XxxFactory(id, parent) {};
virtual bool initialize(QString &errMsg);   // 耗时的初始化操作在此进行，如加载词典

[[nodiscard]] virtual bool contains(const QChar &c) const;              // 单字类型语言使用
[[nodiscard]] virtual bool contains(const QString &input) const;        // 字母语言使用

[[nodiscard]] virtual QString randString() const;                       // 随机生成本语言的单个字符串(象形文字为一个，字母语言为一个单词)
```

样例

```c++
class SpaceAnalyzer final : public SingleCharFactory {
    Q_OBJECT    // 必须声明
public:
    explicit SpaceAnalyzer(const QString &id = "Space", QObject *parent = nullptr)
        : SingleCharFactory(id, parent) {       // Space为单字符类，继承SingleCharFactory
        setAuthor(tr("Xiao Lang"));             // 设置作者、带tr()用于本地化
        setDisplayName(tr("Space"));            // 设置本地化显示的分析器名称
        setDescription(tr("Capture spaces."));  // 简介
        setDiscardResult(true);                 // 无需默认丢弃结果的，不做设置
    }

    // Space仅为单个字符，继承SingleCharFactory，只需重写contains(const QChar &c)、randString()方法。
    [[nodiscard]] bool contains(const QChar &c) const override;
    [[nodiscard]] virtual QString randString() const; 
};
```