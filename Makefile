GDBPORT	:= $(shell expr `id -u` % 5000 + 25000)

DOCKER := docker
DOCKRUN := container run
DOCKBUILD := build
DOCKNETWORK := network create
DOCKOPTS := -it --rm
DOCKOPTSGDB := --net=jos-net
DOCKIMG := jos-env
DOCKNET := jos-net
DOCKBASEIMG := 16

net:
	$(DOCKER) $(DOCKNETWORK) --driver=bridge $(DOCKNET)

build:
	$(DOCKER) $(DOCKBUILD) -t $(DOCKIMG):$(DOCKBASEIMG).04 -f Dockerfile-$(DOCKBASEIMG).04 .

gdb: $(IMAGES)
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKOPTSGDB) --name=gdb $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) gdb GDBSERV=jos:$(GDBPORT)

grade: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) grade

qemu: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) qemu

qemu-nox: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) qemu-nox

qemu-gdb: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKOPTSGDB) --name=jos $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) qemu-gdb GDBSERV=0.0.0.0:$(GDBPORT)

qemu-nox-gdb: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKOPTSGDB) --name=jos $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) qemu-nox-gdb GDBSERV=0.0.0.0:$(GDBPORT)

run-%: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) $@

run-%-nox: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) $@

run-%-gdb: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKOPTSGDB) --name=jos $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) $@ GDBSERV=0.0.0.0:$(GDBPORT)

run-%-nox-gdb: build
	$(DOCKER) $(DOCKRUN) $(DOCKOPTS) $(DOCKOPTSGDB) --name=jos $(DOCKIMG):$(DOCKBASEIMG).04 $(MAKE) $@ GDBSERV=0.0.0.0:$(GDBPORT)

.PHONY: net build gdb grade qemu qemu-nox qemu-gdb
