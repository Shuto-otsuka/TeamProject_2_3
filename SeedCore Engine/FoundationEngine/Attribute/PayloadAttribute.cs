using System;

namespace SeedCore
{
	public enum PayloadType : Byte
	{
		None,
		Texture,
		Model,
		Effect,
		Audio,
		Font,
		Movie,
		Animation,
		MeshCollision,
		Material,
		Skeleton,
		Sky,
		Prefab,
		Actor,
	}

	[AttributeUsage(AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
	public sealed class SeedPayloadFieldAttribute : Attribute
	{
		internal String displayName_;

		internal PayloadType assetType_;

		public SeedPayloadFieldAttribute(PayloadType assetType)
		{
			assetType_ = assetType;
		}

		public SeedPayloadFieldAttribute(String displayName, PayloadType assetType)
		{
			displayName_ = displayName;
			assetType_ = assetType;
		}
	}
}
