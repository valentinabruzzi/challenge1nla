#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/Sparse>
#include <unsupported/Eigen/SparseExtra>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using ImageMatrix = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using ByteMatrix = Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using SparseMatrix = Eigen::SparseMatrix<double>;
using Vector = Eigen::VectorXd;
using Triplet = Eigen::Triplet<double>;

struct ImageData {
    ImageMatrix pixels;
    int rows;
    int cols;
};

unsigned char toByte(double value) {
    const double rounded = std::round(value);
    return static_cast<unsigned char>(std::clamp(rounded, 0.0, 255.0));
}

ImageData loadGreyscaleImage(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* image = stbi_load(path.c_str(), &width, &height, &channels, 1);
    if (!image) {
        throw std::runtime_error("Cannot load input image");
    }

    ImageMatrix pixels(height, width);
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            pixels(i, j) = static_cast<double>(image[i * width + j]);
        }
    }

    stbi_image_free(image);
    return {pixels, height, width};
}

Vector matrixToVector(const ImageMatrix& matrix) {
    Vector vector(matrix.rows() * matrix.cols());
    for (int i = 0; i < matrix.rows(); ++i) {
        for (int j = 0; j < matrix.cols(); ++j) {
            vector(i * matrix.cols() + j) = matrix(i, j);
        }
    }
    return vector;
}

ImageMatrix vectorToMatrix(const Vector& vector, int rows, int cols) {
    if (vector.size() != rows * cols) {
        throw std::runtime_error("Vector size does not match image dimensions");
    }

    ImageMatrix matrix(rows, cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix(i, j) = vector(i * cols + j);
        }
    }
    return matrix;
}

void saveImage(const ImageMatrix& matrix, const std::string& path) {
    ByteMatrix output(matrix.rows(), matrix.cols());
    for (int i = 0; i < matrix.rows(); ++i) {
        for (int j = 0; j < matrix.cols(); ++j) {
            output(i, j) = toByte(matrix(i, j));
        }
    }

    if (stbi_write_png(path.c_str(), matrix.cols(), matrix.rows(), 1, output.data(), matrix.cols()) == 0) {
        throw std::runtime_error("Cannot save output image");
    }
}

ImageMatrix addNoise(const ImageMatrix& original) {
    ImageMatrix noisy(original.rows(), original.cols());
    std::mt19937 generator(20261004);
    std::uniform_int_distribution<int> distribution(-50, 50);

    for (int i = 0; i < original.rows(); ++i) {
        for (int j = 0; j < original.cols(); ++j) {
            noisy(i, j) = std::clamp(original(i, j) + static_cast<double>(distribution(generator)), 0.0, 255.0);
        }
    }
    return noisy;
}

SparseMatrix buildConvolutionMatrix(int rows, int cols, const std::vector<std::vector<double>>& kernel) {
    const int kernelRows = static_cast<int>(kernel.size());
    const int kernelCols = static_cast<int>(kernel.front().size());
    const int rowShift = (kernelRows - 1) / 2;
    const int colShift = (kernelCols - 1) / 2;
    const int total = rows * cols;

    int kernelNonZeros = 0;
    for (const auto& row : kernel) {
        for (double value : row) {
            if (value != 0.0) {
                ++kernelNonZeros;
            }
        }
    }

    std::vector<Triplet> triplets;
    triplets.reserve(static_cast<std::size_t>(total) * kernelNonZeros);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            const int rowIndex = i * cols + j;
            for (int k = 0; k < kernelRows; ++k) {
                for (int l = 0; l < kernelCols; ++l) {
                    const double value = kernel[k][l];
                    if (value == 0.0) {
                        continue;
                    }
                    const int sourceRow = i + k - rowShift;
                    const int sourceCol = j + l - colShift;
                    if (sourceRow < 0 || sourceRow >= rows || sourceCol < 0 || sourceCol >= cols) {
                        continue;
                    }
                    const int colIndex = sourceRow * cols + sourceCol;
                    triplets.emplace_back(rowIndex, colIndex, value);
                }
            }
        }
    }

    SparseMatrix matrix(total, total);
    matrix.setFromTriplets(triplets.begin(), triplets.end());
    matrix.makeCompressed();
    return matrix;
}

