#pragma once

#include "UIScene.h"

class UIScene_ModsMenu : public UIScene
{
public:
    UIScene_ModsMenu(int iPad, void* initData, UILayer* parentLayer);
    virtual ~UIScene_ModsMenu();

    virtual EUIScene getSceneType() override { return eUIScene_ModsMenu; }

    virtual void tick()         override;
    virtual void handleReload() override {}
    virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool& handled) override;
    void ShowModInfoInWindow(int index);

protected:
    virtual void handlePress(F64 controlId, F64 childId) override;
    virtual void handleGainFocus(bool navBack) override;

public:
    bool m_bWindowClosed;

private:
    enum EControls
    {
        eControl_ModList = 0,
        eControl_Back,
    };

    UIControl_ButtonList  m_modList;
    UIControl_Button      m_btnBack;

    UIControl             m_infoPanel;
    UIControl_Label       m_labelName;
    UIControl_Label       m_labelID;
    UIControl_Label       m_labelAuthor;
    UIControl_Label       m_labelVersion;
    UIControl_Label       m_labelDescription;
    UIControl_Label       m_labelNoMods;

    int m_selectedIndex;

    void PopulateModList();
    void ShowModInfo(int index);
    void ClearModInfo();

    virtual wstring getMoviePath() override;

    UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
        UI_MAP_ELEMENT(m_modList, "ModList")
        UI_MAP_ELEMENT(m_btnBack, "BtnBack")
        UI_MAP_ELEMENT(m_labelNoMods, "InfoNoMods")
        UI_MAP_ELEMENT(m_infoPanel, "InfoPanel")
        UI_BEGIN_MAP_CHILD_ELEMENTS(m_infoPanel)
        UI_MAP_ELEMENT(m_labelName, "InfoName")
        UI_MAP_ELEMENT(m_labelID, "InfoID")
        UI_MAP_ELEMENT(m_labelAuthor, "InfoAuthor")
        UI_MAP_ELEMENT(m_labelVersion, "InfoVersion")
        UI_MAP_ELEMENT(m_labelDescription, "InfoDescription")
        UI_END_MAP_CHILD_ELEMENTS()
        UI_END_MAP_ELEMENTS_AND_NAMES()
};