.PHONY: clean All

All:
	@echo "----------Building project:[ step7 - Debug ]----------"
	@cd "step7" && "$(MAKE)" -f  "step7.mk"
clean:
	@echo "----------Cleaning project:[ step7 - Debug ]----------"
	@cd "step7" && "$(MAKE)" -f  "step7.mk" clean
