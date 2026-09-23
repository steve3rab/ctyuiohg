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

/**
 * JNI bridge used by ArchiJavaFxRuntime.
 *
 * Result lifecycle:
 *
 *   OK      -> nativeWindowResult(requestId, 0, values)
 *   Cancel  -> nativeWindowResult(requestId, 1, emptyValues)
 *   X       -> nativeWindowResult(requestId, 1, emptyValues)
 *   Error   -> nativeWindowFailed(requestId, message)
 *
 * Only one native callback is allowed for a requestId.
 */
public final class ArchiCreoJniLauncher {

    private static final Object LOCK = new Object();

    private static final Map<Long, Stage> OPEN_STAGES = new HashMap<>();
    private static final Map<Long, AtomicBoolean> CALLBACK_SENT = new HashMap<>();

    private static final AtomicBoolean INITIALIZED =
        new AtomicBoolean(false);

    private static final AtomicBoolean SHUTTING_DOWN =
        new AtomicBoolean(false);

    private ArchiCreoJniLauncher() {
    }

    /**
     * Called once by the native JVM owner thread.
     */
    public static void initialize() throws Exception {

        if (INITIALIZED.get()) {
            return;
        }

        SHUTTING_DOWN.set(false);

        final CountDownLatch ready = new CountDownLatch(1);
        final AtomicReference<Throwable> startupError =
            new AtomicReference<>();

        try {

            Platform.startup(() -> {
                try {
                    // Do not create a Stage here.
                }
                catch (Throwable t) {
                    startupError.set(t);
                }
                finally {
                    ready.countDown();
                }
            });

        }
        catch (IllegalStateException alreadyStarted) {

            // JavaFX was already started by another component.
            try {
                Platform.runLater(ready::countDown);
            }
            catch (Throwable t) {
                startupError.set(t);
                ready.countDown();
            }
        }

        if (!ready.await(15, TimeUnit.SECONDS)) {
            throw new IllegalStateException(
                "JavaFX startup timeout");
        }

        final Throwable error = startupError.get();

        if (error != null) {
            throw new IllegalStateException(
                "JavaFX startup failed",
                error);
        }

        Platform.setImplicitExit(false);

        INITIALIZED.set(true);
    }

    /**
     * Opens a modeless or modal window asynchronously.
     *
     * mode 0 = Modality.NONE
     * mode 1 = Modality.APPLICATION_MODAL
     */
    public static void openWindow(
        String title,
        String[] args,
        int mode,
        long requestId) {

        if (!INITIALIZED.get()) {
            nativeWindowFailed(
                requestId,
                "JavaFX runtime is not initialized");
            return;
        }

        if (SHUTTING_DOWN.get()) {
            nativeWindowFailed(
                requestId,
                "JavaFX runtime is shutting down");
            return;
        }

        try {

            Platform.runLater(() ->
                createAndShowWindow(
                    title,
                    args,
                    mode,
                    requestId));

        }
        catch (Throwable t) {

            nativeWindowFailed(
                requestId,
                throwableMessage(t));
        }
    }

    /**
     * Must execute on the JavaFX Application Thread.
     */
    private static void createAndShowWindow(
        String title,
        String[] args,
        int mode,
        long requestId) {

        if (SHUTTING_DOWN.get()) {
            nativeWindowFailed(
                requestId,
                "JavaFX runtime is shutting down");
            return;
        }

        Stage stage = null;

        final AtomicBoolean callbackSent =
            new AtomicBoolean(false);

        try {

            stage = new Stage();

            final boolean modal = mode == 1;

            stage.initModality(
                modal
                    ? Modality.APPLICATION_MODAL
                    : Modality.NONE);

            stage.setTitle(
                title == null ? "" : title);

            final Scene scene =
                new Scene(
                    createContent(
                        title,
                        args,
                        requestId),
                    900,
                    650);

            stage.setScene(scene);

            final Stage registeredStage = stage;

            synchronized (LOCK) {
                OPEN_STAGES.put(
                    requestId,
                    registeredStage);

                CALLBACK_SENT.put(
                    requestId,
                    callbackSent);
            }

            /*
             * Closing with the X is considered Cancelled.
             *
             * This is important because the C++ side must receive
             * a terminal result even when the user closes the window
             * without pressing OK or Cancel.
             */
            stage.setOnHidden(event -> {

                final AtomicBoolean sent;

                synchronized (LOCK) {

                    OPEN_STAGES.remove(requestId);

                    sent = CALLBACK_SENT.remove(
                        requestId);
                }

                if (sent != null &&
                    sent.compareAndSet(false, true)) {

                    nativeWindowResult(
                        requestId,
                        1,              // Cancelled
                        new String[0]);
                }
            });

            /*
             * IMPORTANT:
             *
             * Use show(), not showAndWait().
             *
             * The JavaFX Application Thread remains in control
             * of its normal event loop.
             */
            stage.show();
            stage.toFront();

        }
        catch (Throwable t) {

            if (stage != null) {

                try {
                    stage.close();
                }
                catch (Throwable ignored) {
                    // Best effort.
                }
            }

            final AtomicBoolean sent;

            synchronized (LOCK) {

                OPEN_STAGES.remove(requestId);

                sent = CALLBACK_SENT.remove(
                    requestId);
            }

            final AtomicBoolean callbackFlag =
                sent == null
                    ? callbackSent
                    : sent;

            if (callbackFlag.compareAndSet(
                    false,
                    true)) {

                nativeWindowFailed(
                    requestId,
                    throwableMessage(t));
            }
        }
    }

