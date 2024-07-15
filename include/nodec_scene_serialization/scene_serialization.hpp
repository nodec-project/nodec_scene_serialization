#ifndef NODEC_SCENE_SERIALIZATION__SCENE_SERIALIZATION_HPP_
#define NODEC_SCENE_SERIALIZATION__SCENE_SERIALIZATION_HPP_

#include <cassert>
#include <unordered_map>
#include <vector>

#include <nodec_scene/scene_registry.hpp>

#include "serializable_component.hpp"
#include "serializable_entity.hpp"

namespace nodec_scene_serialization {

class SceneSerialization final {
public:
    using SceneRegistry = nodec_scene::SceneRegistry;
    using SceneEntity = nodec_scene::SceneEntity;

private:
    class BaseComponentSerialization {
    public:
        BaseComponentSerialization(const nodec::type_info &component_type_info,
                                   const nodec::type_info &serializable_component_type_info)
            : component_type_info_(component_type_info),
              serializable_component_type_info_(serializable_component_type_info) {}

        nodec::type_info type_info() const noexcept {
            return component_type_info_;
        }
        nodec::type_info serializable_type_info() const noexcept {
            return serializable_component_type_info_;
        }

        virtual void emplace_component(const BaseSerializableComponent *, SceneEntity, SceneRegistry &) const = 0;
        virtual void emplace_or_replace_component(const BaseSerializableComponent *, SceneEntity, SceneRegistry &) const = 0;
        virtual std::unique_ptr<BaseSerializableComponent> make_serializable_component() const = 0;
        virtual std::unique_ptr<BaseSerializableComponent> make_serializable_component(const void *component) const = 0;
        virtual void assign_component(const BaseSerializableComponent *, void *) const = 0;

    private:
        nodec::type_info component_type_info_;
        nodec::type_info serializable_component_type_info_;
    };

    template<typename Component, typename SerializableComponent>
    class ComponentSerialization : public BaseComponentSerialization {
        using Serializer = std::function<std::unique_ptr<SerializableComponent>(const Component &)>;
        using Deserializer = std::function<Component(const SerializableComponent &)>;

        static_assert(std::is_base_of<BaseSerializableComponent, SerializableComponent>::value,
                      "SerializableComponent must be derived from BaseSerializableComponent.");

    public:
        ComponentSerialization()
            : BaseComponentSerialization(nodec::type_id<Component>(), nodec::type_id<SerializableComponent>()) {}

        std::unique_ptr<BaseSerializableComponent> make_serializable_component(const void *component) const override {
            assert(component);

            return serializer(*static_cast<const Component *>(component));
        }

        void emplace_component(const BaseSerializableComponent *source,
                               SceneEntity entity, SceneRegistry &scene_registry) const override {
            assert(source);
            assert(source->type_info() == nodec::type_id<SerializableComponent>());

            auto result = scene_registry.emplace_component<Component>(entity);
            if (!result.second) {
                // If the component is already existing, will not overwrite it.
                return;
            }

            auto comp = deserializer(*static_cast<const SerializableComponent *>(source));
            result.first = std::move(comp);
        }

        void emplace_or_replace_component(const BaseSerializableComponent *source,
                                          SceneEntity entity, SceneRegistry &scene_registry) const override {
            assert(source);
            assert(source->type_info() == nodec::type_id<SerializableComponent>());

            auto result = scene_registry.emplace_component<Component>(entity);
            auto comp = deserializer(*static_cast<const SerializableComponent *>(source));
            result.first = std::move(comp);
        }

        void assign_component(const BaseSerializableComponent *source,
                              void *target_component) const override {
            assert(source);
            assert(source->type_info() == nodec::type_id<SerializableComponent>());
            assert(target_component);

            auto comp = deserializer(*static_cast<const SerializableComponent *>(source));
            *static_cast<Component *>(target_component) = std::move(comp);
        }

        std::unique_ptr<BaseSerializableComponent> make_serializable_component() const override {
            return std::make_unique<SerializableComponent>();
        }

    public:
        Serializer serializer;
        Deserializer deserializer;
    };

public:
    /**
     * @param serializer
     *   @code{.cpp}
     *   std::unique_ptr<SerializableComponent>(const Component&);
     *   @endcode
     *
     * @param deserializer
     *   @code{.cpp}
     *   Component(const SerializableComponent&);
     *   @endcode
     */
    template<typename Component, typename SerializableComponent, typename Serializer, typename Deserializer>
    void register_component(Serializer &&serializer, Deserializer &&deserializer) {
        using namespace nodec;

        type_seq_index_type component_type_id = type_seq_index<Component>::value();
        type_seq_index_type serializable_component_type_id = type_seq_index<SerializableComponent>::value();

        assert(component_dict_.find(component_type_id) == component_dict_.end());
        assert(serializable_component_dict_.find(serializable_component_type_id) == serializable_component_dict_.end());

        auto serialization = std::make_shared<ComponentSerialization<Component, SerializableComponent>>();

        serialization->serializer = serializer;
        serialization->deserializer = deserializer;

        component_dict_[component_type_id] = serialization;
        serializable_component_dict_[serializable_component_type_id] = serialization;
    }

    template<typename Component, typename SerializableComponent>
    void register_component() {
        register_component<Component, SerializableComponent>(
            [](const Component &comp) {
                return std::make_unique<SerializableComponent>(comp);
            },
            [](const SerializableComponent &serializable) {
                return static_cast<Component>(serializable);
            });
    }

