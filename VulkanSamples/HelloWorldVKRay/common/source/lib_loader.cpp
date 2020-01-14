#include <lib_loader.h>

#include <mutex>
#include <unordered_map>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

namespace vksamples {
	namespace common {

		static std::unordered_map<char const *, std::weak_ptr<Library>> s_loaded_libs = {};

		Library::Library(void *const handle)
			: handle_(handle) {
		}

		bool Library::Load(char const *lib_name, std::shared_ptr<Library> &out_library) {

			auto const &existing = s_loaded_libs.find(lib_name);
			if (s_loaded_libs.end() != existing) {
				if ((out_library = existing->second.lock())) {
					return true;
				}
				else {
					s_loaded_libs.erase(existing);
				}
			}

			// In new visual studio 17 projects go to Configuration Properties -> General -> Use multibyte character set, to avoid compilation issue 
			HMODULE handle = LoadLibrary(lib_name);

			if (nullptr == handle) {
				return false;
			}

			out_library = std::shared_ptr<Library>(new Library(handle));

			s_loaded_libs[lib_name] = out_library;

			return true;
		}

		void *Library::Handle() const {
			return handle_;
		}

		Library::~Library() {

			if (handle_) {
				FreeLibrary(static_cast<HMODULE>(handle_));
			}

			handle_ = nullptr;
		}

	} // namespace common
} // namespace vksamples