#pragma once
#include <GraphicsEngine/RenderGraph/RenderAccess.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	class RenderPass;

	enum class RenderStageKind :Uint8
	{
		Raster,
		Compute,
		Raytracing,
		Build,
		Copy,
		Callback,
	};

	enum class RenderQueue :Uint32
	{
		Frame = 0,
		Geometry = 1000,
		Occlusion = 1100,
		Illumination = 1500,
		Lighting = 2000,
		Compose = 2500,
		Background = 2600,
		Transparent = 3000,
		Particle = 3500,
		PostProcess = 4000,
		Diagnostic = 4500,
		Highlight = 4750,
		Overlay = 5000,
	};

	struct RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Compute;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Scene;

		SC_CONSTEX Bool async_ = false;

		SC_CONSTEX std::nullptr_t enableCondition_ = nullptr;

		SC_CONSTEX Uint64 showFlags_ = 0;
	};

	template<class... Types>
	struct RenderSequence
	{
		/// No Code
	};

	class RenderIteration
	{
	public:
		template<class Step>
		static consteval Uint32 Count()
		{
			if constexpr (requires { Step::repeat_; })
			{
				return Step::repeat_;
			}
			else
			{
				return 1;
			}
		}

		template<class Step>
		static consteval RenderQueue Queue(Uint32 iteration)
		{
			if constexpr (std::is_array_v<decltype(Step::queue_)>)
			{
				SC_STATIC_ASSERT(std::extent_v<decltype(Step::queue_)> == Count<Step>(), "回ごとの queue_ の数は repeat_ と同じにすること");
				return Step::queue_[iteration];
			}
			else
			{
				return Step::queue_;
			}
		}

		template<class Step, Uint32 Iteration>
		static consteval const auto& Accesses()
		{
			if constexpr (std::rank_v<decltype(Step::accesses_)> == 2)
			{
				SC_STATIC_ASSERT((std::extent_v<decltype(Step::accesses_), 0> == Count<Step>()), "回ごとの accesses_ の数は repeat_ と同じにすること");
				return Step::accesses_[Iteration];
			}
			else
			{
				return Step::accesses_;
			}
		}

		template<class Slot, class Step>
		static consteval std::string_view Name()
		{
			if constexpr (std::is_same_v<Slot, Step>)
			{
				return std::string_view();
			}
			else
			{
				return RenderName::StepName<Step>();
			}
		}
	};

	template<class Slot, class Step, Uint32 Iteration>
	struct RenderUnit
	{
		SC_CONSTEX RenderStageKind kind_ = Step::kind_;

		SC_CONSTEX RenderQueue queue_ = RenderIteration::Queue<Step>(Iteration);

		SC_CONSTEX RenderViewMask viewMask_ = Step::viewMask_;

		SC_CONSTEX Bool async_ = Step::async_;

		SC_CONSTEX auto enableCondition_ = Step::enableCondition_;

		SC_CONSTEX Uint64 showFlags_ = Step::showFlags_;

		SC_CONSTEX const auto& accesses_ = RenderIteration::Accesses<Step, Iteration>();

		SC_CONSTEX Uint32 iteration_ = Iteration;

		SC_CONSTEX Uint32 repeat_ = RenderIteration::Count<Step>();

		SC_CONSTEX std::string_view slot_ = RenderName::StageName<Slot>();

		SC_CONSTEX std::string_view step_ = RenderIteration::Name<Slot, Step>();

		template<class Feature>
		static void Execute(Feature& feature, RenderPass& pass)
		{
			Step::Execute(feature, pass);
		}
	};

	template<class Slot, class Step, class Iterations = std::make_integer_sequence<Uint32, RenderIteration::Count<Step>()>>
	struct RenderRepeat;

	template<class Slot, class Step, Uint32... Iterations>
	struct RenderRepeat<Slot, Step, std::integer_sequence<Uint32, Iterations...>>
	{
		using Type = RenderSequence<RenderUnit<Slot, Step, Iterations>...>;
	};

	template<class... Sequences>
	struct RenderJoin
	{
		using Type = RenderSequence<>;
	};

	template<class... Units>
	struct RenderJoin<RenderSequence<Units...>>
	{
		using Type = RenderSequence<Units...>;
	};

	template<class... First, class... Second, class... Rest>
	struct RenderJoin<RenderSequence<First...>, RenderSequence<Second...>, Rest...>
	{
		using Type = typename RenderJoin<RenderSequence<First..., Second...>, Rest...>::Type;
	};

	template<class Slot, class Steps>
	struct RenderUnfold;

	template<class Slot, class... Steps>
	struct RenderUnfold<Slot, RenderSequence<Steps...>>
	{
		using Type = typename RenderJoin<typename RenderRepeat<Slot, Steps>::Type...>::Type;
	};

	template<class Slot, class Steps = void>
	struct RenderExpand
	{
		using Type = typename RenderRepeat<Slot, Slot>::Type;
	};

	template<class Slot>
	struct RenderExpand<Slot, std::void_t<typename Slot::Sequence>>
	{
		using Type = typename RenderUnfold<Slot, typename Slot::Sequence>::Type;
	};
}
