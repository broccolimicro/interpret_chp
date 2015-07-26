SRCDIR       =  interpret_chp
CXXFLAGS	 =  -O2 -g -Wall -fmessage-length=0 -I../interpret_arithmetic -I../interpret_ucs -I../parse_expression -I../parse_ucs -I../parse_dot -I../parse_chp -I../petri -I../chp -I../ucs -I../arithmetic -I../parse -I../common
SOURCES	    :=  $(shell find $(SRCDIR) -name '*.cpp')
OBJECTS	    :=  $(SOURCES:%.cpp=%.o)
TARGET		 =  lib$(SRCDIR).a

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rvs $(TARGET) $(OBJECTS)

%.o: $(SRCDIR)/%.cpp 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $<
	
clean:
	rm -f $(OBJECTS) $(TARGET)
