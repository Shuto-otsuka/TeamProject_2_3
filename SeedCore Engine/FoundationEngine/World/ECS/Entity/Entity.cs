using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace SeedCore
{
	[StructLayout(LayoutKind.Sequential)]
	public struct EntityID
	{
		internal UInt32 index_;

		internal UInt32 generation_;
	}

	public readonly struct Entity
	{
		internal readonly EntityID id_;

		internal Entity(EntityID id)
		{
			id_ = id;
		}

		public unsafe ref T Field<T>(String componentName, String fieldName) where T : unmanaged
		{
			AttributeType type;
			if (typeof(T) == typeof(Int32))
			{
				type = AttributeType.Int;
			}
			else if (typeof(T) == typeof(Single))
			{
				type = AttributeType.Float;
			}
			else if (typeof(T) == typeof(Boolean))
			{
				type = AttributeType.Bool;
			}
			else if (typeof(T) == typeof(Vector2))
			{
				type = AttributeType.Vector2;
			}
			else if (typeof(T) == typeof(Vector3))
			{
				type = AttributeType.Vector3;
			}
			else if (typeof(T) == typeof(Color))
			{
				type = AttributeType.Color;
			}
			else
			{
				throw new ArgumentException($"Field は {typeof(T).Name} に対応していません（Int32 / Single / Boolean / Vector2 / Vector3 / Color のみ）");
			}

			Byte[] component = Encoding.UTF8.GetBytes(componentName);
			Byte[] field = Encoding.UTF8.GetBytes(fieldName);
			fixed (Byte* componentPointer = component)
			{
				fixed (Byte* fieldPointer = field)
				{
					void* address = NativeApi.current_->field_(NativeApi.current_->context_, id_, componentPointer, component.Length, fieldPointer, field.Length, (Byte)type);
					if (address == null)
					{
						throw new InvalidOperationException("Field を使える状態ではありません（C# ランタイムに World が渡されていません）");
					}
					return ref Unsafe.AsRef<T>(address);
				}
			}
		}
	}
}
