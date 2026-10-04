#pragma once

namespace wod_server {
	using TCsvData = std::vector<std::vector<std::string>>;
	using TCsvHeaderMap = std::unordered_map<std::string, int>;

	class CsvLoader
	{
	public:
		CsvLoader() {}
		~CsvLoader() {}

		void Load(const std::string& filename);
		virtual void LoadData(const TCsvData& datas, const TCsvHeaderMap& csvHeader);

	protected:
		TCsvData ReadCsv(const std::string& filename);
		TCsvHeaderMap CreateHeaderMap(const std::vector<std::string>& csvHeader);

		template<class T>
		void AssignValue(T& field, const std::string& value);

		template<class T>
		T CreateStructFromCSV(const std::vector<std::string>& row, const TCsvHeaderMap& headerMap);
	};

	template<class T>
	inline void CsvLoader::AssignValue(T& field, const std::string& value)
	{
		try {
			if constexpr (std::is_same_v<T, int> || std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>) {
				field = std::stoi(value);
			}
			else if constexpr (std::is_same_v<T, double>) {
				field = std::stod(value);
			}
			else if constexpr (std::is_same_v<T, float>) {
				field = std::stof(value);
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				field = value;
			}
			else if constexpr (std::is_same_v<T, EPlayerSkill>) {
				field = StringToEnum<EPlayerSkill>(value);
			}
			else if constexpr (std::is_same_v<T, ENpcType>) {
				field = StringToEnum<ENpcType>(value);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Wrong Value: " + value);
		}
	}

	template<class T>
	inline T CsvLoader::CreateStructFromCSV(const std::vector<std::string>& row, const TCsvHeaderMap& headerMap)
	{
		T dataStruct;
		constexpr auto fieldNames = boost::pfr::names_as_array<T>();
		boost::pfr::for_each_field(dataStruct, [&row, &headerMap, &fieldNames, this](auto& field, size_t index) {
			std::string fieldName(fieldNames[index]);
			auto it = headerMap.find(fieldName);
			if (it != headerMap.end()) {
				AssignValue(field, row[it->second]);
			}
			});
		return dataStruct;
	}

}