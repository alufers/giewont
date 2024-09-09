using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("giewont::net");

@0xa67dd58fb8f636a0;
struct BaseNetMessage {
    union {
        dummy @0 :Void;
        loadLevel @1 :LoadLevelNetMessage; # server -> client
        levelLoaded @2 :Void; # client -> server
        syncEntity @3 :SyncEntityNetMessage;
        setCameraFollowedEntity @4 :SetCameraFollowedEntityNetMessage; # server -> client
        interact @5 :InteractNetMessage; # client -> server
        destroyEntity @6 :DestroyEntityNetMessage; # server -> client
        addCameraEffect @7 :AddCameraEffectNetMessage; # server -> client
    }
}

struct LoadLevelNetMessage {
    levelName @0 :Text;
    yourPeerId @1 :UInt32;
}

enum EntityType {
    unknown @0;
    character @1;
    flag @2;
    teamBase @3;
    gameplayManager @4;
}

struct SyncEntityNetMessage {
    isFirstSync @0 :Bool; # not actually set by the rmeote peer, but by the client
    netId @1 :UInt32;
    netOwnerId @2 :UInt32;
    entityType @3 :EntityType;
    position @4 :NetVec2;
    velocity @5 :NetVec2;

    extraData :union {
        characterData @6 :SyncCharacterEntityData;
        flagData @7 :SyncFlagEntityData;
        teamBaseData @8 :SyncTeamBaseEntityData;
        gameplayManagerData @9 :GameplayManagerData;
    }
}

struct SyncFlagEntityData {
    holderNetId @0 :UInt32;
    color @1 :UInt32;
    team @2 :UInt32;
}

struct SyncTeamBaseEntityData {
    color @0 :UInt32;
    team @1 :UInt32;
}

struct SyncCharacterEntityData {
    animationState @0 :UInt32;
    direction @1 :UInt32;
}

struct GameplayManagerData {
    blueTeamScore @0 :UInt32;
    redTeamScore @1 :UInt32;
    message @2 :Text;
    messageTime @3 :Float32;
}

struct NetVec2 {
    x @0 :Float32;
    y @1 :Float32;
}

struct SetCameraFollowedEntityNetMessage {
    netId @0 :UInt32;
}

enum CameraEffectType {
    none @0;
    shake @1;
    vignette @2;
}

struct AddCameraEffectNetMessage {
    type @0 :CameraEffectType;
    intensity @1 :Float32;
    duration @2 :Float32;
    falloffDuration @3 :Float32;
    color @4 :UInt32;
    speed @5 :Float32;
}

struct InteractNetMessage {
   interactorNetId @0 :Int32;
}

struct DestroyEntityNetMessage {
    netId @0 :UInt32;
}   
