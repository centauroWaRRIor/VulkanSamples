#pragma once

namespace vksamples {
	namespace common {

		class NonCopyable {
		public:
			NonCopyable() = default;
			NonCopyable(NonCopyable const &) = delete;
			NonCopyable &operator=(NonCopyable const &) = delete;
		};

	} // namespace common
} // namespace vksamples