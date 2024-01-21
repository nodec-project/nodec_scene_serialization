#ifndef NODEC_SCENE_SERIALIZATION__COMPONENTS__NON_SERIALIZED_HPP_
#define NODEC_SCENE_SERIALIZATION__COMPONENTS__NON_SERIALIZED_HPP_

#include "../serializable_component.hpp"

namespace nodec_scene_serialization {
namespace components {

struct NonSerialized : BaseSerializableComponent {
    NonSerialized()
        : BaseSerializableComponent(this) {}

    template<class Archive>
    void serialize(Archive &archive) {}
};

} // namespace components
} // namespace nodec_scene_serialization

NODEC_SCENE_REGISTER_SERIALIZABLE_COMPONENT(nodec_scene_serialization::components::NonSerialized)

#endif