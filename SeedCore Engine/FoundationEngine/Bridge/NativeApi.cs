using System;
using System.Runtime.CompilerServices;

[assembly: InternalsVisibleTo("SeedCore.Csharp")]

namespace SeedCore
{
	public unsafe struct NativeApi
	{
		internal void* context_;

		internal delegate* unmanaged<Byte, Byte*, Int32, void> log_;

		internal delegate* unmanaged<void*, Byte*, Int32, Byte*, Int32, Int32, UInt32, Int32, void> registerScript_;

		internal delegate* unmanaged<void*, Int32, Byte*, Int32, Byte, Int32, Double, Double, Byte, Byte*, Int32, Byte, void> registerField_;

		internal delegate* unmanaged<void*, Byte*, Int32, Int32, Byte*, Int32, void> registerEnum_;

		internal delegate* unmanaged<Byte*, Int32, Byte*, void> intern_;

		internal delegate* unmanaged<void*, EntityID, Byte*, Int32, Byte*, Int32, Byte, void*> field_;

		internal static NativeApi* current_;
	}
}
