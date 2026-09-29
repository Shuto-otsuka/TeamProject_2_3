using System;
using System.Runtime.InteropServices;

namespace SeedCore
{
	[StructLayout(LayoutKind.Sequential)]
	public struct Vector2
	{
		public Single X;

		public Single Y;

		public Vector2(Single x, Single y)
		{
			X = x;
			Y = y;
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector3
	{
		public Single X;

		public Single Y;

		public Single Z;

		public Vector3(Single x, Single y, Single z)
		{
			X = x;
			Y = y;
			Z = z;
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector4
	{
		public Single X;

		public Single Y;

		public Single Z;

		public Single W;

		public Vector4(Single x, Single y, Single z, Single w)
		{
			X = x;
			Y = y;
			Z = z;
			W = w;
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct Color
	{
		public Single R;

		public Single G;

		public Single B;

		public Single A;

		public Color(Single r, Single g, Single b, Single a)
		{
			R = r;
			G = g;
			B = b;
			A = a;
		}
	}
}
