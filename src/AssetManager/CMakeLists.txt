# Code Modification - Build System: Keep PhysicsFS private to the asset filesystem foundation.
# Scope: AssetManager target. Performance: no work until a VFS session opens. Toggle: caller-owned lifetime.
add_library(AssetManager STATIC FileSystem.cpp FileSystem.h)
target_link_libraries(AssetManager PUBLIC Core Math PRIVATE ThirdParty::physfs)
wiseengine_target(AssetManager)
# End of Code Modification - Build System
