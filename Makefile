CXX ?= g++
EIGEN_INC ?= /usr/include/eigen3
CXXFLAGS ?= -std=c++17 -O2 -DNDEBUG

TARGET = challenge1

all: $(TARGET)

$(TARGET): challenge1.cpp stb_image.h stb_image_write.h
	$(CXX) $(CXXFLAGS) -I$(EIGEN_INC) challenge1.cpp -o $(TARGET)

run: $(TARGET)
	./$(TARGET) deer.jpg

lis-image: $(TARGET)
	./$(TARGET) lis-image /tmp/x_lis.mtx outputs/lis_solution_x.png 656 656

clean:
	rm -f $(TARGET)
