#pragma once

namespace vksamples {
	namespace common {

		template <typename F>
		struct ScopeExit {
			ScopeExit(F f)
				: f(f) {}
			~ScopeExit() {
				if (execute) {
					f();
				}
			}

			void Cancel() { execute = false; }

			F    f;
			bool execute = true;
		};

		template <typename F>
		ScopeExit<F> MakeScopeExit(F f) {
			return ScopeExit<F>(f);
		};

#ifndef STRING_JOIN2
#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#endif
#ifndef DO_STRING_JOIN2
#define DO_STRING_JOIN2(arg1, arg2) arg1##arg2
#endif
#define GFX_MAKE_SCOPE_EXIT(code) vksamples::common::MakeScopeExit([&]() { \
  code;                                                          \
})
#define GFX_SCOPE_EXIT(code) auto STRING_JOIN2(scope_exit_, __LINE__) = GFX_MAKE_SCOPE_EXIT(code)

	} // namespace common
} // namespace vksamples