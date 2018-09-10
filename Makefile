.PHONY: clean All

All:
	@echo "----------Building project:[ step3 - Debug ]----------"
	@cd "step3" && "$(MAKE)" -f  "step3.mk"
clean:
	@echo "----------Cleaning project:[ step3 - Debug ]----------"
	@cd "step3" && "$(MAKE)" -f  "step3.mk" clean
