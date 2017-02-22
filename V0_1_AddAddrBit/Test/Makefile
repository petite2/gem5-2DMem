CXX = g++
CFLAGS = -std=c++11 
NAME = test_prog
SOURCEC = $(NAME).cpp
TARGET = $(NAME)-bin
RM = rm

$(TARGET): $(SOURCEC)
	$(CXX) $(CFLAGS) $(SOURCEC) -o $(TARGET)

clean:
	$(RM) $(TARGET)