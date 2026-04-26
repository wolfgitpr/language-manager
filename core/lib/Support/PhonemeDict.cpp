#include <LangCore/Support/PhonemeDict.h>

#include <cstring>
#include <fstream>

#include <sparsepp/spp.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

namespace LangCore
{

    static std::error_code make_last_error() {
#ifdef _WIN32
        return std::error_code(errno, stdc::windows_utf8_category());
#else
        return std::error_code(errno, std::system_category());
#endif
    }

    struct const_char_hash {
        size_t operator()(const char *key) const noexcept {
            return spp::spp_hash<std::string_view>()(std::string_view(key, std::strlen(key)));
        }
    };

    struct const_char_equal {
        bool operator()(const char *key1, const char *key2) const noexcept { return std::strcmp(key1, key2) == 0; }
    };

    class PhonemeDict::Impl {
    public:
        struct Entry {
            uint32_t offset;
            uint32_t count;
        };
        using MapType = spp::sparse_hash_map<char *, Entry, const_char_hash, const_char_equal>;
        using SppIterator = MapType::const_iterator;

        std::vector<char> filebuf;
        MapType map;

        // Store/load a sparsepp const_iterator to/from the two void* slots (_row, _col).
        // Uses memcpy to safely round-trip without accessing sparsepp internal member names.
        static_assert(sizeof(SppIterator) <= 2 * sizeof(void *),
                      "sparsepp iterator size exceeds two void* slots");

        static SppIterator loadIter(const void *row, const void *col) {
            SppIterator it{};
            const void *buf[2] = {row, col};
            std::memcpy(&it, buf, sizeof(it));
            return it;
        }

        static void storeIter(const SppIterator &it, const void *&row, const void *&col) {
            const void *buf[2] = {};
            std::memcpy(buf, &it, sizeof(it));
            row = buf[0];
            col = buf[1];
        }
    };

    PhonemeDict::PhonemeDict() : _impl(std::make_shared<Impl>()) {}

    PhonemeDict::~PhonemeDict() = default;

    bool PhonemeDict::load(const std::filesystem::path &path, std::error_code *ec) {
        if (ec)
            ec->clear();

        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            if (ec)
                *ec = make_last_error();
            return false;
        }

        __stdc_impl_t;
        auto &filebuf = impl.filebuf;
        auto &map = impl.map;

        file.seekg(0, std::ios::end);
        const std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        filebuf.resize(file_size + 1); // +1 for terminator
        if (!file.read(filebuf.data(), file_size)) {
            if (ec)
                *ec = std::error_code(errno, std::system_category());
            filebuf.clear();
            return false;
        }
        filebuf[file_size] = '\n'; // add terminating line break
        map.clear();

        // Parse the buffer
        const auto buffer_begin = filebuf.data();
        const auto buffer_end = buffer_begin + filebuf.size();

        // Estimate line numbers if the file is too large
        static constexpr size_t large_file_size = 1 * 1024 * 1024;
        if (file_size > large_file_size) {
            const size_t line_cnt = std::count(buffer_begin, buffer_end, '\n') + 1;
            map.reserve(line_cnt);
        }

        // Traverse lines
        {
            auto start = buffer_begin;
            while (start < buffer_end) {
                while (start < buffer_end && (*start == '\r' || *start == '\n')) {
                    *start = '\0';
                    start++;
                }

                const char *value_start = nullptr;
                uint32_t value_cnt = 0;

                // Find tab
                auto p = start + 1;
                while (p < buffer_end) {
                    switch (*p) {
                    case '\t':
                        value_start = p + 1;
                        *p = '\0';
                        goto out_tab_find;

                    case '\r':
                    case '\n':
                        start = p + 1;
                        goto out_next_line;

                    default:
                        break;
                    }
                    ++p;
                }

                // Tab not found
                while (start < buffer_end && *start != '\r' && *start != '\n') {
                    *start = '\0';
                    start++;
                }
                goto out_next_line;

            out_tab_find:
                // Find space or line break
                while (p < buffer_end) {
                    switch (*p) {
                    case ' ':
                        value_cnt++;
                        *p = '\0';
                        break;

                    case '\r':
                    case '\n':
                        value_cnt++;
                        *p = '\0';
                        goto out_success;

                    default:
                        break;
                    }
                    ++p;
                }

            out_success:
                {
                    map[start] = Impl::Entry{static_cast<uint32_t>(value_start - buffer_begin), value_cnt};
                    start = p + 1;
                }
            out_next_line:;
            }
        }
        return true;
    }

    // ==================== Iterator ====================
    // Uses memcpy-based round-tripping of sparsepp iterators through the opaque
    // void* slots. This avoids direct access to sparsepp internal member names.

    void PhonemeDict::iterator::fetch() const {
        if (_copy) {
            return;
        }
        auto it = Impl::loadIter(_row, _col);
        const char *key = it->first;
        PhonemeList value(_buf + it->second.offset, it->second.count);
        _copy = std::make_pair(key, value);
    }

    void PhonemeDict::iterator::next() {
        auto it = Impl::loadIter(_row, _col);
        ++it;
        Impl::storeIter(it, _row, _col);
        _copy.reset();
    }

    void PhonemeDict::iterator::prev() {
        auto it = Impl::loadIter(_row, _col);
        --it;
        Impl::storeIter(it, _row, _col);
        _copy.reset();
    }

    bool PhonemeDict::iterator::equals(const iterator &RHS) const {
        auto lhs = Impl::loadIter(_row, _col);
        auto rhs = Impl::loadIter(RHS._row, RHS._col);
        return lhs == rhs;
    }

    PhonemeDict::iterator PhonemeDict::find(const char *key) const {
        __stdc_impl_t;
        auto &map = impl.map;
        if (!key) {
            return end();
        }
        const auto it = map.find(const_cast<char *>(key));
        if (it == map.end()) {
            return end();
        }
        const void *row = nullptr;
        const void *col = nullptr;
        Impl::storeIter(it, row, col);
        return iterator(impl.filebuf.data(), row, col);
    }

    bool PhonemeDict::contains(const char *key) const {
        __stdc_impl_t;
        auto &map = impl.map;
        if (!key) {
            return false;
        }
        return map.find(const_cast<char *>(key)) != map.end();
    }

    PhonemeList PhonemeDict::operator[](const char *key) const {
        __stdc_impl_t;
        auto &map = impl.map;
        if (!key) {
            return PhonemeList();
        }
        const auto it = map.find(const_cast<char *>(key));
        if (it == map.end()) {
            return PhonemeList();
        }
        return PhonemeList(impl.filebuf.data() + it->second.offset, it->second.count);
    }

    bool PhonemeDict::empty() const {
        __stdc_impl_t;
        return impl.map.empty();
    }

    size_t PhonemeDict::size() const {
        __stdc_impl_t;
        return impl.map.size();
    }

    PhonemeDict::iterator PhonemeDict::begin() const {
        __stdc_impl_t;
        const auto it = impl.map.begin();
        const void *row = nullptr;
        const void *col = nullptr;
        Impl::storeIter(it, row, col);
        return iterator(impl.filebuf.data(), row, col);
    }

    PhonemeDict::iterator PhonemeDict::end() const {
        __stdc_impl_t;
        const auto it = impl.map.end();
        const void *row = nullptr;
        const void *col = nullptr;
        Impl::storeIter(it, row, col);
        return iterator(impl.filebuf.data(), row, col);
    }

} // namespace LangCore
