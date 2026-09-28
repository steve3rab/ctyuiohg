package com.thales.hwb.Archi.launcher;

import javafx.geometry.Insets;
import javafx.scene.control.Button;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.control.ProgressIndicator;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

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

    static BorderPane content(
        Stage stage,
        long requestId,
        String title,
        String[] args) {
        final BorderPane root = new BorderPane();
        root.setPadding(new Insets(16));

        final Label label = new Label(
            title == null || title.isBlank()
                ? "JavaFX window"
                : title);

        final Label modelNameLabel =
            new Label("Model name");
        final TextField modelNameField =
            new TextField();
        modelNameField.setPromptText(
            "Enter model name");
        modelNameField.setPrefColumnCount(24);

        final Label templateLabel =
            new Label("Template");

        final ComboBox<TemplateOption> templateBox =
            new ComboBox<>();
        templateBox.setMaxWidth(Double.MAX_VALUE);

        final Label templateError =
            new Label();
        templateError.setWrapText(true);

        loadTemplates(args, templateBox, templateError);

        final Button cancel =
            new Button("Cancel");
        final Button ok =
            new Button("OK");

        cancel.setOnAction(event ->
            ArchiCreoJniLauncher.cancelWindow(
                requestId));

        ok.setOnAction(event -> {
            final String modelName =
                modelNameField.getText() == null
                    ? ""
                    : modelNameField.getText().trim();

            final TemplateOption template =
                templateBox.getValue();

            if (modelName.isEmpty()) {
                modelNameField.requestFocus();
                modelNameField.selectAll();
                return;
            }

            if (template == null) {
                templateBox.requestFocus();
                return;
            }

            ArchiCreoJniLauncher.acceptWindow(
                requestId,
                modelName,
                template.path().toString());
        });

        final HBox buttons =
            new HBox(8, cancel, ok);
        buttons.setAlignment(
            javafx.geometry.Pos.CENTER_RIGHT);

        final VBox content =
            new VBox(
                12,
                label,
                modelNameLabel,
                modelNameField,
                templateLabel,
                templateBox,
                templateError,
                buttons);

        content.setAlignment(
            javafx.geometry.Pos.CENTER);

        return rootWithContent(root, content);
    }

    private static BorderPane rootWithContent(
        BorderPane root,
        VBox content) {
        root.setCenter(content);
        return root;
    }

    private static void loadTemplates(
        String[] args,
        ComboBox<TemplateOption> templateBox,
        Label errorLabel) {
        templateBox.getItems().clear();

        if (args == null || args.length < 2 ||
            args[1] == null || args[1].isBlank()) {
            errorLabel.setText(
                "Template directory is not configured.");
            return;
        }

        try {
            final Path directory =
                Paths.get(args[1]).toAbsolutePath().normalize();

            if (!Files.isDirectory(directory)) {
                errorLabel.setText(
                    "Template directory does not exist:\\n" +
                    directory);
                return;
            }

            final List<TemplateOption> templates =
                new ArrayList<>();

            try (var stream = Files.list(directory)) {
                stream
                    .filter(Files::isRegularFile)
                    .filter(path ->
                        path.getFileName()
                            .toString()
                            .toLowerCase()
                            .endsWith(".prt"))
                    .sorted(Comparator.comparing(
                        path -> path.getFileName()
                            .toString(),
                        String.CASE_INSENSITIVE_ORDER))
                    .forEach(path ->
                        templates.add(
                            new TemplateOption(
                                path.toAbsolutePath().normalize())));
            }

            templateBox.getItems().setAll(templates);

            if (templates.isEmpty()) {
                errorLabel.setText(
                    "No .prt templates were found in:\\n" +
                    directory);
            } else {
                templateBox.getSelectionModel()
                    .selectFirst();
                errorLabel.setText("");
            }
        } catch (Exception error) {
            errorLabel.setText(
                "Unable to load templates: " +
                error.getMessage());
        }
    }

    private record TemplateOption(Path path) {
        @Override
        public String toString() {
            return path.getFileName().toString();
        }
    }

    static void fitStageToContent(Stage stage) {
        stage.sizeToScene();
        final double width = Math.max(stage.getWidth(), 420.0);
        final double height = Math.max(stage.getHeight(), 220.0);
        stage.setMinWidth(320.0);
        stage.setMinHeight(160.0);
        stage.setWidth(width);
        stage.setHeight(height);
    }

    static void applyMinimumSize(Stage stage) {
        stage.setMinWidth(320.0);
        stage.setMinHeight(160.0);
    }
}
