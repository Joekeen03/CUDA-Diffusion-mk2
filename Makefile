MAIN_DIR = .
SRC_DIR = $(MAIN_DIR)/src
BUILD_DIR = $(MAIN_DIR)/build
OBJECT_DIR = $(BUILD_DIR)/objects
SHADER_SRC_DIR = $(SRC_DIR)/shaders
SHADER_BUILD_DIR = $(BUILD_DIR)/shaders

SRC = $(SRC_DIR)/imageRenderer.cpp
OBJECTS = $(OBJECT_DIR)/imageRenderer.o $(OBJECT_DIR)/main.o
EXEC_NAME = imageRenderer.exe
EXEC_PATH = $(BUILD_DIR)/$(EXEC_NAME)

SHADERS_SRC = $(wildcard $(SHADER_SRC_DIR)/*)
SHADERS_BUILD = $(SHADERS_SRC:$(SHADER_SRC_DIR)/%=$(SHADER_BUILD_DIR)/%)

clean:
	-rm $(OBJECTS)

diffusion: ORIG_DIR = $(CURDIR)
diffusion: $(EXEC_PATH) shaders
	cd $(BUILD_DIR)
	$(info $(ORIG_DIR))
	$(info $(CURDIR))
	$(EXEC_PATH)
	cd $(ORIG_DIR)

shaders: $(SHADERS_BUILD)

$(EXEC_PATH): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ -llibfreeglut -lopengl32 -lglew32

.SECONDEXPANSION:
$(OBJECT_DIR)/%.o : $(SRC_DIR)/%.cpp | $$(@D)/
	$(CXX) -c $< -o $@ -llibfreeglut -lopengl32 -lglew32

.SECONDEXPANSION:
$(SHADER_BUILD_DIR)/%.fs : $(SHADER_SRC_DIR)/%.fs | $$(@D)/
	cp $< $@

.SECONDEXPANSION:
$(SHADER_BUILD_DIR)/%.vs : $(SHADER_SRC_DIR)/%.vs | $$(@D)/
	cp $< $@


.SECONDARY:

%/:
	# assume linux environment - mkdir uses -p for recursive, isn't recursive by default
	-mkdir $@ -p