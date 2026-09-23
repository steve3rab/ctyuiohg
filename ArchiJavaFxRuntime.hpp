#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace jnifx {

    class ArchiJavaFxRuntime final {
      public:
        using Arguments = std::vector<std::string>;
        using RequestId = std::uint64_t;

        enum class State : std::uint8_t {
            Stopped,
            Starting,
            Ready,
            Stopping,
            Terminated,
            Failed
        };

        enum class WindowMode : std::uint8_t {
            Modeless = 0,
            Modal = 1
        };

        struct JavaFxResult final {
            enum class Status : std::uint8_t { Accepted = 0, Cancelled = 1, Failed = 2 };
            RequestId requestId = 0;
            WindowMode mode = WindowMode::Modeless;
            Status status = Status::Cancelled;
            std::vector<std::string> values;
            std::string error;
        };

        using ResultCallback = std::function<void(JavaFxResult)>;

        struct EmbeddedClass {
            std::string binaryName;
            std::vector<std::uint8_t> bytecode;
        };

        struct Config {
            std::string classPath;
            std::vector<std::string> jvmOptions;
            std::string bridgeClass;
            std::chrono::milliseconds modalBlockTimeout{2000};
            std::string jvmPath;
            std::string jreEnvironmentVariable = "HCP_JAVA_HOME";
            std::vector<EmbeddedClass> embeddedClasses;
        };

        explicit ArchiJavaFxRuntime(Config config);
        ~ArchiJavaFxRuntime();

        ArchiJavaFxRuntime(const ArchiJavaFxRuntime&) = delete;
        ArchiJavaFxRuntime& operator=(const ArchiJavaFxRuntime&) = delete;
        ArchiJavaFxRuntime(ArchiJavaFxRuntime&&) = delete;
        ArchiJavaFxRuntime& operator=(ArchiJavaFxRuntime&&) = delete;

        // Lance le JVM sur un thread dédié. Ne fait aucun JNI sur le thread appelant.
        void preloadAsync();

        // Les deux API sont non bloquantes côté appelant.
        // Retourne un RequestId non nul si la demande est acceptée, 0 sinon.
        // Le RequestId permet au code Creo d'associer la réponse JavaFX au
        // bouton/action qui a ouvert la fenêtre.
        RequestId tryOpenWindow(std::string title, Arguments arguments = {});
        RequestId tryOpenModalWindow(std::string title, Arguments arguments = {});

        State state() const noexcept;
        bool isReady() const noexcept;
        std::string lastError() const;

        // Le callback est exécuté sur le thread Java qui appelle le callback JNI
        // (normalement le JavaFX Application Thread lorsque finishWindow() est
        // appelé depuis l'UI JavaFX). Il ne doit pas appeler directement Creo TOOLKIT.
        void setResultCallback(ResultCallback callback);

        // Arrêt complet : au retour, la JVM et JavaFX sont arrêtés et jvm.dll peut
        // être déchargée en sécurité.
        void shutdown() noexcept;

      private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace jnifx
