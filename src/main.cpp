#include <thread>
#include "modloader/shared/modloader.hpp"
#include "GorillaLocomotion/Player.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils-functions.h"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils-methods.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "GlobalNamespace/OVRInput.hpp"
#include "GlobalNamespace/OVRInput_Button.hpp"
#include "gorilla-utils/shared/GorillaUtils.hpp"
#include "gorilla-utils/shared/CustomProperties/Player.hpp"
#include "gorilla-utils/shared/Utils/Player.hpp"
#include "gorilla-utils/shared/Callbacks/InRoomCallbacks.hpp"
#include "gorilla-utils/shared/Callbacks/MatchMakingCallbacks.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/ForceMode.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Rigidbody.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/CapsuleCollider.hpp"
#include "UnityEngine/SphereCollider.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RaycastHit.hpp"
#include "UnityEngine/XR/InputDevice.hpp"
#include "PhrogMonkeWatchView.hpp"
#include "config.hpp"
#include "monkecomputer/shared/GorillaUI.hpp"
#include "monkecomputer/shared/Register.hpp"
#include "custom-types/shared/register.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

ModInfo modInfo;

#define INFO(value...) getLogger().info(value)
#define ERROR(value...) getLogger().error(value)

using namespace UnityEngine;
using namespace UnityEngine::XR;
using namespace GorillaLocomotion;

Logger& getLogger()
{
    static Logger* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

bool isRoom = false;
bool fist = false;
bool phrogModeEnabled = false;
float thrust = 45000;
bool recharged = false;

void UpdateButton()
{

    using namespace GlobalNamespace;
    bool rightThumbstick = false;

    rightThumbstick = OVRInput::Get(OVRInput::Button::PrimaryThumbstick, OVRInput::Controller::RTouch);

    if (isRoom && config.enabled)
    {
        if (rightThumbstick)
        {
            fist = true;
        }
        else
        {
            fist = false;
        }
    }
    else return;
}

#include "GlobalNamespace/GorillaTagManager.hpp"

MAKE_HOOK_MATCH(Player_GetSlidePercentage, &GorillaLocomotion::Player::GetSlidePercentage, float, Player* self, UnityEngine::RaycastHit ray) {

    if(isRoom && !recharged && config.enabled) {
        recharged = true;
    }

    return Player_GetSlidePercentage(self, ray);
}

MAKE_HOOK_MATCH(GorillaTagManager_Update, &GlobalNamespace::GorillaTagManager::Update, void, GlobalNamespace::GorillaTagManager* self) {
    using namespace GlobalNamespace;
    using namespace GorillaLocomotion;
    GorillaTagManager_Update(self);
    UpdateButton();

    Player* playerInstance = Player::get_Instance();
    if(playerInstance == nullptr) return;
    GameObject* go = playerInstance->get_gameObject();
    auto* player = go->GetComponent<GorillaLocomotion::Player*>();

    Rigidbody* playerPhysics = playerInstance->playerRigidBody;
    if(playerPhysics == nullptr) return;

    GameObject* playerGameObject = playerPhysics->get_gameObject();
    if(playerGameObject == nullptr) return;

    Transform* rightHandT = player->rightHandTransform;
    if(isRoom && config.enabled) {
        if(recharged) {
            if(fist) {
                if(phrogModeEnabled) {
                    phrogModeEnabled = false;
                    playerPhysics->set_useGravity(false);
                    playerPhysics->AddForce(rightHandT->get_forward() * thrust);
                    recharged = false;
                } else if(!phrogModeEnabled){
                    phrogModeEnabled = true;
                }
            }
        } else if(!recharged) {
            playerPhysics->set_useGravity(true);
        }
    } else {
        playerPhysics->set_useGravity(true);
    }
}

MAKE_HOOK_MATCH(Player_Awake, &GorillaLocomotion::Player::Awake, void, GorillaLocomotion::Player* self) {
    Player_Awake(self);

    GorillaUtils::MatchMakingCallbacks::onJoinedRoomEvent() += {[&]() {
        Il2CppObject* currentRoom = CRASH_UNLESS(il2cpp_utils::RunMethod("Photon.Pun", "PhotonNetwork", "get_CurrentRoom"));

        if (currentRoom)
        {
            isRoom = !CRASH_UNLESS(il2cpp_utils::RunMethod<bool>(currentRoom, "get_IsVisible"));
        } else isRoom = true;
        if(!isRoom) {
            fist = false;
            phrogModeEnabled = false;
            recharged = false;
        }
    }
    };
}

extern "C" void setup(ModInfo& info)
{
    info.id = ID;
    info.version = VERSION;

    modInfo = info;
}

extern "C" void load()
{
    GorillaUI::Innit();

    INSTALL_HOOK(getLogger(), Player_Awake)
    INSTALL_HOOK(getLogger(), Player_GetSlidePercentage);
    INSTALL_HOOK(getLogger(), GorillaTagManager_Update);

    custom_types::Register::AutoRegister();

    GorillaUI::Register::RegisterWatchView<PhrogMonke::PhrogMonkeWatchView*>("Phrog Monke", VERSION);
}