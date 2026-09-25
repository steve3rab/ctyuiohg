package com.thales.hwb.Archi.launcher;

import javafx.geometry.Insets;
import javafx.scene.control.Label;
import javafx.scene.control.ProgressIndicator;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.VBox;

/**
 * Small JavaFX view factory shared by the JNI launcher.
 *
 * Java 17 source level. All methods are called on the JavaFX Application Thread.
 */
final class ArchiJavaFxViews {

    private ArchiJavaFxViews() {
    }

    static BorderPane loading() {
        final ProgressIndicator indicator = new ProgressIndicator();
        final Label label = new Label("Loading...");
        final VBox box = new VBox(12, indicator, label);
        box.setAlignment(javafx.geometry.Pos.CENTER);

        final BorderPane root = new BorderPane();
        root.setPadding(new Insets(16));
        root.setCenter(box);
        return root;
    }

    static BorderPane processing(String message) {
        final ProgressIndicator indicator = new ProgressIndicator();
        final Label label = new Label(
            message == null || message.isBlank() ? "Processing..." : message);
        final VBox box = new VBox(12, indicator, label);
        box.setAlignment(javafx.geometry.Pos.CENTER);

        final BorderPane root = new BorderPane();
        root.setPadding(new Insets(16));
        root.setCenter(box);
        return root;
    }

    static BorderPane error(String message) {
        final BorderPane root = new BorderPane();
        root.setPadding(new Insets(16));
        root.setCenter(new Label(
            message == null || message.isBlank()
                ? "An unexpected error occurred."
                : message));
        return root;
    }

    static BorderPane content(String title) {
        final BorderPane root = new BorderPane();
        root.setPadding(new Insets(16));
        root.setCenter(new Label(title == null ? "JavaFX window" : title));
        return root;
    }
}
