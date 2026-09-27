using System;
using System.Runtime.InteropServices;

namespace SeedCore
{
    public static unsafe class DLLMain
    {
        [UnmanagedCallersOnly]
        public static Int32 Initialize(CsharpNativeApi* nativeApi)
        {
            try
            {
                CsharpNativeApi.current_ = nativeApi;
                Log.Notice($"C# を起動しました（.NET {Environment.Version}）。C# を用いたスクリプトを使用することができます。");
                return 0;
            }
            catch(Exception)
            {
                return 1;
            }
        }
    }
}
