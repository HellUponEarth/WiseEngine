// Code Modification - Rendering: Check both offline shader outputs through the pinned DXC.
// Scope: compiler smoke test, never dispatched. Performance: no runtime cost. Toggle: WISEENGINE_BUILD_TESTS.
RWStructuredBuffer<uint> Output : register(u0);
[numthreads(8, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    Output[id.x] = id.x + 1;
}
// End of Code Modification - Rendering
