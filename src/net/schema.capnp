using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("giewont::net");

@0xa67dd58fb8f636a0;
struct BaseNetMessage {
    union {
        dummy @0 :Void;
        loadLevel @1 :LoadLevelNetMessage; # server -> client
        levelLoaded @2 :Void; # client -> server
        syncEntity @3 :SyncEntityNetMessage;
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

}

struct NetVec2 {
    x @0 :Float32;
    y @1 :Float32;
}
