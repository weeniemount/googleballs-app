DEVKITA64_IMAGE ?= devkitpro/devkita64:20260219
PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
DOCKER_RUN := docker run --rm \
	--user $(shell id -u):$(shell id -g) \
	--env HOME=/tmp \
	--volume "$(PROJECT_DIR):/project" \
	--workdir /project \
	$(DEVKITA64_IMAGE)

.PHONY: build clean shell

build:
	$(DOCKER_RUN) make

clean:
	$(DOCKER_RUN) make clean

shell:
	docker run --rm -it \
		--user $(shell id -u):$(shell id -g) \
		--env HOME=/tmp \
		--volume "$(PROJECT_DIR):/project" \
		--workdir /project \
		$(DEVKITA64_IMAGE) bash
