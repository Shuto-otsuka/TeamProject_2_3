using System;
using System.ComponentModel;
using System.Text;

namespace SeedCore
{
	[EditorBrowsable(EditorBrowsableState.Never)]
	public enum AttributeType : Byte
	{
		Unknown,
		Int,
		Float,
		Bool,
		Vector2,
		Vector3,
		String,
		Color,
		Enum,
		Struct,
	}

	[Flags]
	[EditorBrowsable(EditorBrowsableState.Never)]
	public enum ScriptFieldFlag : Byte
	{
		None = 0,
		Reflection = 1 << 0,
		Condition = 1 << 1,
		Serialize = 1 << 2,
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public sealed class ScriptField
	{
		internal String name_;

		internal AttributeType type_;

		internal Int32 offset_;

		internal Double min_;

		internal Double max_;

		internal ScriptFieldFlag flag_;

		internal Type enumType_;

		internal PayloadType assetType_;

		public ScriptField(String name, AttributeType type, Int32 offset, Double min, Double max, ScriptFieldFlag flag, Type enumType, PayloadType assetType)
		{
			name_ = name;
			type_ = type;
			offset_ = offset;
			min_ = min;
			max_ = max;
			flag_ = flag;
			enumType_ = enumType;
			assetType_ = assetType;
		}
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public unsafe delegate void ScriptBlockCopy(SeedScript script, Byte* block);

	[EditorBrowsable(EditorBrowsableState.Never)]
	public delegate Boolean ScriptCondition(SeedScript script, Int32 fieldIndex);

	[EditorBrowsable(EditorBrowsableState.Never)]
	public sealed class ScriptSchema
	{
		internal ScriptField[] fields_;

		internal Int32 blockSize_;

		internal Int32 stringCount_;

		internal ScriptBlockCopy push_;

		internal ScriptBlockCopy pull_;

		internal ScriptCondition condition_;

		public ScriptSchema(ScriptField[] fields, Int32 blockSize, Int32 stringCount, ScriptBlockCopy push, ScriptBlockCopy pull, ScriptCondition condition)
		{
			fields_ = fields;
			blockSize_ = blockSize;
			stringCount_ = stringCount;
			push_ = push;
			pull_ = pull;
			condition_ = condition;
		}
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public static unsafe class ScriptBlock
	{
		public static String PushString(SeedScript script, Int32 stringIndex, Byte* source)
		{
			Byte* data = *(Byte**)source;
			Int64 length = *(Int64*)(source + 8);
			String value = (data == null || length == 0) ? "" : Encoding.UTF8.GetString(data, (Int32)length);

			String[] cache = script.strings_ ??= new String[script.schema_.stringCount_];
			cache[stringIndex] = value;
			return value;
		}

		public static void PullString(SeedScript script, Int32 stringIndex, Byte* destination, String value)
		{
			String[] cache = script.strings_ ??= new String[script.schema_.stringCount_];
			if (cache[stringIndex] != null && ReferenceEquals(cache[stringIndex], value))
			{
				return;
			}
			cache[stringIndex] = value;

			Byte[] bytes = Encoding.UTF8.GetBytes(value ?? "");
			fixed (Byte* text = bytes)
			{
				NativeApi.current_->intern_(text, bytes.Length, destination);
			}
		}
	}
}