    template<typename Component>
    void register_component() {
        register_component<Component, Component>(
            [](const Component &comp) {
                return std::make_unique<Component>(comp);
            },
            [](const Component &serializable) {
                return serializable;
            });
    }

    std::unique_ptr<SerializableEntity> make_serializable_entity(
        const nodec_scene::SceneEntity &entity,
        const nodec_scene::SceneRegistry &scene_registry) const {
        using namespace nodec;

        auto serializable_entity = std::make_unique<SerializableEntity>();

        scene_registry.visit(entity, [=, &serializable_entity](const nodec::type_info type_info, void *comp) {
            assert(comp);

            auto iter = component_dict_.find(type_info.seq_index());
            if (iter == component_dict_.end()) return;

            auto &serialization = iter->second;
            assert(static_cast<bool>(serialization));

            auto serializable_component = serialization->make_serializable_component(comp);
            serializable_entity->components.push_back(std::move(serializable_component));
        });

        return serializable_entity;
    }

    /**
     * @brief Makes the entity from the serializable entity.
     */
    nodec_scene::SceneEntity make_entity(
        const SerializableEntity *serializable_entity,
        nodec_scene::SceneRegistry &scene_registry) const {
        using namespace nodec;

        if (!serializable_entity) return entities::null_entity;

        auto entity = scene_registry.create_entity();

        for (auto &comp : serializable_entity->components) {
            if (!comp) continue;

            auto iter = serializable_component_dict_.find(comp->type_info().seq_index());
            if (iter == serializable_component_dict_.end()) continue;

            auto &serialization = iter->second;
            assert(static_cast<bool>(serialization));

            serialization->emplace_component(comp.get(), entity, scene_registry);
        }

        return entity;
    }

    /**
     * @brief Emplaces the source's components into the target entity.
     *
     * @param source
     * @param target
     * @param scene_registry
     */
    void emplace_components(const SerializableEntity *source, const nodec_scene::SceneEntity &target, nodec_scene::SceneRegistry &scene_registry) const {
        if (!source) return;

        for (const auto &comp : source->components) {
            if (!comp) continue;

            auto iter = serializable_component_dict_.find(comp->type_info().seq_index());
            if (iter == serializable_component_dict_.end()) continue;

            auto &serialization = iter->second;
            assert(static_cast<bool>(serialization));

            serialization->emplace_component(comp.get(), target, scene_registry);
        }
    }

    void emplace_or_replace_component(const BaseSerializableComponent *source,
                                      const nodec_scene::SceneEntity &target,
                                      nodec_scene::SceneRegistry &scene_registry) const {
        if (!source) return;

        auto iter = serializable_component_dict_.find(source->type_info().seq_index());
        if (iter == serializable_component_dict_.end()) return;

        auto &serialization = iter->second;
        assert(static_cast<bool>(serialization));

        serialization->emplace_or_replace_component(source, target, scene_registry);
    }

    void assign_component(const BaseSerializableComponent *source,
                          const nodec::type_info &target_component_type_info,
                          void *target_component) const {
        if (!source) return;

        auto iter = serializable_component_dict_.find(source->type_info().seq_index());
        if (iter == serializable_component_dict_.end()) return;

        auto &serialization = iter->second;
        if (serialization->type_info() != target_component_type_info) return;

        serialization->assign_component(source, target_component);
    }

    /**
     * @brief Makes the serializable component resolved from the given runtime component type.
     */
    std::unique_ptr<BaseSerializableComponent> make_serializable_component(const nodec::type_info &type) const {
        auto iter = component_dict_.find(type.seq_index());
        if (iter == component_dict_.end()) return nullptr;

        return iter->second->make_serializable_component();
    }

    /**
     * @brief Makes the serializable component resolved from the given runtime component.
     *
     * @param type_info
     * @param component
     * @return std::unique_ptr<BaseSerializableComponent>
     */
    std::unique_ptr<BaseSerializableComponent>
    make_serializable_component(
        const nodec::type_info &type_info, void *component) const {
        auto iter = component_dict_.find(type_info.seq_index());
        if (iter == component_dict_.end()) return nullptr;

        return iter->second->make_serializable_component(component);
    }

    /**
     * @brief Gets the type info of runtime component from the given serializable component type info.
     */
    nodec::type_info get_component_type_info(const nodec::type_info &serializable_component_type_info) const {
        auto iter = serializable_component_dict_.find(serializable_component_type_info.seq_index());
        if (iter == serializable_component_dict_.end()) return nodec::type_id<nullptr_t>();

        return iter->second->type_info();
    }

    /**
     * @brief Gets the type info of serializable component from the given runtime component type info.
     */
    nodec::type_info get_serializable_component_type_info(const nodec::type_info &component_type_info) const {
        auto iter = component_dict_.find(component_type_info.seq_index());
        if (iter == component_dict_.end()) return nodec::type_id<nullptr_t>();

        return iter->second->serializable_type_info();
    }

private:
    std::unordered_map<nodec::type_seq_index_type, std::shared_ptr<BaseComponentSerialization>> component_dict_;
    std::unordered_map<nodec::type_seq_index_type, std::shared_ptr<BaseComponentSerialization>> serializable_component_dict_;
};

} // namespace nodec_scene_serialization

#endif
