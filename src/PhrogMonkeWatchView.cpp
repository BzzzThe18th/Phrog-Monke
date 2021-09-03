
#include "PhrogMonkeWatchView.hpp"
#include "config.hpp"
#include "monkecomputer/shared/ViewLib/MonkeWatch.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"

DEFINE_TYPE(PhrogMonke, PhrogMonkeWatchView);

using namespace GorillaUI;
using namespace UnityEngine;

extern bool isRoom;

namespace PhrogMonke
{
    void PhrogMonkeWatchView::Awake()
    {
        toggleHandler = new UIToggleInputHandler(EKeyboardKey::Enter, EKeyboardKey::Enter, true);
    }

    void PhrogMonkeWatchView::DidActivate(bool firstActivation)
    {
        std::function<void(bool)> fun = std::bind(&PhrogMonkeWatchView::OnToggle, this, std::placeholders::_1);
        toggleHandler->toggleCallback = fun;
        Redraw();
    }

    void PhrogMonkeWatchView::Redraw()
    {
        text = "";

        DrawHeader();
        DrawBody();

        watch->Redraw();
    }

    void PhrogMonkeWatchView::DrawHeader()
    {
        text += "<color=#136CFF>== <color=#FF2F3C>Phrog Monke</color> ==</color>\n";
    }

    void PhrogMonkeWatchView::DrawBody()
    {
        text += "\nPhrog Monke is:\n\n  ";
        text += config.enabled ? "<color=#00ff00>> Enabled</color>" : "<color=#ff0000>> Disabled</color>";

        if (config.enabled && !isRoom)
        {
            text += "\n\nBut is disabled\ndue to not being in\na private room\n";
        }
    }

    void PhrogMonkeWatchView::OnToggle(bool value)
    {
        config.enabled = value;

        SaveConfig();
    }

    void PhrogMonkeWatchView::OnKeyPressed(int key)
    {
        toggleHandler->HandleKey((EKeyboardKey)key);
        Redraw();
    }
}