double symmetryNorm(const SparseMatrix& matrix) {
    SparseMatrix difference = matrix - SparseMatrix(matrix.transpose());
    return difference.norm();
}

void saveLisVector(const Vector& vector, const std::string& path) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Cannot save LIS vector");
    }

    output << "%%MatrixMarket vector coordinate real general\n";
    output << vector.size() << "\n";
    output << std::setprecision(17);
    for (Eigen::Index i = 0; i < vector.size(); ++i) {
        output << i + 1 << " " << vector(i) << "\n";
    }
}

Vector readLisVector(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot read LIS vector");
    }

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line[0] != '%') {
            break;
        }
    }

    if (line.empty()) {
        throw std::runtime_error("Invalid LIS vector");
    }

    const int size = std::stoi(line);
    Vector vector = Vector::Zero(size);
    int index = 0;
    double value = 0.0;
    int count = 0;
    while (input >> index >> value) {
        if (index < 1 || index > size) {
            throw std::runtime_error("Invalid LIS vector index");
        }
        vector(index - 1) = value;
        ++count;
    }

    if (count == 0) {
        input.clear();
        input.seekg(0);
        std::vector<double> values;
        double plainValue = 0.0;
        while (input >> plainValue) {
            values.push_back(plainValue);
        }
        if (static_cast<int>(values.size()) != size) {
            throw std::runtime_error("Unsupported LIS vector format");
        }
        for (int i = 0; i < size; ++i) {
            vector(i) = values[i];
        }
    }

    return vector;
}

void appendKeyValue(std::ofstream& output, const std::string& key, const std::string& value) {
    output << key << ": " << value << "\n";
    output.flush();
}

void appendKeyValue(std::ofstream& output, const std::string& key, double value) {
    output << key << ": " << std::setprecision(16) << value << "\n";
    output.flush();
}

void appendKeyValue(std::ofstream& output, const std::string& key, long long value) {
    output << key << ": " << value << "\n";
    output.flush();
}

