#ifndef NODEC_SCENE_SERIALIZATION__ARCHIVE_CONTEXT_HPP_
#define NODEC_SCENE_SERIALIZATION__ARCHIVE_CONTEXT_HPP_

#include <nodec/resource_management/resource_registry.hpp>

#include "scene_serialization.hpp"

namespace nodec_scene_serialization {

class ArchiveContext {
public:
    ArchiveContext(SceneSerialization &scene_serialization,
                   nodec::resource_management::ResourceRegistry &resource_registry)
        : scene_serialization_(scene_serialization),
          resource_registry_(resource_registry) {}

    SceneSerialization &scene_serialization() {
        return scene_serialization_;
    }

    nodec::resource_management::ResourceRegistry &resource_registry() {
        return resource_registry_;
    }

private:
    SceneSerialization &scene_serialization_;
    nodec::resource_management::ResourceRegistry &resource_registry_;
};
} // namespace nodec_scene_serialization

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>

#define CEREAL_FUTURE_EXPERIMENTAL
#include <cereal/archives/adapters.hpp>

namespace nodec_scene_serialization {
namespace internal {
// We couldn't write like this...
// CEREAL_REGISTER_ARCHIVE(cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONInputArchive>)
using JSONInputArchiveWithContext = cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONInputArchive>;
using JSONOutputArchiveWithContext = cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::JSONOutputArchive>;
using BinaryInputArchiveWithContext = cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::BinaryInputArchive>;
using BinaryOutputArchiveWithContext = cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::BinaryOutputArchive>;
using PortableBinaryInputArchiveWithContext = cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::PortableBinaryInputArchive>;
using PortableBinaryOutputArchiveWithContext = cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::PortableBinaryOutputArchive>;
using XMLOutputArchiveWithContext = cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::XMLOutputArchive>;
using XMLInputArchiveWithContext = cereal::UserDataAdapter<nodec_scene_serialization::ArchiveContext, cereal::XMLInputArchive>;

} // namespace internal
} // namespace nodec_scene_serialization

CEREAL_REGISTER_ARCHIVE(nodec_scene_serialization::internal::JSONInputArchiveWithContext)
CEREAL_REGISTER_ARCHIVE(nodec_scene_serialization::internal::JSONOutputArchiveWithContext)
CEREAL_REGISTER_ARCHIVE(nodec_scene_serialization::internal::BinaryInputArchiveWithContext)
CEREAL_REGISTER_ARCHIVE(nodec_scene_serialization::internal::BinaryOutputArchiveWithContext)
CEREAL_REGISTER_ARCHIVE(nodec_scene_serialization::internal::PortableBinaryInputArchiveWithContext)
CEREAL_REGISTER_ARCHIVE(nodec_scene_serialization::internal::PortableBinaryOutputArchiveWithContext)
CEREAL_REGISTER_ARCHIVE(nodec_scene_serialization::internal::XMLOutputArchiveWithContext)
CEREAL_REGISTER_ARCHIVE(nodec_scene_serialization::internal::XMLInputArchiveWithContext)

#endif
