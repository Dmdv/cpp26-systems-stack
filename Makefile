# cpp26-systems-stack — developer workflows
#
# Usage:
#   make help
#   make            # configure base + build + test
#   make full       # Folly + HPX
#   make run
#   make docs            # list library guides

.PHONY: help all configure configure-folly configure-hpx configure-full \
	build rebuild test run smoke examples bench \
	folly hpx full clean distclean \
	reconfigure compile-commands \
	install-folly install-hpx install-industry deps-check docs

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------
CMAKE       ?= cmake
CTEST       ?= ctest
BUILD_TYPE  ?= Release
JOBS        ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

HOMEBREW_PREFIX ?= $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
HPX_ROOT        ?= $(HOME)/cpp-deps/hpx

# Separate build trees so option combos don't stomp each other
BUILD_DIR      ?= build
BUILD_FOLLY    ?= build_folly
BUILD_HPX      ?= build_hpx
BUILD_FULL     ?= build_full

CMAKE_COMMON = -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	-DCMAKE_PREFIX_PATH="$(HOMEBREW_PREFIX);$(HPX_ROOT)" \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Default goal
all: configure build test

help:
	@echo "cpp26-systems-stack — C++26 systems ecosystem"
	@echo ""
	@echo "Build / test"
	@echo "  make / make all     Configure base, build, test"
	@echo "  make configure      CMake base (no Folly/HPX)"
	@echo "  make build          Build current BUILD_DIR ($(BUILD_DIR))"
	@echo "  make rebuild        Clean + configure + build + test"
	@echo "  make test           ctest in BUILD_DIR"
	@echo "  make run / smoke    Run lib_smoke binary"
	@echo "  make examples       Build+run ll + industry examples"
	@echo "  make bench          Build+run Google Benchmark queue microbenches"
	@echo "  make clean          Remove object files (keep CMake cache)"
	@echo "  make distclean      Remove all build_* trees"
	@echo ""
	@echo "Optional stacks"
	@echo "  make folly          Configure+build+test with Folly"
	@echo "  make hpx            Configure+build+test with HPX"
	@echo "  make full           Configure+build+test with Folly+HPX"
	@echo "  make configure-folly / configure-hpx / configure-full"
	@echo ""
	@echo "Dependencies"
	@echo "  make deps-check     Show Homebrew + HPX + industry install status"
	@echo "  make install-folly  brew install folly"
	@echo "  make install-hpx    Build/install HPX to HPX_ROOT ($(HPX_ROOT))"
	@echo "  make install-industry  brew: hwloc flatbuffers google-benchmark mimalloc"
	@echo ""
	@echo "Docs"
	@echo "  make docs           List library guides + blueprint + tutorials"
	@echo ""
	@echo "Variables: BUILD_TYPE=$(BUILD_TYPE) JOBS=$(JOBS) BUILD_DIR=$(BUILD_DIR)"
	@echo "           HPX_ROOT=$(HPX_ROOT) HOMEBREW_PREFIX=$(HOMEBREW_PREFIX)"

# ---------------------------------------------------------------------------
# Configure
# ---------------------------------------------------------------------------
configure:
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_COMMON) \
		-DLIB_SMOKE_WITH_FOLLY=OFF \
		-DLIB_SMOKE_WITH_HPX=OFF

configure-folly:
	$(CMAKE) -S . -B $(BUILD_FOLLY) $(CMAKE_COMMON) \
		-DLIB_SMOKE_WITH_FOLLY=ON \
		-DLIB_SMOKE_WITH_HPX=OFF

configure-hpx:
	$(CMAKE) -S . -B $(BUILD_HPX) $(CMAKE_COMMON) \
		-DLIB_SMOKE_WITH_FOLLY=OFF \
		-DLIB_SMOKE_WITH_HPX=ON \
		-DLIB_SMOKE_HPX_ROOT="$(HPX_ROOT)"

configure-full:
	$(CMAKE) -S . -B $(BUILD_FULL) $(CMAKE_COMMON) \
		-DLIB_SMOKE_WITH_FOLLY=ON \
		-DLIB_SMOKE_WITH_HPX=ON \
		-DLIB_SMOKE_HPX_ROOT="$(HPX_ROOT)"

reconfigure: configure

# ---------------------------------------------------------------------------
# Build / test / run
# ---------------------------------------------------------------------------
build:
	@test -d $(BUILD_DIR) || $(MAKE) configure
	$(CMAKE) --build $(BUILD_DIR) -j$(JOBS)

rebuild: distclean all

test:
	@test -d $(BUILD_DIR) || $(MAKE) build
	cd $(BUILD_DIR) && $(CTEST) --output-on-failure -j$(JOBS)

run smoke:
	@test -x $(BUILD_DIR)/lib_smoke || $(MAKE) build
	./$(BUILD_DIR)/lib_smoke

