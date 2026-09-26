using System;
using System.Runtime.CompilerServices;

[assembly: InternalsVisibleTo("SeedCore.Csharp")]

namespace SeedCore
{
    public unsafe struct CsharpNativeApi
    {
        internal delegate* unmanaged<Byte, Byte*, Int32, void> log_;

        internal static CsharpNativeApi* current_;
    }
}
