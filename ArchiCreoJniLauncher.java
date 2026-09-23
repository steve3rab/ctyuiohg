package com.thales.hwb.Archi.launcher;

import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.VBox;
import javafx.stage.Modality;
import javafx.stage.Stage;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Minimal JavaFX bridge used by the native runtime.
 * The application-specific UI can replace createContent() while keeping the
 * native callback contract unchanged.
 */
public final class ArchiCreoJniLauncher {
    private static final ConcurrentHashMap<Long, Stage> OPEN_STAGES = new ConcurrentHashMap<>();
    private static final AtomicBoolean FX_STARTED = new AtomicBoolean(false);

    private ArchiCreoJniLauncher() {}

    public static void initialize() {
        if (FX_STARTED.compareAndSet(false, true)) {
            try {
                Platform.startup(() -> { });
            } catch (IllegalStateException alreadyStarted) {
                // JavaFX was already started by the host/application.
            }
        }
    }

    public static void openWindow(String title, String[] arguments, int mode, long requestId) {
        Platform.runLater(() -> {
            final AtomicBoolean completed = new AtomicBoolean(false);
            final Stage stage = new Stage();
            OPEN_STAGES.put(requestId, stage);

            if (mode == 1) {
                stage.initModality(Modality.APPLICATION_MODAL);
            }

            stage.setTitle(title == null ? "JavaFX" : title);
            stage.setScene(new Scene(createContent(stage, requestId, arguments, completed), 420, 220));

            stage.setOnHidden(event -> {
                OPEN_STAGES.remove(requestId, stage);
                if (completed.compareAndSet(false, true)) {
                    nativeWindowResult(requestId, "CANCELLED", new String[0]);
                }
            });

            stage.show();
        });
    }

    private static VBox createContent(
            Stage stage,
            long requestId,
            String[] arguments,
            AtomicBoolean completed) {

        final TextField value = new TextField();
        value.setPromptText("Réponse utilisateur");

        final Label info = new Label(
                arguments == null || arguments.length == 0
                        ? "Aucun argument"
                        : String.join(" | ", arguments));

        final Button ok = new Button("OK");
        ok.setOnAction(event -> {
            if (completed.compareAndSet(false, true)) {
                nativeWindowResult(requestId, "OK", new String[] { value.getText() });
            }
            stage.close();
        });

        final Button cancel = new Button("Annuler");
        cancel.setOnAction(event -> {
            if (completed.compareAndSet(false, true)) {
                nativeWindowResult(requestId, "CANCELLED", new String[0]);
            }
            stage.close();
        });

        final VBox root = new VBox(12, info, value, ok, cancel);
        root.setPadding(new Insets(16));
        return root;
    }

    public static void shutdown() {
        Platform.runLater(() -> {
            for (Stage stage : OPEN_STAGES.values()) {
                try {
                    stage.close();
                } catch (RuntimeException ignored) {
                    // Shutdown must not propagate back through JNI.
                }
            }
            OPEN_STAGES.clear();
            Platform.exit();
        });
    }

    private static native void nativeWindowClosed(long requestId);
    private static native void nativeWindowResult(long requestId, String status, String[] values);
    private static native void nativeWindowFailed(long requestId, String message);
}
