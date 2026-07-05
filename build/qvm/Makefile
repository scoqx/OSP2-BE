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

ifndef JOB_SLOTS
JOB_SLOTS := $(shell nproc 2>/dev/null)
ifeq ($(JOB_SLOTS),)
  ifdef NUMBER_OF_PROCESSORS
    JOB_SLOTS := $(NUMBER_OF_PROCESSORS)
  else
    JOB_SLOTS := $(shell getconf _NPROCESSORS_ONLN 2>/dev/null)
  endif
endif
ifeq ($(JOB_SLOTS),)
JOB_SLOTS := 8
endif
endif
MAKEFLAGS += -j$(JOB_SLOTS)

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

ASSETS_DIR = ../../assets
ASSETS_PACK_SHA = .assets-pack-sha
ASSETS_PACKED_SHA = .assets-packed.sha

srcs = $(foreach file,$1,$(if $(patsubst %.asm,,$(file)),$2/$(file).asm,$(file)))
qa_asm = $(call srcs,$(QA_SRC),vm/game)
cg_asm = $(call srcs,$(CG_SRC),vm/cgame)
ui_asm = $(call srcs,$(UI_SRC),vm/ui)

ifeq ($(MINGW),1)
  PYTHON ?= $(shell command -v python3 2>/dev/null || command -v python 2>/dev/null || echo python3)
  define MINGW_PY_CMD
if command -v python3 >/dev/null 2>&1; then python3 $(1); elif command -v python >/dev/null 2>&1; then python $(1); elif command -v py >/dev/null 2>&1; then py -3 $(1); elif test -x /c/Windows/py.exe; then /c/Windows/py.exe -3 $(1); else exit 1; fi
  endef
else
  PYTHON ?= python3
endif

all: dirs $(PK3)

# Fast path: cgame.qvm + ui.qvm in parallel (-j); no cross-deps between modules
cgame: vm/cgame.qvm
ui: vm/ui.qvm
vm: cgame ui

vm/cgame.qvm vm/ui.qvm: | dirs

dirs:
	mkdir -p vm/game vm/cgame vm/cgame/chatWindow vm/cgame/superhud vm/cgame/superhud/elements vm/ui
ifeq ($(PLATFORM),linux)
	@if [ -f tools/q3lcc ]; then chmod +x tools/q3lcc; fi
	@if [ -f tools/q3asm ]; then chmod +x tools/q3asm; fi
	@if [ -f tools/q3cpp ]; then chmod +x tools/q3cpp; fi
	@if [ -f tools/q3rcc ]; then chmod +x tools/q3rcc; fi
endif

.PHONY: all vm cgame ui dirs FORCE check_cvardesc check_status_infostring

check_cvardesc: FORCE
ifeq ($(MINGW),1)
	@if $(call MINGW_PY_CMD,../../scripts/check_cvardesc_len.py); then \
		: ; \
	else \
		echo "Warning: Python not found, skipping cvar description check"; \
	fi
else
	@$(PYTHON) ../../scripts/check_cvardesc_len.py
endif

check_status_infostring: ../../assets/version.txt ../../assets/version_changelog.txt
ifeq ($(MINGW),1)
	@if $(call MINGW_PY_CMD,../../scripts/check_status_infostring.py); then \
		: ; \
	else \
		echo "Warning: Python not found, skipping status infostring check"; \
	fi
else
	@$(PYTHON) ../../scripts/check_status_infostring.py
endif

FORCE:

../../assets/version.txt: $(COMDIR)/osp_version.h
	@echo "Extracting version from $(COMDIR)/osp_version.h"
	@grep -E '^#define OSP_VERSION "' $(COMDIR)/osp_version.h | sed -E 's/^#define OSP_VERSION "([^"]*)".*/\1/' > $@
	@echo "Version file created: $$(cat $@)"

../../assets/docs/changelog_be_r.txt: ../../changelog_be.txt
	@echo "Reversing full changelog for UI"
	@mkdir -p ../../assets/docs
	@awk ' \
		BEGIN { n = 0; block = "" } \
		/^[0-9][0-9]\.[0-9][0-9]\.[0-9][0-9][0-9][0-9] be v/ { \
			if (block != "") { n++; blocks[n] = block } \
			block = $$0; \
			next \
		} \
		{ \
			if (block != "") block = block "\n" $$0; \
		} \
		END { \
			if (block != "") { n++; blocks[n] = block } \
			if (n == 0) { \
				print "No version blocks found in changelog!" > "/dev/stderr"; \
				exit 1 \
			} \
			for (i = n; i >= 1; i--) { \
				sub(/\n+$$/, "", blocks[i]); \
				print blocks[i]; \
				if (i > 1) print "" \
			} \
		} \
	' ../../changelog_be.txt > $@.tmp && mv $@.tmp $@

