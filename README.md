# LangMgr

## Intro

The language analyzer is used to segment mixed language text, recognize note languages, while its subordinate G2p module
is used to convert text into syllables.

## Install

[cpp-pinyin](https://github.com/wolfgitpr/cpp-pinyin)

[cpp-kana](https://github.com/wolfgitpr/cpp-kana)

```bash
-DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
-DCMAKE_PREFIX_PATH=path/to/qt/6.6.1/msvc2019_64;
-DCMAKE_INSTALL_PREFIX=install
```

## Doc

[Language Analyzer](./docs/语言分析器.md)

[G2p Module](./docs/G2p.md)