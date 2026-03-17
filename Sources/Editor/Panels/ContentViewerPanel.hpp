#pragma once

#include "Editor/Panel.hpp"
#include <Termina/Asset/Material/MaterialAsset.hpp>
#include <Termina/Asset/AssetHandle.hpp>
#include <filesystem>
#include <string>

class ContentViewerPanel : public Panel
{
public:
    ContentViewerPanel(EditorContext& context)
        : Panel("Content Viewer", context)
        , m_CurrentPath("Assets") {}

    void OnImGuiRender() override;

    const std::filesystem::path& GetCurrentPath() const { return m_CurrentPath; }

private:
    void Navigate(const std::filesystem::path& path);

    std::filesystem::path m_CurrentPath;
    std::string m_SelectedEntry;

    // History
    std::vector<std::filesystem::path> m_BackHistory;
    std::vector<std::filesystem::path> m_ForwardHistory;

    // Modal state
    bool        m_ShowNewFolderModal  = false;
    bool        m_ShowRenameModal     = false;
    bool        m_ShowMoveModal       = false;
    char        m_ModalInputBuf[256]  = {};
    std::string m_ModalTargetPath;   // path being renamed/moved

    // Keep the currently-inspected material asset alive
    Termina::AssetHandle<Termina::MaterialAsset> m_InspectedMaterial;

    const char* GetFileIcon(const std::filesystem::path& path) const;

    void DrawContextMenuBackground();
    void DrawContextMenuEntry(const std::string& entryPath, bool isDir);
    void DrawModals();

    void CreateDefaultMaterial(const std::filesystem::path& dir);
    void CreateDefaultWorld(const std::filesystem::path& dir);
    static std::string UniquePath(const std::filesystem::path& path);
};
