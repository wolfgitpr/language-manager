# language-manager

## Intro

The G2p module is used for text to speech conversion, and its subordinate LanguageAnalyzer recognizes the language of
the
current G2p.

Provide support for the [dsinfer](https://github.com/diffscope/dsinfer/blob/main/docs/ds-spec-2.3.md) format sound
library for [ds-editor-lite](https://github.com/flutydeer/ds-editor-lite).

## Optional G2P

|    G2p Id    |        Details        | Language Id      |
|:------------:|:---------------------:|------------------|
|  cmn-pinyin  |    Mandarin Pinyin    | cmn cmn-pinyin   |
| yue-jyutping |       Jyutping        | yue yue-jyutping |
|  jpn-romaji  | Japanese Romanization | kana jpn-romaji  |
|   eng-cmu    |        English        | eng              |
|   unknown    |        Unknown        | unknown          |

## Call Flow

![Call Flow](./docs/image/g2p.png)

## How To Use

```c++
#include <LangCore/Runtime/Manager.h>

const auto langMgr = LangCore::Manager::instance();  // 获取单例

// 初始化 Manager
std::string errorMessage;
if (!langMgr->initialize(errorMessage)) {
    std::cerr << "Failed to initialize: " << errorMessage << std::endl;
    return -1;
}

// 文本分割
std::string text = "xxx好的123";
auto segments = langMgr->split(text);

// 语言标记
auto tags = langMgr->tag(segments, false, false);

// G2p 转换
std::vector<LangCore::G2pInput *> g2pInput;
for (const auto &tag : tags) {
    g2pInput.emplace_back(new LangCore::G2pInput(tag.lyric, tag.language));
}
auto results = langMgr->convert(g2pInput);

// 清理
for (auto *input : g2pInput) {
    delete input;
}
```

## Dependencies

Temporarily using the vcpkg environment of [ds-editor-lite](https://github.com/flutydeer/ds-editor-lite).

[cpp-pinyin](https://github.com/wolfgitpr/cpp-pinyin)

[cpp-kana](https://github.com/wolfgitpr/cpp-kana)

```bash
-DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/cmake/buildsystems/vcpkg.cmake
-DCMAKE_PREFIX_PATH=path/to/qmsetup;path/to/qt/6.10.2/msvc2022_64;
-DCMAKE_INSTALL_PREFIX=install
```

## Add New G2p

Referring to the 3-digit code in [iso-639-3.tab](./docs/iso-639-3.tab), add a suffix of '-' to the commonly used
phonetic notation system, such as "eng-cmu", "cmn-pinyin", "jpn-romaji".

Name the new G2p according to the standard and add it to the above table.

Refer to [PRD](./docs/PRD-v1.0.md) and [API Usage Guide](./docs/API-Usage-Guide.md) for development details.