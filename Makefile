CXX ?= g++
EIGEN_INC ?= /usr/include/eigen3
CXXFLAGS ?= -std=c++17 -O2 -DNDEBUG

TARGET = challenge1
IMAGE ?= deer.jpg
LIS_TEST ?= /shared-folder/lis2.1.13_test/test1

.PHONY: all run eigen-run lis-image clean

all: $(TARGET)

$(TARGET): challenge1.cpp stb_image.h stb_image_write.h
	$(CXX) $(CXXFLAGS) -I$(EIGEN_INC) challenge1.cpp -o $(TARGET)

run: $(TARGET)
	bash run_all.sh "$(IMAGE)" "$(LIS_TEST)"

eigen-run: $(TARGET)
	./$(TARGET) "$(IMAGE)"

lis-image: $(TARGET)
	./$(TARGET) lis-image /tmp/x_lis.mtx outputs/lis_solution_x.png $$(awk '/^image_rows_m:/{print $$2}' outputs/results.txt) $$(awk '/^image_cols_n:/{print $$2}' outputs/results.txt)

clean:
	rm -f $(TARGET)
