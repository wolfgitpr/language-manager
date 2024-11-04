# G2p

## 简介

G2p可将自然语言转换为相应的音节，下属语言分析器自动捕获字符，由[ILanguageManager.h](../include/language-manager/ILanguageManager.h)调用。

优先级：clip默认语种、声库支持语种、全局默认语种。

## 创建一个G2p

### id命名规范

参考[iso-639-3.tab](./iso-639-3.tab)中的三位语言代码, 若为注音符号(pinyin等)、以"-"分隔加上注音符号名称，如 "eng", "cmn", "cmn-pinyin", "jpn-romaji"。

### 1、创建G2p

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

新建G2p类需公开继承基类[IG2pFactory.h](../include/language-manager/IG2pFactory.h)，类名建议为首字母大写的语言名称

公共方法

```c++
// LanguageAnalyzer分割后的字符串列表、转换为当前语种的注音符号
QList<LangNote> convert(const QStringList &input) const override;

// 耗时初始化操作在此进行
virtual bool initialize(QString &errMsg);

// 有个性化选项的G2p（如是否输出声调）才需要重写，写法参照 src/G2p/MandarinG2p.h
void loadG2pConfig(const QJsonObject &config) override;
QJsonObject defaultConfig() override;
```

样例

每个G2p至少有一个语言分析器，见[LanguageFactory](./LanguageAnalyzer)

```c++
class Mandarin final : public IG2pFactory {
    Q_OBJECT    // 用于本地化的tr()函数
public:
    // 继承自五种类之一，类名建议使用语言/注音体系名+G2p、驼峰命名，如"MandarinG2p"、"EnglishG2p"。
    // id命名规范见此小节首
    Mandarin::Mandarin(QObject *parent) : IG2pFactory("Mandarin", parent) {
        setAuthor(tr("Xiao Lang"));         // 设置作者、tr()用于本地化 
        setDisplayName(tr("Mandarin"));     // 设置本地化显示的G2p名称
        setDescription(tr("Using Pinyin as the phonetic notation method."));
        m_langFactory.insert("cmn", new MandarinAnalysis());        // 创建语种分析器
        m_langFactory.insert("cmn-pinyin", new PinyinAnalysis());
    }
    ~Mandarin() override;
    
    bool initialize(QString &errMsg) override;  // 耗时初始化操作
    
    // LanguageAnalyzer分割后的字符串列表、转换为当前语种的注音符号
    QList<LangNote> convert(const QStringList &input) const override;
    
    // 自定义选项才需要重写
    QJsonObject config() override;
    QWidget *configWidget(QJsonObject *config) override;
}
```

### 2、添加G2p至管理器

[ILanguageManager.h](../include/language-manager/ILanguageManager.h)

```c++
ILanguageManager::ILanguageManager(ILanguageManagerPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
    // ...
    
    // 添加新建G2p
    addG2p(new MandarinG2p());
}
```

### 3、自定义选项

若G2p需要个性化选项，需重写以下方法

```c++
virtual QJsonObject config();
virtual QWidget *configWidget(QJsonObject *config);
```

样例

```c++
QJsonObject MandarinG2p::defaultConfig() {
    QJsonObject config;
    config["tone"] = m_tone;
    config.insert("languageConfig", languageDefaultConfig()); // 基类已实现，添加此行即可
    return config;
}

void MandarinG2p::loadG2pConfig(const QJsonObject &config) {
    if (config.contains("tone")) {
        m_tone = config.value("tone").toBool();
    }
    m_config->insert("tone", m_tone);
}
```