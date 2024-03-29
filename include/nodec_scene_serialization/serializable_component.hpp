#ifndef NODEC_SCENE_SERIALIZATION__SERIALIZABLE_COMPONENT_HPP_
#define NODEC_SCENE_SERIALIZATION__SERIALIZABLE_COMPONENT_HPP_

#include <cereal/types/polymorphic.hpp>
#include <nodec/type_info.hpp>

namespace nodec_scene_serialization {

struct BaseSerializableComponent {
    template<class Derived>
    BaseSerializableComponent(Derived *)
        : type_info_{&nodec::type_id<Derived>()} {
    }

    virtual ~BaseSerializableComponent() = 0;

    const nodec::type_info &type_info() const noexcept {
        return *type_info_;
    }

private:
    const nodec::type_info *type_info_;
};

inline BaseSerializableComponent::~BaseSerializableComponent() {}

} // namespace nodec_scene_serialization

// NOTE: We pre-include cereal archives.
//  SerializableComponent will be inherited from BaseSerializableComponent class,
//  and should be registered in cereal by using CEREAL_REGISTER_TYPE, CEREAL_REGISTER_POLYMORPHIC_RELATION.
//  Before registering the class in cereal, we need to include the archives we plan to use.
//  So, we pre-include these headers.
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>

#define NODEC_SCENE_REGISTER_SERIALIZABLE_COMPONENT(T) \
    CEREAL_REGISTER_TYPE(T) \
    CEREAL_REGISTER_POLYMORPHIC_RELATION(nodec_scene_serialization::BaseSerializableComponent, T)

#endif