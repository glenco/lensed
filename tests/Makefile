TESTS = \
	foreground/sky.ini \
	lens/epl.ini \
	lens/nsie.ini \
	lens/nsis.ini \
	lens/point_mass.ini \
	lens/sie.ini \
	lens/sie_plus_shear.ini \
	lens/sis.ini \
	lens/sis_plus_shear.ini \
	source/devauc.ini \
	source/exponential.ini \
	source/gauss.ini \
	source/sersic.ini \

OPTIONS = 

.PHONY: test $(TESTS)

test: $(TESTS)
	@echo "------------------------------"
	@echo " $(shell echo $^ | wc -w) tests total"

$(TESTS):
	@echo $(shell ../bin/lensed --batch $@ $(OPTIONS) | awk '{print $$3;}') $@
