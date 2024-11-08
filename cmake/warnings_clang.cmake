if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(
		-Werror

		-WCL4
		-Warc-repeated-use-of-weak
		-Wassign-enum
		-Watomic-implicit-seq-cst
		-Wc++11-extensions
		-Wcalled-once-parameter
		-Wcast-align
		# -Wcast-qual # JUCE
		-Wcomma
		-Wconditional-uninitialized
		# -Wdate-time # JUCE
		-Wdeprecated
		-Wdeprecated-implementations
		# -Wdocumentation # JUCE
		-Wduplicate-enum
		-Wduplicate-method-arg
		-Wduplicate-method-match
		-Wexplicit-ownership-type
		-Wextra-semi
		# -Wextra-semi-stmt # JUCE
		-Wformat=2
		-Wfour-char-constants
		-Wimplicit-atomic-properties
		# -Wimplicit-fallthrough # JUCE
		-Wimplicit-retain-self
		-Wloop-analysis
		-Wmissing-prototypes
		-Wnewline-eof
		-Wnon-gcc
		-Wnon-virtual-dtor
		-Wnullable-to-nonnull-conversion
		-Wobjc-interface-ivars
		-Wobjc-missing-property-synthesis
		# -Wold-style-cast # JUCE
		-Wpedantic
		-Wpragma-pack
		-Wquoted-include-in-framework-header
		# -Wredundant-parens # JUCE
		-Wshadow-all
		-Wstrict-prototypes
		# -Wstrict-selector-match # JUCE
		# -Wsuggest-destructor-override # JUCE
		-Wsuggest-override
		-Wswitch-enum
		-Wundeclared-selector
		# -Wundef # JUCE
		-Wunguarded-availability
		-Wunreachable-code-aggressive
	)
endif()
