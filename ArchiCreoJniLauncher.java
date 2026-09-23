package com.thales.hwb.Archi.launcher;

import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.Modality;
import javafx.stage.Stage;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

public final class ArchiCreoJniLauncher {

    private static final int STATUS_ACCEPTED = 0;
    private static final int STATUS_CANCELLED = 1;

    private static final long STARTUP_TIMEOUT_SECONDS = 15L;

    private static final Map<Long, WindowRequest> REQUESTS =
        new ConcurrentHashMap<>();

    private static final AtomicBoolean INITIALIZED =
        new AtomicBoolean(false);

    private static final AtomicBoolean SHUTTING_DOWN =
        new AtomicBoolean(false);

    private record WindowRequest(
        long requestId,
        Stage stage,
        AtomicBoolean completed) {
    }

    private ArchiCreoJniLauncher() {
    }

    /** Called by the native JVM owner thread. */
    public static void initialize() throws Exception {
        if (INITIALIZED.get()) {
            return;
        }

        SHUTTING_DOWN.set(false);

        final CountDownLatch ready = new CountDownLatch(1);
        final AtomicReference<Throwable> startupError =
            new AtomicReference<>();

        try {
            Platform.startup(() -> ready.countDown());
        } catch (IllegalStateException alreadyStarted) {
            try {
                Platform.runLater(ready::countDown);
            } catch (Throwable error) {
                startupError.set(error);
                ready.countDown();
            }
        }

        if (!ready.await(STARTUP_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
            throw new IllegalStateException("JavaFX startup timeout");
        }

        final Throwable error = startupError.get();
        if (error != null) {
            throw new IllegalStateException("JavaFX startup failed", error);
        }

        Platform.setImplicitExit(false);
        INITIALIZED.set(true);
    }

    /** Opens a modeless (0) or application-modal (1) window. */
    public static void openWindow(
        String title,
        String[] args,
        int mode,
        long requestId) {

        if (!INITIALIZED.get()) {
            nativeWindowFailed(requestId, "JavaFX is not initialized");
            return;
        }

        if (SHUTTING_DOWN.get()) {
            nativeWindowFailed(requestId, "JavaFX is shutting down");
            return;
        }

        final String safeTitle = title == null ? "" : title;
        final String[] safeArgs = args == null ? new String[0] : args.clone();

        try {
            Platform.runLater(() -> createAndShowWindow(
                safeTitle, safeArgs, mode, requestId));
        } catch (Throwable error) {
            nativeWindowFailed(requestId, throwableMessage(error));
        }
    }

    /** Runs only on the JavaFX Application Thread. */
    private static void createAndShowWindow(
        String title,
        String[] args,
        int mode,
        long requestId) {

        if (SHUTTING_DOWN.get()) {
            nativeWindowFailed(requestId, "JavaFX is shutting down");
            return;
        }

        final Stage stage = new Stage();
        final WindowRequest request = new WindowRequest(
            requestId,
            stage,
            new AtomicBoolean(false));

        try {
            stage.setTitle(title);
            stage.initModality(
                mode == 1 ? Modality.APPLICATION_MODAL : Modality.NONE);
            stage.setScene(new Scene(
                createContent(title, args, requestId),
                900,
                650));

            if (REQUESTS.putIfAbsent(requestId, request) != null) {
                nativeWindowFailed(
                    requestId,
                    "Duplicate requestId: " + requestId);
                return;
            }

            stage.setOnHidden(event -> cancelIfNeeded(request));

            stage.show();
            stage.toFront();
        } catch (Throwable error) {
            REQUESTS.remove(requestId, request);
            if (request.completed().compareAndSet(false, true)) {
                nativeWindowFailed(requestId, throwableMessage(error));
            }
        }
    }

    /** Completes a request. Safe to call from any Java thread. */
    public static void finishWindow(
        long requestId,
        int status,
        String[] values) {

        final String[] safeValues =
            values == null ? new String[0] : values.clone();

        final Runnable completion = () -> {
            final WindowRequest request = REQUESTS.get(requestId);
            if (request == null) {
                return;
            }

            if (!request.completed().compareAndSet(false, true)) {
                return;
            }

            REQUESTS.remove(requestId, request);

            try {
                nativeWindowResult(requestId, status, safeValues);
            } finally {
                try {
                    request.stage().close();
                } catch (Throwable ignored) {
                    // The native result has already been delivered.
                }
            }
        };

        if (Platform.isFxApplicationThread()) {
            completion.run();
        } else {
            try {
                Platform.runLater(completion);
            } catch (Throwable error) {
                REQUESTS.computeIfPresent(requestId, (id, request) -> {
                    if (request.completed().compareAndSet(false, true)) {
                        nativeWindowFailed(id, throwableMessage(error));
                        return null;
                    }
                    return request;
                });
            }
        }
    }

    public static void cancelWindow(long requestId) {
        finishWindow(requestId, STATUS_CANCELLED, new String[0]);
    }

    /** Runs only on the JavaFX Application Thread. */
    private static void cancelIfNeeded(WindowRequest request) {
        if (!request.completed().compareAndSet(false, true)) {
            return;
        }

        REQUESTS.remove(request.requestId(), request);

        nativeWindowResult(
            request.requestId(),
            STATUS_CANCELLED,
            new String[0]);
    }

    /**
     * Example UI. Replace createContent() with the real application UI.
     * The example returns the edited text through finishWindow().
     */
    private static BorderPane createContent(
        String title,
        String[] args,
        long requestId) {

        final BorderPane root = new BorderPane();
        root.setPadding(new Insets(20));

        final Label label = new Label(
            title.isBlank() ? "JavaFX dialog" : title);

        final TextField valueField = new TextField();
        if (args.length > 0) {
            valueField.setText(args[0]);
        }

        final Button ok = new Button("OK");
        final Button cancel = new Button("Cancel");

        ok.setOnAction(event -> finishWindow(
            requestId,
            STATUS_ACCEPTED,
            new String[]{valueField.getText()}));

        cancel.setOnAction(event -> cancelWindow(requestId));

        final HBox buttons = new HBox(10, ok, cancel);
        final VBox content = new VBox(12, label, valueField, buttons);

        root.setCenter(content);
        return root;
    }

    /** Idempotent JavaFX shutdown. */
    public static void shutdown() {
        if (!INITIALIZED.get()) {
            return;
        }

        if (!SHUTTING_DOWN.compareAndSet(false, true)) {
            return;
        }

        final Runnable shutdown = () -> {
            final ArrayList<WindowRequest> requests =
                new ArrayList<>(REQUESTS.values());

            for (WindowRequest request : requests) {
                if (request.completed().compareAndSet(false, true)) {
                    nativeWindowResult(
                        request.requestId(),
                        STATUS_CANCELLED,
                        new String[0]);
                }

                try {
                    request.stage().close();
                } catch (Throwable ignored) {
                    // Continue shutdown.
                }
            }

            REQUESTS.clear();
            INITIALIZED.set(false);
            Platform.exit();
        };

        try {
            if (Platform.isFxApplicationThread()) {
                shutdown.run();
            } else {
                Platform.runLater(shutdown);
            }
        } catch (Throwable ignored) {
            REQUESTS.clear();
            INITIALIZED.set(false);
        }
    }

    private static String throwableMessage(Throwable error) {
        final String message = error.getMessage();
        return error.getClass().getName()
            + (message == null || message.isBlank() ? "" : ": " + message);
    }

    private static native void nativeWindowClosed(long requestId);

    private static native void nativeWindowResult(
        long requestId,
        int status,
        String[] values);

    private static native void nativeWindowFailed(
        long requestId,
        String message);
}
