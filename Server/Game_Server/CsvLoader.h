#pragma once

namespace wod_server {
	using TCsvData = std::vector<std::vector<std::string>>;
	using TCsvHeaderMap = std::unordered_map<std::string, int>;

	class CsvLoader
	{
	public:
		CsvLoader() {}
		~CsvLoader() {}

		void Load(const std::string& _filename);
		virtual void LoadData(const TCsvData& _datas, const TCsvHeaderMap& _csvHeader);

	protected:
		TCsvData ReadCsv(const std::string& _filename);
		TCsvHeaderMap CreateHeaderMap(const std::vector<std::string>& _csvHeader);

		template<class T>
		void AssignValue(T& _field, const std::string& _value);

		template<class T>
		T CreateStructFromCSV(const std::vector<std::string>& _row, const TCsvHeaderMap& _headerMap);
	};

	template<class T>
	inline void CsvLoader::AssignValue(T& _field, const std::string& _value)
	{
		try {
			if constexpr (std::is_same_v<T, int> || std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>) {
				_field = std::stoi(_value);
			}
			else if constexpr (std::is_same_v<T, double>) {
				_field = std::stod(_value);
			}
			else if constexpr (std::is_same_v<T, float>) {
				_field = std::stof(_value);
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				_field = _value;
			}
			else if constexpr (std::is_same_v<T, EPlayerSkill>) {
				_field = StringToEnum<EPlayerSkill>(_value);
			}
			else if constexpr (std::is_same_v<T, ENpcType>) {
				_field = StringToEnum<ENpcType>(_value);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Wrong Value: " + _value);
		}
	}

	template<class T>
	inline T CsvLoader::CreateStructFromCSV(const std::vector<std::string>& _row, const TCsvHeaderMap& _headerMap)
	{
		T dataStruct;
		constexpr auto fieldNames = boost::pfr::names_as_array<T>();
		boost::pfr::for_each_field(dataStruct, [&_row, &_headerMap, &fieldNames, this](auto& _field, size_t _index) {
			std::string fieldName(fieldNames[_index]);
			auto it = _headerMap.find(fieldName);
			if (it != _headerMap.end()) {
				AssignValue(_field, _row[it->second]);
			}
			});
		return dataStruct;
	}

}