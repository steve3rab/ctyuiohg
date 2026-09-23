#include "ArchiJavaFxRuntime.hpp"
#include "ArchiJavaFxRuntimeImpl.hpp"
#include <utility>

namespace jnifx {
ArchiJavaFxRuntime::ArchiJavaFxRuntime(Config config) : impl_(std::make_unique<Impl>(std::move(config))) {}
ArchiJavaFxRuntime::~ArchiJavaFxRuntime() = default;
void ArchiJavaFxRuntime::preloadAsync() { impl_->startAsync(); }
bool ArchiJavaFxRuntime::tryOpenWindow(std::string title, Arguments arguments, ResultCallback callback) { return impl_->tryOpenWindow(std::move(title), std::move(arguments), std::move(callback)); }
bool ArchiJavaFxRuntime::tryOpenModalWindow(std::string title, Arguments arguments, ResultCallback callback) { return impl_->tryOpenModalWindow(std::move(title), std::move(arguments), std::move(callback)); }
void ArchiJavaFxRuntime::shutdown() noexcept { impl_->shutdown(); }
} // namespace jnifx
