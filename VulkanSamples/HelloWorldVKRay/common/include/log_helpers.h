#pragma once

#include <array>
#include <numeric>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

#include "unused.h"

namespace vksamples {
	namespace common {

		class LogHelpers {
		public:
			template <typename... Args>
			static std::string StringFormat(char const *format, Args... args) {
				std::vector<char> buffer;
				buffer.resize(static_cast<size_t>(snprintf(nullptr, 0, format, args...)) + 1);
				snprintf(buffer.data(), buffer.size(), format, args...);
				return std::string(buffer.data(), buffer.data() + buffer.size() - 1);
			}

			static std::string Join(std::vector<std::string> &strings, std::string const &delim) {
				if (strings.empty()) {
					return std::string();
				}
				return std::accumulate(strings.begin() + 1, strings.end(), strings[0], [&delim](std::string &ss, std::string &s) {
					return ss.empty() ? s : ss + delim + s;
				});
			}

			static std::string FormatBytes(uint64_t const bytes) {
				if (bytes < 1024) {
					return StringFormat("%llu B", bytes);
				}

				static std::array<std::string const, 6> const suffixes = { {"KB", "MB", "GB", "TB", "PB", "EB"} };
				float                                         size = static_cast<float>(bytes);
				for (std::string const &suffix : suffixes) {
					size /= 1024.0f;
					if (size < 1024.0f) {
						return StringFormat("%.3f %s", size, suffix.c_str());
					}
				}
				return StringFormat("%.3f %s", size, suffixes.end()->c_str());
			}

			// output to log in a piece meal fashion, allowing for unlimited size string logging
			static void LogLargeString(const char *output) {
				std::istringstream       log_stream(output);
				std::vector<std::string> output_log = {};
				int                      line_count = 0;
				std::string              current_log_line;
				while (std::getline(log_stream, current_log_line)) {
					++line_count;
					output_log.push_back(LogHelpers::StringFormat("%d:  %s", line_count, current_log_line.c_str()));
					if (25 <= output_log.size()) {
						std::cout << LogHelpers::Join(output_log, "\n").c_str() << std::endl;
						output_log.clear();
					}
				}

				if (0 < output_log.size()) {
					std::cout << LogHelpers::Join(output_log, "\n").c_str() << std::endl;
				}
			}
		};

	} // namespace common
} // namespace vksamples