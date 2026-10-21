/*
 * Example: enumerate the components of the current Creo assembly.
 *
 * With recursive == true, components are returned in traversal order,
 * including components inside subassemblies.
 *
 * This example intentionally performs no modification of the assembly.
 */

#include "ArchiCreoAssemblyHandler.hpp"
#include "ArchiCreoModelHandler.hpp"

#include <vector>

class ArchiCreoAssemblyExample final {
  public:
    ArchiCreoAssemblyExample() = delete;

    static std::vector<creo::ArchiCreoAssemblyComponent>
    collectCurrentAssemblyComponents(
        bool recursive = true) {

        const auto assembly =
            creo::ArchiCreoModelHandler::fromCurrentWindow();

        return creo::ArchiCreoAssemblyHandler::collectComponents(
            assembly,
            recursive);
    }

    static std::size_t
    countCurrentAssemblyComponents() {

        return collectCurrentAssemblyComponents(true).size();
    }
};
