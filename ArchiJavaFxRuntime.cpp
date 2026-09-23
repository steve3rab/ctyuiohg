#include "ArchiJavaFxRuntime.hpp"

#include <utility>
#include "ArchiJavaFxRuntimeImpl.hpp"

namespace jnifx {
ArchiJavaFxRuntime::ArchiJavaFxRuntime(Config config) : impl_(std::make_unique<Impl>(std::move(config))) {}
ArchiJavaFxRuntime::~ArchiJavaFxRuntime() = default;
void ArchiJavaFxRuntime::preloadAsync() { impl_->startAsync(); }
ArchiJavaFxRuntime::RequestId ArchiJavaFxRuntime::tryOpenWindow(std::string title, Arguments arguments) { return impl_->tryOpenWindow(std::move(title), std::move(arguments)); }
ArchiJavaFxRuntime::RequestId ArchiJavaFxRuntime::tryOpenModalWindow(std::string title, Arguments arguments) { return impl_->tryOpenModalWindow(std::move(title), std::move(arguments)); }
ArchiJavaFxRuntime::State ArchiJavaFxRuntime::state() const noexcept { return impl_->state(); }
bool ArchiJavaFxRuntime::isReady() const noexcept { return impl_->isReady(); }
std::string ArchiJavaFxRuntime::lastError() const { return impl_->lastError(); }
void ArchiJavaFxRuntime::setResultCallback(ResultCallback callback) { impl_->setResultCallback(std::move(callback)); }
void ArchiJavaFxRuntime::shutdown() noexcept { impl_->shutdown(); }
}
