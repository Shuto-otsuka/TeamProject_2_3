using System;
using System.IO;
using System.Text;
using System.Reflection;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace SeedCore
{
	internal sealed class ScriptLoadContext : AssemblyLoadContext
	{
		public ScriptLoadContext() : base("UserProject.Csharp", isCollectible: true)
		{
			/// No Code
		}

		protected override Assembly Load(AssemblyName assemblyName)
		{
			return null;
		}
	}

	internal sealed class ScriptType
	{
		internal Type type_;

		internal MethodInfo[] methods_ = new MethodInfo[Enum.GetValues<ScriptHook>().Length];

		internal UInt32 hookMask_;

		internal ScriptSchema schema_;
	}

	internal static unsafe class ScriptHost
	{
		private static ScriptLoadContext context_;

		private static List<ScriptType> types_ = new List<ScriptType>();

		private static Dictionary<Type, ScriptSchema> schemas_ = new Dictionary<Type, ScriptSchema>();

		private static HashSet<Type> enums_ = new HashSet<Type>();

		private static ScriptSchema emptySchema_ = new ScriptSchema(Array.Empty<ScriptField>(), 0, 0, (script, block) => { }, (script, block) => { }, (script, fieldIndex) => true);

		[UnmanagedCallersOnly]
		internal static Int32 Load(Byte* path, Int32 length)
		{
			try
			{
				String assemblyPath = Encoding.UTF8.GetString(path, length);
				String symbolPath = Path.ChangeExtension(assemblyPath, ".pdb");

				MemoryStream assemblyStream = new MemoryStream(File.ReadAllBytes(assemblyPath));
				MemoryStream symbolStream = File.Exists(symbolPath) ? new MemoryStream(File.ReadAllBytes(symbolPath)) : null;

				context_ = new ScriptLoadContext();
				Assembly assembly = context_.LoadFromStream(assemblyStream, symbolStream);

				schemas_.Clear();
				MethodInfo registerSchemas = assembly.GetType("SeedCore.Generated.ScriptReflection")?.GetMethod("Register", BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
				if (registerSchemas != null)
				{
					registerSchemas.Invoke(null, new Object[] { schemas_ });
				}
				else
				{
					Log.Warning("C# スクリプトのフィールド情報が見つかりません（UserProject.Csharp が SeedCore.Generator を参照しているか確認してください）");
				}

				types_.Clear();
				enums_.Clear();
				foreach (Type type in assembly.GetTypes())
				{
					if (type.IsAbstract || !type.IsSubclassOf(typeof(SeedScript)))
					{
						continue;
					}

					Int32 typeIndex = types_.Count;
					ScriptType scriptType = Detect(type);
					types_.Add(scriptType);
					Register(type, typeIndex, scriptType);
				}

				return 0;
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトを読み込めませんでした: {exception}");
				return 1;
			}
		}

		[UnmanagedCallersOnly]
		internal static Int32 Unload()
		{
			try
			{
				WeakReference context = Release();
				for (Int32 attempt = 0; attempt < 10 && context.IsAlive; ++attempt)
				{
					GC.Collect();
					GC.WaitForPendingFinalizers();
				}

				if (context.IsAlive)
				{
					Log.Warning("C# スクリプトの古いアセンブリを解放できませんでした（static 変数やイベントがスクリプトの型を参照していないか確認してください）。新しいアセンブリで続行します");
					return 1;
				}
				return 0;
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトを解放できませんでした: {exception}");
				return 1;
			}
		}

		[UnmanagedCallersOnly]
		internal static IntPtr Create(Int32 typeIndex, EntityID entity)
		{
			try
			{
				ScriptType scriptType = types_[typeIndex];
				SeedScript script = (SeedScript)Activator.CreateInstance(scriptType.type_);
				script.entity_ = entity;
				script.schema_ = scriptType.schema_;
				script.awake_ = Bind<Action>(scriptType, ScriptHook.Awake, script);
				script.start_ = Bind<Action>(scriptType, ScriptHook.Start, script);
				script.tick_ = Bind<Action<Single>>(scriptType, ScriptHook.Tick, script);
				script.lateTick_ = Bind<Action<Single>>(scriptType, ScriptHook.LateTick, script);
				script.fixedTick_ = Bind<Action<Single>>(scriptType, ScriptHook.FixedTick, script);
                script.editorTick_ = Bind<Action<Single>>(scriptType, ScriptHook.EditorTick, script);
                script.destroy_ = Bind<Action>(scriptType, ScriptHook.Destroy, script);
				script.inspectorGUI_ = Bind<Action>(scriptType, ScriptHook.InspectorGUI, script);
				script.collisionEnter_ = Bind<Action<Entity>>(scriptType, ScriptHook.CollisionEnter, script);
				script.collisionStay_ = Bind<Action<Entity>>(scriptType, ScriptHook.CollisionStay, script);
				script.collisionExit_ = Bind<Action<Entity>>(scriptType, ScriptHook.CollisionExit, script);
				script.triggerEnter_ = Bind<Action<Entity>>(scriptType, ScriptHook.TriggerEnter, script);
				script.triggerStay_ = Bind<Action<Entity>>(scriptType, ScriptHook.TriggerStay, script);
				script.triggerExit_ = Bind<Action<Entity>>(scriptType, ScriptHook.TriggerExit, script);
				return GCHandle.ToIntPtr(GCHandle.Alloc(script));
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトを生成できませんでした: {exception}");
				return IntPtr.Zero;
			}
		}

		[UnmanagedCallersOnly]
		internal static void InvokeLifecycle(IntPtr handle, ScriptHook hook, Single deltaTime)
		{
			try
			{
				SeedScript script = (SeedScript)GCHandle.FromIntPtr(handle).Target;
				script.DispatchLifecycle(hook, deltaTime);
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトで例外が発生しました（{hook}）: {exception}");
			}
		}

		[UnmanagedCallersOnly]
		internal static void InvokeContact(IntPtr handle, ScriptHook hook, EntityID other)
		{
			try
			{
				SeedScript script = (SeedScript)GCHandle.FromIntPtr(handle).Target;
				script.DispatchContact(hook, new Entity(other));
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトで例外が発生しました（{hook}）: {exception}");
			}
		}

		[UnmanagedCallersOnly]
		internal static void Push(IntPtr handle, Byte* block)
		{
			try
			{
				SeedScript script = (SeedScript)GCHandle.FromIntPtr(handle).Target;
				script.schema_.push_(script, block);
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトへフィールドの値を書き込めませんでした: {exception}");
			}
		}

		[UnmanagedCallersOnly]
		internal static void Pull(IntPtr handle, Byte* block)
		{
			try
			{
				SeedScript script = (SeedScript)GCHandle.FromIntPtr(handle).Target;
				script.schema_.pull_(script, block);
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトからフィールドの値を読み込めませんでした: {exception}");
			}
		}

		[UnmanagedCallersOnly]
		internal static Byte Condition(IntPtr handle, Int32 fieldIndex)
		{
			try
			{
				SeedScript script = (SeedScript)GCHandle.FromIntPtr(handle).Target;
				return script.schema_.condition_(script, fieldIndex) ? (Byte)1 : (Byte)0;
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトの表示条件で例外が発生しました: {exception}");
				return 1;
			}
		}

		[UnmanagedCallersOnly]
		internal static void Free(IntPtr handle)
		{
			try
			{
				GCHandle.FromIntPtr(handle).Free();
			}
			catch (Exception exception)
			{
				Log.Error($"C# スクリプトを解放できませんでした: {exception}");
			}
		}

		[MethodImpl(MethodImplOptions.NoInlining)]
		private static WeakReference Release()
		{
			types_.Clear();
			schemas_.Clear();
			enums_.Clear();

			WeakReference context = new WeakReference(context_);
			context_?.Unload();
			context_ = null;
			return context;
		}

		private static ScriptType Detect(Type type)
		{
			ScriptType scriptType = new ScriptType();
			scriptType.type_ = type;
			scriptType.schema_ = schemas_.TryGetValue(type, out ScriptSchema schema) ? schema : emptySchema_;

			BindingFlags flags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic;
			foreach (ScriptHook hook in Enum.GetValues<ScriptHook>())
			{
				Type[] parameters = hook switch
				{
					ScriptHook.Tick or
					ScriptHook.LateTick or
					ScriptHook.FixedTick or
                    ScriptHook.EditorTick => new Type[] { typeof(Single) },

					ScriptHook.CollisionEnter or
					ScriptHook.CollisionStay or
					ScriptHook.CollisionExit or
					ScriptHook.TriggerEnter or
					ScriptHook.TriggerStay or
					ScriptHook.TriggerExit => new Type[] { typeof(Entity) },

					_ => Type.EmptyTypes,
				};

				String name = "On" + hook;
				MethodInfo method = type.GetMethod(name, flags, parameters);
				if (method == null || method.ReturnType != typeof(void))
				{
					if (type.GetMember(name, MemberTypes.Method, flags).Length > 0)
					{
						Log.Warning($"{type.Name}.{name} は引数か戻り値の型が違うため呼ばれません");
					}
					continue;
				}

				scriptType.methods_[(Int32)hook] = method;
				scriptType.hookMask_ |= 1u << (Int32)hook;
			}
			return scriptType;
		}

		private static T Bind<T>(ScriptType scriptType, ScriptHook hook, SeedScript script) where T : Delegate
		{
			MethodInfo method = scriptType.methods_[(Int32)hook];
			if (method == null)
			{
				return null;
			}
			return method.CreateDelegate<T>(script);
		}

		private static void Register(Type type, Int32 typeIndex, ScriptType scriptType)
		{
			SeedCategoryAttribute attribute = type.GetCustomAttribute<SeedCategoryAttribute>();
			String categoryName = (attribute != null && !String.IsNullOrWhiteSpace(attribute.category_)) ? attribute.category_ : "Custom";

			Byte[] name = Encoding.UTF8.GetBytes(type.Name);
			Byte[] category = Encoding.UTF8.GetBytes(categoryName);
			fixed (Byte* namePointer = name)
			{
				fixed (Byte* categoryPointer = category)
				{
					NativeApi.current_->registerScript_(NativeApi.current_->context_, namePointer, name.Length, categoryPointer, category.Length, typeIndex, scriptType.hookMask_, scriptType.schema_.blockSize_);
				}
			}

			foreach (ScriptField field in scriptType.schema_.fields_)
			{
				Byte[] enumName = Encoding.UTF8.GetBytes(field.enumType_?.FullName ?? "");
				if (field.enumType_ != null && enums_.Add(field.enumType_))
				{
					Array values = Enum.GetValues(field.enumType_);
					String[] names = Enum.GetNames(field.enumType_);
					for (Int32 entryIndex = 0; entryIndex < names.Length; ++entryIndex)
					{
						Byte[] entryName = Encoding.UTF8.GetBytes(names[entryIndex]);
						fixed (Byte* enumNamePointer = enumName)
						{
							fixed (Byte* entryNamePointer = entryName)
							{
								NativeApi.current_->registerEnum_(NativeApi.current_->context_, enumNamePointer, enumName.Length, Convert.ToInt32(values.GetValue(entryIndex)), entryNamePointer, entryName.Length);
							}
						}
					}
				}

				Byte[] fieldName = Encoding.UTF8.GetBytes(field.name_);
				fixed (Byte* fieldNamePointer = fieldName)
				{
					fixed (Byte* enumNamePointer = enumName)
					{
						NativeApi.current_->registerField_(NativeApi.current_->context_, typeIndex, fieldNamePointer, fieldName.Length, (Byte)field.type_, field.offset_, field.min_, field.max_, (Byte)field.flag_, enumNamePointer, enumName.Length, (Byte)field.assetType_);
					}
				}
			}
		}
	}
}
