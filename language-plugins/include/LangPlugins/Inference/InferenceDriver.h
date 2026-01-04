#ifndef DSINFER_INFERENCEDRIVER_H
#define DSINFER_INFERENCEDRIVER_H

#include <LangMgr/Core/NamedObject.h>
#include <LangMgr/Support/Expected.h>

namespace LangPlugins
{

    class InferenceDriverInitArgs : public LangMgr::NamedObject {
    public:
        InferenceDriverInitArgs(std::string name, const int version) : NamedObject(std::move(name)), version(version) {}
        int version;
    };

    class InferenceSession;

    /// InferenceDriver - G2p inference driver interface.
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
        ~InferenceDriver() override;

        /// Related g2p arch.
        virtual std::string arch() const = 0;

        /// Driver backend identifier.
        virtual std::string backend() const = 0;

        virtual LangMgr::Expected<void> initialize(const LangMgr::NO<InferenceDriverInitArgs> &args) = 0;

        virtual LangMgr::NO<InferenceSession> createSession() = 0;
    };
    inline InferenceDriver::~InferenceDriver() = default;

} // namespace LangPlugins

#endif // DSINFER_INFERENCEDRIVER_H
