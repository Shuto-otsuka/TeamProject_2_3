#pragma once
#include <GraphicsEngine/RenderGraph/RenderStage.h>

namespace SeedCore
{
	struct RenderScene;

	class RenderFrame;
	class RenderDevice;
	class RenderUpload;

	template<class Feature>
	class RenderFeature
	{
	public:
		using Self = Feature;

		SC_CONSTEX StaticArray<const RenderResource*, 0> resources_ = {};

	public:
		void Create(RenderDevice& device)
		{
			/// No Code
		}

		void Gather(const RenderScene& scene, RenderFrame& frame)
		{
			/// No Code
		}

		void Prepare(const RenderFrame& frame, RenderUpload& upload)
		{
			/// No Code
		}
	};
}

#define SC_RENDER_EXPAND(...) __VA_ARGS__
#define SC_RENDER_CONCAT(First, Second) SC_RENDER_CONCAT_I(First, Second)
#define SC_RENDER_CONCAT_I(First, Second) First##Second

#define SC_RENDER_COUNT(...) SC_RENDER_EXPAND(SC_RENDER_COUNT_I(__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))
#define SC_RENDER_COUNT_I(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, Count, ...) Count

#define SC_RENDER_FOR_EACH(Macro, Data, ...) SC_RENDER_FOR_EACH_I(SC_RENDER_COUNT(__VA_ARGS__), Macro, Data, __VA_ARGS__)
#define SC_RENDER_FOR_EACH_I(Count, Macro, Data, ...) SC_RENDER_EXPAND(SC_RENDER_CONCAT(SC_RENDER_FOR_EACH_, Count)(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_1(Macro, Data, Entry) Macro(Data, Entry)
#define SC_RENDER_FOR_EACH_2(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_1(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_3(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_2(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_4(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_3(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_5(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_4(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_6(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_5(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_7(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_6(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_8(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_7(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_9(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_8(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_10(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_9(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_11(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_10(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_12(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_11(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_13(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_12(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_14(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_13(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_15(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_14(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_16(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_15(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_17(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_16(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_18(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_17(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_19(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_18(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_20(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_19(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_21(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_20(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_22(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_21(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_23(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_22(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_24(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_23(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_25(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_24(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_26(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_25(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_27(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_26(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_28(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_27(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_29(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_28(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_30(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_29(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_31(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_30(Macro, Data, __VA_ARGS__))
#define SC_RENDER_FOR_EACH_32(Macro, Data, Entry, ...) Macro(Data, Entry) SC_RENDER_EXPAND(SC_RENDER_FOR_EACH_31(Macro, Data, __VA_ARGS__))

#define SC_RENDER_CONTRACT(Name, ...) (public, Contract, Name, __VA_ARGS__)
#define SC_RENDER_ACCUMULATION(Name, ...) (public, Accumulation, Name, __VA_ARGS__)
#define SC_RENDER_INTERNAL(Name, ...) (private, Internal, Name, __VA_ARGS__)
#define SC_RENDER_DEBUG(Name, ...) (friend class DebugViewFeature; private, Debug, Name, __VA_ARGS__)

#define SC_RENDER_DECLARE_RESOURCE(Data, Entry) SC_RENDER_DECLARE_RESOURCE_I Entry
#define SC_RENDER_DECLARE_RESOURCE_I(Access, Kind, Name, ...) Access: SC_CONSTEX ::SeedCore::RenderResource Name = ::SeedCore::RenderResource(::SeedCore::RenderName::FeatureName<Self>(), ::SeedCore::RenderResourceName<Self, #Name>.View(), ::SeedCore::RenderResourceKind::Kind, __VA_ARGS__);
#define SC_RENDER_ADDRESS_RESOURCE(Data, Entry) SC_RENDER_ADDRESS_RESOURCE_I Entry
#define SC_RENDER_ADDRESS_RESOURCE_I(Access, Kind, Name, ...) &Name,

#define SC_RENDER_RESOURCES(...) \
	SC_RENDER_FOR_EACH(SC_RENDER_DECLARE_RESOURCE, ~, __VA_ARGS__) \
	public: SC_CONSTEX const ::SeedCore::RenderResource* resources_[] = { SC_RENDER_FOR_EACH(SC_RENDER_ADDRESS_RESOURCE, ~, __VA_ARGS__) }
