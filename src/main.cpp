#include <thread>
#include "modloader/shared/modloader.hpp"
#include "custom-types/shared/register.hpp"
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
bool dashModeEnabled = false;
float thrust = 45000.0F;
bool recharged = false;

MAKE_HOOK_OFFSETLESS(PhotonNetworkController_OnJoinedRoom, void, Il2CppObject* self)
{
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
    if(!isRoom) {
        fist = false; return;
    }

    using namespace GlobalNamespace;
    bool AInput = false;
	bool BInput = false;
	bool XInput = false;
    bool YInput = false;
	bool startInput = false;
    bool leftGripInput = false;
    bool rightGripInput = false;
    bool leftTriggerInput = false;
    bool rightTriggerInput = false;
    bool rightThumbstick = false;
	//Remove whatever inputs you dont need
    AInput = OVRInput::Get(OVRInput::Button::One, OVRInput::Controller::RTouch);
    BInput = OVRInput::Get(OVRInput::Button::Two, OVRInput::Controller::RTouch);
    XInput = OVRInput::Get(OVRInput::Button::One, OVRInput::Controller::LTouch);
    YInput = OVRInput::Get(OVRInput::Button::Two, OVRInput::Controller::LTouch);
    startInput = OVRInput::Get(OVRInput::Button::Start, OVRInput::Controller::LTouch);
    leftGripInput = OVRInput::Get(OVRInput::Button::PrimaryHandTrigger, OVRInput::Controller::LTouch);
    rightGripInput = OVRInput::Get(OVRInput::Button::PrimaryHandTrigger, OVRInput::Controller::RTouch);
    leftTriggerInput = OVRInput::Get(OVRInput::Button::PrimaryIndexTrigger, OVRInput::Controller::LTouch);
    rightTriggerInput = OVRInput::Get(OVRInput::Button::PrimaryIndexTrigger, OVRInput::Controller::RTouch);
    rightThumbstick = OVRInput::Get(OVRInput::Button::PrimaryThumbstick, OVRInput::Controller::RTouch);

    if (isRoom)
    {
        // If you want just a single button press then do if (AInput) replace AInput with whatever button you want
        if (rightThumbstick)
        {
            INFO("Dash on");
            fist = true;
        }
        // For checking if a specific button is not being pressed then do if (!AInput)
        else
        {
            INFO("Dash off");
            fist = false;
        }
    }
    else return;
}

    #include "GlobalNamespace/GorillaTagManager.hpp"

    MAKE_HOOK_OFFSETLESS(Player_GetSlidePercentage, /*this is the check for hands touching ground*/  float, Il2CppObject* self, RaycastHit raycastHit) {

        if(isRoom && !recharged) {
            recharged = true;
        }

        return Player_GetSlidePercentage(self, raycastHit);
    }//this is the code in which it only activates when you are touching the ground, but i cant get it to function properly, for instance when you're in the air, itll still be active

    MAKE_HOOK_OFFSETLESS(GorillaTagManager_Update, void, GlobalNamespace::GorillaTagManager* self) {

        if(!isRoom) {
            dashModeEnabled = false; return;
        }

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

        Transform* turnParent = playerGameObject->get_transform()->GetChild(0);

        Transform* mainCamera = turnParent->GetChild(0);

        Vector3 velocityForward = mainCamera->get_forward();

        Vector3 lookingAt = velocityForward;

        Transform* rightHandT = player->rightHandTransform;

        if(isRoom) {
            if(recharged) {
                if(fist) {
                    if(dashModeEnabled) {
                        dashModeEnabled = false;
                        playerPhysics->set_useGravity(false);
                        playerPhysics->AddForce(rightHandT->get_forward() * thrust);
                        recharged = false;
                    } else if(!dashModeEnabled){
                        dashModeEnabled = true;
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
}

extern "C" void setup(ModInfo& info)
{
    info.id = ID;
    info.version = VERSION;

    modInfo = info;
}

extern "C" void load()
{
    getLogger().info("Loading mod...");

    INSTALL_HOOK_OFFSETLESS(getLogger(), Player_GetSlidePercentage, il2cpp_utils::FindMethodUnsafe("GorillaLocomotion", "Player", "GetSlidePercentage", 1));
    INSTALL_HOOK_OFFSETLESS(getLogger(), PhotonNetworkController_OnJoinedRoom, il2cpp_utils::FindMethodUnsafe("", "PhotonNetworkController", "OnJoinedRoom", 0));
	INSTALL_HOOK_OFFSETLESS(getLogger(), Player_Update, il2cpp_utils::FindMethodUnsafe("GorillaLocomotion", "Player", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), GorillaTagManager_Update, il2cpp_utils::FindMethodUnsafe("", "GorillaTagManager", "Update", 0));

    getLogger().info("Mod loaded!");
}