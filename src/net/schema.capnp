using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("giewont::net");

@0xa67dd58fb8f636a0;
struct BaseNetMessage {
    union {
        dummy @0 :Void;
        loadLevel @1 :LoadLevelNetMessage; # server -> client
        levelLoaded @2 :Void; # client -> server
        syncEntity @3 :SyncEntityNetMessage;
        setCameraFollowedEntity @4 :SetCameraFollowedEntityNetMessage;
    }
}

struct LoadLevelNetMessage {
    levelName @0 :Text;
    yourPeerId @1 :UInt32;
}

enum EntityType {
    unknown @0;
    character @1;
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

        dummy @7 :Void;
    }
}

struct SyncCharacterEntityData {
    animationState @0 :UInt32;
    direction @1 :UInt32;

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

struct CameraEffect {
    type @0 :CameraEffectType;
    intensity @1 :Float32;
    duration @2 :Float32;
    falloffDuration @3 :Float32;
}
