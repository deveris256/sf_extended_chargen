#pragma once

namespace parser
{
	class CommandParser
	{
	public:
		using ValueType = std::variant<int, double, std::string>;

		struct ParameterSpec
		{
			std::string shortName;
			bool        required;
			ValueType   defaultValue;
		};

		CommandParser(const std::unordered_map<std::string, ParameterSpec>& spec)
		{
			for (const auto& [longName, paramSpec] : spec) {
				specs[longName] = paramSpec;
				if (!paramSpec.shortName.empty()) {
					shortToLong[paramSpec.shortName] = longName;
				}
			}
		}

		virtual bool parse(std::string_view& command)
		{
			return parse(std::string(command));
		}

		virtual bool parse(const std::string& command)
		{
			clear();

			std::vector<std::string> tokens = tokenize(command);

			for (size_t i = 0; i < tokens.size(); ++i) {
				const std::string& arg = tokens[i];
				std::string        longName = resolveLongName(arg);

				if (!longName.empty() && specs.count(longName)) {
					if (i + 1 < tokens.size()) {
						if (!storeArgument(longName, tokens[++i])) {
							lastError = std::format("Invalid value for argument: {}", arg);
							return false;
						}
					} else if (specs.at(longName).required) {
						lastError = std::format("Missing value for required argument: {}", arg);
						return false;
					}
				} else if (i == 0) {
					parsedArgs["name"] = arg;
				}
			}

			for (const auto& [key, spec] : specs) {
				if (spec.required && parsedArgs.find(key) == parsedArgs.end()) {
					lastError = std::format("Missing required argument: {}", key);
					return false;
				}
			}

			return true;
		}

		const std::string getLastError() const
		{
			return lastError;
		}

		void clear()
		{
			parsedArgs.clear();
		}

		bool hasArgument(const std::string& arg) const
		{
			return parsedArgs.find(arg) != parsedArgs.end();
		}

		template <typename T>
		std::optional<T> getArgument(const std::string& arg) const
		{
			auto it = parsedArgs.find(arg);
			if (it != parsedArgs.end()) {
				if (auto val = std::get_if<T>(&it->second)) {
					return *val;
				}
			}

			auto specIt = specs.find(arg);
			if (specIt != specs.end()) {
				if (auto val = std::get_if<T>(&specIt->second.defaultValue)) {
					return *val;
				}
			}

			return std::nullopt;
		}

	private:
		std::unordered_map<std::string, ParameterSpec> specs;
		std::unordered_map<std::string, std::string>   shortToLong;
		std::unordered_map<std::string, ValueType>     parsedArgs;
		std::string                                    lastError;

		std::vector<std::string> tokenize(const std::string& command) const
		{
			std::regex           tokenRegex(R"(\S+)");
			std::sregex_iterator it(command.begin(), command.end(), tokenRegex);
			std::sregex_iterator end;

			std::vector<std::string> tokens;
			while (it != end) {
				tokens.push_back(it->str());
				++it;
			}
			return tokens;
		}

		std::string resolveLongName(const std::string& name) const
		{
			if (specs.count(name)) {
				return name;
			} else if (shortToLong.count(name)) {
				return shortToLong.at(name);
			}
			return "";
		}

		bool storeArgument(const std::string& arg, const std::string& value)
		{
			const auto& spec = specs.at(arg);
			try {
				if (std::holds_alternative<int>(spec.defaultValue)) {
					parsedArgs[arg] = std::stoi(value);
				} else if (std::holds_alternative<double>(spec.defaultValue)) {
					parsedArgs[arg] = std::stod(value);
				} else {
					parsedArgs[arg] = value;
				}
			} catch (...) {
				return false;
			}
			return true;
		}
	};

	class ArmorKeywordMorphParser : public CommandParser
	{
	public:
		ArmorKeywordMorphParser() :
			CommandParser({ { "--add", { "-a", false, 0.0 } },
				{ "--set", { "-s", false, 0.0 } },
				{ "--priority", { "-p", false, 1 } } })
		{}

		std::string getMorphName() const
		{
			if (auto val = getArgument<std::string>("name")) {
				return *val;
			}
			return "";
		}

		float getAdd() const
		{
			if (auto val = getArgument<double>("--add")) {
				return static_cast<float>(*val);
			}
			return 0.0f;
		}

		float getSet() const
		{
			if (auto val = getArgument<double>("--set")) {
				return static_cast<float>(*val);
			}
			return 0.0f;
		}

		bool hasSet() const
		{
			return hasArgument("--set");
		}

		bool hasAdd() const
		{
			return hasArgument("--add");
		}

		uint32_t getPriority() const
		{
			if (auto val = getArgument<int>("--priority")) {
				return static_cast<uint32_t>(*val);
			}
			return 1;
		}
	};
}