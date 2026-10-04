#pragma once

namespace wod_server {

class RandomUtil
{
public:
	static std::unordered_set<uint8_t> GenerateUniqueRandomNumbers(int min, int max, int count) {
		std::random_device rd;
		std::mt19937 engine(rd());

		std::unordered_set<uint8_t> uniqueNumbers;
		std::uniform_int_distribution<int> distribution(min, max);

		while (uniqueNumbers.size() < count) {
			int randomNumber = distribution(engine);
			uniqueNumbers.insert(randomNumber);
		}

		return uniqueNumbers;
	}
};

}