../../assets/version_changelog.txt: ../../changelog_be.txt ../../assets/version.txt
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

$(ASSETS_PACK_SHA): FORCE
	@if [ -f $(ASSETS_PACK_SHA) ] && ! find $(ASSETS_DIR) -type f -newer $(ASSETS_PACK_SHA) -print -quit | grep -q .; then \
		: ; \
	else \
		find $(ASSETS_DIR) -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | awk '{print $$1}' > $(ASSETS_PACK_SHA).tmp && mv $(ASSETS_PACK_SHA).tmp $(ASSETS_PACK_SHA); \
	fi

#$(PK3): vm/qagame.qvm vm/cgame.qvm vm/ui.qvm
$(PK3): check_cvardesc check_status_infostring vm/cgame.qvm vm/ui.qvm ../../assets/version.txt ../../assets/version_changelog.txt ../../assets/docs/changelog_be_r.txt $(ASSETS_PACK_SHA)
	@echo "Creating package with version files..."
	@echo "Version: $$(cat ../../assets/version.txt)"
	@FULL=0; \
	if [ ! -f $@ ]; then FULL=1; fi; \
	$(7Z) $@ vm/*.qvm vm/*.jts vm/*.map; \
	if [ "$$FULL" = 1 ] || [ ! -f $(ASSETS_PACKED_SHA) ] || ! cmp -s $(ASSETS_PACK_SHA) $(ASSETS_PACKED_SHA) 2>/dev/null; then \
		echo "Updating assets in pk3 (mx=9, threads: $(7Z_MT))..."; \
		$(7Z) $@ $(ASSETS_DIR)/*; \
		cp $(ASSETS_PACK_SHA) $(ASSETS_PACKED_SHA); \
	fi


cc = mkdir -p $(dir $@) && $(Q3LCC) $2 -o $@ $<
qa_cc = $(call cc,game,$(QA_CFLAGS))
cg_cc = $(call cc,cgame,$(CG_CFLAGS))
ui_cc = $(call cc,ui,$(UI_CFLAGS))

#vm/qagame.qvm: $(qa_asm)
#	$(Q3ASM) -o $@ $(qa_asm)

vm/cgame.qvm: $(cg_asm)
	$(Q3ASM) -o $@ $(cg_asm)

vm/ui.qvm: $(ui_asm)
	$(Q3ASM) -o $@ $(ui_asm)

vm/game/%.asm: $(QADIR)/%.c
	$(qa_cc)

vm/cgame/%.asm: $(QADIR)/%.c
	$(cg_cc)

vm/cgame/%.asm: $(CGDIR)/%.c
	$(cg_cc)

# chatWindow/*.c: compile from $(CGDIR) so q3cpp resolves cgame/game includes like flat cgame sources
vm/cgame/chatWindow/%.asm: $(CGDIR)/chatWindow/%.c
	mkdir -p $(dir $@)
	cd $(CGDIR) && $(Q3LCC_BIN) -DQ3_VM -S -Wf-g $(CG_CHATWINDOW_CFLAGS) -o $(abspath $@) chatWindow/$*.c

vm/cgame/superhud/%.asm: $(CGDIR)/superhud/%.c
	mkdir -p $(dir $@)
	cd $(CGDIR) && $(Q3LCC_BIN) -DQ3_VM -S -Wf-g $(CG_SUPERHUD_CFLAGS) -o $(abspath $@) superhud/$*.c

vm/cgame/superhud/elements/%.asm: $(CGDIR)/superhud/elements/%.c
	mkdir -p $(dir $@)
	cd $(CGDIR) && $(Q3LCC_BIN) -DQ3_VM -S -Wf-g $(CG_SUPERHUD_ELEMENT_CFLAGS) -o $(abspath $@) superhud/elements/$*.c

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
	$(RM) -rf *.pk3 vm/ ../cgame/*.asm ../game/*.asm ../ui/*.asm $(ASSETS_PACKED_SHA)
