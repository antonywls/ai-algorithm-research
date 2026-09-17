#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <limits>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

constexpr int PIXELS_PER_IMAGE = 780;
constexpr int NUMBER_OF_CLASSES = 10;

struct Image {
	int					label;
	std::vector<int>	pixels;
};

struct Params {
	int numThreads = static_cast<int>(std::thread::hardware_concurrency());
};

struct Config {
	std::ostream* log;
	const std::vector<Image>* trainImages;
	Params						params;
};

namespace {
	std::vector<Image> readInput(std::ifstream& in) {
		std::vector<Image> input;
		std::string line;
		while (std::getline(in, line)) {
			Image image;
			std::stringstream ss(line);
			std::string token;

			for (int i = 0; i < PIXELS_PER_IMAGE; ++i) {
				std::getline(ss, token, ',');
				image.pixels.push_back(std::stoi(token));
			}

			std::getline(ss, token, ',');
			image.label = std::stoi(token);

			input.emplace_back(image);
		}
		return input;
	}

	void writeResult(std::ofstream& out, const std::vector<int>& results) {
		for (const int result : results) {
			out << result << "\n";
		}
	}

	double macroF1(const std::vector<Image>* trainImages, const std::vector<int>& classifications) {
		double sum = 0;

		auto f1 = [](const double precision, const double recall) {
			return (2 * precision * recall) / (precision + recall);
			};

		for (int i = 0; i < NUMBER_OF_CLASSES; ++i) {
			double truePositive = 0, falsePositive = 0, falseNegative = 0;
			for (size_t j = 0; j < classifications.size(); ++j) {
				if (classifications[j] == i && i == (*trainImages)[j].label) {
					truePositive++;
				}
				else if (classifications[j] == i && i != (*trainImages)[j].label) {
					falsePositive++;
				}
				else if (classifications[j] != i && i == (*trainImages)[j].label) {
					falseNegative++;
				}
			}
			const double precision = truePositive / (truePositive + falsePositive);
			const double recall = truePositive / (truePositive + falseNegative);
			sum += f1(precision, recall);
		}
		return sum / NUMBER_OF_CLASSES;
	}
}

class NearestNeighbor {
public:
	explicit NearestNeighbor(const Config& config)
		: config(config) {
	}

	int classify(const std::vector<int>& pixels) const {
		int bestMatch = 0;
		int bestMatchDistance = std::numeric_limits<int>::max();

		for (const Image& image : *config.trainImages) {
			bool earlyAbandon = false;
			int distance = 0;
			for (int p = 0; p < PIXELS_PER_IMAGE; ++p) {
				distance += (image.pixels[p] - pixels[p]) * (image.pixels[p] - pixels[p]);
				if (distance > bestMatchDistance) {
					earlyAbandon = true;
					break;
				}
			}
			if (earlyAbandon) continue;
			if (distance < bestMatchDistance) {
				bestMatch = image.label;
				bestMatchDistance = distance;
			}
		}
		return bestMatch;
	}

	std::vector<int> classifyBatch(const std::vector<Image>& images) const {
		std::vector<int> results(images.size());
		std::atomic<size_t> currentIndex(0);
		std::mutex logMutex; 

		auto worker = [&]() {
			size_t index;
			while ((index = currentIndex.fetch_add(1)) < images.size()) {
				results[index] = classify(images[index].pixels);

				if (index % 1000 == 0) {
					std::lock_guard<std::mutex> lock(logMutex);
					*config.log << "Processed " << index << "/" << images.size() << " images\n";
				}
			}
			};

		std::vector<std::thread> threads;
		threads.reserve(config.params.numThreads);

		for (int i = 0; i < config.params.numThreads; ++i) {
			threads.emplace_back(worker);
		}

		for (auto& thread : threads) {
			thread.join();
		}

		return results;
	}

private:
	Config config;
};

int main() {

	using Clock = std::chrono::steady_clock;

	auto t0 = Clock::now();

	std::ifstream mnistTrain("mnist_train.csv");
	std::ifstream mnistTest("mnist_test.csv");

	std::ofstream resultTrain("result_train.csv");
	std::ofstream resultTest("result_test.csv");
	std::ofstream log("log.csv");

	const std::vector<Image> trainImages = readInput(mnistTrain);
	const std::vector<Image> testImages = readInput(mnistTest);

	Params params;

	Config config = {
		&log, &trainImages, params
	};

	auto t1 = Clock::now();

	log << "Config setup. Using " << params.numThreads << " threads. Time taken: "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "\n";

	NearestNeighbor nearestNeighbor(config);

	auto t2 = Clock::now();

	log << "Setup complete. Time taken: "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "\n";

	std::vector<int> trainClassifications = nearestNeighbor.classifyBatch(trainImages);

	auto t3 = Clock::now();

	log << "Training images classified. Time taken: "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << "\n";

	writeResult(resultTrain, trainClassifications);

	log << "Training image classification f1-score: " << macroF1(&trainImages, trainClassifications) << "\n";

	std::vector<int> testClassifications = nearestNeighbor.classifyBatch(testImages);

	auto t4 = Clock::now();

	log << "Test images classified. Time taken: "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count() << "\n";

	writeResult(resultTest, testClassifications);

	log << "Test image classification f1-score: " << macroF1(&testImages, testClassifications) << "\n";

	return 0;
}