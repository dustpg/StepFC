.PHONY: clean All

All:
	@echo "----------Building project:[ step4 - Debug ]----------"
	@cd "step4" && "$(MAKE)" -f  "step4.mk"
clean:
	@echo "----------Cleaning project:[ step4 - Debug ]----------"
	@cd "step4" && "$(MAKE)" -f  "step4.mk" clean
