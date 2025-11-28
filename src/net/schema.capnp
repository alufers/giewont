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
        hurtEntity @8 :HurtEntityNetMessage; # server -> client (sent only in case the client is the owner of that entity)
        instantiatePrefab @9 :InstantiatePrefabNetMessage; # server -> client
        applyPhysicsImpulse @10 :ApplyPhysicsImpulseNetMessage; # server -> client
        guiInteraction @11 :GuiInteractionNetMessage; # client -> server
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
    grenade @5;
    teamChoiceGUI @6;
    tombstone @7;
    projectile @8;
    bonus @9;
}

struct SyncEntityNetMessage {
    isFirstSync @0 :Bool; # not actually set by the remote peer, but by the client
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
        grenadeData @10 :GrenadeData;
        tombstoneData @11 :SyncTombstoneEntityData;
        projectileData @12 :SyncProjectileEntityData;
        bonusData @13 :SyncBonusEntityData;
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
    maxHealth @2 :UInt32;
    health @3 :UInt32;
    immunityTime @4 :Float32;
    team @5 :UInt32;
    name @6 :Text;
}

struct GameplayManagerData {
    blueTeamScore @0 :UInt32;
    redTeamScore @1 :UInt32;
    message @2 :Text;
    messageTime @3 :Float32;
}

struct GrenadeData {
    fuseTotalTime @0 :Float32;
    fuseTimeLeft @1 :Float32;
}

struct SyncTombstoneEntityData {
    totalLifetime @0 :Float32;
    lifetime @1 :Float32;
    fadeOutTime @2 :Float32;
    fadingOut @3 :Bool;
    deadPlayerPeerId @4 :UInt32;
}

struct SyncProjectileEntityData {
    lifetime @0 :Float32;
    lifetimeLeft @1 :Float32;
}

enum BonusEntityType {
    healthkit @0;
}

struct SyncBonusEntityData {
    bonusType @0 :BonusEntityType;
    lifetime @1 :Float32;
    totalLifetime @2 :Float32;
    amount @3 :UInt32; # For healthkit, this is the amount of health it restores
}


struct InstantiatePrefabNetMessage {
    prefabPath @0 :Text;
    position @1 :NetVec2;
}

struct ApplyPhysicsImpulseNetMessage {
    netId @0 :UInt32;
    impulse @1 :NetVec2;
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
    clearOthers @6 :Bool; # if true, clear all other effects
}

enum InteractionType {
    useInteraction @0;
    primaryClick @1;
    secondaryClick @2;
    throwInteraction @3;
    aiNotifyFlagStuck @4; # Used by AI characters when it cannot pathfind to the enemy base while holding the flag
}


struct InteractNetMessage {
   interactorNetId @0 :Int32;
   characterToMouseOffset @1 :NetVec2;
   type @2 :InteractionType;
   targetHint :union {
         none @3 :Void;
         netId @4 :UInt32; # Net ID of the entity to interact with
   }
}



struct DestroyEntityNetMessage {
    netId @0 :UInt32;
}   

enum HurtReason {
    unknown @0;
    environment @1;
    projectile @2;
    explosion @3;
}

struct HurtEntityNetMessage {
    netId @0 :UInt32;
    damage @1 :Int32;
    reason @2 :HurtReason;
}

struct GuiInteractionNetMessage {
    netId @0 :UInt32; # Net ID of the GUI element / entity
    name @1 :Text;
}

# Contains global variables which affect the gameplay and are used by multiple subsystems.
# For example both the flag entity and the AI must know how close to the flag the AI should be to pick it up.
struct GameplayVariable {
  union {
    gravity @0 :NetVec2;
    entityInteractionRange @1 :Float32;
    baseActivationDist @2 :Float32; # The range at which a flag is considered being captured by a base.
    gameWinPoints @3 :UInt32;
    teamMaxPlayers @4 :UInt32;
    teamMinPlayers @5 :UInt32;
    
    max @6 :Void; # Sentinel value
  }
}

