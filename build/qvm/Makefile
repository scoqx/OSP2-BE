#
# GNU Make required
#

COMPILE_PLATFORM=$(shell uname | sed -e 's/_.*//' | tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
COMPILE_ARCH=$(shell uname -m | sed -e 's/i.86/x86/' | sed -e 's/^arm.*/arm/')

ifeq ($(COMPILE_PLATFORM),mingw32)
  ifeq ($(COMPILE_ARCH),i386)
    COMPILE_ARCH=x86
  endif
endif

#############################################################################
#
# If you require a different configuration from the defaults below, create a
# new file named "Makefile.local" in the same directory as this file and define
# your parameters there. This allows you to change configuration without
# causing problems with keeping up to date with the repository.
#
#############################################################################
-include Makefile.local

ifndef MOD
MOD=osp2
endif

ifeq ($(COMPILE_PLATFORM),cygwin)
  PLATFORM=mingw32
endif

ifndef PLATFORM
PLATFORM=$(COMPILE_PLATFORM)
endif
export PLATFORM

ifeq ($(PLATFORM),mingw32)
  MINGW=1
endif
ifeq ($(PLATFORM),mingw64)
  MINGW=1
endif
ifeq ($(PLATFORM),windows)
  MINGW=1
endif

ifeq ($(COMPILE_ARCH),i86pc)
  COMPILE_ARCH=x86
endif

ifeq ($(COMPILE_ARCH),amd64)
  COMPILE_ARCH=x86_64
endif
ifeq ($(COMPILE_ARCH),x64)
  COMPILE_ARCH=x86_64
endif

ifndef ARCH
ARCH=$(COMPILE_ARCH)
endif
export ARCH

ifneq ($(PLATFORM),$(COMPILE_PLATFORM))
  CROSS_COMPILING=1
else
  CROSS_COMPILING=0

  ifneq ($(ARCH),$(COMPILE_ARCH))
    CROSS_COMPILING=1
  endif
endif
export CROSS_COMPILING


include config.mk

srcs = $(foreach file,$1,$(if $(patsubst %.asm,,$(file)),$2/$(file).asm,$(file)))
qa_asm = $(call srcs,$(QA_SRC),vm/game)
cg_asm = $(call srcs,$(CG_SRC),vm/cgame)
ui_asm = $(call srcs,$(UI_SRC),vm/ui)

ifeq ($(MINGW),1)
  PYTHON ?= python
else
  PYTHON ?= python3
endif

all: dirs $(PK3)

dirs:
	mkdir -p vm/game vm/cgame vm/ui
ifeq ($(PLATFORM),linux)
	@if [ -f tools/q3lcc ]; then chmod +x tools/q3lcc; fi
	@if [ -f tools/q3asm ]; then chmod +x tools/q3asm; fi
	@if [ -f tools/q3cpp ]; then chmod +x tools/q3cpp; fi
	@if [ -f tools/q3rcc ]; then chmod +x tools/q3rcc; fi
endif

.PHONY: all dirs FORCE check_cvardesc

check_cvardesc: FORCE
	@$(PYTHON) ../../scripts/check_cvardesc_len.py

FORCE:

../../assets/version.txt: FORCE $(CGDIR)/cg_local.h
	@echo "Extracting version from $(CGDIR)/cg_local.h"
	@grep -E '^#define OSP_VERSION' $(CGDIR)/cg_local.h | sed -E 's/^#define OSP_VERSION "([^"]*)".*/\1/' > $@
	@echo "Version file created: $$(cat $@)"

../../assets/version_changelog.txt: FORCE ../../changelog_be.txt ../../assets/version.txt
	@echo "Extracting changelog for current version"
	@VERSION_NUM=$$(cat ../../assets/version.txt | sed 's/be-//'); \
	awk -v ver="$$VERSION_NUM" ' \
		BEGIN { found=0; printing=0 } \
		/^[0-9][0-9]\.[0-9][0-9]\.[0-9][0-9][0-9][0-9] be v/ { \
			if (printing) exit; \
			if ($$0 ~ "be v" ver) { \
				found=1; \
				printing=1; \
			} else { \
				printing=0; \
			} \
		} \
		printing { print } \
		END { \
			if (!found) { \
				print "Version not found: be v" ver > "/dev/stderr"; \
				exit 0; \
			} \
		} \
	' ../../changelog_be.txt > $@.tmp; \
	if [ -s $@.tmp ]; then \
		mv $@.tmp $@; \
		echo "Changelog file created for version: $$(head -n 1 $@)"; \
	else \
		rm -f $@.tmp; \
		if [ -f $@ ]; then \
			echo "Version be v$$VERSION_NUM not found in changelog, keeping existing version_changelog.txt"; \
		else \
			echo "Version be v$$VERSION_NUM not found in changelog, creating placeholder"; \
			echo "Version be v$$VERSION_NUM not found in changelog" > $@; \
		fi; \
	fi

#$(PK3): vm/qagame.qvm vm/cgame.qvm vm/ui.qvm
$(PK3): vm/cgame.qvm vm/ui.qvm ../../assets/version.txt ../../assets/version_changelog.txt
	@echo "Creating package with version files..."
	@echo "Version: $$(cat ../../assets/version.txt)"
	@$(7Z) $@ vm/*.qvm vm/*.jts ../../assets/*

cc = cd vm/$1 && $(Q3LCC) $2 -o $(notdir $@).tmp ../../$< && mv $(notdir $@).tmp $(notdir $@)
qa_cc = $(call cc,game,$(QA_CFLAGS))
cg_cc = $(call cc,cgame,$(CG_CFLAGS))
ui_cc = $(call cc,ui,$(UI_CFLAGS))

#vm/qagame.qvm: $(qa_asm)
#	$(Q3ASM) -o $@ $(qa_asm)

vm/cgame.qvm: check_cvardesc $(cg_asm)
	$(Q3ASM) -o $@ $(cg_asm)

vm/ui.qvm: $(ui_asm)
	$(Q3ASM) -o $@ $(ui_asm)

vm/game/%.asm: $(QADIR)/%.c
	$(qa_cc)

vm/cgame/%.asm: $(QADIR)/%.c
	$(cg_cc)

vm/cgame/%.asm: $(CGDIR)/%.c
	$(cg_cc)

vm/cgame/%.asm: $(COMDIR)/%.c
	$(cg_cc)

vm/ui/%.asm: $(UIDIR)/%.c
	$(ui_cc)
	@touch $@

vm/ui/%.asm: $(QADIR)/%.c
	$(ui_cc)
	@touch $@

vm/ui/%.asm: $(COMDIR)/%.c
	$(ui_cc)
	@touch $@

clean:
	$(RM) -rf *.pk3 vm/ ../cgame/*.asm ../game/*.asm ../ui/*.asm
