package com.thales.hwb.Archi.launcher;

import javafx.geometry.Insets;
import javafx.scene.control.Label;
import javafx.scene.control.ProgressIndicator;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

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

    static BorderPane content(String title, String[] args) {
        final BorderPane root = new BorderPane();
        root.setPadding(new Insets(16));
        root.setCenter(new Label(title == null ? "JavaFX window" : title));
        return root;
    }

    static void fitStageToContent(Stage stage) {
        stage.sizeToScene();
        final double width = Math.max(stage.getWidth(), 420.0);
        final double height = Math.max(stage.getHeight(), 220.0);
        stage.setMinWidth(420.0);
        stage.setMinHeight(220.0);
        stage.setWidth(width);
        stage.setHeight(height);
    }

    static void applyMinimumSize(Stage stage) {
        stage.setMinWidth(420.0);
        stage.setMinHeight(220.0);
    }
}