int prepare(const std::string& imagePath) {
    std::filesystem::create_directories("outputs");
    std::filesystem::create_directories("data");

    const auto image = loadGreyscaleImage(imagePath);
    const ImageMatrix noisy = addNoise(image.pixels);
    const Vector v = matrixToVector(image.pixels);
    const Vector w = matrixToVector(noisy);
    const int total = image.rows * image.cols;

    saveImage(noisy, "outputs/noisy.png");
    saveLisVector(w, "data/w_lis.mtx");

    std::ofstream results("outputs/results.txt");
    appendKeyValue(results, "image_rows_m", static_cast<long long>(image.rows));
    appendKeyValue(results, "image_cols_n", static_cast<long long>(image.cols));
    appendKeyValue(results, "mn", static_cast<long long>(total));
    appendKeyValue(results, "v_size", static_cast<long long>(v.size()));
    appendKeyValue(results, "w_size", static_cast<long long>(w.size()));
    appendKeyValue(results, "euclidean_norm_v", v.norm());

    const std::vector<std::vector<double>> smoothing = {
        {1.0 / 12.0, 1.0 / 12.0, 1.0 / 12.0},
        {1.0 / 12.0, 4.0 / 12.0, 1.0 / 12.0},
        {1.0 / 12.0, 1.0 / 12.0, 1.0 / 12.0}
    };

    {
        SparseMatrix a1 = buildConvolutionMatrix(image.rows, image.cols, smoothing);
        appendKeyValue(results, "A1_nonzeros", static_cast<long long>(a1.nonZeros()));
        saveImage(vectorToMatrix(a1 * w, image.rows, image.cols), "outputs/smoothing_A1w.png");
    }

    const std::vector<std::vector<double>> sharpening = {
        {0.0, -3.0, 0.0},
        {-1.0, 9.0, -3.0},
        {0.0, -1.0, 0.0}
    };

    {
        SparseMatrix a2 = buildConvolutionMatrix(image.rows, image.cols, sharpening);
        const double a2SymmetryNorm = symmetryNorm(a2);
        appendKeyValue(results, "A2_nonzeros", static_cast<long long>(a2.nonZeros()));
        appendKeyValue(results, "A2_symmetry_norm", a2SymmetryNorm);
        appendKeyValue(results, "A2_is_symmetric", a2SymmetryNorm < 1.0e-12 ? "yes" : "no");
        saveImage(vectorToMatrix(a2 * v, image.rows, image.cols), "outputs/sharpening_A2v.png");
        Eigen::saveMarket(a2, "data/A2.mtx");
    }

    const std::vector<std::vector<double>> edgeDetection = {
        {-1.0, 0.0, 1.0},
        {-2.0, 0.0, 2.0},
        {-1.0, 0.0, 1.0}
    };

    {
        SparseMatrix a3 = buildConvolutionMatrix(image.rows, image.cols, edgeDetection);
        const double a3SymmetryNorm = symmetryNorm(a3);
        appendKeyValue(results, "A3_nonzeros", static_cast<long long>(a3.nonZeros()));
        appendKeyValue(results, "A3_symmetry_norm", a3SymmetryNorm);
        appendKeyValue(results, "A3_is_symmetric", a3SymmetryNorm < 1.0e-12 ? "yes" : "no");
        saveImage(vectorToMatrix(a3 * v, image.rows, image.cols), "outputs/edge_A3v.png");

        SparseMatrix identity(total, total);
        identity.setIdentity();
        a3 = a3 + 4.0 * identity;
        a3.makeCompressed();

        Eigen::BiCGSTAB<SparseMatrix, Eigen::DiagonalPreconditioner<double>> solver;
        solver.setTolerance(1.0e-10);
        solver.setMaxIterations(2000);
        const auto start = std::chrono::steady_clock::now();
        solver.compute(a3);
        const Vector initialGuess = w / 4.0;
        const Vector y = solver.solveWithGuess(w, initialGuess);
        const auto stop = std::chrono::steady_clock::now();

        const double elapsed = std::chrono::duration<double>(stop - start).count();
        const double trueResidual = (a3 * y - w).norm() / w.norm();
        appendKeyValue(results, "Eigen_solver", "BiCGSTAB with DiagonalPreconditioner and initial guess w/4");
        appendKeyValue(results, "Eigen_tolerance", 1.0e-10);
        appendKeyValue(results, "Eigen_iterations", static_cast<long long>(solver.iterations()));
        appendKeyValue(results, "Eigen_reported_residual", solver.error());
        appendKeyValue(results, "Eigen_true_relative_residual", trueResidual);
        appendKeyValue(results, "Eigen_elapsed_seconds", elapsed);
        saveImage(vectorToMatrix(y, image.rows, image.cols), "outputs/eigen_solution_y.png");
    }

    return 0;
}

int lisImage(const std::string& vectorPath, const std::string& outputPath, int rows, int cols) {
    const Vector x = readLisVector(vectorPath);
    saveImage(vectorToMatrix(x, rows, cols), outputPath);
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            return prepare(argv[1]);
        }

        if (argc == 6 && std::string(argv[1]) == "lis-image") {
            return lisImage(argv[2], argv[3], std::stoi(argv[4]), std::stoi(argv[5]));
        }

        std::cerr << "Usage:\n";
        std::cerr << "  " << argv[0] << " <input_image>\n";
        std::cerr << "  " << argv[0] << " lis-image <lis_vector> <output_png> <rows> <cols>\n";
        return 1;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << "\n";
        return 1;
    }
}
