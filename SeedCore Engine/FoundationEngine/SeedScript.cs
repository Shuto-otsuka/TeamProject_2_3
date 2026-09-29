using System;

namespace SeedCore
{
    public abstract class SeedScript
    {
        internal EntityID entity_;

        internal ScriptSchema schema_;

        internal String[] strings_;

        internal Action awake_;

        internal Action start_;

        internal Action<Single> tick_;

        internal Action<Single> lateTick_;

        internal Action<Single> fixedTick_;

        internal Action destroy_;

        internal Action inspectorGUI_;

        internal Action<Entity> collisionEnter_;

        internal Action<Entity> collisionStay_;

        internal Action<Entity> collisionExit_;

        internal Action<Entity> triggerEnter_;

        internal Action<Entity> triggerStay_;

        internal Action<Entity> triggerExit_;

        internal void DispatchLifecycle(ScriptHook hook,Single elapsedTime)
        {
            switch(hook)
            {
                case ScriptHook.Awake:
                    awake_?.Invoke();
                    break;
                case ScriptHook.Start:
                    start_?.Invoke();
                    break;
                case ScriptHook.Tick:
                    tick_?.Invoke(elapsedTime);
                    break;
                case ScriptHook.LateTick:
                    lateTick_?.Invoke(elapsedTime);
                    break;
                case ScriptHook.FixedTick:
                    fixedTick_?.Invoke(elapsedTime);
                    break;
                case ScriptHook.Destroy:
                    destroy_?.Invoke();
                    break;
                case ScriptHook.InspectorGUI:
                    inspectorGUI_?.Invoke();
                    break;
            }
        }

        internal void DispatchContact(ScriptHook hook, Entity other)
        {
            switch (hook)
            {
                case ScriptHook.CollisionEnter:
                    collisionEnter_?.Invoke(other);
                    break;
                case ScriptHook.CollisionStay:
                    collisionStay_?.Invoke(other);
                    break;
                case ScriptHook.CollisionExit:
                    collisionExit_?.Invoke(other);
                    break;
                case ScriptHook.TriggerEnter:
                    triggerEnter_?.Invoke(other);
                    break;
                case ScriptHook.TriggerStay:
                    triggerStay_?.Invoke(other);
                    break;
                case ScriptHook.TriggerExit:
                    triggerExit_?.Invoke(other);
                    break;
            }
        }
    }
}
