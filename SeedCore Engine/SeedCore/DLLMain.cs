using System;
using System.Runtime.InteropServices;

namespace SeedCore
{
    public static unsafe class DLLMain
    {
        [UnmanagedCallersOnly]
        public static Int32 Initialize(NativeApi* nativeApi, ManagedApi* managedApi)
        {
            try
            {
                NativeApi.current_ = nativeApi;
                managedApi->load_ = &ScriptHost.Load;
                managedApi->unload_ = &ScriptHost.Unload;
                managedApi->create_ = &ScriptHost.Create;
                managedApi->invokeLifecycle_ = &ScriptHost.InvokeLifecycle;
                managedApi->invokeContact_ = &ScriptHost.InvokeContact;
                managedApi->push_ = &ScriptHost.Push;
                managedApi->pull_ = &ScriptHost.Pull;
                managedApi->condition_ = &ScriptHost.Condition;
                managedApi->free_ = &ScriptHost.Free;
                Log.Notice($"C# ランタイム を起動しました（.NET {Environment.Version}）。C# を用いたスクリプトを使用することができます。");
                return 0;
            }
            catch(Exception)
            {
                return 1;
            }
        }
    }
}
