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
float thrust = 45000.0F;
bool recharged = false;

MAKE_HOOK_OFFSETLESS(PhotonNetworkController_OnJoinedRoom, void, Il2CppObject* self)
{
    INFO("Checking if private BUZZ");
    
    PhotonNetworkController_OnJoinedRoom(self);

    Il2CppObject* currentRoom = CRASH_UNLESS(il2cpp_utils::RunMethod("Photon.Pun", "PhotonNetwork", "get_CurrentRoom"));

    if (currentRoom)
    {
        isRoom = !CRASH_UNLESS(il2cpp_utils::RunMethod<bool>(currentRoom, "get_IsVisible"));
    }
    else isRoom = true;
    
}

void UpdateButton()
{
    INFO("Checking for right thumbstick press");

    using namespace GlobalNamespace;
    bool rightThumbstick = false;

    rightThumbstick = OVRInput::Get(OVRInput::Button::PrimaryThumbstick, OVRInput::Controller::RTouch);

    if (isRoom)
    {
        if (rightThumbstick)
        {
            INFO("Dash on");
            fist = true;
        }
        else
        {
            INFO("Dash off");
            fist = false;
        }
    }
    else return;
}

    #include "GlobalNamespace/GorillaTagManager.hpp"

    MAKE_HOOK_OFFSETLESS(Player_GetSlidePercentage, float, Il2CppObject* self, RaycastHit raycastHit) {
        INFO("Checking for hands on surface BUZZ");

        if(isRoom && !recharged) {
            recharged = true;
        }

        return Player_GetSlidePercentage(self, raycastHit);
    }

    MAKE_HOOK_OFFSETLESS(GorillaTagManager_Update, void, GlobalNamespace::GorillaTagManager* self) {
        INFO("Running GTManager hook BUZZ");

        using namespace GlobalNamespace;
        using namespace GorillaLocomotion;

        Player* playerInstance = Player::get_Instance();
        if(playerInstance == nullptr) return;
        GameObject* go = playerInstance->get_gameObject();
        auto* player = go->GetComponent<GorillaLocomotion::Player*>();

        Rigidbody* playerPhysics = playerInstance->playerRigidBody;
        if(playerPhysics == nullptr) return;

        GameObject* playerGameObject = playerPhysics->get_gameObject();
        if(playerGameObject == nullptr) return;

        Transform* rightHandT = player->rightHandTransform;
        if(isRoom) {
            INFO("In private room BUZZ");
            if(recharged) {
                INFO("Is recharged BUZZ");
                if(fist) {
                    INFO("Clicking thumbstick BUZZ");
                    if(phrogModeEnabled) {
                        phrogModeEnabled = false;
                        playerPhysics->set_useGravity(false);
                        playerPhysics->AddForce(rightHandT->get_forward() * thrust);
                        INFO("Attempted leap BUZZ");
                        recharged = false;
                    } else if(!phrogModeEnabled){
                        phrogModeEnabled = true;
                    }
                }
            } else if(!recharged) {
                playerPhysics->set_useGravity(true);
            }
        }
    }

MAKE_HOOK_OFFSETLESS(Player_Update, void, Il2CppObject* self)
{
    using namespace UnityEngine;
    using namespace GlobalNamespace;
    INFO("player update was called");
    Player_Update(self);
    UpdateButton();
    OVRInput::Update();
    OVRInput::FixedUpdate();
}

extern "C" void setup(ModInfo& info)
{
    info.id = ID;
    info.version = VERSION;

    modInfo = info;
}

extern "C" void load()
{
    getLogger().info("Loading phrog monke BUZZ...");

    INSTALL_HOOK_OFFSETLESS(getLogger(), Player_GetSlidePercentage, il2cpp_utils::FindMethodUnsafe("GorillaLocomotion", "Player", "GetSlidePercentage", 1));
    INSTALL_HOOK_OFFSETLESS(getLogger(), PhotonNetworkController_OnJoinedRoom, il2cpp_utils::FindMethodUnsafe("", "PhotonNetworkController", "OnJoinedRoom", 0));
	INSTALL_HOOK_OFFSETLESS(getLogger(), Player_Update, il2cpp_utils::FindMethodUnsafe("GorillaLocomotion", "Player", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), GorillaTagManager_Update, il2cpp_utils::FindMethodUnsafe("", "GorillaTagManager", "Update", 0));

    getLogger().info("Phrog monke loaded BUZZ!");
}