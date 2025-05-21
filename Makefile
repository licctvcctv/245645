CXX = g++
CXXFLAGS = -std=c++11 -O2
OBJS = main.o Simulator.o Car.o
TARGET = car.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	del *.o *.exe 