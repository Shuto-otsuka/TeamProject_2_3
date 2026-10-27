using System;

namespace SeedCore
{
    public enum ScriptHook : Byte
    {
        Attach,
        Detach,
        Awake,
        Start,
        Tick,
        LateTick,
        FixedTick,
        EditorTick,
        Destroy,
        InspectorGUI,
        CollisionEnter,
        CollisionStay,
        CollisionExit,
        TriggerEnter,
        TriggerStay,
        TriggerExit,
        Revision,
    }

    public unsafe struct ManagedApi
    {
        internal delegate* unmanaged<Byte*, Int32, Int32> load_;

        internal delegate* unmanaged<Int32> unload_;

        internal delegate* unmanaged<Int32, EntityID, IntPtr> create_;

        internal delegate* unmanaged<IntPtr, ScriptHook, Single, void> invokeLifecycle_;

        internal delegate* unmanaged<IntPtr, ScriptHook, EntityID, void> invokeContact_;

        internal delegate* unmanaged<IntPtr, Byte*, void> push_;

        internal delegate* unmanaged<IntPtr, Byte*, void> pull_;

        internal delegate* unmanaged<IntPtr, Int32, Byte> condition_;

        internal delegate* unmanaged<IntPtr, void> free_;
    }
}
