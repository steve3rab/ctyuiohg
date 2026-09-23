#pragma once

#include "ArchiJavaFxRuntime.hpp"

void configureJavaFxResultCallback();

jnifx::ArchiJavaFxRuntime::RequestId onCreatePart();
jnifx::ArchiJavaFxRuntime::RequestId onCreateAssembly();

void createPartFromJavaFx(
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result);

void createAssemblyFromJavaFx(
    const jnifx::ArchiJavaFxRuntime::JavaFxResult& result);
