include config.mk

srcs = $(foreach file,$1,$(if $(patsubst %.asm,,$(file)),$2/$(file).asm,$(file)))
qa_asm = $(call srcs,$(QA_SRC),vm/game)
cg_asm = $(call srcs,$(CG_SRC),vm/cgame)
ui_asm = $(call srcs,$(UI_SRC),vm/ui)

all: dirs $(PK3)

dirs:
	mkdir -p vm/game vm/cgame vm/ui

.PHONY: all dirs

../../assets/version.txt: $(CGDIR)/cg_local.h
	@echo "Extracting version from $(CGDIR)/cg_local.h"
	@grep -E '^#define OSP_VERSION' $(CGDIR)/cg_local.h | sed -E 's/^#define OSP_VERSION "([^"]*)".*/\1/' > $@
	@echo "Version file created: $$(cat $@)"

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
				exit 1; \
			} \
		} \
	' ../../changelog_be.txt > $@
	@echo "Changelog file created for version: $$(head -n 1 $@)"

$(PK3): vm/cgame.qvm vm/ui.qvm ../../assets/version.txt ../../assets/version_changelog.txt
	$(7Z) $@ vm/*.qvm vm/*.jts ../../assets/*

cc = cd vm/$1 && $(Q3LCC) $2 -o $(notdir $@).tmp ../../$< && mv $(notdir $@).tmp $(notdir $@)
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
	$(RM) -rf *.pk3 vm/ ../cgame/*.asm ../game/*.asm
