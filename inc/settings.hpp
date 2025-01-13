#pragma once

#include "json_file.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct SettingsData {
public:
    // Window settings
    std::string WindowTitle;
    int WindowWidth, WindowHeight;
    int WindowPositionX, WindowPositionY;
    bool FullScreen, UseDeferredShading, ShowDebugInfo;

    // Shaders
    std::string ForwardShadingVertexShaderFile, ForwardShadingFragmentShaderFile;
    std::string DeferredShadingFirstPassVertexShaderFile, DeferredShadingFirstPassFragmentShaderFile;
    std::string DeferredShadingSecondPassVertexShaderFile, DeferredShadingSecondPassFragmentShaderFile;

    // Text renderer settings
    std::string FontFile;
    int FontSize;
    std::string TextVertexShaderFile, TextFragmentShaderFile;

    // Level settings
    std::string LevelMapFile, LevelTextureFile;

    // Weapon settings
    std::string LeftWeaponModelFile, LeftWeaponTextureFile;
    glm::vec3 LeftWeaponPositionOffset, LeftWeaponRotationOffset, LeftWeaponScale;
    std::string RightWeaponModelFile, RightWeaponTextureFile;
    glm::vec3 RightWeaponPositionOffset, RightWeaponRotationOffset, RightWeaponScale;

    // Lighting settings
    glm::vec3 TorchColor;
    float TorchInnerCutoff, TorchOuterCutoff;
    float TorchAttenuationConstant, TorchAttenuationLinear, TorchAttenuationQuadratic;
    glm::vec3 AmbientColor;
    float AmbientIntensity, SpecularShininess, SpecularIntensity;
    float AttenuationConstant, AttenuationLinear, AttenuationQuadratic;

    // Audio settings
    std::string AmbientMusicFile;
    std::vector<std::string> FootstepsSoundFiles;
};

inline SettingsData LoadSettingsFile(const std::string& path)
{
    SettingsData settings;

    JsonFile& json = JsonFile::GetInstance();
    json.Load(path);

    settings.WindowTitle = json.GetNested<std::string>("window.title");
    settings.WindowWidth = json.GetNested<int>("window.width");
    settings.WindowHeight = json.GetNested<int>("window.height");
    settings.FullScreen = json.GetNested<bool>("window.fullScreen");
    settings.ShowDebugInfo = json.GetNested<bool>("window.showDebugInfo");

    settings.UseDeferredShading = json.GetNested<bool>("renderer.useDeferredShading");
    settings.ForwardShadingVertexShaderFile = json.GetNested<std::string>("renderer.forwardSinglePass.shaders.vertex");
    settings.ForwardShadingFragmentShaderFile = json.GetNested<std::string>("renderer.forwardSinglePass.shaders.fragment");
    settings.DeferredShadingFirstPassVertexShaderFile = json.GetNested<std::string>("renderer.deferredFirstPass.shaders.vertex");
    settings.DeferredShadingFirstPassFragmentShaderFile = json.GetNested<std::string>("renderer.deferredFirstPass.shaders.fragment");
    settings.DeferredShadingSecondPassVertexShaderFile = json.GetNested<std::string>("renderer.deferredSecondPass.shaders.vertex");
    settings.DeferredShadingSecondPassFragmentShaderFile = json.GetNested<std::string>("renderer.deferredSecondPass.shaders.fragment");

    settings.FontFile = json.GetNested<std::string>("textRenderer.fontFile");
    settings.FontSize = json.GetNested<int>("textRenderer.fontSize");
    settings.TextVertexShaderFile = json.GetNested<std::string>("textRenderer.shaders.vertex");
    settings.TextFragmentShaderFile = json.GetNested<std::string>("textRenderer.shaders.fragment");

    settings.LevelMapFile = json.GetNested<std::string>("level.mapFile");
    settings.LevelTextureFile = json.GetNested<std::string>("level.textureFile");

    settings.LeftWeaponModelFile = json.GetNested<std::string>("weapons.left.modelFile");
    settings.LeftWeaponTextureFile = json.GetNested<std::string>("weapons.left.textureFile");
    settings.LeftWeaponPositionOffset = json.GetNested<glm::vec3>("weapons.left.positionOffset");
    settings.LeftWeaponRotationOffset = json.GetNested<glm::vec3>("weapons.left.rotationOffset");
    settings.LeftWeaponScale = json.GetNested<glm::vec3>("weapons.left.scale");
    settings.RightWeaponModelFile = json.GetNested<std::string>("weapons.right.modelFile");
    settings.RightWeaponTextureFile = json.GetNested<std::string>("weapons.right.textureFile");
    settings.RightWeaponPositionOffset = json.GetNested<glm::vec3>("weapons.right.positionOffset");
    settings.RightWeaponRotationOffset = json.GetNested<glm::vec3>("weapons.right.rotationOffset");
    settings.RightWeaponScale = json.GetNested<glm::vec3>("weapons.right.scale");

    settings.TorchColor = json.GetNested<glm::vec3>("lighting.torch.color");
    settings.TorchInnerCutoff = json.GetNested<float>("lighting.torch.innerCutoff");
    settings.TorchOuterCutoff = json.GetNested<float>("lighting.torch.outerCutoff");
    settings.TorchAttenuationConstant = json.GetNested<float>("lighting.torch.attenuation.constant");
    settings.TorchAttenuationLinear = json.GetNested<float>("lighting.torch.attenuation.linear");
    settings.TorchAttenuationQuadratic = json.GetNested<float>("lighting.torch.attenuation.quadratic");
    settings.AmbientColor = json.GetNested<glm::vec3>("lighting.ambient.color");
    settings.AmbientIntensity = json.GetNested<float>("lighting.ambient.intensity");
    settings.SpecularShininess = json.GetNested<float>("lighting.specular.shininess");
    settings.SpecularIntensity = json.GetNested<float>("lighting.specular.intensity");
    settings.AttenuationConstant = json.GetNested<float>("lighting.attenuation.constant");
    settings.AttenuationLinear = json.GetNested<float>("lighting.attenuation.linear");
    settings.AttenuationQuadratic = json.GetNested<float>("lighting.attenuation.quadratic");

    settings.AmbientMusicFile = json.GetNested<std::string>("audio.ambientMusicFile");
    settings.FootstepsSoundFiles = json.GetNested<std::vector<std::string>>("audio.footstepsSoundFiles");

    return settings;
}