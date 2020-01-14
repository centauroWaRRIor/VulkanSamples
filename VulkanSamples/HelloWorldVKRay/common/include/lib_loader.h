#include <memory>

namespace vksamples {
	namespace common {

		class Library {
		public:
			~Library();

			void *Handle() const;

			static bool Load(char const *lib_name, std::shared_ptr<Library> &out_library);

		private:
			Library(void *const handle);

			void *handle_ = nullptr;
		};

	} // namespace common
} // namespace vksamples
