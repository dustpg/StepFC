.PHONY: clean All

All:
	@echo "----------Building project:[ step2 - Debug ]----------"
	@cd "step2" && "$(MAKE)" -f  "step2.mk"
clean:
	@echo "----------Cleaning project:[ step2 - Debug ]----------"
	@cd "step2" && "$(MAKE)" -f  "step2.mk" clean