examples:
	@test -d $(BUILD_DIR) || $(MAKE) configure
	$(CMAKE) --build $(BUILD_DIR) -j$(JOBS) --target \
		example_spsc example_arena example_memory_order example_tsc \
		example_pmr example_hdr example_sbe_style example_industry_queues
	@echo "== example_spsc =="
	./$(BUILD_DIR)/example_spsc
	@echo "== example_arena =="
	./$(BUILD_DIR)/example_arena
	@echo "== example_memory_order =="
	./$(BUILD_DIR)/example_memory_order
	@echo "== example_tsc =="
	./$(BUILD_DIR)/example_tsc
	@echo "== example_pmr =="
	./$(BUILD_DIR)/example_pmr
	@echo "== example_hdr =="
	./$(BUILD_DIR)/example_hdr
	@echo "== example_sbe_style =="
	./$(BUILD_DIR)/example_sbe_style
	@echo "== example_industry_queues =="
	./$(BUILD_DIR)/example_industry_queues

bench:
	@test -d $(BUILD_DIR) || $(MAKE) configure
	@if $(CMAKE) --build $(BUILD_DIR) -j$(JOBS) --target bench_queues 2>/dev/null; then \
		./$(BUILD_DIR)/bench_queues --benchmark_min_time=0.1s; \
	else \
		echo "bench_queues not configured (brew install google-benchmark && reconfigure)"; \
		exit 1; \
	fi

# Convenience stacks (own build dirs)
folly: configure-folly
	$(CMAKE) --build $(BUILD_FOLLY) -j$(JOBS)
	cd $(BUILD_FOLLY) && $(CTEST) --output-on-failure -j$(JOBS)
	./$(BUILD_FOLLY)/lib_smoke

hpx: configure-hpx
	$(CMAKE) --build $(BUILD_HPX) -j$(JOBS)
	cd $(BUILD_HPX) && $(CTEST) --output-on-failure -j$(JOBS)
	./$(BUILD_HPX)/lib_smoke

full: configure-full
	$(CMAKE) --build $(BUILD_FULL) -j$(JOBS)
	cd $(BUILD_FULL) && $(CTEST) --output-on-failure -j$(JOBS)
	./$(BUILD_FULL)/lib_smoke

compile-commands: configure
	@ln -sfn $(BUILD_DIR)/compile_commands.json compile_commands.json
	@echo "Linked compile_commands.json -> $(BUILD_DIR)/compile_commands.json"

# ---------------------------------------------------------------------------
# Clean
# ---------------------------------------------------------------------------
clean:
	@if [ -d $(BUILD_DIR) ]; then $(CMAKE) --build $(BUILD_DIR) --target clean; fi

distclean:
	rm -rf $(BUILD_DIR) $(BUILD_FOLLY) $(BUILD_HPX) $(BUILD_FULL)
	rm -f compile_commands.json

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
deps-check:
	@echo "== Homebrew (base) =="
	@brew --prefix >/dev/null 2>&1 && echo "  brew: $(HOMEBREW_PREFIX)" || echo "  brew: MISSING"
	@for p in fmt spdlog tbb asio boost taskflow folly catch2 googletest \
		nlohmann-json simdjson eigen protobuf grpc range-v3 abseil; do \
		if brew list --versions $$p >/dev/null 2>&1; then \
			printf "  %-16s %s\n" "$$p" "$$(brew list --versions $$p)"; \
		else \
			printf "  %-16s MISSING\n" "$$p"; \
		fi; \
	done
	@echo "== Homebrew (industry / low-latency) =="
	@for p in hwloc flatbuffers google-benchmark mimalloc jemalloc; do \
		if brew list --versions $$p >/dev/null 2>&1; then \
			printf "  %-16s %s\n" "$$p" "$$(brew list --versions $$p)"; \
		else \
			printf "  %-16s MISSING (make install-industry)\n" "$$p"; \
		fi; \
	done
	@echo "== HPX (local) =="
	@if [ -f "$(HPX_ROOT)/lib/cmake/HPX/HPXConfig.cmake" ]; then \
		echo "  installed: $(HPX_ROOT)"; \
	else \
		echo "  not found at $(HPX_ROOT)  (run: make install-hpx)"; \
	fi

install-folly:
	brew install folly

install-industry:
	brew install hwloc flatbuffers google-benchmark mimalloc

install-hpx:
	@chmod +x scripts/install_hpx.sh
	HPX_ROOT="$(HPX_ROOT)" ./scripts/install_hpx.sh

# ---------------------------------------------------------------------------
# Docs
# ---------------------------------------------------------------------------
docs:
	@echo "Library guides (docs/libraries/):"
	@ls -1 docs/libraries/*.md 2>/dev/null | sed 's|^|  |' || echo "  (none yet)"
	@echo ""
	@echo "Low-latency blueprint (docs/blueprint/):"
	@ls -1 docs/blueprint/*.md 2>/dev/null | sed 's|^|  |' || echo "  (none yet)"
	@echo ""
	@echo "Tutorials (docs/tutorials/):"
	@ls -1 docs/tutorials/*.md 2>/dev/null | sed 's|^|  |' || echo "  (none yet)"
	@echo ""
	@echo "Start here: docs/libraries/README.md"
	@echo "LL audit:   docs/blueprint/AUDIT.md"
	@echo "Industry:   docs/tutorials/industry-stack.md"
	@echo "LL focus:   docs/blueprint/README.md"
