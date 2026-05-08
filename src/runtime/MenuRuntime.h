#pragma once

#include "math/Types.h"
#include "runtime/RuntimeMode.h"

#include <memory>

namespace engine
{
class MenuScene;
class SceneMetasset;

class MenuRuntime final : public RuntimeMode
{
  public:
    MenuRuntime();
    ~MenuRuntime() override;

    const char* name() const override;
    void activate(ActivationContext& activationContext) override;
    void deactivate(Renderer& renderer) override;
    void update(const UpdateContext& updateContext) override;
    void render(const RenderContext& renderContext) override;

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    void drawDebugUi(const DebugUiContext& debugUiContext) override;
#endif

  private:
    enum class Selection
    {
        StartExploration,
        Quit,
    };

    struct MenuInputState final
    {
        bool navigatePrevious = false;
        bool navigateNext = false;
        bool confirm = false;
        bool cancel = false;
    };

    MenuInputState interpretInput(const RawInputState& inputState) const;
    Color activeClearColor(float timeSeconds) const;
    void applyOverlayTexture(Renderer& renderer) const;

    std::unique_ptr<SceneMetasset> m_sceneMetasset;
    std::unique_ptr<MenuScene> m_scene;
    Selection m_selectedAction = Selection::StartExploration;
};
} // namespace engine
