#include "pch.h"
#include "CsvLoader.h"

namespace wod_server {
    void CsvLoader::Load(const std::string& _filename)
    {
        const auto& csvData = ReadCsv(_filename);

        if (csvData.empty()) {
            LogPrinter::PrintMsg("Failed To Load " + _filename);
            exit(0);
        }

        TCsvHeaderMap headerMap = CreateHeaderMap(csvData[0]);
        LoadData(csvData, headerMap);
    }

    void CsvLoader::LoadData(const TCsvData& _datas, const TCsvHeaderMap& _csvHeader)
    {
    }

    std::vector<std::vector<std::string>> CsvLoader::ReadCsv(const std::string& _filename)
    {
        std::ifstream file(_filename);
        if (!file.is_open()) {
            LogPrinter::PrintMsg("Failed To Open " + _filename);
            exit(0);
        }

        std::vector<std::vector<std::string>> data;
        std::string line;

        while (std::getline(file, line)) {
            std::vector<std::string> row;
            std::stringstream lineStream(line);
            std::string cell;

            while (std::getline(lineStream, cell, ',')) {
                row.push_back(cell);
            }
            data.push_back(row);
        }
        return data;
    }

    std::unordered_map<std::string, int> CsvLoader::CreateHeaderMap(const std::vector<std::string>& _csvHeader)
    {
        std::unordered_map<std::string, int> headerMap;
        for (auto i = 0; i < _csvHeader.size(); ++i) {
            std::string cleanedHeader = _csvHeader[i];

            size_t openParenPos;
            while ((openParenPos = cleanedHeader.find('(')) != std::string::npos) {
                size_t closeParenPos = cleanedHeader.find(')', openParenPos);
                if (closeParenPos != std::string::npos) {
                    cleanedHeader.erase(openParenPos, closeParenPos - openParenPos + 1);
                }
                else {
                    cleanedHeader.erase(openParenPos, std::string::npos);
                }
            }

            cleanedHeader.erase(std::remove(cleanedHeader.begin(), cleanedHeader.end(), '#'), cleanedHeader.end());
            headerMap[cleanedHeader] = i;
        }

        return headerMap;
    }

}