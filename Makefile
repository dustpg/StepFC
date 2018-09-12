.PHONY: clean All

All:
	@echo "----------Building project:[ step5 - Debug ]----------"
	@cd "step5" && "$(MAKE)" -f  "step5.mk"
clean:
	@echo "----------Cleaning project:[ step5 - Debug ]----------"
	@cd "step5" && "$(MAKE)" -f  "step5.mk" clean
