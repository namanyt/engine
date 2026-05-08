#include "runtime/MenuRuntime.h"

#include "Application.h"
#include "assets/TextureAsset.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"
#include "core/Log.h"
#include "metassets/MenuScene.metasset.h"
#include "runtime/ExplorationRuntime.h"
#include "runtime/LoadingRuntime.h"
#include "scenes/MenuScene.h"

#include <cmath>

namespace engine
{
MenuRuntime::MenuRuntime() = default;

MenuRuntime::~MenuRuntime() = default;

const char* MenuRuntime::name() const
{
    return "MenuRuntime";
}

void MenuRuntime::activate(ActivationContext& activationContext)
{
    m_sceneMetasset = std::make_unique<MenuSceneMetasset>();
    m_scene = std::make_unique<MenuScene>(*m_sceneMetasset);

    activationContext.renderer.prepareOverlayRenderingResources();

    SceneRuntime::AssetScope assetScope{
        activationContext.assetManager, activationContext.shaderLibrary,
        activationContext.assetRootDirectory, activationContext.shaderDirectory};
    m_scene->activate(assetScope);
    activationContext.application.setCursorCaptured(false);
    applyOverlayTexture(activationContext.renderer);
    Log::info("MenuRuntime", "Activated menu runtime.");
}

void MenuRuntime::deactivate(Renderer& renderer)
{
    if (m_scene != nullptr)
    {
        m_scene->deactivate(renderer);
        m_scene.reset();
    }

    m_sceneMetasset.reset();
}

void MenuRuntime::update(const UpdateContext& updateContext)
{
    if (m_scene == nullptr)
    {
        return;
    }

    const MenuInputState inputState = interpretInput(updateContext.inputState);

    if (inputState.navigatePrevious || inputState.navigateNext)
    {
        m_selectedAction = m_selectedAction == Selection::StartExploration
                               ? Selection::Quit
                               : Selection::StartExploration;
    }

    if (inputState.cancel)
    {
        updateContext.application.requestQuit();
        return;
    }

    if (!inputState.confirm)
    {
        return;
    }

    if (m_selectedAction == Selection::StartExploration)
    {
        requestTransition(std::make_unique<LoadingRuntime>(std::make_unique<ExplorationRuntime>(),
                                                           0.35f, "ExplorationRuntime"));
        return;
    }

    updateContext.application.requestQuit();
}

void MenuRuntime::render(const RenderContext& renderContext)
{
    if (m_scene == nullptr)
    {
        return;
    }

    renderContext.renderer.setViewport(renderContext.framebufferWidth,
                                       renderContext.framebufferHeight);
    renderContext.renderPipeline.renderOverlayFrame(activeClearColor(renderContext.timeSeconds));
}

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
void MenuRuntime::drawDebugUi(const DebugUiContext& debugUiContext)
{
    (void)debugUiContext;
}
#endif

MenuRuntime::MenuInputState MenuRuntime::interpretInput(const RawInputState& inputState) const
{
    MenuInputState menuInput{};
    menuInput.navigatePrevious = inputState.keyUpArrow.pressed;
    menuInput.navigateNext = inputState.keyDownArrow.pressed;
    menuInput.confirm = inputState.keyEnter.pressed;
    menuInput.cancel = inputState.keyEscape.pressed;
    return menuInput;
}

Color MenuRuntime::activeClearColor(float timeSeconds) const
{
    const Color baseColor =
        m_scene != nullptr ? m_scene->clearColor() : Color{0.04f, 0.06f, 0.10f, 1.0f};
    const float pulse = 0.5f + 0.5f * std::sin(timeSeconds * 1.35f);
    if (m_selectedAction == Selection::StartExploration)
    {
        return Color{baseColor.r + 0.05f * pulse, baseColor.g + 0.04f * pulse,
                     baseColor.b + 0.08f * pulse, 1.0f};
    }

    return Color{baseColor.r + 0.04f * pulse, baseColor.g + 0.01f * pulse,
                 baseColor.b + 0.01f * pulse, 1.0f};
}

void MenuRuntime::applyOverlayTexture(Renderer& renderer) const
{
    if (m_scene == nullptr || m_scene->overlayTexture() == nullptr)
    {
        renderer.clearRuntimeOverlayTexture();
        return;
    }

    renderer.setRuntimeOverlayTexture(m_scene->overlayTexture()->textureId(),
                                      m_scene->overlayTexture()->width(),
                                      m_scene->overlayTexture()->height());
}
} // namespace engine
