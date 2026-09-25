package com.thales.hwb.Archi.launcher;

import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.scene.layout.StackPane;
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
    private static final Map<Long, AtomicBoolean> CALLBACK_SENT = new HashMap<>();
    private static final Map<Long, AtomicBoolean> PROCESSING = new HashMap<>();
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
            final StackPane root = new StackPane(ArchiJavaFxViews.loading());
            final Scene scene = new Scene(root, 900, 650);
            stage.setScene(scene);

            final Stage registeredStage = stage;
            synchronized (LOCK) {
                OPEN_STAGES.put(requestId, registeredStage);
                CALLBACK_SENT.put(requestId, callbackSent);
                PROCESSING.put(requestId, new AtomicBoolean(false));
            }

            stage.setOnCloseRequest(event -> {
                final AtomicBoolean processing;
                synchronized (LOCK) {
                    processing = PROCESSING.get(requestId);
                }
                if (processing != null && processing.get()) {
                    event.consume();
                }
            });

            stage.setOnHidden(event -> {
                final AtomicBoolean sent;
                synchronized (LOCK) {
                    OPEN_STAGES.remove(requestId);
                    PROCESSING.remove(requestId);
                    sent = CALLBACK_SENT.remove(requestId);
                }
                // Native side uses requestId to release the native Creo modal block.
                if (sent != null && sent.compareAndSet(false, true)) {
                    nativeWindowClosed(requestId);
                }
            });

            // show(), not showAndWait(): the JavaFX Application Thread remains the
            // normal event-processing thread, including for modal stages.
            stage.show();
            stage.toFront();
            Platform.runLater(() -> {
                if (SHUTTING_DOWN.get()) return;
                try {
                    root.getChildren().setAll(ArchiJavaFxViews.content(title, args));
                } catch (Throwable t) {
                    root.getChildren().setAll(ArchiJavaFxViews.error(throwableMessage(t)));
                }
            });
        } catch (Throwable t) {
            if (stage != null) {
                try {
                    stage.close();
                } catch (Throwable ignored) {
                    // best effort
                }
            }
            final AtomicBoolean sent;
            synchronized (LOCK) {
                OPEN_STAGES.remove(requestId);
                sent = CALLBACK_SENT.remove(requestId);
            }
            if ((sent == null ? callbackSent : sent).compareAndSet(false, true)) {
                nativeWindowFailed(requestId, throwableMessage(t));
            }
        }
    }

    /**
     * Completes a window with an application result. status 0 = accepted,
     * 1 = cancelled. The native side correlates the result with requestId.
     */
    public static void finishWindow(long requestId, int status, String[] values) {
        final String[] safeValues = values == null ? new String[0] : values.clone();
        if (!Platform.isFxApplicationThread()) {
            try {
                Platform.runLater(() -> finishWindow(requestId, status, safeValues));
            } catch (Throwable t) {
                nativeWindowFailed(requestId, throwableMessage(t));
            }
            return;
        }

        final Stage stage;
        final AtomicBoolean sent;
        final AtomicBoolean processing;
        synchronized (LOCK) {
            stage = OPEN_STAGES.get(requestId);
            sent = CALLBACK_SENT.get(requestId);
            processing = PROCESSING.get(requestId);
        }
        if (stage == null || sent == null || processing == null) {
            nativeWindowFailed(requestId, "JavaFX request is no longer active");
            return;
        }

        if (status == 0) {
            processing.set(true);
            showProcessingState(stage, "Processing...");
            if (sent.compareAndSet(false, true)) {
                nativeWindowResult(requestId, status, safeValues);
            }
            return;
        }

        if (sent.compareAndSet(false, true)) {
            nativeWindowResult(requestId, status, safeValues);
        }
        stage.close();
    }

    public static void completeProcessing(long requestId, boolean success, String message) {
        if (!Platform.isFxApplicationThread()) {
            try {
                Platform.runLater(() -> completeProcessing(requestId, success, message));
            } catch (Throwable t) {
                nativeWindowFailed(requestId, throwableMessage(t));
            }
            return;
        }

        final Stage stage;
        final AtomicBoolean sent;
        final AtomicBoolean processing;
        synchronized (LOCK) {
            stage = OPEN_STAGES.get(requestId);
            sent = CALLBACK_SENT.get(requestId);
            processing = PROCESSING.get(requestId);
        }
        if (stage == null || sent == null || processing == null) return;

        if (success) {
            processing.set(false);
            stage.close();
            nativeProcessingFinished(requestId, true);
            return;
        }

        processing.set(false);
        sent.set(false);
        showErrorState(stage, message == null || message.isEmpty()
            ? "Processing failed. Please check the input and try again."
            : message);
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
                    CALLBACK_SENT.clear();
                    PROCESSING.clear();
                }
                Platform.exit();
            });
        } catch (Throwable t) {
            // Let DestroyJavaVM finish the remaining JVM lifecycle. Native side
            // still clears its modal host guard after the worker exits.
        }
    }

    private static BorderPane createLoadingView() {
        final ProgressIndicator indicator = new ProgressIndicator();
        final Label label = new Label("Loading...");
        final VBox box = new VBox(12, indicator, label);
        box.setAlignment(javafx.geometry.Pos.CENTER);
        final BorderPane root = new BorderPane();
        root.setCenter(box);
        return root;
    }

    private static BorderPane createProcessingView(String message) {
        final ProgressIndicator indicator = new ProgressIndicator();
        final Label label = new Label(message);
        final VBox box = new VBox(12, indicator, label);
        box.setAlignment(javafx.geometry.Pos.CENTER);
        final BorderPane root = new BorderPane();
        root.setCenter(box);
        return root;
    }

    private static BorderPane createErrorView(String message) {
        final BorderPane root = new BorderPane();
        root.setCenter(new Label(message));
        return root;
    }

    private static void showProcessingState(Stage stage, String message) {
        stage.getScene().setRoot(ArchiJavaFxViews.processing(message));
    }

    private static void showErrorState(Stage stage, String message) {
        stage.getScene().setRoot(ArchiJavaFxViews.error(message));
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
    private static native void nativeProcessingFinished(long requestId, boolean success);
}
