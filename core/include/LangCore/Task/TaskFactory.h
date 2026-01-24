#ifndef LANGCORE_ENGINEFACTORY_H
#define LANGCORE_ENGINEFACTORY_H

#include <LangCore/Module/Module.h>
#include <LangCore/Support/Expected.h>

namespace LangCore
{
    class TaskFactory : public NamedObject {
    public:
        /// The highest inference API version currently supported by this interpreter.
        virtual int apiLevel() const = 0;

        /// Called when \c InferenceDefinition loads.
        virtual Expected<NO<TaskConfiguration>> createConfiguration(const ModuleSpec *spec) const = 0;

        /// Called when it's about to execute an inference.
        virtual Expected<NO<Task>> createTask(const ModuleSpec *spec, const NO<TaskRuntimeOptions> &runtimeOptions) = 0;
    };

    /// SessionFactory - G2p inference driver interface.
    ///
    /// \note An instance of \c SessionFactory needs to be added to the \c G2pCategory with
    /// the ID "g2pOnnxDriver" before it can be called by the inference interpreters.
    ///
    /// It is used like the following.
    /// \code
    ///     void init(SynthUnit &su, SessionFactory *driver) {
    ///         ContribCategory &ic = *su.category("inference");
    ///         ic.addObject("g2pOnnxDriver", driver);
    ///     }
    /// \endcode
    class SessionFactory : public NamedObject {
    public:
        ~SessionFactory() override = default;

        /// Related arch.
        virtual std::string arch() const = 0;

        /// Driver backend identifier.
        virtual std::string backend() const = 0;

        virtual Expected<void> initialize(const NO<TaskInitArgs> &args) = 0;

        virtual NO<SessionTask> createSession() = 0;
    };


} // namespace LangCore

#endif // LANGCORE_ENGINEFACTORY_H
