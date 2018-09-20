.PHONY: clean All

All:
	@echo "----------Building project:[ stepb - Debug ]----------"
	@cd "stepb" && "$(MAKE)" -f  "stepb.mk"
clean:
	@echo "----------Cleaning project:[ stepb - Debug ]----------"
	@cd "stepb" && "$(MAKE)" -f  "stepb.mk" clean