    /**
     * Completes the JavaFX window.
     *
     * status:
     *
     *   0 = Accepted
     *   1 = Cancelled
     *   2 = Failed
     */
    public static void finishWindow(
        long requestId,
        int status,
        String[] values) {

        final String[] safeValues =
            values == null
                ? new String[0]
                : values.clone();

        /*
         * finishWindow() may be called from any Java thread.
         *
         * Stage manipulation and the actual result delivery are
         * serialized on the JavaFX Application Thread.
         */
        if (!Platform.isFxApplicationThread()) {

            try {

                Platform.runLater(() ->
                    finishWindow(
                        requestId,
                        status,
                        safeValues));

            }
            catch (Throwable t) {

                nativeWindowFailed(
                    requestId,
                    throwableMessage(t));
            }

            return;
        }

        final Stage stage;
        final AtomicBoolean callbackFlag;

        synchronized (LOCK) {

            stage = OPEN_STAGES.get(requestId);

            callbackFlag =
                CALLBACK_SENT.get(requestId);
        }

        /*
         * The request may already have been closed.
         */
        if (callbackFlag == null) {
            return;
        }

        /*
         * Exactly one terminal callback is allowed.
         */
        if (!callbackFlag.compareAndSet(
                false,
                true)) {

            return;
        }

        /*
         * Deliver the result BEFORE closing the Stage.
         *
         * This guarantees that the native side receives the
         * values before onHidden() is triggered.
         */
        try {

            nativeWindowResult(
                requestId,
                status,
                safeValues);

        }
        catch (Throwable t) {

            /*
             * There is normally nothing useful we can do here,
             * because the native callback itself failed.
             */
        }

        /*
         * Remove the stage from the active list here.
         *
         * onHidden() will also execute, but callbackSent is already
         * true, therefore no second native result is generated.
         */
        synchronized (LOCK) {

            OPEN_STAGES.remove(requestId);
            CALLBACK_SENT.remove(requestId);
        }

        if (stage != null) {

            try {
                stage.close();
            }
            catch (Throwable ignored) {
                // Result was already delivered.
            }
        }
    }

    /**
     * Convenience method for Cancel.
     */
    public static void cancelWindow(long requestId) {

        finishWindow(
            requestId,
            1,
            new String[0]);
    }

    /**
     * Example JavaFX content.
     *
     * Replace the fields/buttons with the real application UI.
     *
     * The important point is that the OK action calls finishWindow().
     */
    private static BorderPane createContent(
        String title,
        String[] args,
        long requestId) {

        final BorderPane root =
            new BorderPane();

        root.setPadding(
            new Insets(20));

        final Label titleLabel =
            new Label(
                title == null
                    ? "JavaFX window"
                    : title);

        final TextField valueField =
            new TextField();

        /*
         * Example:
         *
         * C++:
         *
         * openModalWindow(
         *     "My dialog",
         *     {"PART_001", "123.45"});
         *
         * Java receives:
         *
         * args[0] = PART_001
         * args[1] = 123.45
         */

        if (args != null &&
            args.length > 0) {

            valueField.setText(
                args[0]);
        }

        final Button okButton =
            new Button("OK");

        final Button cancelButton =
            new Button("Cancel");

        /*
         * OK
         *
         * Return the data to C++.
         */
        okButton.setOnAction(event -> {

            final String value =
                valueField.getText();

            finishWindow(
                requestId,
                0,              // Accepted
                new String[] {
                    value
                });
        });

        /*
         * Cancel
         */
        cancelButton.setOnAction(event -> {

            finishWindow(
                requestId,
                1,              // Cancelled
                new String[0]);
        });

        final HBox buttons =
            new HBox(
                10,
                okButton,
                cancelButton);

        final VBox content =
            new VBox(
                12,
                titleLabel,
                valueField,
                buttons);

        root.setCenter(content);

        return root;
    }

    /**
     * Shutdown.
     *
     * The JavaFX Application Thread performs the Stage cleanup
     * and then exits JavaFX.
     */
    public static void shutdown() {

        if (!INITIALIZED.get()) {
            return;
        }

        if (!SHUTTING_DOWN.compareAndSet(
                false,
                true)) {

            return;
        }

        try {

            Platform.runLater(() -> {

                final ArrayList<Stage> stages;

                synchronized (LOCK) {

                    stages =
                        new ArrayList<>(
                            OPEN_STAGES.values());
                }

                for (Stage stage : stages) {

                    try {
                        stage.close();
                    }
                    catch (Throwable ignored) {
                        // Continue shutting down.
                    }
                }

                synchronized (LOCK) {

                    OPEN_STAGES.clear();
                    CALLBACK_SENT.clear();
                }

                INITIALIZED.set(false);

                Platform.exit();
            });

        }
        catch (Throwable ignored) {

            /*
             * Native DestroyJavaVM() remains responsible for
             * completing the JVM lifecycle.
             */
        }
    }

    private static String throwableMessage(
        Throwable t) {

        final String message =
            t.getMessage();

        return t.getClass().getName()
            + (
                message == null ||
                message.isEmpty()
                    ? ""
                    : ": " + message
              );
    }

    /*
     * Registered from C++ with RegisterNatives().
     */
    private static native void nativeWindowClosed(
        long requestId);

    private static native void nativeWindowResult(
        long requestId,
        int status,
        String[] values);

    private static native void nativeWindowFailed(
        long requestId,
        String message);
}
