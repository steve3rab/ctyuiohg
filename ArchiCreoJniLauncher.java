package com.thales.hwb.Archi.launcher;

import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.scene.layout.BorderPane;
import javafx.stage.Modality;
import javafx.stage.Stage;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * JNI bridge expected by ArchiJavaFxRuntime_v2.
 *
 * Keep all real Stage/Scene construction inside createContent(). The important
 * integration contract is the lifecycle and threading code around it.
 */
public final class ArchiCreoJniLauncher {
    private static final Object LOCK = new Object();
    private static final Map<Long, Stage> OPEN_STAGES = new HashMap<>();
    private static final AtomicBoolean INITIALIZED = new AtomicBoolean(false);
    private static final AtomicBoolean SHUTTING_DOWN = new AtomicBoolean(false);

    private ArchiCreoJniLauncher() {
    }

    /** Called exactly once by the native JVM owner thread. */
    public static void initialize() throws Exception {
        if (INITIALIZED.get()) {
            return;
        }

        SHUTTING_DOWN.set(false);
        Platform.setImplicitExit(false);

        final CountDownLatch ready = new CountDownLatch(1);
        final AtomicReference<Throwable> startupError = new AtomicReference<>();

        try {
            Platform.startup(() -> {
                try {
                    // Do not create any Stage here. This callback only confirms
                    // that the JavaFX Application Thread is alive.
                } catch (Throwable t) {
                    startupError.set(t);
                } finally {
                    ready.countDown();
                }
            });
        } catch (IllegalStateException alreadyStarted) {
            // A JVM component may already have started JavaFX. We do not start it
            // a second time; wait for a normal runLater callback instead.
            try {
                Platform.runLater(ready::countDown);
            } catch (Throwable t) {
                startupError.set(t);
                ready.countDown();
            }
        }

        if (!ready.await(15, TimeUnit.SECONDS)) {
            throw new IllegalStateException("JavaFX startup timeout");
        }
        final Throwable error = startupError.get();
        if (error != null) {
            throw new IllegalStateException("JavaFX startup failed", error);
        }
        INITIALIZED.set(true);
    }

    /**
     * Opens either a modeless or modal window asynchronously.
     * mode 0 = Modality.NONE
     * mode 1 = Modality.APPLICATION_MODAL
     */
    public static void openWindow(String title, String[] args, int mode, long requestId) {
        if (!INITIALIZED.get() || SHUTTING_DOWN.get()) {
            nativeWindowFailed(requestId, "JavaFX runtime is not accepting requests");
            return;
        }

        try {
            Platform.runLater(() -> createAndShowWindow(title, args, mode, requestId));
        } catch (Throwable t) {
            nativeWindowFailed(requestId, throwableMessage(t));
        }
    }

    private static void createAndShowWindow(String title, String[] args, int mode, long requestId) {
        if (SHUTTING_DOWN.get()) {
            nativeWindowFailed(requestId, "JavaFX runtime is shutting down");
            return;
        }

        Stage stage = null;
        final AtomicBoolean callbackSent = new AtomicBoolean(false);
        try {
            stage = new Stage();
            final boolean modal = mode == 1;
            stage.initModality(modal ? Modality.APPLICATION_MODAL : Modality.NONE);
            stage.setTitle(title == null ? "" : title);

            // Existing application-specific UI should be constructed here.
            final Scene scene = new Scene(createContent(title, args), 900, 650);
            stage.setScene(scene);

            final Stage registeredStage = stage;
            synchronized (LOCK) {
                OPEN_STAGES.put(requestId, registeredStage);
            }

            stage.setOnHidden(event -> {
                synchronized (LOCK) {
                    OPEN_STAGES.remove(requestId);
                }
                // Native side uses requestId to release the native Creo modal block.
                if (callbackSent.compareAndSet(false, true)) {
                    nativeWindowClosed(requestId);
                }
            });

            // show(), not showAndWait(): the JavaFX Application Thread remains the
            // normal event-processing thread, including for modal stages.
            stage.show();
            stage.toFront();
        } catch (Throwable t) {
            if (stage != null) {
                try {
                    stage.close();
                } catch (Throwable ignored) {
                    // best effort
                }
            }
            synchronized (LOCK) {
                OPEN_STAGES.remove(requestId);
            }
            if (callbackSent.compareAndSet(false, true)) {
                nativeWindowFailed(requestId, throwableMessage(t));
            }
        }
    }

    /**
     * Completes a window with an application result. status 0 = accepted,
     * 1 = cancelled. The native side correlates the result with requestId.
     */
    public static void finishWindow(long requestId, int status, String[] values) {
        final Stage stage;
        synchronized (LOCK) {
            stage = OPEN_STAGES.get(requestId);
        }
        nativeWindowResult(requestId, status, values == null ? new String[0] : values);
        if (stage != null) {
            try {
                stage.close();
            } catch (Throwable ignored) {
                // The result has already been delivered to native code.
            }
        }
    }

    /**
     * Shutdown is intentionally asynchronous from the JavaFX thread. DestroyJavaVM
     * is subsequently called by the native JVM owner thread and therefore waits for
     * the JavaFX application thread to terminate cleanly.
     */
    public static void shutdown() {
        if (!INITIALIZED.get()) {
            return;
        }

        if (!SHUTTING_DOWN.compareAndSet(false, true)) {
            return;
        }

        try {
            Platform.runLater(() -> {
                final ArrayList<Stage> stages;
                synchronized (LOCK) {
                    stages = new ArrayList<>(OPEN_STAGES.values());
                }

                for (Stage stage : stages) {
                    try {
                        stage.close();
                    } catch (Throwable ignored) {
                        // A close failure must not prevent Platform.exit().
                    }
                }

                synchronized (LOCK) {
                    OPEN_STAGES.clear();
                }
                Platform.exit();
            });
        } catch (Throwable t) {
            // Let DestroyJavaVM finish the remaining JVM lifecycle. Native side
            // still clears its modal host guard after the worker exits.
        }
    }

    private static BorderPane createContent(String title, String[] args) {
        final BorderPane root = new BorderPane();
        root.setPadding(new Insets(16));
        root.setCenter(new Label(title == null ? "JavaFX window" : title));
        // Replace this method with the existing application-specific UI factory.
        return root;
    }

    private static String throwableMessage(Throwable t) {
        final String message = t.getMessage();
        return t.getClass().getName() + (message == null || message.isEmpty() ? "" : ": " + message);
    }

    // These methods are registered by C++ using RegisterNatives(). No JNI symbol
    // export from the Creo plugin DLL is required.
    private static native void nativeWindowClosed(long requestId);
    private static native void nativeWindowResult(long requestId, int status, String[] values);
    private static native void nativeWindowFailed(long requestId, String message);
}
