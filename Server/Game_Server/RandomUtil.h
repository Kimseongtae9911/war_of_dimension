#pragma once

namespace wod_server {

class RandomUtil
{
public:
	static std::unordered_set<uint8_t> GenerateUniqueRandomNumbers(int _min, int _max, int _count) {
		std::random_device rd;
		std::mt19937 engine(rd());

		std::unordered_set<uint8_t> uniqueNumbers;
		std::uniform_int_distribution<int> distribution(_min, _max);

		while (uniqueNumbers.size() < _count) {
			int randomNumber = distribution(engine);
			uniqueNumbers.insert(randomNumber);
		}

		return uniqueNumbers;
	}
};

}