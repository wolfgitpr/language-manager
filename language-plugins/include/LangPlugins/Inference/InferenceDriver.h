#ifndef DSINFER_INFERENCEDRIVER_H
#define DSINFER_INFERENCEDRIVER_H

#include <LangMgr/Support/JSON.h>
#include <LangMgr/Support/Expected.h>
#include <LangMgr/Core/NamedObject.h>

namespace LangPlugins {

    class InferenceDriverInitArgs : public LangMgr::NamedObject {
    public:
        inline InferenceDriverInitArgs(std::string name, int version)
            : LangMgr::NamedObject(std::move(name)), version(version) {
        }

        int version;
    };

    class InferenceSession;

    /// InferenceDriver - DiffSinger inference driver interface.
    ///
    /// \note An instance of \c InferenceDriver needs to be added to the \c InferenceCategory with
    /// the ID "g2pOnnxDriver" before it can be called by the inference interpreters.
    ///
    /// It is used like the following.
    /// \code
    ///     void init(LangMgr::SynthUnit &su, InferenceDriver *driver) {
    ///         ContribCategory &ic = *su.category("inference");
    ///         ic.addObject("g2pOnnxDriver", driver);
    ///     }
    /// \endcode
    class InferenceDriver : public LangMgr::NamedObject {
    public:
        virtual ~InferenceDriver() = default;

        /// Related singer arch.
        virtual std::string arch() const = 0;

        /// Driver backend identifier.
        virtual std::string backend() const = 0;

        virtual LangMgr::Expected<void> initialize(const LangMgr::NO<InferenceDriverInitArgs> &args) = 0;

        virtual LangMgr::NO<InferenceSession> createSession() = 0;
    };

}

#endif // DSINFER_INFERENCEDRIVER_H