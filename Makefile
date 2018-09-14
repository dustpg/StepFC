.PHONY: clean All

All:
	@echo "----------Building project:[ step8 - Debug ]----------"
	@cd "step8" && "$(MAKE)" -f  "step8.mk"
clean:
	@echo "----------Cleaning project:[ step8 - Debug ]----------"
	@cd "step8" && "$(MAKE)" -f  "step8.mk" clean
