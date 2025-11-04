BUILD ?= obj

CFLAGS += -g -O3 -mrtm -D_GNU_SOURCE=1 
LDFLAGS += -no-pie
LIBS += -L./source/libcrypto -Wl,-rpath=./source/libcrypto -lcrypto

all: flush_reload meltdown spectre
.PHONY: clean

-include source/Makefile

flush_reload-obj = $(addprefix ${BUILD}/, ${flush_reload-obj-y})
meltdown-obj = $(addprefix ${BUILD}/, ${meltdown-obj-y})
spectre-obj = $(addprefix ${BUILD}/, ${spectre-obj-y})

deps += $(flush_reload-obj:.o=.d)
deps += $(meltdown-obj:.o=.d)
deps += $(spectre-obj:.o=.d)

-include ${deps}

flush_reload: ${flush_reload-obj}
	@echo "LD $(notdir $@)"
	@mkdir -p "$(dir $@)"
	@${CC} ${flush_reload-obj} -o $@ ${LDFLAGS} ${CFLAGS} ${LIBS}
	@rm -rf $(BUILD)

meltdown: ${meltdown-obj}
	@echo "LD $(notdir $@)"
	@mkdir -p "$(dir $@)"
	@${CC} ${meltdown-obj} -o $@ ${LDFLAGS} ${CFLAGS}
	@rm -rf $(BUILD)

spectre: ${spectre-obj}
	@echo "LD $(notdir $@)"
	@mkdir -p "$(dir $@)"
	@${CC} ${spectre-obj} -o $@ ${LDFLAGS} ${CFLAGS}
	@rm -rf $(BUILD)

$(BUILD)/%.o: %.c
	@echo "CC $<"
	@mkdir -p "$(dir $@)"
	@${CC} -c $< -o $@ ${CFLAGS} -MT $@ -MMD -MP -MF $(@:.o=.d)

clean:
	@rm -f flush_reload meltdown spectre
