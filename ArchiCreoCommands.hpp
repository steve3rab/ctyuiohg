#pragma once

#include "ArchiJavaFxRuntime.hpp"

#include <string>

void configureJavaFxResultCallback();

jnifx::ArchiJavaFxRuntime::RequestId onCreatePart();
jnifx::ArchiJavaFxRuntime::RequestId onCreateAssembly();

void createPartFromJavaFx(
    const std::string& modelName,
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result);

void createAssemblyFromJavaFx(
    const std::string& modelName,
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result);
